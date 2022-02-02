
#include <mpi.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
         intptr_t  lower_bound,
                   extent,
                   sm;
} CFI_dim_t;

/* Maximum rank supported by the companion Fortran processor */

/* Changed from 15 to F2003 value of 7 (CER) */
#define CFI_MAX_RANK  7

/* Struct CFI_cdesc_t for holding all the information about a
   descriptor-based Fortran object */

typedef struct {
  void *        base_addr;          /* base address of object                      */
  size_t        elem_len;           /* length of one element, in bytes             */
  int           rank;               /* object rank, 0 .. CF_MAX_RANK               */
  int           type;               /* identifier for type of object               */
  int           attribute;          /* object attribute: 0..2, or -1               */
  int           state;              /* allocation/association state: 0 or 1        */
//Removed (CER)
//void *        fdesc;              /* pointer to corresponding Fortran descriptor */
  CFI_dim_t     dim[CFI_MAX_RANK];  /* dimension triples                           */
} CFI_cdesc_t;


void mpi_start_(
MPI_Request* request,
int * ierror);
void mpi_start_f08(
MPI_Request* request,
int * ierror)
{
mpi_start_( request,
ierror);
}

void mpi_imrecv_(
void* buf,
int * count,
MPI_Datatype * datatype,
MPI_Message* message,
MPI_Request* status,
int * ierror);
void mpi_imrecv_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
MPI_Message* message,
MPI_Request* status,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_imrecv_( buf,
&count,
&datatype,
message,
status,
ierror);
}
/* MPI_File_write_all_begin NOT IMPLEMENTED in MPC */



void mpi_win_post_(
MPI_Group * group,
int * assert,
MPI_Win * win,
int * ierror);
void mpi_win_post_f08(
MPI_Group group,
int assert,
MPI_Win win,
int * ierror)
{
mpi_win_post_( &group,
&assert,
&win,
ierror);
}

void mpi_session_create_errhandler_(
MPI_Session_errhandler_function* errhfn,
MPI_Errhandler* errh,
int * ierror);
void mpi_session_create_errhandler_f08(
MPI_Session_errhandler_function* errhfn,
MPI_Errhandler* errh,
int * ierror)
{
mpi_session_create_errhandler_( errhfn,
errh,
ierror);
}

void mpi_win_get_errhandler_(
MPI_Win * win,
MPI_Errhandler* errhandler,
int * ierror);
void mpi_win_get_errhandler_f08(
MPI_Win win,
MPI_Errhandler* errhandler,
int * ierror)
{
mpi_win_get_errhandler_( &win,
errhandler,
ierror);
}
/* MPI_File_get_group NOT IMPLEMENTED in MPC */



void mpi_sendrecv_(
void* sendbuf,
int * sendcount,
MPI_Datatype * sendtype,
int * dest,
int * sendtag,
void* recvbuf,
int * recvcount,
MPI_Datatype * recvtype,
int * source,
int * recvtag,
MPI_Comm * comm,
MPI_Status* status,
int * ierror);
void mpi_sendrecv_f08(
CFI_cdesc_t* sendbuf_ptr,
int sendcount,
MPI_Datatype sendtype,
int dest,
int sendtag,
CFI_cdesc_t* recvbuf_ptr,
int recvcount,
MPI_Datatype recvtype,
int source,
int recvtag,
MPI_Comm comm,
MPI_Status* status,
int * ierror)
{
void* sendbuf = sendbuf_ptr->base_addr;
void* recvbuf = recvbuf_ptr->base_addr;
mpi_sendrecv_( sendbuf,
&sendcount,
&sendtype,
&dest,
&sendtag,
recvbuf,
&recvcount,
&recvtype,
&source,
&recvtag,
&comm,
status,
ierror);
}

void mpi_scan_(
void* sendbuf,
void* recvbuf,
int * count,
MPI_Datatype * datatype,
MPI_Op * op,
MPI_Comm * comm,
int * ierror);
void mpi_scan_f08(
CFI_cdesc_t* sendbuf_ptr,
CFI_cdesc_t* recvbuf_ptr,
int count,
MPI_Datatype datatype,
MPI_Op op,
MPI_Comm comm,
int * ierror)
{
void* sendbuf = sendbuf_ptr->base_addr;
void* recvbuf = recvbuf_ptr->base_addr;
mpi_scan_( sendbuf,
recvbuf,
&count,
&datatype,
&op,
&comm,
ierror);
}

void mpi_startall_(
int * count,
MPI_Request* array_of_requests,
int * ierror);
void mpi_startall_f08(
int count,
MPI_Request* array_of_requests,
int * ierror)
{
mpi_startall_( &count,
&array_of_requests,
ierror);
}

void mpi_attr_delete_(
MPI_Comm * comm,
int * keyval,
int * ierror);
void mpi_attr_delete_f08(
MPI_Comm comm,
int keyval,
int * ierror)
{
mpi_attr_delete_( &comm,
&keyval,
ierror);
}
/* MPI_File_iwrite_shared NOT IMPLEMENTED in MPC */



void mpi_comm_get_attr_(
MPI_Comm * comm,
int * comm_keyval,
void* attribute_val,
int* flag,
int * ierror);
void mpi_comm_get_attr_f08(
MPI_Comm comm,
int comm_keyval,
CFI_cdesc_t* attribute_val_ptr,
int* flag,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
mpi_comm_get_attr_( &comm,
&comm_keyval,
attribute_val,
&flag,
ierror);
}
/* MPI_File_get_info NOT IMPLEMENTED in MPC */



void mpi_type_delete_attr_(
MPI_Datatype * type,
int * type_keyval,
int * ierror);
void mpi_type_delete_attr_f08(
MPI_Datatype type,
int type_keyval,
int * ierror)
{
mpi_type_delete_attr_( &type,
&type_keyval,
ierror);
}

void mpi_error_class_(
int * errorcode,
int* errorclass,
int * ierror);
void mpi_error_class_f08(
int errorcode,
int* errorclass,
int * ierror)
{
mpi_error_class_( &errorcode,
errorclass,
ierror);
}

void mpi_session_get_num_psets_(
MPI_Session * session,
MPI_Info * info,
int* npset_names,
int * ierror);
void mpi_session_get_num_psets_f08(
MPI_Session session,
MPI_Info info,
int* npset_names,
int * ierror)
{
mpi_session_get_num_psets_( &session,
&info,
npset_names,
ierror);
}

void mpi_free_mem_(
void* base,
int * ierror);
void mpi_free_mem_f08(
CFI_cdesc_t* base_ptr,
int * ierror)
{
void* base = base_ptr->base_addr;
mpi_free_mem_( base,
ierror);
}

void mpi_info_dup_(
MPI_Info * info,
MPI_Info* newinfo,
int * ierror);
void mpi_info_dup_f08(
MPI_Info info,
MPI_Info* newinfo,
int * ierror)
{
mpi_info_dup_( &info,
newinfo,
ierror);
}

void mpi_type_lb_(
MPI_Datatype * type,
MPI_Aint* lb,
int * ierror);
void mpi_type_lb_f08(
MPI_Datatype type,
MPI_Aint* lb,
int * ierror)
{
mpi_type_lb_( &type,
lb,
ierror);
}

void mpi_cart_get_(
MPI_Comm * comm,
int * maxdims,
int* dims,
int** periods,
int* coords,
int * ierror);
void mpi_cart_get_f08(
MPI_Comm comm,
int maxdims,
int* dims,
int** periods,
int* coords,
int * ierror)
{
mpi_cart_get_( &comm,
&maxdims,
&dims,
&periods,
&coords,
ierror);
}

void mpi_add_error_class_(
int* errorclass,
int * ierror);
void mpi_add_error_class_f08(
int* errorclass,
int * ierror)
{
mpi_add_error_class_( errorclass,
ierror);
}
/* MPI_File_write_shared NOT IMPLEMENTED in MPC */



void mpi_buffer_detach_(
void* buffer,
int* size,
int * ierror);
void mpi_buffer_detach_f08(
CFI_cdesc_t* buffer_ptr,
int* size,
int * ierror)
{
void* buffer = buffer_ptr->base_addr;
mpi_buffer_detach_( buffer,
size,
ierror);
}
/* MPI_File_set_size NOT IMPLEMENTED in MPC */



void mpi_intercomm_create_(
MPI_Comm * local_comm,
int * local_leader,
MPI_Comm * bridge_comm,
int * remote_leader,
int * tag,
MPI_Comm* newintercomm,
int * ierror);
void mpi_intercomm_create_f08(
MPI_Comm local_comm,
int local_leader,
MPI_Comm bridge_comm,
int remote_leader,
int tag,
MPI_Comm* newintercomm,
int * ierror)
{
mpi_intercomm_create_( &local_comm,
&local_leader,
&bridge_comm,
&remote_leader,
&tag,
newintercomm,
ierror);
}/* Skipped function MPI_Group_from_session_psetwith conversion */

/* MPI_File_iread_at NOT IMPLEMENTED in MPC */



void mpi_allreduce_(
void* sendbuf,
void* recvbuf,
int * count,
MPI_Datatype * datatype,
MPI_Op * op,
MPI_Comm * comm,
int * ierror);
void mpi_allreduce_f08(
CFI_cdesc_t* sendbuf_ptr,
CFI_cdesc_t* recvbuf_ptr,
int count,
MPI_Datatype datatype,
MPI_Op op,
MPI_Comm comm,
int * ierror)
{
void* sendbuf = sendbuf_ptr->base_addr;
void* recvbuf = recvbuf_ptr->base_addr;
mpi_allreduce_( sendbuf,
recvbuf,
&count,
&datatype,
&op,
&comm,
ierror);
}

