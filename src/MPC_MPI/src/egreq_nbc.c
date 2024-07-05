#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpc_mpi.h>

#include "mpc_common_debug.h"
#include <mpc_thread.h>
#include "sctk_alloc.h"
#include "mpc_lowcomm_msg.h"
#include "egreq_nbc.h"

#include "mpc_common_spinlock.h"

/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */

//#define EG_DEBUG

#ifdef EG_DEBUG
#define LOG(...) fprintf(__VA_ARGS__)

static const char * op_to_string(MPI_Op op) {
    switch(op) {
        case MPI_MAX: return "MPI_MAX";
        case MPI_MIN: return "MPI_MIN";
        case MPI_SUM: return "MPI_SUM";
        case MPI_PROD: return "MPI_PROD";
        case MPI_LAND: return "MPI_LAND";
        case MPI_BAND: return "MPI_BAND";
        case MPI_LOR: return "MPI_LOR";
        case MPI_BOR: return "MPI_BOR";
        case MPI_LXOR: return "MPI_LXOR";
        case MPI_BXOR: return "MPI_BXOR";
        case MPI_MAXLOC: return "MPI_MAXLOC";
        case MPI_MINLOC: return "MPI_MINLOC";
    }
    return "Unknown MPI_Op";
}

#else
#define LOG(...)
#endif

typedef enum {
    TYPE_NULL,
    TYPE_SEND=3,
    TYPE_IBCAST,
    TYPE_RECV,
    TYPE_WAIT,
    TYPE_MPI_OP,
    TYPE_FREE,
    TYPE_COMM_FREE,
    TYPE_COUNT
}nbc_op_type;

__UNUSED__ static const char * const strops[TYPE_COUNT] = {
    "NULL",
    "SEND",
    "RECV",
    "IBCAST",
    "WAIT",
    "MPI_OP",
    "FREE",
    "COMM_FREE"
};


typedef enum {
    NBC_NO_OP,
    NBC_NEXT_OP,
    NBC_WAIT_OP
}stepper;

struct nbc_op{
    nbc_op_type t;
    short trig;
    short done;
    int remote;
    MPI_Comm comm;
    void * buff;
    void * buff2;
    void * out_buff;
    int tag;
    MPI_Datatype datatype;
    int count;
    MPI_Op mpi_op;
    MPI_Request request;
};


typedef struct _xMPI_Request
{
    int size;
    int current_off;
    MPI_Request * myself;
    mpc_common_spinlock_t lock;
    struct _xMPI_Request * next;
    struct nbc_op op[0];
} xMPI_Request;


static inline void nbc_op_init( struct nbc_op * op,
        nbc_op_type type,
        int remote,
        MPI_Comm comm,
        MPI_Datatype datatype,
        int count,
        void * buff,
        int tag )
{
    op->trig = 0;
    op->done = 0;
    op->t = type;
    op->remote = remote;
    op->comm = comm;
    op->buff = buff;
    op->buff2 = NULL;
    op->out_buff = NULL;

    op->datatype = datatype;
    op->count = count;
    op->tag = tag;
}

static inline void nbc_op_wait_init(struct nbc_op * op) {
    op->trig = 0;
    op->done = 0;
    op->t = TYPE_WAIT;
}

static inline void nbc_op_free_init(struct nbc_op * op, void * buff) {
    op->trig = 0;
    op->done = 0;
    op->t = TYPE_FREE;
    op->buff = buff;
}

__UNUSED__ static inline void nbc_op_comm_free_init(struct nbc_op * op, MPI_Comm comm) {
    op->trig = 0;
    op->done = 0;
    op->t = TYPE_COMM_FREE;
    op->comm = comm;
}

static inline void nbc_op_mpi_op_init(struct nbc_op * op,
        MPI_Datatype datatype,
        int count,
        void * buff,
        void * buff2,
        void * out_buff,
        MPI_Op mpi_op)
{
    op->trig = 0;
    op->done = 0;
    op->t = TYPE_MPI_OP;
    op->datatype = datatype;
    op->count = count;
    op->buff = buff;
    op->buff2 = buff2;
    op->out_buff = out_buff;
    op->mpi_op = mpi_op;
}