void mpi_comm_create_keyval_(
MPI_Comm_copy_attr_function* comm_copy_attr_fn,
MPI_Comm_delete_attr_function* comm_delete_attr_fn,
int* comm_keyval,
void* extra_state,
int * ierror);
void mpi_comm_create_keyval_f08(
MPI_Comm_copy_attr_function* comm_copy_attr_fn,
MPI_Comm_delete_attr_function* comm_delete_attr_fn,
int* comm_keyval,
CFI_cdesc_t* extra_state_ptr,
int * ierror)
{
void* extra_state = extra_state_ptr->base_addr;
mpi_comm_create_keyval_( comm_copy_attr_fn,
comm_delete_attr_fn,
comm_keyval,
extra_state,
ierror);
}

void mpi_ibsend_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
MPI_Request* request,
int * ierror);
void mpi_ibsend_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_ibsend_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
request,
ierror);
}
/* MPI_File_read_all_end NOT IMPLEMENTED in MPC */



void mpi_comm_remote_size_(
MPI_Comm * comm,
int* size,
int * ierror);
void mpi_comm_remote_size_f08(
MPI_Comm comm,
int* size,
int * ierror)
{
mpi_comm_remote_size_( &comm,
size,
ierror);
}

void mpi_type_contiguous_(
int * count,
MPI_Datatype * oldtype,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_contiguous_f08(
int count,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_contiguous_( &count,
&oldtype,
newtype,
ierror);
}

void mpi_send_init_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
MPI_Request* request,
int * ierror);
void mpi_send_init_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_send_init_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
request,
ierror);
}/* Skipped function MPI_Type_get_namewith conversion */


void mpi_session_get_info_(
MPI_Session * session,
MPI_Info* infoused,
int * ierror);
void mpi_session_get_info_f08(
MPI_Session session,
MPI_Info* infoused,
int * ierror)
{
mpi_session_get_info_( &session,
infoused,
ierror);
}

void mpi_type_commit_(
MPI_Datatype* type,
int * ierror);
void mpi_type_commit_f08(
MPI_Datatype* type,
int * ierror)
{
mpi_type_commit_( type,
ierror);
}

void mpi_session_set_errhandler_(
MPI_Session * session,
MPI_Errhandler * errh,
int * ierror);
void mpi_session_set_errhandler_f08(
MPI_Session session,
MPI_Errhandler errh,
int * ierror)
{
mpi_session_set_errhandler_( &session,
&errh,
ierror);
}

void mpi_type_create_f90_integer_(
int * r,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_create_f90_integer_f08(
int r,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_create_f90_integer_( &r,
newtype,
ierror);
}

void mpi_testany_(
int * count,
MPI_Request* array_of_requests,
int* index,
int* flag,
MPI_Status* status,
int * ierror);
void mpi_testany_f08(
int count,
MPI_Request* array_of_requests,
int* index,
int* flag,
MPI_Status* status,
int * ierror)
{
mpi_testany_( &count,
&array_of_requests,
index,
&flag,
status,
ierror);
}

void mpi_type_extent_(
MPI_Datatype * type,
MPI_Aint* extent,
int * ierror);
void mpi_type_extent_f08(
MPI_Datatype type,
MPI_Aint* extent,
int * ierror)
{
mpi_type_extent_( &type,
extent,
ierror);
}
/* MPI_File_preallocate NOT IMPLEMENTED in MPC */


/* MPI_File_get_position NOT IMPLEMENTED in MPC */



void mpi_sendrecv_replace_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * sendtag,
int * source,
int * recvtag,
MPI_Comm * comm,
MPI_Status* status,
int * ierror);
void mpi_sendrecv_replace_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int sendtag,
int source,
int recvtag,
MPI_Comm comm,
MPI_Status* status,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_sendrecv_replace_( buf,
&count,
&datatype,
&dest,
&sendtag,
&source,
&recvtag,
&comm,
status,
ierror);
}

void mpi_type_get_extent_(
MPI_Datatype * type,
MPI_Aint* lb,
MPI_Aint* extent,
int * ierror);
void mpi_type_get_extent_f08(
MPI_Datatype type,
MPI_Aint* lb,
MPI_Aint* extent,
int * ierror)
{
mpi_type_get_extent_( &type,
lb,
extent,
ierror);
}

void mpi_keyval_create_(
MPI_Copy_function* copy_fn,
MPI_Delete_function* delete_fn,
int* keyval,
void* extra_state,
int * ierror);
void mpi_keyval_create_f08(
MPI_Copy_function* copy_fn,
MPI_Delete_function* delete_fn,
int* keyval,
CFI_cdesc_t* extra_state_ptr,
int * ierror)
{
void* extra_state = extra_state_ptr->base_addr;
mpi_keyval_create_( copy_fn,
delete_fn,
keyval,
extra_state,
ierror);
}

void mpi_mrecv_(
void* buf,
int * count,
MPI_Datatype * datatype,
MPI_Message* message,
MPI_Status* status,
int * ierror);
void mpi_mrecv_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
MPI_Message* message,
MPI_Status* status,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_mrecv_( buf,
&count,
&datatype,
message,
status,
ierror);
}

void mpi_type_hvector_(
int * count,
int * blocklength,
int * stride,
MPI_Datatype * oldtype,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_hvector_f08(
int count,
int blocklength,
int stride,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_hvector_( &count,
&blocklength,
&stride,
&oldtype,
newtype,
ierror);
}

void mpi_group_translate_ranks_(
MPI_Group * group1,
int * n,
int* ranks1,
MPI_Group * group2,
int* ranks2,
int * ierror);
void mpi_group_translate_ranks_f08(
MPI_Group group1,
int n,
int* ranks1,
MPI_Group group2,
int* ranks2,
int * ierror)
{
mpi_group_translate_ranks_( &group1,
&n,
&ranks1,
&group2,
&ranks2,
ierror);
}/* Skipped function MPI_Testsomewith conversion */


void mpi_recv_init_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * source,
int * tag,
MPI_Comm * comm,
MPI_Request* request,
int * ierror);
void mpi_recv_init_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int source,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_recv_init_( buf,
&count,
&datatype,
&source,
&tag,
&comm,
request,
ierror);
}/* Skipped function MPI_Win_set_namewith conversion */


void mpi_type_dup_(
MPI_Datatype * type,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_dup_f08(
MPI_Datatype type,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_dup_( &type,
newtype,
ierror);
}

void mpi_comm_group_(
MPI_Comm * comm,
MPI_Group* group,
int * ierror);
void mpi_comm_group_f08(
MPI_Comm comm,
MPI_Group* group,
int * ierror)
{
mpi_comm_group_( &comm,
group,
ierror);
}

void mpi_add_error_code_(
int * errorclass,
int* errorcode,
int * ierror);
void mpi_add_error_code_f08(
int errorclass,
int* errorcode,
int * ierror)
{
mpi_add_error_code_( &errorclass,
errorcode,
ierror);
}

void mpi_type_create_resized_(
MPI_Datatype * oldtype,
MPI_Aint * lb,
MPI_Aint * extent,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_create_resized_f08(
MPI_Datatype oldtype,
MPI_Aint lb,
MPI_Aint extent,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_create_resized_( &oldtype,
&lb,
&extent,
newtype,
ierror);
}
/* MPI_File_seek_shared NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Unpublish_namewith conversion */


void mpi_get_address_(
void* location,
MPI_Aint* address,
int * ierror);
void mpi_get_address_f08(
CFI_cdesc_t* location_ptr,
MPI_Aint* address,
int * ierror)
{
void* location = location_ptr->base_addr;
mpi_get_address_( location,
address,
ierror);
}

void mpi_is_thread_main_(
int* flag,
int * ierror);
void mpi_is_thread_main_f08(
int* flag,
int * ierror)
{
mpi_is_thread_main_( &flag,
ierror);
}

void mpi_irecv_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * source,
int * tag,
MPI_Comm * comm,
MPI_Request* request,
int * ierror);
void mpi_irecv_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int source,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_irecv_( buf,
&count,
&datatype,
&source,
&tag,
&comm,
request,
ierror);
}

void mpi_type_create_indexed_block_(
int * count,
int * blocklength,
int* array_of_displacements,
MPI_Datatype * oldtype,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_create_indexed_block_f08(
int count,
int blocklength,
int* array_of_displacements,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_create_indexed_block_( &count,
&blocklength,
&array_of_displacements,
&oldtype,
newtype,
ierror);
}

void mpi_initialized_(
int* flag,
int * ierror);
void mpi_initialized_f08(
int* flag,
int * ierror)
{
mpi_initialized_( &flag,
ierror);
}
/* MPI_File_iwrite NOT IMPLEMENTED in MPC */



void mpi_bsend_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
int * ierror);
void mpi_bsend_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_bsend_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
ierror);
}

void mpi_group_excl_(
MPI_Group * group,
int * n,
int* ranks,
MPI_Group* newgroup,
int * ierror);
void mpi_group_excl_f08(
MPI_Group group,
int n,
int* ranks,
MPI_Group* newgroup,
int * ierror)
{
mpi_group_excl_( &group,
&n,
&ranks,
newgroup,
ierror);
}

void mpi_get_count_(
MPI_Status* status,
MPI_Datatype * datatype,
int* count,
int * ierror);
void mpi_get_count_f08(
MPI_Status* status,
MPI_Datatype datatype,
int* count,
int * ierror)
{
mpi_get_count_( status,
&datatype,
count,
ierror);
}/* Skipped function MPI_Error_stringwith conversion */