static inline int nbc_op_trigger( struct nbc_op * op )
{

    if( op->trig )
        return NBC_NO_OP;

    if( op->t == TYPE_NULL )
    {
        op->trig = 1;
        return NBC_NEXT_OP;
    }

    if( op->t == TYPE_WAIT )
    {
        if( op->done )
        {
            op->trig = 1;
            return NBC_NEXT_OP;
        }
        else {
            return NBC_WAIT_OP;
        }
    }

#ifdef EG_DEBUG
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif

    switch (op->t) {
        case TYPE_SEND:
            LOG(stderr, "%d ----> SEND %d\n", rank, op->remote);
            PMPI_Isend(op->buff, op->count, op->datatype, op->remote, op->tag, op->comm, &op->request);
            break;
        case TYPE_RECV:
            LOG(stderr, "%d <---- RECV %d\n", rank, op->remote);
            PMPI_Irecv(op->buff, op->count, op->datatype, op->remote, op->tag, op->comm, &op->request);
            break;
        case TYPE_IBCAST:
            LOG(stderr, "%d <---- IBCAST %d\n", rank, op->remote);
            MPI_Ixbcast(op->buff, op->count, op->datatype, op->remote, op->comm, &op->request);
            break;
        case TYPE_MPI_OP:
            LOG(stderr, "%d ----- OP %s\n", rank, op_to_string(op->mpi_op));
            NBC_Operation(op->out_buff, op->buff, op->buff2, op->mpi_op, op->datatype, op->count);
            break;
        case TYPE_FREE:
            LOG(stderr, "%d ----- FREE %p\n", rank, op->buff);
            sctk_free(op->buff);
            break;
        case TYPE_COMM_FREE:
            LOG(stderr, "%d ----- COMM_FREE %p\n", rank, &op->buff);
            PMPI_Comm_free(&op->comm);
            break;
        default:
            LOG(stderr, "BAD OP TYPE\n");
            mpc_common_debug_abort();
    }

    op->trig = 1;

    return NBC_NEXT_OP;
}


static inline int nbc_op_test( struct nbc_op * op  )
{
#ifdef EG_DEBUG
    int rank;
    PMPI_Comm_rank( MPI_COMM_WORLD , &rank );
#endif
    if( op->t == TYPE_WAIT )
    {
        //LOG(stderr, "IS W\n");
        return 1;
    }

    if( op->t == TYPE_NULL )
    {
        //LOG(stderr, "IS N\n");
        return 1;
    }

    if( op->t == TYPE_MPI_OP) {
        return 1;
    }

    if( op->t == TYPE_FREE) {
        return 1;
    }

    if( op->t == TYPE_COMM_FREE) {
        return 1;
    }

    if( op->done )
    {
        //LOG(stderr, "IS D\n");
        return 1;
    }

    int flag=0;
    PMPI_Test( &op->request , &flag,  MPI_STATUS_IGNORE );

    if( flag )
    {
        //LOG(stderr, "%d COMPLETED (%s to %d)\n", rank, strops[op->t], op->remote );
        op->done = 1;
    }
    else
    {
        //LOG(stderr, "%d NOT COMPLETED (%s to %d)\n", rank, strops[op->t], op->remote );
    }

    return flag;
}


static mpc_common_spinlock_t rpool_lock = MPC_COMMON_SPINLOCK_INITIALIZER;
static int rpool_count = 0;
static xMPI_Request * rpool = NULL;

static inline xMPI_Request * xMPI_Request_from_pool( int  size )
{
    xMPI_Request * ret = NULL;
    mpc_common_spinlock_lock( &rpool_lock );

    if( rpool_count )
    {
        if( rpool )
        {
            if( size <= rpool->size )
            {
                rpool_count--;
                ret = rpool;
                rpool = rpool->next;
            }
        }
    }

    mpc_common_spinlock_unlock( &rpool_lock );

    return ret;
}


static inline int xMPI_Request_to_pool( xMPI_Request * req )
{
    int ret = 0;
    mpc_common_spinlock_lock( &rpool_lock );

    if( rpool_count < 100 )
    {
        rpool_count++;
        req->next = rpool;
        rpool = req;
        ret = 1;
    }

    mpc_common_spinlock_unlock( &rpool_lock );

    return ret;
}