void mpi_grequest_start_(
MPI_Grequest_query_function* query_fn,
MPI_Grequest_free_function* free_fn,
MPI_Grequest_cancel_function* cancel_fn,
void* extra_state,
MPI_Request* request,
int * ierror);
void mpi_grequest_start_f08(
MPI_Grequest_query_function* query_fn,
MPI_Grequest_free_function* free_fn,
MPI_Grequest_cancel_function* cancel_fn,
CFI_cdesc_t* extra_state_ptr,
MPI_Request* request,
int * ierror)
{
void* extra_state = extra_state_ptr->base_addr;
mpi_grequest_start_( query_fn,
free_fn,
cancel_fn,
extra_state,
request,
ierror);
}

void mpi_cartdim_get_(
MPI_Comm * comm,
int* ndims,
int * ierror);
void mpi_cartdim_get_f08(
MPI_Comm comm,
int* ndims,
int * ierror)
{
mpi_cartdim_get_( &comm,
ndims,
ierror);
}

void mpi_allgather_(
void* sendbuf,
int * sendcount,
MPI_Datatype * sendtype,
void* recvbuf,
int * recvcount,
MPI_Datatype * recvtype,
MPI_Comm * comm,
int * ierror);
void mpi_allgather_f08(
CFI_cdesc_t* sendbuf_ptr,
int sendcount,
MPI_Datatype sendtype,
CFI_cdesc_t* recvbuf_ptr,
int recvcount,
MPI_Datatype recvtype,
MPI_Comm comm,
int * ierror)
{
void* sendbuf = sendbuf_ptr->base_addr;
void* recvbuf = recvbuf_ptr->base_addr;
mpi_allgather_( sendbuf,
&sendcount,
&sendtype,
recvbuf,
&recvcount,
&recvtype,
&comm,
ierror);
}

void mpi_cart_coords_(
MPI_Comm * comm,
int * rank,
int * maxdims,
int* coords,
int * ierror);
void mpi_cart_coords_f08(
MPI_Comm comm,
int rank,
int maxdims,
int* coords,
int * ierror)
{
mpi_cart_coords_( &comm,
&rank,
&maxdims,
&coords,
ierror);
}/* Skipped function MPI_Scattervwith conversion */


void mpi_issend_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
MPI_Request* request,
int * ierror);
void mpi_issend_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_issend_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
request,
ierror);
}
/* MPI_File_sync NOT IMPLEMENTED in MPC */



void mpi_rsend_(
void* ibuf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
int * ierror);
void mpi_rsend_f08(
CFI_cdesc_t* ibuf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
void* ibuf = ibuf_ptr->base_addr;
mpi_rsend_( ibuf,
&count,
&datatype,
&dest,
&tag,
&comm,
ierror);
}
/* MPI_File_get_amode NOT IMPLEMENTED in MPC */



void mpi_abort_(
MPI_Comm * comm,
int * errorcode,
int * ierror);
void mpi_abort_f08(
MPI_Comm comm,
int errorcode,
int * ierror)
{
mpi_abort_( &comm,
&errorcode,
ierror);
}

void mpi_session_call_errhandler_(
MPI_Session * session,
int * errorcode,
int * ierror);
void mpi_session_call_errhandler_f08(
MPI_Session session,
int errorcode,
int * ierror)
{
mpi_session_call_errhandler_( &session,
&errorcode,
ierror);
}

void mpi_grequest_complete_(
MPI_Request * request,
int * ierror);
void mpi_grequest_complete_f08(
MPI_Request request,
int * ierror)
{
mpi_grequest_complete_( &request,
ierror);
}

void mpi_pack_(
void* inbuf,
int * incount,
MPI_Datatype * datatype,
void* outbuf,
int * outsize,
int* position,
MPI_Comm * comm,
int * ierror);
void mpi_pack_f08(
CFI_cdesc_t* inbuf_ptr,
int incount,
MPI_Datatype datatype,
CFI_cdesc_t* outbuf_ptr,
int outsize,
int* position,
MPI_Comm comm,
int * ierror)
{
void* inbuf = inbuf_ptr->base_addr;
void* outbuf = outbuf_ptr->base_addr;
mpi_pack_( inbuf,
&incount,
&datatype,
outbuf,
&outsize,
position,
&comm,
ierror);
}

void mpi_win_set_attr_(
MPI_Win * win,
int * win_keyval,
void* attribute_val,
int * ierror);
void mpi_win_set_attr_f08(
MPI_Win win,
int win_keyval,
CFI_cdesc_t* attribute_val_ptr,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
mpi_win_set_attr_( &win,
&win_keyval,
attribute_val,
ierror);
}

void mpi_info_create_(
MPI_Info* info,
int * ierror);
void mpi_info_create_f08(
MPI_Info* info,
int * ierror)
{
mpi_info_create_( info,
ierror);
}
/* MPI_File_open NOT IMPLEMENTED in MPC */



void mpi_type_create_f90_complex_(
int * p,
int * r,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_create_f90_complex_f08(
int p,
int r,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_create_f90_complex_( &p,
&r,
newtype,
ierror);
}/* Skipped function MPI_Gathervwith conversion */


void mpi_type_get_attr_(
MPI_Datatype * type,
int * type_keyval,
void* attribute_val,
int* flag,
int * ierror);
void mpi_type_get_attr_f08(
MPI_Datatype type,
int type_keyval,
CFI_cdesc_t* attribute_val_ptr,
int* flag,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
mpi_type_get_attr_( &type,
&type_keyval,
attribute_val,
&flag,
ierror);
}/* Skipped function MPI_Comm_set_namewith conversion */


void mpi_comm_disconnect_(
MPI_Comm* comm,
int * ierror);
void mpi_comm_disconnect_f08(
MPI_Comm* comm,
int * ierror)
{
mpi_comm_disconnect_( comm,
ierror);
}

void mpi_comm_remote_group_(
MPI_Comm * comm,
MPI_Group* group,
int * ierror);
void mpi_comm_remote_group_f08(
MPI_Comm comm,
MPI_Group* group,
int * ierror)
{
mpi_comm_remote_group_( &comm,
group,
ierror);
}

void mpi_cart_shift_(
MPI_Comm * comm,
int * direction,
int * disp,
int* rank_source,
int* rank_dest,
int * ierror);
void mpi_cart_shift_f08(
MPI_Comm comm,
int direction,
int disp,
int* rank_source,
int* rank_dest,
int * ierror)
{
mpi_cart_shift_( &comm,
&direction,
&disp,
rank_source,
rank_dest,
ierror);
}

void mpi_group_incl_(
MPI_Group * group,
int * n,
int* ranks,
MPI_Group* newgroup,
int * ierror);
void mpi_group_incl_f08(
MPI_Group group,
int n,
int* ranks,
MPI_Group* newgroup,
int * ierror)
{
mpi_group_incl_( &group,
&n,
&ranks,
newgroup,
ierror);
}

void mpi_comm_size_(
MPI_Comm * comm,
int* size,
int * ierror);
void mpi_comm_size_f08(
MPI_Comm comm,
int* size,
int * ierror)
{
mpi_comm_size_( &comm,
size,
ierror);
}

void mpi_type_create_hindexed_(
int * count,
int* array_of_blocklengths,
MPI_Aint* array_of_displacements,
MPI_Datatype * oldtype,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_create_hindexed_f08(
int count,
int* array_of_blocklengths,
MPI_Aint* array_of_displacements,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_create_hindexed_( &count,
&array_of_blocklengths,
&array_of_displacements,
&oldtype,
newtype,
ierror);
}
/* MPI_File_iread_shared NOT IMPLEMENTED in MPC */



void mpi_file_set_errhandler_(
MPI_File * file,
MPI_Errhandler * errhandler,
int * ierror);
void mpi_file_set_errhandler_f08(
MPI_File file,
MPI_Errhandler errhandler,
int * ierror)
{
mpi_file_set_errhandler_( &file,
&errhandler,
ierror);
}/* Skipped function MPI_Register_datarepwith conversion */

/* MPI_File_read_ordered NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Waitsomewith conversion */


void mpi_group_difference_(
MPI_Group * group1,
MPI_Group * group2,
MPI_Group* newgroup,
int * ierror);
void mpi_group_difference_f08(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
mpi_group_difference_( &group1,
&group2,
newgroup,
ierror);
}

void mpi_attr_get_(
MPI_Comm * comm,
int * keyval,
void* attribute_val,
int* flag,
int * ierror);
void mpi_attr_get_f08(
MPI_Comm comm,
int keyval,
CFI_cdesc_t* attribute_val_ptr,
int* flag,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
mpi_attr_get_( &comm,
&keyval,
attribute_val,
&flag,
ierror);
}/* Skipped function MPI_Reduce_scatterwith conversion */
/* Skipped function MPI_Win_get_namewith conversion */


void mpi_type_create_f90_real_(
int * p,
int * r,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_create_f90_real_f08(
int p,
int r,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_create_f90_real_( &p,
&r,
newtype,
ierror);
}

void mpi_win_create_keyval_(
MPI_Win_copy_attr_function* win_copy_attr_fn,
MPI_Win_delete_attr_function* win_delete_attr_fn,
int* win_keyval,
void* extra_state,
int * ierror);
void mpi_win_create_keyval_f08(
MPI_Win_copy_attr_function* win_copy_attr_fn,
MPI_Win_delete_attr_function* win_delete_attr_fn,
int* win_keyval,
CFI_cdesc_t* extra_state_ptr,
int * ierror)
{
void* extra_state = extra_state_ptr->base_addr;
mpi_win_create_keyval_( win_copy_attr_fn,
win_delete_attr_fn,
win_keyval,
extra_state,
ierror);
}

void mpi_wait_(
MPI_Request* request,
MPI_Status* status,
int * ierror);
void mpi_wait_f08(
MPI_Request* request,
MPI_Status* status,
int * ierror)
{
mpi_wait_( request,
status,
ierror);
}

void mpi_session_get_errhandler_(
MPI_Session * session,
MPI_Errhandler* errh,
int * ierror);
void mpi_session_get_errhandler_f08(
MPI_Session session,
MPI_Errhandler* errh,
int * ierror)
{
mpi_session_get_errhandler_( &session,
errh,
ierror);
}
/* MPI_File_write_all NOT IMPLEMENTED in MPC */


/* MPI_File_set_info NOT IMPLEMENTED in MPC */



void mpi_irsend_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
MPI_Request* request,
int * ierror);
void mpi_irsend_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_irsend_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
request,
ierror);
}