static inline xMPI_Request * xMPI_Request_new(MPI_Request * parent, int size)
{
    /* We had an issue when polling the last event
     * adding a null event at the end seems to solve it*/
    //size++;
    xMPI_Request * ret = xMPI_Request_from_pool(size);

    if( !ret )
        ret = sctk_malloc( sizeof(xMPI_Request) + (sizeof(struct nbc_op) * size));

    if( !ret )
    {
        perror("malloc");
        abort();
    }

    ret->size = size;
    int i;
    for (i = 0; i < size; ++i) {
        //ret->op[i] = TYPE_NULL;
        //nbc_op_init( &ret->op[i],
        //        TYPE_NULL,
        //        -1,
        //        MPI_COMM_NULL,
        //        MPI_DATATYPE_NULL,
        //        0,
        //        NULL,
        //        0 );
        ret->op[i].t = TYPE_NULL;
        ret->op[i].trig = 0;
    }

    ret->myself = parent;
    ret->current_off = 1;

    mpc_common_spinlock_init(&ret->lock, 0);
    ret->next = NULL;


    return ret;
}

/** Extended Generalized Request Interface **/

int xMPI_Request_query_fn( __UNUSED__ void * pxreq, MPI_Status * status )
{
    status->MPI_ERROR = MPI_SUCCESS;

    return MPI_Status_set_elements( status , MPI_CHAR , 1 );
}

/*
 * Returns :
 * 0 -> DONE
 * 1 -> NOT DONE
 *
 */