void mpi_get_version_(
int* version,
int* subversion,
int * ierror);
void mpi_get_version_f08(
int* version,
int* subversion,
int * ierror)
{
mpi_get_version_( version,
subversion,
ierror);
}

void mpi_file_call_errhandler_(
MPI_File * fh,
int * errorcode,
int * ierror);
void mpi_file_call_errhandler_f08(
MPI_File fh,
int errorcode,
int * ierror)
{
mpi_file_call_errhandler_( &fh,
&errorcode,
ierror);
}

void mpi_comm_create_errhandler_(
MPI_Comm_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror);
void mpi_comm_create_errhandler_f08(
MPI_Comm_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
mpi_comm_create_errhandler_( function,
errhandler,
ierror);
}/* Skipped function MPI_Session_get_pset_infowith conversion */
/* Skipped function MPI_Comm_connectwith conversion */


void mpi_group_compare_(
MPI_Group * group1,
MPI_Group * group2,
int* result,
int * ierror);
void mpi_group_compare_f08(
MPI_Group group1,
MPI_Group group2,
int* result,
int * ierror)
{
mpi_group_compare_( &group1,
&group2,
result,
ierror);
}

void mpi_address_(
void* location,
MPI_Aint* address,
int * ierror);
void mpi_address_f08(
CFI_cdesc_t* location_ptr,
MPI_Aint* address,
int * ierror)
{
void* location = location_ptr->base_addr;
mpi_address_( location,
address,
ierror);
}

void mpi_comm_compare_(
MPI_Comm * comm1,
MPI_Comm * comm2,
int* result,
int * ierror);
void mpi_comm_compare_f08(
MPI_Comm comm1,
MPI_Comm comm2,
int* result,
int * ierror)
{
mpi_comm_compare_( &comm1,
&comm2,
result,
ierror);
}

void mpi_win_unlock_(
int * rank,
MPI_Win * win,
int * ierror);
void mpi_win_unlock_f08(
int rank,
MPI_Win win,
int * ierror)
{
mpi_win_unlock_( &rank,
&win,
ierror);
}/* Skipped function MPI_Graph_mapwith conversion */


void mpi_request_free_(
MPI_Request* request,
int * ierror);
void mpi_request_free_f08(
MPI_Request* request,
int * ierror)
{
mpi_request_free_( request,
ierror);
}

void mpi_topo_test_(
MPI_Comm * comm,
int* status,
int * ierror);
void mpi_topo_test_f08(
MPI_Comm comm,
int* status,
int * ierror)
{
mpi_topo_test_( &comm,
status,
ierror);
}
/* MPI_File_read NOT IMPLEMENTED in MPC */



void mpi_query_thread_(
int* provided,
int * ierror);
void mpi_query_thread_f08(
int* provided,
int * ierror)
{
mpi_query_thread_( provided,
ierror);
}

void mpi_win_call_errhandler_(
MPI_Win * win,
int * errorcode,
int * ierror);
void mpi_win_call_errhandler_f08(
MPI_Win win,
int errorcode,
int * ierror)
{
mpi_win_call_errhandler_( &win,
&errorcode,
ierror);
}

void mpi_win_get_group_(
MPI_Win * win,
MPI_Group* group,
int * ierror);
void mpi_win_get_group_f08(
MPI_Win win,
MPI_Group* group,
int * ierror)
{
mpi_win_get_group_( &win,
group,
ierror);
}

void mpi_errhandler_create_(
MPI_Handler_function* function,
MPI_Errhandler* errhandler,
int * ierror);
void mpi_errhandler_create_f08(
MPI_Handler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
mpi_errhandler_create_( function,
errhandler,
ierror);
}

void mpi_cart_create_(
MPI_Comm * old_comm,
int * ndims,
int* dims,
int** periods,
int* reorder,
MPI_Comm* comm_cart,
int * ierror);
void mpi_cart_create_f08(
MPI_Comm old_comm,
int ndims,
int* dims,
int** periods,
int* reorder,
MPI_Comm* comm_cart,
int * ierror)
{
mpi_cart_create_( &old_comm,
&ndims,
&dims,
&periods,
&reorder,
comm_cart,
ierror);
}

void mpi_status_set_cancelled_(
MPI_Status* status,
int* flag,
int * ierror);
void mpi_status_set_cancelled_f08(
MPI_Status* status,
int* flag,
int * ierror)
{
mpi_status_set_cancelled_( status,
&flag,
ierror);
}
/* MPI_Status_f2c NOT IMPLEMENTED in MPC */



void mpi_type_struct_(
int * count,
int* array_of_blocklengths,
int* array_of_displacements,
MPI_Datatype* array_of_types,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_struct_f08(
int count,
int* array_of_blocklengths,
int* array_of_displacements,
MPI_Datatype* array_of_types,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_struct_( &count,
&array_of_blocklengths,
&array_of_displacements,
&array_of_types,
newtype,
ierror);
}

void mpi_graph_neighbors_count_(
MPI_Comm * comm,
int * rank,
int* nneighbors,
int * ierror);
void mpi_graph_neighbors_count_f08(
MPI_Comm comm,
int rank,
int* nneighbors,
int * ierror)
{
mpi_graph_neighbors_count_( &comm,
&rank,
nneighbors,
ierror);
}
/* MPI_File_get_view NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Allgathervwith conversion */

/* MPI_File_get_position_shared NOT IMPLEMENTED in MPC */



void mpi_graph_neighbors_(
MPI_Comm * comm,
int * rank,
int * maxneighbors,
int* neighbors,
int * ierror);
void mpi_graph_neighbors_f08(
MPI_Comm comm,
int rank,
int maxneighbors,
int* neighbors,
int * ierror)
{
mpi_graph_neighbors_( &comm,
&rank,
&maxneighbors,
&neighbors,
ierror);
}

void mpi_dims_create_(
int * nnodes,
int * ndims,
int* dims,
int * ierror);
void mpi_dims_create_f08(
int nnodes,
int ndims,
int* dims,
int * ierror)
{
mpi_dims_create_( &nnodes,
&ndims,
&dims,
ierror);
}
/* MPI_File_iread NOT IMPLEMENTED in MPC */



void mpi_scatter_(
void* sendbuf,
int * sendcount,
MPI_Datatype * sendtype,
void* recvbuf,
int * recvcount,
MPI_Datatype * recvtype,
int * root,
MPI_Comm * comm,
int * ierror);
void mpi_scatter_f08(
CFI_cdesc_t* sendbuf_ptr,
int sendcount,
MPI_Datatype sendtype,
CFI_cdesc_t* recvbuf_ptr,
int recvcount,
MPI_Datatype recvtype,
int root,
MPI_Comm comm,
int * ierror)
{
void* sendbuf = sendbuf_ptr->base_addr;
void* recvbuf = recvbuf_ptr->base_addr;
mpi_scatter_( sendbuf,
&sendcount,
&sendtype,
recvbuf,
&recvcount,
&recvtype,
&root,
&comm,
ierror);
}

void mpi_comm_set_attr_(
MPI_Comm * comm,
int * comm_keyval,
void* attribute_val,
int * ierror);
void mpi_comm_set_attr_f08(
MPI_Comm comm,
int comm_keyval,
CFI_cdesc_t* attribute_val_ptr,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
mpi_comm_set_attr_( &comm,
&comm_keyval,
attribute_val,
ierror);
}

void mpi_comm_free_keyval_(
int* comm_keyval,
int * ierror);
void mpi_comm_free_keyval_f08(
int* comm_keyval,
int * ierror)
{
mpi_comm_free_keyval_( comm_keyval,
ierror);
}

void mpi_op_create_(
MPI_User_function* function,
int* commute,
MPI_Op* op,
int * ierror);
void mpi_op_create_f08(
MPI_User_function* function,
int* commute,
MPI_Op* op,
int * ierror)
{
mpi_op_create_( function,
&commute,
op,
ierror);
}
/* MPI_File_seek NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Add_error_stringwith conversion */


void mpi_session_finalize_(
MPI_Session* session,
int * ierror);
void mpi_session_finalize_f08(
MPI_Session* session,
int * ierror)
{
mpi_session_finalize_( session,
ierror);
}

void mpi_ssend_init_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
MPI_Request* request,
int * ierror);
void mpi_ssend_init_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_ssend_init_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
request,
ierror);
}

void mpi_rsend_init_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
MPI_Request* request,
int * ierror);
void mpi_rsend_init_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_rsend_init_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
request,
ierror);
}