static inline int xMPI_Request_gen_poll( xMPI_Request *xreq )
{
    int i, j;

#ifdef EG_DEBUG
    int rank;
    PMPI_Comm_rank( MPI_COMM_WORLD , &rank );
#endif

    int failed_lock = mpc_common_spinlock_trylock( &xreq->lock );

    if( failed_lock )
        return 1;

    int time_to_leave = 0;
    while(1)
    {
        if( time_to_leave )
            break;


        for (i = 0; i < xreq->current_off ; ++i)
        {
            //LOG(stderr, "%d @ %d (%d / %d)\n", rank,  i, xreq->current_off, xreq->size);
            int ret = nbc_op_trigger( &xreq->op[i] );


            if( ret == NBC_WAIT_OP )
            {
                int wret = 1;

                for (j = 0; j < i; ++j) {
                    int tmp = nbc_op_test( &xreq->op[j] );
                    wret *= tmp;
                    //LOG(stderr, "%d Wait @ %d == %d  !! %d\n", rank,  j, tmp, wret);
                }


                if( wret )
                {
                    xreq->op[i].done = 1;

                    //LOG(stderr, "%d STEP %d WAIT DONE\n", rank, i);
                    xreq->current_off++;
                    time_to_leave = 1;
                }

                break;
            }
            else if( ret == NBC_NEXT_OP )
            {
                //LOG(stderr, "%d TRIG @ %d\n", rank, i);
                time_to_leave=1;
                xreq->current_off++;
            }

            /* We may complete on a NULL op */
            if( xreq->current_off == xreq->size )
            {
                // //LOG(stderr, "DONE ON NULL\n");
                time_to_leave=1;
                break;
            }
        }

    }

    //LOG(stderr, "%d POLL DONE %d / %d\n", rank,  xreq->current_off, xreq->size );

    int ret;

    if( xreq->current_off == xreq->size )
    {
        LOG( stderr, "==== %d DONE %d / %d\n", rank, xreq->current_off, xreq->size);
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    mpc_common_spinlock_unlock( &xreq->lock );

    return ret;
}



int xMPI_Request_poll_fn( void * pxreq, __UNUSED__ MPI_Status * status )
{
    xMPI_Request * xreq = (xMPI_Request*) pxreq;

    int not_done = xMPI_Request_gen_poll( xreq );

    if( not_done == 0)
    {
        MPI_Grequest_complete(*xreq->myself);
    }

    return MPI_SUCCESS;
}

int xMPI_Request_wait_fn( int cnt, void ** array_of_states, __UNUSED__ double timeout, __UNUSED__ MPI_Status * st )
{
    /* Simple implementation */
    int i;

    int completed = 0;
    char _done_array[128] = {0};
    char *done_array = _done_array;
    if( 128 <= cnt )
    {
        done_array = sctk_calloc( cnt ,  sizeof(char));
        if( !done_array )
        {
            perror("malloc");
            return MPI_ERR_INTERN;
        }
    }

    int r;
    MPI_Comm_rank( MPI_COMM_WORLD , &r );

    while( completed != cnt )
    {
        for( i = 0 ; i < cnt ; i++ )
        {
            if( done_array[i] )
                continue;

            xMPI_Request * xreq = (xMPI_Request*) array_of_states[i];

            int not_done = xMPI_Request_gen_poll( xreq );

            if( !not_done )
            {
                MPI_Grequest_complete(*xreq->myself);
                //LOG(stderr, "[%d] Completed %d\n", r , i);
                completed++;
                done_array[i] = 1;
            }
        }
    }

    if( done_array != _done_array )
        sctk_free( done_array );


    return MPI_SUCCESS;
}

int xMPI_Request_free_fn( void * pxreq )
{
    xMPI_Request * r = (xMPI_Request*)pxreq;
    ////LOG(stderr, "FREEING %p\n", r->myself);
    //
    if( xMPI_Request_to_pool( r ) == 0 )
        sctk_free( pxreq );
    return MPI_SUCCESS;
}



int xMPI_Request_cancel_fn( __UNUSED__ void * pxreq, int complete )
{
    if(!complete)
        return MPI_SUCCESS;
    return MPI_SUCCESS;
}

static inline int adjust_rank(int rank, int root) {
    if(rank == root) {
        return 0;
    }
    if(rank == 0) {
        return root;
    }
    return rank;
}

static inline void setup_binary_tree(MPI_Comm comm, int root, int * out_rank, int * out_size, int * out_parent, int * out_lc, int * out_rc) {

    int parent, lc, rc;

    int rank, size;
    PMPI_Comm_rank( comm , &rank );
    PMPI_Comm_size( comm , &size );

    rank = adjust_rank(rank, root);

    parent = (rank+1)/2 -1;
    lc = (rank + 1 )*2 -1;
    rc= (rank + 1)*2;

    if(rank == 0)
        parent = -1;

    if(size <= lc)
        lc = -1;

    if(size <= rc)
        rc = -1;

    *out_rank = rank;
    *out_size = size;
    *out_parent = adjust_rank(parent, root);
    *out_lc = adjust_rank(lc, root);
    *out_rc = adjust_rank(rc, root);
}

static inline int start_request(xMPI_Request * xreq, MPI_Request * request) {
    return MPIX_Grequest_start( xMPI_Request_query_fn,
            xMPI_Request_free_fn,
            xMPI_Request_cancel_fn,
            xMPI_Request_poll_fn,
            xreq,
            request);
}


int MPI_Ixscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request) {
    // Currently this is a simple linear gather as in libnbc.
    // TODO replace with better algorithm
    // TODO MPI_IN_PLACE

    int rank, size;
    PMPI_Comm_rank( comm , &rank );
    PMPI_Comm_size( comm , &size );

    int send_size, recv_size;
    PMPI_Type_size(sendtype, &send_size);
    PMPI_Type_size(recvtype, &recv_size);
    const int send_bytes = send_size * recvcount;
    const int my_offset = send_bytes * rank;

    xMPI_Request * xreq;

    if(rank == root) {
        // If I am the root, I send to all other ranks.
        xreq = xMPI_Request_new(request, size + 2);
        memcpy(recvbuf, sendbuf + my_offset, send_bytes);
        int i;
        for(i = 0; i < size; ++i) {
            if(i == root) continue;
            const int their_offset = send_bytes * i;
            nbc_op_init(&xreq->op[i], TYPE_SEND, i, comm, sendtype, sendcount, (void*)sendbuf + their_offset, 12345);
        }
        nbc_op_wait_init(&xreq->op[size]);
    } else {
        // Otherwise, I receive from the root.
        xreq = xMPI_Request_new(request, 3);
        nbc_op_init( &xreq->op[0], TYPE_RECV, root, comm, recvtype, recvcount, recvbuf, 12345 );
        nbc_op_wait_init(&xreq->op[1]);
    }

    return start_request(xreq, request);
}