void mpi_info_free_(
MPI_Info* info,
int * ierror);
void mpi_info_free_f08(
MPI_Info* info,
int * ierror)
{
mpi_info_free_( info,
ierror);
}/* Skipped function MPI_Publish_namewith conversion */


void mpi_bcast_(
void* buffer,
int * count,
MPI_Datatype * datatype,
int * root,
MPI_Comm * comm,
int * ierror);
void mpi_bcast_f08(
CFI_cdesc_t* buffer_ptr,
int count,
MPI_Datatype datatype,
int root,
MPI_Comm comm,
int * ierror)
{
void* buffer = buffer_ptr->base_addr;
mpi_bcast_( buffer,
&count,
&datatype,
&root,
&comm,
ierror);
}/* Skipped function MPI_Get_processor_namewith conversion */
/* Skipped function MPI_Info_setwith conversion */

/* MPI_File_write_ordered_end NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Graph_createwith conversion */


void mpi_comm_free_(
MPI_Comm* comm,
int * ierror);
void mpi_comm_free_f08(
MPI_Comm* comm,
int * ierror)
{
mpi_comm_free_( comm,
ierror);
}
/* MPI_File_write_at_all NOT IMPLEMENTED in MPC */



void mpi_errhandler_get_(
MPI_Comm * comm,
MPI_Errhandler* errhandler,
int * ierror);
void mpi_errhandler_get_f08(
MPI_Comm comm,
MPI_Errhandler* errhandler,
int * ierror)
{
mpi_errhandler_get_( &comm,
errhandler,
ierror);
}

void mpi_pack_size_(
int * incount,
MPI_Datatype * datatype,
MPI_Comm * comm,
int* size,
int * ierror);
void mpi_pack_size_f08(
int incount,
MPI_Datatype datatype,
MPI_Comm comm,
int* size,
int * ierror)
{
mpi_pack_size_( &incount,
&datatype,
&comm,
size,
ierror);
}

void mpi_comm_call_errhandler_(
MPI_Comm * comm,
int * errorcode,
int * ierror);
void mpi_comm_call_errhandler_f08(
MPI_Comm comm,
int errorcode,
int * ierror)
{
mpi_comm_call_errhandler_( &comm,
&errorcode,
ierror);
}

void mpi_comm_test_inter_(
MPI_Comm * comm,
int* flag,
int * ierror);
void mpi_comm_test_inter_f08(
MPI_Comm comm,
int* flag,
int * ierror)
{
mpi_comm_test_inter_( &comm,
&flag,
ierror);
}/* Skipped function MPI_Comm_create_from_groupwith conversion */


void mpi_intercomm_merge_(
MPI_Comm * intercomm,
int* high,
MPI_Comm* newintercomm,
int * ierror);
void mpi_intercomm_merge_f08(
MPI_Comm intercomm,
int* high,
MPI_Comm* newintercomm,
int * ierror)
{
mpi_intercomm_merge_( &intercomm,
&high,
newintercomm,
ierror);
}

void mpi_win_complete_(
MPI_Win * win,
int * ierror);
void mpi_win_complete_f08(
MPI_Win win,
int * ierror)
{
mpi_win_complete_( &win,
ierror);
}/* Skipped function MPI_Pack_externalwith conversion */

/* MPI_File_get_type_extent NOT IMPLEMENTED in MPC */


/* MPI_File_read_all_begin NOT IMPLEMENTED in MPC */



void mpi_type_set_attr_(
MPI_Datatype * type,
int * type_keyval,
void* attr_val,
int * ierror);
void mpi_type_set_attr_f08(
MPI_Datatype type,
int type_keyval,
CFI_cdesc_t* attr_val_ptr,
int * ierror)
{
void* attr_val = attr_val_ptr->base_addr;
mpi_type_set_attr_( &type,
&type_keyval,
attr_val,
ierror);
}/* Skipped function MPI_Info_get_valuelenwith conversion */


void mpi_put_(
void* origin_addr,
int * origin_count,
MPI_Datatype * origin_datatype,
int * target_rank,
MPI_Aint * target_disp,
int * target_count,
MPI_Datatype * target_datatype,
MPI_Win * win,
int * ierror);
void mpi_put_f08(
CFI_cdesc_t* origin_addr_ptr,
int origin_count,
MPI_Datatype origin_datatype,
int target_rank,
MPI_Aint target_disp,
int target_count,
MPI_Datatype target_datatype,
MPI_Win win,
int * ierror)
{
void* origin_addr = origin_addr_ptr->base_addr;
mpi_put_( origin_addr,
&origin_count,
&origin_datatype,
&target_rank,
&target_disp,
&target_count,
&target_datatype,
&win,
ierror);
}
/* MPI_File_read_at_all NOT IMPLEMENTED in MPC */


/* MPI_File_read_ordered_end NOT IMPLEMENTED in MPC */



void mpi_isend_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
MPI_Request* request,
int * ierror);
void mpi_isend_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_isend_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
request,
ierror);
}

void mpi_type_get_envelope_(
MPI_Datatype * type,
int* num_integers,
int* num_addresses,
int* num_datatypes,
int* combiner,
int * ierror);
void mpi_type_get_envelope_f08(
MPI_Datatype type,
int* num_integers,
int* num_addresses,
int* num_datatypes,
int* combiner,
int * ierror)
{
mpi_type_get_envelope_( &type,
num_integers,
num_addresses,
num_datatypes,
combiner,
ierror);
}

void mpi_group_rank_(
MPI_Group * group,
int* rank,
int * ierror);
void mpi_group_rank_f08(
MPI_Group group,
int* rank,
int * ierror)
{
mpi_group_rank_( &group,
rank,
ierror);
}

void mpi_alltoall_(
void* sendbuf,
int * sendcount,
MPI_Datatype * sendtype,
void* recvbuf,
int * recvcount,
MPI_Datatype * recvtype,
MPI_Comm * comm,
int * ierror);
void mpi_alltoall_f08(
CFI_cdesc_t* sendbuf_ptr,
int sendcount,
MPI_Datatype sendtype,
CFI_cdesc_t* recvbuf_ptr,
int recvcount,
MPI_Datatype recvtype,
MPI_Comm comm,
int * ierror)
{
void* sendbuf = sendbuf_ptr->base_addr;
void* recvbuf = recvbuf_ptr->base_addr;
mpi_alltoall_( sendbuf,
&sendcount,
&sendtype,
recvbuf,
&recvcount,
&recvtype,
&comm,
ierror);
}

void mpi_buffer_attach_(
void* buffer,
int * size,
int * ierror);
void mpi_buffer_attach_f08(
CFI_cdesc_t* buffer_ptr,
int size,
int * ierror)
{
void* buffer = buffer_ptr->base_addr;
mpi_buffer_attach_( buffer,
&size,
ierror);
}/* Skipped function MPI_Info_getwith conversion */

/* MPI_File_iwrite_at NOT IMPLEMENTED in MPC */



void mpi_group_intersection_(
MPI_Group * group1,
MPI_Group * group2,
MPI_Group* newgroup,
int * ierror);
void mpi_group_intersection_f08(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
mpi_group_intersection_( &group1,
&group2,
newgroup,
ierror);
}

void mpi_type_free_keyval_(
int* type_keyval,
int * ierror);
void mpi_type_free_keyval_f08(
int* type_keyval,
int * ierror)
{
mpi_type_free_keyval_( type_keyval,
ierror);
}

void mpi_type_create_struct_(
int * count,
int* array_of_block_lengths,
MPI_Aint* array_of_displacements,
MPI_Datatype* array_of_types,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_create_struct_f08(
int count,
int* array_of_block_lengths,
MPI_Aint* array_of_displacements,
MPI_Datatype* array_of_types,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_create_struct_( &count,
&array_of_block_lengths,
&array_of_displacements,
&array_of_types,
newtype,
ierror);
}

void mpi_type_get_contents_(
MPI_Datatype * mtype,
int * max_integers,
int * max_addresses,
int * max_datatypes,
int* array_of_integers,
MPI_Aint* array_of_addresses,
MPI_Datatype* array_of_datatypes,
int * ierror);
void mpi_type_get_contents_f08(
MPI_Datatype mtype,
int max_integers,
int max_addresses,
int max_datatypes,
int* array_of_integers,
MPI_Aint* array_of_addresses,
MPI_Datatype* array_of_datatypes,
int * ierror)
{
mpi_type_get_contents_( &mtype,
&max_integers,
&max_addresses,
&max_datatypes,
&array_of_integers,
&array_of_addresses,
&array_of_datatypes,
ierror);
}

void mpi_reduce_local_(
void* inbuf,
void* inoutbuf,
int * count,
MPI_Datatype * datatype,
MPI_Op * op,
int * ierror);
void mpi_reduce_local_f08(
CFI_cdesc_t* inbuf_ptr,
CFI_cdesc_t* inoutbuf_ptr,
int count,
MPI_Datatype datatype,
MPI_Op op,
int * ierror)
{
void* inbuf = inbuf_ptr->base_addr;
void* inoutbuf = inoutbuf_ptr->base_addr;
mpi_reduce_local_( inbuf,
inoutbuf,
&count,
&datatype,
&op,
ierror);
}

void mpi_group_union_(
MPI_Group * group1,
MPI_Group * group2,
MPI_Group* newgroup,
int * ierror);
void mpi_group_union_f08(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
mpi_group_union_( &group1,
&group2,
newgroup,
ierror);
}

void mpi_type_free_(
MPI_Datatype* type,
int * ierror);
void mpi_type_free_f08(
MPI_Datatype* type,
int * ierror)
{
mpi_type_free_( type,
ierror);
}/* Skipped function MPI_Info_get_nthkeywith conversion */

/* MPI_File_write_at_all_begin NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Unpack_externalwith conversion */


void mpi_errhandler_set_(
MPI_Comm * comm,
MPI_Errhandler * errhandler,
int * ierror);
void mpi_errhandler_set_f08(
MPI_Comm comm,
MPI_Errhandler errhandler,
int * ierror)
{
mpi_errhandler_set_( &comm,
&errhandler,
ierror);
}
/* MPI_File_read_at_all_begin NOT IMPLEMENTED in MPC */



void mpi_comm_get_errhandler_(
MPI_Comm * comm,
MPI_Errhandler* erhandler,
int * ierror);
void mpi_comm_get_errhandler_f08(
MPI_Comm comm,
MPI_Errhandler* erhandler,
int * ierror)
{
mpi_comm_get_errhandler_( &comm,
erhandler,
ierror);
}

void mpi_test_cancelled_(
MPI_Status* status,
int* flag,
int * ierror);
void mpi_test_cancelled_f08(
MPI_Status* status,
int* flag,
int * ierror)
{
mpi_test_cancelled_( status,
&flag,
ierror);
}

void mpi_win_lock_(
int * lock_type,
int * rank,
int * assert,
MPI_Win * win,
int * ierror);
void mpi_win_lock_f08(
int lock_type,
int rank,
int assert,
MPI_Win win,
int * ierror)
{
mpi_win_lock_( &lock_type,
&rank,
&assert,
&win,
ierror);
}

void mpi_win_create_(
void* base,
MPI_Aint * size,
int * disp_unit,
MPI_Info * info,
MPI_Comm * comm,
MPI_Win* win,
int * ierror);
void mpi_win_create_f08(
CFI_cdesc_t* base_ptr,
MPI_Aint size,
int disp_unit,
MPI_Info info,
MPI_Comm comm,
MPI_Win* win,
int * ierror)
{
void* base = base_ptr->base_addr;
mpi_win_create_( base,
&size,
&disp_unit,
&info,
&comm,
win,
ierror);
}

void mpi_test_(
MPI_Request* request,
int* flag,
MPI_Status* status,
int * ierror);
void mpi_test_f08(
MPI_Request* request,
int* flag,
MPI_Status* status,
int * ierror)
{
mpi_test_( request,
&flag,
status,
ierror);
}

void mpi_type_create_hvector_(
int * count,
int * blocklength,
MPI_Aint * stride,
MPI_Datatype * oldtype,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_create_hvector_f08(
int count,
int blocklength,
MPI_Aint stride,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_create_hvector_( &count,
&blocklength,
&stride,
&oldtype,
newtype,
ierror);
}
/* MPI_File_write_all_end NOT IMPLEMENTED in MPC */



void mpi_info_get_nkeys_(
MPI_Info * info,
int* nkeys,
int * ierror);
void mpi_info_get_nkeys_f08(
MPI_Info info,
int* nkeys,
int * ierror)
{
mpi_info_get_nkeys_( &info,
nkeys,
ierror);
}

void mpi_win_start_(
MPI_Group * group,
int * assert,
MPI_Win * win,
int * ierror);
void mpi_win_start_f08(
MPI_Group group,
int assert,
MPI_Win win,
int * ierror)
{
mpi_win_start_( &group,
&assert,
&win,
ierror);
}
/* MPI_File_get_size NOT IMPLEMENTED in MPC */



void mpi_finalized_(
int* flag,
int * ierror);
void mpi_finalized_f08(
int* flag,
int * ierror)
{
mpi_finalized_( &flag,
ierror);
}

void mpi_reduce_(
void* sendbuf,
void* recvbuf,
int * count,
MPI_Datatype * datatype,
MPI_Op * op,
int * root,
MPI_Comm * comm,
int * ierror);
void mpi_reduce_f08(
CFI_cdesc_t* sendbuf_ptr,
CFI_cdesc_t* recvbuf_ptr,
int count,
MPI_Datatype datatype,
MPI_Op op,
int root,
MPI_Comm comm,
int * ierror)
{
void* sendbuf = sendbuf_ptr->base_addr;
void* recvbuf = recvbuf_ptr->base_addr;
mpi_reduce_( sendbuf,
recvbuf,
&count,
&datatype,
&op,
&root,
&comm,
ierror);
}

void mpi_win_free_keyval_(
int* win_keyval,
int * ierror);
void mpi_win_free_keyval_f08(
int* win_keyval,
int * ierror)
{
mpi_win_free_keyval_( win_keyval,
ierror);
}

void mpi_waitany_(
int * count,
MPI_Request* array_of_requests,
int* index,
MPI_Status* status,
int * ierror);
void mpi_waitany_f08(
int count,
MPI_Request* array_of_requests,
int* index,
MPI_Status* status,
int * ierror)
{
mpi_waitany_( &count,
&array_of_requests,
index,
status,
ierror);
}/* Skipped function MPI_Open_portwith conversion */


void mpi_type_indexed_(
int * count,
int* array_of_blocklengths,
int* array_of_displacements,
MPI_Datatype * oldtype,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_indexed_f08(
int count,
int* array_of_blocklengths,
int* array_of_displacements,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_indexed_( &count,
&array_of_blocklengths,
&array_of_displacements,
&oldtype,
newtype,
ierror);
}

void mpi_op_commutative_(
MPI_Op * op,
int* commute,
int * ierror);
void mpi_op_commutative_f08(
MPI_Op op,
int* commute,
int * ierror)
{
mpi_op_commutative_( &op,
&commute,
ierror);
}

void mpi_gather_(
void* sendbuf,
int * sendcount,
MPI_Datatype * sendtype,
void* recvbuf,
int * recvcount,
MPI_Datatype * recvtype,
int * root,
MPI_Comm * comm,
int * ierror);
void mpi_gather_f08(
CFI_cdesc_t* sendbuf_ptr,
int sendcount,
MPI_Datatype sendtype,
CFI_cdesc_t* recvbuf_ptr,
int recvcount,
MPI_Datatype recvtype,
int root,
MPI_Comm comm,
int * ierror)
{
void* sendbuf = sendbuf_ptr->base_addr;
void* recvbuf = recvbuf_ptr->base_addr;
mpi_gather_( sendbuf,
&sendcount,
&sendtype,
recvbuf,
&recvcount,
&recvtype,
&root,
&comm,
ierror);
}

void mpi_type_vector_(
int * count,
int * blocklength,
int * stride,
MPI_Datatype * oldtype,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_vector_f08(
int count,
int blocklength,
int stride,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_vector_( &count,
&blocklength,
&stride,
&oldtype,
newtype,
ierror);
}

void mpi_get_elements_(
MPI_Status* status,
MPI_Datatype * datatype,
int* count,
int * ierror);
void mpi_get_elements_f08(
MPI_Status* status,
MPI_Datatype datatype,
int* count,
int * ierror)
{
mpi_get_elements_( status,
&datatype,
count,
ierror);
}
/* MPI_File_write NOT IMPLEMENTED in MPC */



void mpi_comm_get_parent_(
MPI_Comm* parent,
int * ierror);
void mpi_comm_get_parent_f08(
MPI_Comm* parent,
int * ierror)
{
mpi_comm_get_parent_( parent,
ierror);
}
/* MPI_File_read_at_all_end NOT IMPLEMENTED in MPC */



void mpi_probe_(
int * source,
int * tag,
MPI_Comm * comm,
MPI_Status* status,
int * ierror);
void mpi_probe_f08(
int source,
int tag,
MPI_Comm comm,
MPI_Status* status,
int * ierror)
{
mpi_probe_( &source,
&tag,
&comm,
status,
ierror);
}
/* MPI_File_set_view NOT IMPLEMENTED in MPC */



void mpi_unpack_(
void* inbuf,
int * insize,
int* position,
void* outbuf,
int * outcount,
MPI_Datatype * datatype,
MPI_Comm * comm,
int * ierror);
void mpi_unpack_f08(
CFI_cdesc_t* inbuf_ptr,
int insize,
int* position,
CFI_cdesc_t* outbuf_ptr,
int outcount,
MPI_Datatype datatype,
MPI_Comm comm,
int * ierror)
{
void* inbuf = inbuf_ptr->base_addr;
void* outbuf = outbuf_ptr->base_addr;
mpi_unpack_( inbuf,
&insize,
position,
outbuf,
&outcount,
&datatype,
&comm,
ierror);
}

void mpi_type_ub_(
MPI_Datatype * mtype,
MPI_Aint* ub,
int * ierror);
void mpi_type_ub_f08(
MPI_Datatype mtype,
MPI_Aint* ub,
int * ierror)
{
mpi_type_ub_( &mtype,
ub,
ierror);
}

void mpi_status_set_elements_(
MPI_Status* status,
MPI_Datatype * datatype,
int * count,
int * ierror);
void mpi_status_set_elements_f08(
MPI_Status* status,
MPI_Datatype datatype,
int count,
int * ierror)
{
mpi_status_set_elements_( status,
&datatype,
&count,
ierror);
}

void mpi_win_delete_attr_(
MPI_Win * win,
int * win_keyval,
int * ierror);
void mpi_win_delete_attr_f08(
MPI_Win win,
int win_keyval,
int * ierror)
{
mpi_win_delete_attr_( &win,
&win_keyval,
ierror);
}

void mpi_type_hindexed_(
int * count,
int* array_of_blocklengths,
int* array_of_displacements,
MPI_Datatype * oldtype,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_hindexed_f08(
int count,
int* array_of_blocklengths,
int* array_of_displacements,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_hindexed_( &count,
&array_of_blocklengths,
&array_of_displacements,
&oldtype,
newtype,
ierror);
}
/* MPI_File_set_atomicity NOT IMPLEMENTED in MPC */



void mpi_op_free_(
MPI_Op* op,
int * ierror);
void mpi_op_free_f08(
MPI_Op* op,
int * ierror)
{
mpi_op_free_( op,
ierror);
}

void mpi_group_range_incl_(
MPI_Group * group,
int * n,
int** ranges,
MPI_Group* newgroup,
int * ierror);
void mpi_group_range_incl_f08(
MPI_Group group,
int n,
int** ranges,
MPI_Group* newgroup,
int * ierror)
{
mpi_group_range_incl_( &group,
&n,
&ranges,
newgroup,
ierror);
}

void mpi_get_(
void* origin_addr,
int * origin_count,
MPI_Datatype * origin_datatype,
int * target_rank,
MPI_Aint * target_disp,
int * target_count,
MPI_Datatype * target_datatype,
MPI_Win * win,
int * ierror);
void mpi_get_f08(
CFI_cdesc_t* origin_addr_ptr,
int origin_count,
MPI_Datatype origin_datatype,
int target_rank,
MPI_Aint target_disp,
int target_count,
MPI_Datatype target_datatype,
MPI_Win win,
int * ierror)
{
void* origin_addr = origin_addr_ptr->base_addr;
mpi_get_( origin_addr,
&origin_count,
&origin_datatype,
&target_rank,
&target_disp,
&target_count,
&target_datatype,
&win,
ierror);
}

void mpi_iprobe_(
int * source,
int * tag,
MPI_Comm * comm,
int* flag,
MPI_Status* status,
int * ierror);
void mpi_iprobe_f08(
int source,
int tag,
MPI_Comm comm,
int* flag,
MPI_Status* status,
int * ierror)
{
mpi_iprobe_( &source,
&tag,
&comm,
&flag,
status,
ierror);
}
/* MPI_File_write_at_all_end NOT IMPLEMENTED in MPC */



void mpi_type_get_true_extent_(
MPI_Datatype * datatype,
MPI_Aint* true_lb,
MPI_Aint* true_extent,
int * ierror);
void mpi_type_get_true_extent_f08(
MPI_Datatype datatype,
MPI_Aint* true_lb,
MPI_Aint* true_extent,
int * ierror)
{
mpi_type_get_true_extent_( &datatype,
true_lb,
true_extent,
ierror);
}

void mpi_graph_get_(
MPI_Comm * comm,
int * maxindex,
int * maxedges,
int* index,
int* edges,
int * ierror);
void mpi_graph_get_f08(
MPI_Comm comm,
int maxindex,
int maxedges,
int* index,
int* edges,
int * ierror)
{
mpi_graph_get_( &comm,
&maxindex,
&maxedges,
&index,
&edges,
ierror);
}/* Skipped function MPI_Info_deletewith conversion */


void mpi_win_get_attr_(
MPI_Win * win,
int * win_keyval,
void* attribute_val,
int* flag,
int * ierror);
void mpi_win_get_attr_f08(
MPI_Win win,
int win_keyval,
CFI_cdesc_t* attribute_val_ptr,
int* flag,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
mpi_win_get_attr_( &win,
&win_keyval,
attribute_val,
&flag,
ierror);
}/* Skipped function MPI_Cart_rankwith conversion */


void mpi_finalize_(
int * ierror);
void mpi_finalize_f08(
int * ierror)
{
mpi_finalize_( ierror);
}

void mpi_comm_create_(
MPI_Comm * comm,
MPI_Group * group,
MPI_Comm* newcomm,
int * ierror);
void mpi_comm_create_f08(
MPI_Comm comm,
MPI_Group group,
MPI_Comm* newcomm,
int * ierror)
{
mpi_comm_create_( &comm,
&group,
newcomm,
ierror);
}/* Skipped function MPI_Pack_external_sizewith conversion */


void mpi_testall_(
int * count,
MPI_Request* array_of_requests,
int* flag,
MPI_Status* array_of_statuses,
int * ierror);
void mpi_testall_f08(
int count,
MPI_Request* array_of_requests,
int* flag,
MPI_Status* array_of_statuses,
int * ierror)
{
mpi_testall_( &count,
&array_of_requests,
&flag,
&array_of_statuses,
ierror);
}

void mpi_comm_join_(
int * fd,
MPI_Comm* intercomm,
int * ierror);
void mpi_comm_join_f08(
int fd,
MPI_Comm* intercomm,
int * ierror)
{
mpi_comm_join_( &fd,
intercomm,
ierror);
}
/* MPI_File_read_at NOT IMPLEMENTED in MPC */



void mpi_keyval_free_(
int* keyval,
int * ierror);
void mpi_keyval_free_f08(
int* keyval,
int * ierror)
{
mpi_keyval_free_( keyval,
ierror);
}

void mpi_win_wait_(
MPI_Win * win,
int * ierror);
void mpi_win_wait_f08(
MPI_Win win,
int * ierror)
{
mpi_win_wait_( &win,
ierror);
}

void mpi_alloc_mem_(
MPI_Aint * size,
MPI_Info * info,
void* baseptr,
int * ierror);
void mpi_alloc_mem_f08(
MPI_Aint size,
MPI_Info info,
CFI_cdesc_t* baseptr_ptr,
int * ierror)
{
void* baseptr = baseptr_ptr->base_addr;
mpi_alloc_mem_( &size,
&info,
baseptr,
ierror);
}

void mpi_improbe_(
int * source,
int * tag,
MPI_Comm * comm,
int* flag,
MPI_Message* message,
MPI_Status* status,
int * ierror);
void mpi_improbe_f08(
int source,
int tag,
MPI_Comm comm,
int* flag,
MPI_Message* message,
MPI_Status* status,
int * ierror)
{
mpi_improbe_( &source,
&tag,
&comm,
&flag,
message,
status,
ierror);
}

void mpi_type_size_(
MPI_Datatype * type,
int* size,
int * ierror);
void mpi_type_size_f08(
MPI_Datatype type,
int* size,
int * ierror)
{
mpi_type_size_( &type,
size,
ierror);
}/* Skipped function MPI_Lookup_namewith conversion */

/* MPI_File_get_atomicity NOT IMPLEMENTED in MPC */



void mpi_mprobe_(
int * source,
int * tag,
MPI_Comm * comm,
MPI_Message* message,
MPI_Status* status,
int * ierror);
void mpi_mprobe_f08(
int source,
int tag,
MPI_Comm comm,
MPI_Message* message,
MPI_Status* status,
int * ierror)
{
mpi_mprobe_( &source,
&tag,
&comm,
message,
status,
ierror);
}
/* MPI_File_read_shared NOT IMPLEMENTED in MPC */



void mpi_type_create_darray_(
int * size,
int * rank,
int * ndims,
int* gsize_array,
int* distrib_array,
int* darg_array,
int* psize_array,
int * order,
MPI_Datatype * oldtype,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_create_darray_f08(
int size,
int rank,
int ndims,
int* gsize_array,
int* distrib_array,
int* darg_array,
int* psize_array,
int order,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_create_darray_( &size,
&rank,
&ndims,
&gsize_array,
&distrib_array,
&darg_array,
&psize_array,
&order,
&oldtype,
newtype,
ierror);
}

void mpi_win_create_errhandler_(
MPI_Win_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror);
void mpi_win_create_errhandler_f08(
MPI_Win_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
mpi_win_create_errhandler_( function,
errhandler,
ierror);
}

void mpi_cart_map_(
MPI_Comm * comm,
int * ndims,
int* dims,
int** periods,
int* newrank,
int * ierror);
void mpi_cart_map_f08(
MPI_Comm comm,
int ndims,
int* dims,
int** periods,
int* newrank,
int * ierror)
{
mpi_cart_map_( &comm,
&ndims,
&dims,
&periods,
newrank,
ierror);
}
/* MPI_File_write_at NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Comm_acceptwith conversion */
/* Skipped function MPI_Type_set_namewith conversion */
/* Skipped function MPI_Close_portwith conversion */

/* MPI_File_close NOT IMPLEMENTED in MPC */



void mpi_type_create_subarray_(
int * ndims,
int* size_array,
int* subsize_array,
int* start_array,
int * order,
MPI_Datatype * oldtype,
MPI_Datatype* newtype,
int * ierror);
void mpi_type_create_subarray_f08(
int ndims,
int* size_array,
int* subsize_array,
int* start_array,
int order,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
mpi_type_create_subarray_( &ndims,
&size_array,
&subsize_array,
&start_array,
&order,
&oldtype,
newtype,
ierror);
}
/* MPI_File_delete NOT IMPLEMENTED in MPC */


/* MPI_File_get_byte_offset NOT IMPLEMENTED in MPC */


/* MPI_File_read_all NOT IMPLEMENTED in MPC */



void mpi_recv_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * source,
int * tag,
MPI_Comm * comm,
MPI_Status* status,
int * ierror);
void mpi_recv_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int source,
int tag,
MPI_Comm comm,
MPI_Status* status,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_recv_( buf,
&count,
&datatype,
&source,
&tag,
&comm,
status,
ierror);
}

void mpi_comm_dup_(
MPI_Comm * comm,
MPI_Comm* newcomm,
int * ierror);
void mpi_comm_dup_f08(
MPI_Comm comm,
MPI_Comm* newcomm,
int * ierror)
{
mpi_comm_dup_( &comm,
newcomm,
ierror);
}

void mpi_waitall_(
int * count,
MPI_Request* array_of_requests,
MPI_Status* array_of_statuses,
int * ierror);
void mpi_waitall_f08(
int count,
MPI_Request* array_of_requests,
MPI_Status* array_of_statuses,
int * ierror)
{
mpi_waitall_( &count,
&array_of_requests,
&array_of_statuses,
ierror);
}

void mpi_comm_delete_attr_(
MPI_Comm * comm,
int * comm_keyval,
int * ierror);
void mpi_comm_delete_attr_f08(
MPI_Comm comm,
int comm_keyval,
int * ierror)
{
mpi_comm_delete_attr_( &comm,
&comm_keyval,
ierror);
}

void mpi_ssend_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
int * ierror);
void mpi_ssend_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_ssend_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
ierror);
}

void mpi_group_size_(
MPI_Group * group,
int* size,
int * ierror);
void mpi_group_size_f08(
MPI_Group group,
int* size,
int * ierror)
{
mpi_group_size_( &group,
size,
ierror);
}

void mpi_file_get_errhandler_(
MPI_File * file,
MPI_Errhandler* errhandler,
int * ierror);
void mpi_file_get_errhandler_f08(
MPI_File file,
MPI_Errhandler* errhandler,
int * ierror)
{
mpi_file_get_errhandler_( &file,
errhandler,
ierror);
}

void mpi_attr_put_(
MPI_Comm * comm,
int * keyval,
void* attribute_val,
int * ierror);
void mpi_attr_put_f08(
MPI_Comm comm,
int keyval,
CFI_cdesc_t* attribute_val_ptr,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
mpi_attr_put_( &comm,
&keyval,
attribute_val,
ierror);
}

void mpi_barrier_(
MPI_Comm * comm,
int * ierror);
void mpi_barrier_f08(
MPI_Comm comm,
int * ierror)
{
mpi_barrier_( &comm,
ierror);
}

void mpi_win_free_(
MPI_Win* win,
int * ierror);
void mpi_win_free_f08(
MPI_Win* win,
int * ierror)
{
mpi_win_free_( win,
ierror);
}/* Skipped function MPI_Alltoallwwith conversion */
/* Skipped function MPI_Alltoallvwith conversion */


void mpi_bsend_init_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
MPI_Request* request,
int * ierror);
void mpi_bsend_init_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_bsend_init_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
request,
ierror);
}