int MPI_Ixgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request * request) {
    // Currently this is a simple linear gather as in libnbc.
    // TODO replace with better algorithm
    // TODO MPI_IN_PLACE

    int rank, size;
    PMPI_Comm_rank( comm , &rank );
    PMPI_Comm_size( comm , &size );

    int send_size, recv_size;
    PMPI_Type_size(sendtype, &send_size);
    PMPI_Type_size(recvtype, &recv_size);
    const int recv_bytes = recv_size * recvcount;
    const int my_offset = recv_bytes * rank;

    xMPI_Request * xreq;

    if(rank == root) {
        // If I am the root, I receive from all other ranks.
        xreq = xMPI_Request_new(request, size + 2);
        memcpy(recvbuf + my_offset, sendbuf, recv_bytes);
        int i;
        for(i = 0; i < size; ++i) {
            if(i == root) continue;
            const int their_offset = recv_bytes * i;
            nbc_op_init(&xreq->op[i], TYPE_RECV, i, comm, recvtype, recvcount, recvbuf + their_offset, 12345);
        }
        nbc_op_wait_init(&xreq->op[size]);
    } else {
        // Otherwise, I send to the root.
        xreq = xMPI_Request_new(request, 3);
        nbc_op_init( &xreq->op[0], TYPE_SEND, root, comm, sendtype, sendcount, (void*)sendbuf, 12345 );
        nbc_op_wait_init(&xreq->op[1]);
    }

    return start_request(xreq, request);
}

int MPI_Ixreduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request) {

    // TODO User defined operations
    // TODO MPI_IN_PLACE
    xMPI_Request * xreq = xMPI_Request_new(request, 13);

    int type_size;
    PMPI_Type_size(datatype, &type_size);
    const int buff_size = count * type_size;

    int rank, size, parent, lc, rc;
    setup_binary_tree(comm, root, &rank, &size, &parent, &lc, &rc);

    if(size > 1) {

        void * lc_buff = NULL;
        int free_lc_buff = 0;
        void * rc_buff = NULL;
        int free_rc_buff = 0;
        void * work_buff = NULL;
        int free_work_buff = 0;
        void * out_buff = NULL;
        int free_out_buff = 0;

        if( 0 <= lc ) {
            // If I have a left child, allocate a buffer and receive into it.
            lc_buff = sctk_malloc(buff_size);
            free_lc_buff = 1;
            nbc_op_init( &xreq->op[0], TYPE_RECV, lc, comm, datatype, count, lc_buff, 12345 );
        }

        if( 0 <= rc ) {
            // If I have a right child, allocate a buffer and receive into it.
            rc_buff = sctk_malloc(buff_size);
            free_rc_buff = 1;
            nbc_op_init( &xreq->op[1], TYPE_RECV, rc, comm, datatype, count, rc_buff, 12345 );
        }

        // Then wait for the receives from my children to finish.
        nbc_op_wait_init(&xreq->op[2]);

        if( lc_buff != NULL && rc_buff != NULL ) {
            // If I have two children, I need to reduce them together.
            work_buff = sctk_malloc(buff_size);
            free_work_buff = 1;
            nbc_op_mpi_op_init(&xreq->op[3], datatype, count, lc_buff, rc_buff, work_buff, op);
        } else if( lc_buff != NULL ) {
            // If I have only one child, I just save it for the next step.
            work_buff = lc_buff;
        }

        if(work_buff != NULL) {
            if(parent == -1) {
                // If am the root, I save the results into my recvbuffer,
                out_buff = recvbuf;
            } else {
                // otherwise into an intermediate buffer.
                out_buff = sctk_malloc(buff_size);
                free_out_buff = 1;
            }
            // If I have any children, I need to reduce my buffer with the values from my children.
            nbc_op_mpi_op_init(&xreq->op[4], datatype, count, (void*)sendbuf, work_buff, out_buff, op);
        } else {
            // If I have no children, I need to send my unmodified input buffer
            out_buff = (void*)sendbuf;
        }

        // Send the output to my parent, if I have one.
        if(0 <= parent) {
            nbc_op_init( &xreq->op[5], TYPE_SEND, parent, comm, datatype, count, out_buff, 12345 );
        }

        // And wait for completion
        nbc_op_wait_init(&xreq->op[6]);

        // Finally, free any memory we allocated
        if(free_lc_buff) {
            nbc_op_free_init(&xreq->op[7], lc_buff);
        }
        if(free_rc_buff) {
            nbc_op_free_init(&xreq->op[8], rc_buff);
        }
        if(free_work_buff) {
            nbc_op_free_init(&xreq->op[9], work_buff);
        }
        if(free_out_buff) {
            nbc_op_free_init(&xreq->op[10], out_buff);
        }

    } else {
        // Special case for only one rank: just copy sendbuf to recvbuf
        memcpy(recvbuf, sendbuf, buff_size);
    }

    return start_request(xreq, request);
}