void mpi_exscan_(
void* sendbuf,
void* recvbuf,
int * count,
MPI_Datatype * datatype,
MPI_Op * op,
MPI_Comm * comm,
int * ierror);
void mpi_exscan_f08(
CFI_cdesc_t* sendbuf_ptr,
CFI_cdesc_t* recvbuf_ptr,
int count,
MPI_Datatype datatype,
MPI_Op op,
MPI_Comm comm,
int * ierror)
{
void* sendbuf = sendbuf_ptr->base_addr;
void* recvbuf = recvbuf_ptr->base_addr;
mpi_exscan_( sendbuf,
recvbuf,
&count,
&datatype,
&op,
&comm,
ierror);
}/* Skipped function MPI_Cart_subwith conversion */


void mpi_win_set_errhandler_(
MPI_Win * win,
MPI_Errhandler * errhandler,
int * ierror);
void mpi_win_set_errhandler_f08(
MPI_Win win,
MPI_Errhandler errhandler,
int * ierror)
{
mpi_win_set_errhandler_( &win,
&errhandler,
ierror);
}

void mpi_accumulate_(
void* origin_addr,
int * origin_count,
MPI_Datatype * origin_datatype,
int * target_rank,
MPI_Aint * target_disp,
int * target_count,
MPI_Datatype * target_datatype,
MPI_Op * op,
MPI_Win * win,
int * ierror);
void mpi_accumulate_f08(
CFI_cdesc_t* origin_addr_ptr,
int origin_count,
MPI_Datatype origin_datatype,
int target_rank,
MPI_Aint target_disp,
int target_count,
MPI_Datatype target_datatype,
MPI_Op op,
MPI_Win win,
int * ierror)
{
void* origin_addr = origin_addr_ptr->base_addr;
mpi_accumulate_( origin_addr,
&origin_count,
&origin_datatype,
&target_rank,
&target_disp,
&target_count,
&target_datatype,
&op,
&win,
ierror);
}/* Skipped function MPI_Comm_get_namewith conversion */