int MPI_Ixbcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request) {
    // This is more or less the same as the barrier, except that we start from the root
    // of the tree instead of the leaves, and the tree can have an arbitrary root
    // (which is accomplished by swapping 0 and the user-specified root)
    xMPI_Request * xreq = xMPI_Request_new(request, 6);

    int rank, size, parent, lc, rc;
    setup_binary_tree(comm, root, &rank, &size, &parent, &lc, &rc);

    if(0 <= parent) {
        // If this node has a parent, receive from it
        nbc_op_init(&xreq->op[0], TYPE_RECV, parent, comm, datatype, count, buffer, 12345);
        // and wait until I receive from it
        nbc_op_wait_init(&xreq->op[1]);
    }

    if(0 <= lc) {
        // If this node has a left child, send the payload to it
        nbc_op_init(&xreq->op[2], TYPE_SEND, lc, comm, datatype, count, buffer, 12345);
    }

    if(0 <= rc) {
        // If this node has a right child, send the payload to it
        nbc_op_init(&xreq->op[3], TYPE_SEND, rc, comm, datatype, count, buffer, 12345);
    }

    if( (0 <= lc) || (0 <= rc ) )
    nbc_op_wait_init(&xreq->op[4]);

    return start_request(xreq, request);
}

int MPI_Ixbarrier( MPI_Comm comm , MPI_Request * req )
{
    xMPI_Request * xreq = xMPI_Request_new(req, 9);

    ////LOG(stderr, "INIT on %p\n", req);

    int rank, size, parent, lc, rc;
    setup_binary_tree(comm, 0, &rank, &size, &parent, &lc, &rc);

    //LOG(stderr, "P: %d LC : %d RC : %d\n", parent, lc , rc);

    char c;

    if( 0 <= lc ){
        //LOG(stderr, "POST %d RECV from Lc %d\n", rank, lc );
        nbc_op_init( &xreq->op[0], TYPE_RECV, lc, comm, MPI_CHAR, 1, &c, 12345 );
    }


    if( 0 <= rc )
    {
        //LOG(stderr, "POST %d RECV from Rc %d\n", rank, rc );
        nbc_op_init( &xreq->op[1], TYPE_RECV, rc, comm, MPI_CHAR, 1, &c, 12345 );
    }

    if( (0 <= lc) || (0 <= rc) )
        nbc_op_wait_init(&xreq->op[2]);
    //LOG(stderr, "POST %d WAIT\n", rank );
    //nbc_op_init( &xreq->op[2], TYPE_WAIT, 0, comm, 0, 0, NULL, 0 );


    if( 0 <= parent ) {
        //LOG(stderr, "POST %d SEND to Par %d\n", rank, parent );
        nbc_op_init( &xreq->op[3], TYPE_SEND, parent, comm, MPI_CHAR, 1, &c, 12345 );
        //LOG(stderr, "POST %d RECV from Par %d\n", rank, parent );
        nbc_op_init( &xreq->op[4], TYPE_RECV, parent, comm, MPI_CHAR, 1, &c, 12345 );

        nbc_op_wait_init(&xreq->op[8]);
    }



    //LOG(stderr, "POST %d WAIT\n", rank );
    //nbc_op_init( &xreq->op[5], TYPE_WAIT, parent, comm, MPI_CHAR, 1, &c, 12345 );


    if( 0 <= lc )
    {
        //LOG(stderr, "POST %d SEND to Lc %d\n", rank, lc );
        nbc_op_init( &xreq->op[6], TYPE_SEND, lc, comm, MPI_CHAR, 1, &c, 12345 );
    }

    if( 0 <= rc )
    {
        //LOG(stderr, "POST %d SEND to Rc %d\n", rank, rc );
        nbc_op_init( &xreq->op[7], TYPE_SEND, rc, comm, MPI_CHAR, 1, &c, 12345 );
    }

    //LOG(stderr, "POST %d WAIT\n", rank );
    //nbc_op_init( &xreq->op[8], TYPE_WAIT, parent, comm, MPI_CHAR, 1, &c, 12345 );
    if( (0 <= lc) || (0 <= rc) )
        nbc_op_wait_init(&xreq->op[8]);

    return start_request(xreq, req);
}

/* NOLINTEND(clang-diagnostic-unused-function) */