void mpi_file_create_errhandler_(
MPI_File_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror);
void mpi_file_create_errhandler_f08(
MPI_File_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
mpi_file_create_errhandler_( function,
errhandler,
ierror);
}
/* MPI_File_write_ordered NOT IMPLEMENTED in MPC */



void mpi_group_range_excl_(
MPI_Group * group,
int * n,
int** ranges,
MPI_Group* newgroup,
int * ierror);
void mpi_group_range_excl_f08(
MPI_Group group,
int n,
int** ranges,
MPI_Group* newgroup,
int * ierror)
{
mpi_group_range_excl_( &group,
&n,
&ranges,
newgroup,
ierror);
}

void mpi_comm_split_(
MPI_Comm * comm,
int * color,
int * key,
MPI_Comm* newcomm,
int * ierror);
void mpi_comm_split_f08(
MPI_Comm comm,
int color,
int key,
MPI_Comm* newcomm,
int * ierror)
{
mpi_comm_split_( &comm,
&color,
&key,
newcomm,
ierror);
}

void mpi_comm_set_errhandler_(
MPI_Comm * comm,
MPI_Errhandler * errhandler,
int * ierror);
void mpi_comm_set_errhandler_f08(
MPI_Comm comm,
MPI_Errhandler errhandler,
int * ierror)
{
mpi_comm_set_errhandler_( &comm,
&errhandler,
ierror);
}

void mpi_request_get_status_(
MPI_Request * request,
int* flag,
MPI_Status* status,
int * ierror);
void mpi_request_get_status_f08(
MPI_Request request,
int* flag,
MPI_Status* status,
int * ierror)
{
mpi_request_get_status_( &request,
&flag,
status,
ierror);
}

void mpi_group_free_(
MPI_Group* group,
int * ierror);
void mpi_group_free_f08(
MPI_Group* group,
int * ierror)
{
mpi_group_free_( group,
ierror);
}

void mpi_type_create_keyval_(
MPI_Type_copy_attr_function* type_copy_attr_fn,
MPI_Type_delete_attr_function* type_delete_attr_fn,
int* type_keyval,
void* extra_state,
int * ierror);
void mpi_type_create_keyval_f08(
MPI_Type_copy_attr_function* type_copy_attr_fn,
MPI_Type_delete_attr_function* type_delete_attr_fn,
int* type_keyval,
CFI_cdesc_t* extra_state_ptr,
int * ierror)
{
void* extra_state = extra_state_ptr->base_addr;
mpi_type_create_keyval_( type_copy_attr_fn,
type_delete_attr_fn,
type_keyval,
extra_state,
ierror);
}
/* MPI_File_write_ordered_begin NOT IMPLEMENTED in MPC */


/* MPI_File_read_ordered_begin NOT IMPLEMENTED in MPC */



void mpi_graphdims_get_(
MPI_Comm * comm,
int* nnodes,
int* nedges,
int * ierror);
void mpi_graphdims_get_f08(
MPI_Comm comm,
int* nnodes,
int* nedges,
int * ierror)
{
mpi_graphdims_get_( &comm,
nnodes,
nedges,
ierror);
}

void mpi_comm_rank_(
MPI_Comm * comm,
int* rank,
int * ierror);
void mpi_comm_rank_f08(
MPI_Comm comm,
int* rank,
int * ierror)
{
mpi_comm_rank_( &comm,
rank,
ierror);
}/* Skipped function MPI_Session_get_nth_psetwith conversion */


void mpi_cancel_(
MPI_Request* request,
int * ierror);
void mpi_cancel_f08(
MPI_Request* request,
int * ierror)
{
mpi_cancel_( request,
ierror);
}

void mpi_win_fence_(
int * assert,
MPI_Win * win,
int * ierror);
void mpi_win_fence_f08(
int assert,
MPI_Win win,
int * ierror)
{
mpi_win_fence_( &assert,
&win,
ierror);
}

void mpi_session_init_(
MPI_Info * info,
MPI_Errhandler * errh,
MPI_Session* session,
int * ierror);
void mpi_session_init_f08(
MPI_Info info,
MPI_Errhandler errh,
MPI_Session* session,
int * ierror)
{
mpi_session_init_( &info,
&errh,
session,
ierror);
}

void mpi_errhandler_free_(
MPI_Errhandler* errhandler,
int * ierror);
void mpi_errhandler_free_f08(
MPI_Errhandler* errhandler,
int * ierror)
{
mpi_errhandler_free_( errhandler,
ierror);
}

void mpi_win_test_(
MPI_Win * win,
int* flag,
int * ierror);
void mpi_win_test_f08(
MPI_Win win,
int* flag,
int * ierror)
{
mpi_win_test_( &win,
&flag,
ierror);
}

void mpi_type_match_size_(
int * typeclass,
int * size,
MPI_Datatype* type,
int * ierror);
void mpi_type_match_size_f08(
int typeclass,
int size,
MPI_Datatype* type,
int * ierror)
{
mpi_type_match_size_( &typeclass,
&size,
type,
ierror);
}

void mpi_send_(
void* buf,
int * count,
MPI_Datatype * datatype,
int * dest,
int * tag,
MPI_Comm * comm,
int * ierror);
void mpi_send_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
void* buf = buf_ptr->base_addr;
mpi_send_( buf,
&count,
&datatype,
&dest,
&tag,
&comm,
ierror);
}