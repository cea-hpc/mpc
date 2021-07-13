
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


void mpi_start_f08(
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Start( request);
}

void mpi_imrecv_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
MPI_Message* message,
MPI_Request* status,
int * ierror)
{
void* buf = buf_ptr->base_addr;
*ierror = MPI_Imrecv( buf,
count,
datatype,
message,
status);
}
/* MPI_File_write_all_begin NOT IMPLEMENTED in MPC */



void mpi_win_post_f08(
MPI_Group group,
int assert,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_post( group,
assert,
win);
}

void mpi_session_create_errhandler_f08(
MPI_Session_errhandler_function* errhfn,
MPI_Errhandler* errh,
int * ierror)
{
*ierror = MPI_Session_create_errhandler( errhfn,
errh);
}

void mpi_win_get_errhandler_f08(
MPI_Win win,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Win_get_errhandler( win,
errhandler);
}
/* MPI_File_get_group NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Sendrecv( sendbuf,
sendcount,
sendtype,
dest,
sendtag,
recvbuf,
recvcount,
recvtype,
source,
recvtag,
comm,
status);
}

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
*ierror = MPI_Scan( sendbuf,
recvbuf,
count,
datatype,
op,
comm);
}

void mpi_startall_f08(
int count,
MPI_Request* array_of_requests,
int * ierror)
{
*ierror = MPI_Startall( count,
array_of_requests);
}

void mpi_attr_delete_f08(
MPI_Comm comm,
int keyval,
int * ierror)
{
*ierror = MPI_Attr_delete( comm,
keyval);
}
/* MPI_File_iwrite_shared NOT IMPLEMENTED in MPC */



void mpi_comm_get_attr_f08(
MPI_Comm comm,
int comm_keyval,
CFI_cdesc_t* attribute_val_ptr,
int* flag,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
*ierror = MPI_Comm_get_attr( comm,
comm_keyval,
attribute_val,
flag);
}
/* MPI_File_get_info NOT IMPLEMENTED in MPC */



void mpi_type_delete_attr_f08(
MPI_Datatype type,
int type_keyval,
int * ierror)
{
*ierror = MPI_Type_delete_attr( type,
type_keyval);
}

void mpi_error_class_f08(
int errorcode,
int* errorclass,
int * ierror)
{
*ierror = MPI_Error_class( errorcode,
errorclass);
}

void mpi_session_get_num_psets_f08(
MPI_Session session,
MPI_Info info,
int* npset_names,
int * ierror)
{
*ierror = MPI_Session_get_num_psets( session,
info,
npset_names);
}

void mpi_free_mem_f08(
CFI_cdesc_t* base_ptr,
int * ierror)
{
void* base = base_ptr->base_addr;
*ierror = MPI_Free_mem( base);
}

void mpi_info_dup_f08(
MPI_Info info,
MPI_Info* newinfo,
int * ierror)
{
*ierror = MPI_Info_dup( info,
newinfo);
}

void mpi_type_lb_f08(
MPI_Datatype type,
MPI_Aint* lb,
int * ierror)
{
*ierror = MPI_Type_lb( type,
lb);
}

void mpi_cart_get_f08(
MPI_Comm comm,
int maxdims,
int* dims,
int** periods,
int* coords,
int * ierror)
{
*ierror = MPI_Cart_get( comm,
maxdims,
dims,
periods,
coords);
}

void mpi_add_error_class_f08(
int* errorclass,
int * ierror)
{
*ierror = MPI_Add_error_class( errorclass);
}
/* MPI_File_write_shared NOT IMPLEMENTED in MPC */



void mpi_buffer_detach_f08(
CFI_cdesc_t* buffer_ptr,
int* size,
int * ierror)
{
void* buffer = buffer_ptr->base_addr;
*ierror = MPI_Buffer_detach( buffer,
size);
}
/* MPI_File_set_size NOT IMPLEMENTED in MPC */



void mpi_intercomm_create_f08(
MPI_Comm local_comm,
int local_leader,
MPI_Comm bridge_comm,
int remote_leader,
int tag,
MPI_Comm* newintercomm,
int * ierror)
{
*ierror = MPI_Intercomm_create( local_comm,
local_leader,
bridge_comm,
remote_leader,
tag,
newintercomm);
}/* Skipped function MPI_Group_from_session_psetwith conversion */

/* MPI_File_iread_at NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Allreduce( sendbuf,
recvbuf,
count,
datatype,
op,
comm);
}

void mpi_comm_create_keyval_f08(
MPI_Comm_copy_attr_function* comm_copy_attr_fn,
MPI_Comm_delete_attr_function* comm_delete_attr_fn,
int* comm_keyval,
CFI_cdesc_t* extra_state_ptr,
int * ierror)
{
void* extra_state = extra_state_ptr->base_addr;
*ierror = MPI_Comm_create_keyval( comm_copy_attr_fn,
comm_delete_attr_fn,
comm_keyval,
extra_state);
}

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
*ierror = MPI_Ibsend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}
/* MPI_File_read_all_end NOT IMPLEMENTED in MPC */



void mpi_comm_remote_size_f08(
MPI_Comm comm,
int* size,
int * ierror)
{
*ierror = MPI_Comm_remote_size( comm,
size);
}

void mpi_type_contiguous_f08(
int count,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_contiguous( count,
oldtype,
newtype);
}

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
*ierror = MPI_Send_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}/* Skipped function MPI_Type_get_namewith conversion */


void mpi_session_get_info_f08(
MPI_Session session,
MPI_Info* infoused,
int * ierror)
{
*ierror = MPI_Session_get_info( session,
infoused);
}

void mpi_type_commit_f08(
MPI_Datatype* type,
int * ierror)
{
*ierror = MPI_Type_commit( type);
}

void mpi_session_set_errhandler_f08(
MPI_Session session,
MPI_Errhandler errh,
int * ierror)
{
*ierror = MPI_Session_set_errhandler( session,
errh);
}

void mpi_type_create_f90_integer_f08(
int r,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_f90_integer( r,
newtype);
}

void mpi_testany_f08(
int count,
MPI_Request* array_of_requests,
int* index,
int* flag,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Testany( count,
array_of_requests,
index,
flag,
status);
}

void mpi_type_extent_f08(
MPI_Datatype type,
MPI_Aint* extent,
int * ierror)
{
*ierror = MPI_Type_extent( type,
extent);
}
/* MPI_File_preallocate NOT IMPLEMENTED in MPC */


/* MPI_File_get_position NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Sendrecv_replace( buf,
count,
datatype,
dest,
sendtag,
source,
recvtag,
comm,
status);
}

void mpi_type_get_extent_f08(
MPI_Datatype type,
MPI_Aint* lb,
MPI_Aint* extent,
int * ierror)
{
*ierror = MPI_Type_get_extent( type,
lb,
extent);
}

void mpi_keyval_create_f08(
MPI_Copy_function* copy_fn,
MPI_Delete_function* delete_fn,
int* keyval,
CFI_cdesc_t* extra_state_ptr,
int * ierror)
{
void* extra_state = extra_state_ptr->base_addr;
*ierror = MPI_Keyval_create( copy_fn,
delete_fn,
keyval,
extra_state);
}

void mpi_mrecv_f08(
CFI_cdesc_t* buf_ptr,
int count,
MPI_Datatype datatype,
MPI_Message* message,
MPI_Status* status,
int * ierror)
{
void* buf = buf_ptr->base_addr;
*ierror = MPI_Mrecv( buf,
count,
datatype,
message,
status);
}

void mpi_type_hvector_f08(
int count,
int blocklength,
int stride,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_hvector( count,
blocklength,
stride,
oldtype,
newtype);
}

void mpi_group_translate_ranks_f08(
MPI_Group group1,
int n,
int* ranks1,
MPI_Group group2,
int* ranks2,
int * ierror)
{
*ierror = MPI_Group_translate_ranks( group1,
n,
ranks1,
group2,
ranks2);
}/* Skipped function MPI_Testsomewith conversion */


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
*ierror = MPI_Recv_init( buf,
count,
datatype,
source,
tag,
comm,
request);
}/* Skipped function MPI_Win_set_namewith conversion */


void mpi_type_dup_f08(
MPI_Datatype type,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_dup( type,
newtype);
}

void mpi_comm_group_f08(
MPI_Comm comm,
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Comm_group( comm,
group);
}

void mpi_add_error_code_f08(
int errorclass,
int* errorcode,
int * ierror)
{
*ierror = MPI_Add_error_code( errorclass,
errorcode);
}

void mpi_type_create_resized_f08(
MPI_Datatype oldtype,
MPI_Aint lb,
MPI_Aint extent,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_resized( oldtype,
lb,
extent,
newtype);
}
/* MPI_File_seek_shared NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Unpublish_namewith conversion */


void mpi_get_address_f08(
CFI_cdesc_t* location_ptr,
MPI_Aint* address,
int * ierror)
{
void* location = location_ptr->base_addr;
*ierror = MPI_Get_address( location,
address);
}

void mpi_is_thread_main_f08(
int* flag,
int * ierror)
{
*ierror = MPI_Is_thread_main( flag);
}

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
*ierror = MPI_Irecv( buf,
count,
datatype,
source,
tag,
comm,
request);
}

void mpi_type_create_indexed_block_f08(
int count,
int blocklength,
int* array_of_displacements,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_indexed_block( count,
blocklength,
array_of_displacements,
oldtype,
newtype);
}

void mpi_initialized_f08(
int* flag,
int * ierror)
{
*ierror = MPI_Initialized( flag);
}
/* MPI_File_iwrite NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Bsend( buf,
count,
datatype,
dest,
tag,
comm);
}

void mpi_group_excl_f08(
MPI_Group group,
int n,
int* ranks,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_excl( group,
n,
ranks,
newgroup);
}

void mpi_get_count_f08(
MPI_Status* status,
MPI_Datatype datatype,
int* count,
int * ierror)
{
*ierror = MPI_Get_count( status,
datatype,
count);
}/* Skipped function MPI_Error_stringwith conversion */


void mpi_grequest_start_f08(
MPI_Grequest_query_function* query_fn,
MPI_Grequest_free_function* free_fn,
MPI_Grequest_cancel_function* cancel_fn,
CFI_cdesc_t* extra_state_ptr,
MPI_Request* request,
int * ierror)
{
void* extra_state = extra_state_ptr->base_addr;
*ierror = MPI_Grequest_start( query_fn,
free_fn,
cancel_fn,
extra_state,
request);
}

void mpi_cartdim_get_f08(
MPI_Comm comm,
int* ndims,
int * ierror)
{
*ierror = MPI_Cartdim_get( comm,
ndims);
}

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
*ierror = MPI_Allgather( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
comm);
}

void mpi_cart_coords_f08(
MPI_Comm comm,
int rank,
int maxdims,
int* coords,
int * ierror)
{
*ierror = MPI_Cart_coords( comm,
rank,
maxdims,
coords);
}/* Skipped function MPI_Scattervwith conversion */


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
*ierror = MPI_Issend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}
/* MPI_File_sync NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Rsend( ibuf,
count,
datatype,
dest,
tag,
comm);
}
/* MPI_File_get_amode NOT IMPLEMENTED in MPC */



void mpi_abort_f08(
MPI_Comm comm,
int errorcode,
int * ierror)
{
*ierror = MPI_Abort( comm,
errorcode);
}

void mpi_session_call_errhandler_f08(
MPI_Session session,
int errorcode,
int * ierror)
{
*ierror = MPI_Session_call_errhandler( session,
errorcode);
}

void mpi_grequest_complete_f08(
MPI_Request request,
int * ierror)
{
*ierror = MPI_Grequest_complete( request);
}

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
*ierror = MPI_Pack( inbuf,
incount,
datatype,
outbuf,
outsize,
position,
comm);
}

void mpi_win_set_attr_f08(
MPI_Win win,
int win_keyval,
CFI_cdesc_t* attribute_val_ptr,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
*ierror = MPI_Win_set_attr( win,
win_keyval,
attribute_val);
}

void mpi_info_create_f08(
MPI_Info* info,
int * ierror)
{
*ierror = MPI_Info_create( info);
}
/* MPI_File_open NOT IMPLEMENTED in MPC */



void mpi_type_create_f90_complex_f08(
int p,
int r,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_f90_complex( p,
r,
newtype);
}/* Skipped function MPI_Gathervwith conversion */


void mpi_type_get_attr_f08(
MPI_Datatype type,
int type_keyval,
CFI_cdesc_t* attribute_val_ptr,
int* flag,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
*ierror = MPI_Type_get_attr( type,
type_keyval,
attribute_val,
flag);
}/* Skipped function MPI_Comm_set_namewith conversion */


void mpi_comm_disconnect_f08(
MPI_Comm* comm,
int * ierror)
{
*ierror = MPI_Comm_disconnect( comm);
}

void mpi_comm_remote_group_f08(
MPI_Comm comm,
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Comm_remote_group( comm,
group);
}

void mpi_cart_shift_f08(
MPI_Comm comm,
int direction,
int disp,
int* rank_source,
int* rank_dest,
int * ierror)
{
*ierror = MPI_Cart_shift( comm,
direction,
disp,
rank_source,
rank_dest);
}

void mpi_group_incl_f08(
MPI_Group group,
int n,
int* ranks,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_incl( group,
n,
ranks,
newgroup);
}

void mpi_comm_size_f08(
MPI_Comm comm,
int* size,
int * ierror)
{
*ierror = MPI_Comm_size( comm,
size);
}

void mpi_type_create_hindexed_f08(
int count,
int* array_of_blocklengths,
MPI_Aint* array_of_displacements,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_hindexed( count,
array_of_blocklengths,
array_of_displacements,
oldtype,
newtype);
}
/* MPI_File_iread_shared NOT IMPLEMENTED in MPC */



void mpi_file_set_errhandler_f08(
MPI_File file,
MPI_Errhandler errhandler,
int * ierror)
{
*ierror = MPI_File_set_errhandler( file,
errhandler);
}/* Skipped function MPI_Register_datarepwith conversion */

/* MPI_File_read_ordered NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Waitsomewith conversion */


void mpi_group_difference_f08(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_difference( group1,
group2,
newgroup);
}

void mpi_attr_get_f08(
MPI_Comm comm,
int keyval,
CFI_cdesc_t* attribute_val_ptr,
int* flag,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
*ierror = MPI_Attr_get( comm,
keyval,
attribute_val,
flag);
}/* Skipped function MPI_Reduce_scatterwith conversion */
/* Skipped function MPI_Win_get_namewith conversion */


void mpi_type_create_f90_real_f08(
int p,
int r,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_f90_real( p,
r,
newtype);
}

void mpi_win_create_keyval_f08(
MPI_Win_copy_attr_function* win_copy_attr_fn,
MPI_Win_delete_attr_function* win_delete_attr_fn,
int* win_keyval,
CFI_cdesc_t* extra_state_ptr,
int * ierror)
{
void* extra_state = extra_state_ptr->base_addr;
*ierror = MPI_Win_create_keyval( win_copy_attr_fn,
win_delete_attr_fn,
win_keyval,
extra_state);
}

void mpi_wait_f08(
MPI_Request* request,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Wait( request,
status);
}

void mpi_session_get_errhandler_f08(
MPI_Session session,
MPI_Errhandler* errh,
int * ierror)
{
*ierror = MPI_Session_get_errhandler( session,
errh);
}
/* MPI_File_write_all NOT IMPLEMENTED in MPC */


/* MPI_File_set_info NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Irsend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void mpi_get_version_f08(
int* version,
int* subversion,
int * ierror)
{
*ierror = MPI_Get_version( version,
subversion);
}

void mpi_file_call_errhandler_f08(
MPI_File fh,
int errorcode,
int * ierror)
{
*ierror = MPI_File_call_errhandler( fh,
errorcode);
}

void mpi_comm_create_errhandler_f08(
MPI_Comm_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Comm_create_errhandler( function,
errhandler);
}/* Skipped function MPI_Session_get_pset_infowith conversion */
/* Skipped function MPI_Comm_connectwith conversion */


void mpi_group_compare_f08(
MPI_Group group1,
MPI_Group group2,
int* result,
int * ierror)
{
*ierror = MPI_Group_compare( group1,
group2,
result);
}

void mpi_address_f08(
CFI_cdesc_t* location_ptr,
MPI_Aint* address,
int * ierror)
{
void* location = location_ptr->base_addr;
*ierror = MPI_Address( location,
address);
}

void mpi_comm_compare_f08(
MPI_Comm comm1,
MPI_Comm comm2,
int* result,
int * ierror)
{
*ierror = MPI_Comm_compare( comm1,
comm2,
result);
}

void mpi_win_unlock_f08(
int rank,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_unlock( rank,
win);
}/* Skipped function MPI_Graph_mapwith conversion */


void mpi_request_free_f08(
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Request_free( request);
}

void mpi_topo_test_f08(
MPI_Comm comm,
int* status,
int * ierror)
{
*ierror = MPI_Topo_test( comm,
status);
}
/* MPI_File_read NOT IMPLEMENTED in MPC */



void mpi_query_thread_f08(
int* provided,
int * ierror)
{
*ierror = MPI_Query_thread( provided);
}

void mpi_win_call_errhandler_f08(
MPI_Win win,
int errorcode,
int * ierror)
{
*ierror = MPI_Win_call_errhandler( win,
errorcode);
}

void mpi_win_get_group_f08(
MPI_Win win,
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Win_get_group( win,
group);
}

void mpi_errhandler_create_f08(
MPI_Handler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_create( function,
errhandler);
}

void mpi_cart_create_f08(
MPI_Comm old_comm,
int ndims,
int* dims,
int** periods,
int* reorder,
MPI_Comm* comm_cart,
int * ierror)
{
*ierror = MPI_Cart_create( old_comm,
ndims,
dims,
periods,
reorder,
comm_cart);
}

void mpi_status_set_cancelled_f08(
MPI_Status* status,
int* flag,
int * ierror)
{
*ierror = MPI_Status_set_cancelled( status,
flag);
}
/* MPI_Status_f2c NOT IMPLEMENTED in MPC */



void mpi_type_struct_f08(
int count,
int* array_of_blocklengths,
int* array_of_displacements,
MPI_Datatype* array_of_types,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_struct( count,
array_of_blocklengths,
array_of_displacements,
array_of_types,
newtype);
}

void mpi_graph_neighbors_count_f08(
MPI_Comm comm,
int rank,
int* nneighbors,
int * ierror)
{
*ierror = MPI_Graph_neighbors_count( comm,
rank,
nneighbors);
}
/* MPI_File_get_view NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Allgathervwith conversion */

/* MPI_File_get_position_shared NOT IMPLEMENTED in MPC */



void mpi_graph_neighbors_f08(
MPI_Comm comm,
int rank,
int maxneighbors,
int* neighbors,
int * ierror)
{
*ierror = MPI_Graph_neighbors( comm,
rank,
maxneighbors,
neighbors);
}

void mpi_dims_create_f08(
int nnodes,
int ndims,
int* dims,
int * ierror)
{
*ierror = MPI_Dims_create( nnodes,
ndims,
dims);
}
/* MPI_File_iread NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Scatter( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
root,
comm);
}

void mpi_comm_set_attr_f08(
MPI_Comm comm,
int comm_keyval,
CFI_cdesc_t* attribute_val_ptr,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
*ierror = MPI_Comm_set_attr( comm,
comm_keyval,
attribute_val);
}

void mpi_comm_free_keyval_f08(
int* comm_keyval,
int * ierror)
{
*ierror = MPI_Comm_free_keyval( comm_keyval);
}

void mpi_op_create_f08(
MPI_User_function* function,
int* commute,
MPI_Op* op,
int * ierror)
{
*ierror = MPI_Op_create( function,
commute,
op);
}
/* MPI_File_seek NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Add_error_stringwith conversion */


void mpi_session_finalize_f08(
MPI_Session* session,
int * ierror)
{
*ierror = MPI_Session_finalize( session);
}

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
*ierror = MPI_Ssend_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

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
*ierror = MPI_Rsend_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void mpi_info_free_f08(
MPI_Info* info,
int * ierror)
{
*ierror = MPI_Info_free( info);
}/* Skipped function MPI_Publish_namewith conversion */


void mpi_bcast_f08(
CFI_cdesc_t* buffer_ptr,
int count,
MPI_Datatype datatype,
int root,
MPI_Comm comm,
int * ierror)
{
void* buffer = buffer_ptr->base_addr;
*ierror = MPI_Bcast( buffer,
count,
datatype,
root,
comm);
}/* Skipped function MPI_Get_processor_namewith conversion */
/* Skipped function MPI_Info_setwith conversion */

/* MPI_File_write_ordered_end NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Graph_createwith conversion */


void mpi_comm_free_f08(
MPI_Comm* comm,
int * ierror)
{
*ierror = MPI_Comm_free( comm);
}
/* MPI_File_write_at_all NOT IMPLEMENTED in MPC */



void mpi_errhandler_get_f08(
MPI_Comm comm,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_get( comm,
errhandler);
}

void mpi_pack_size_f08(
int incount,
MPI_Datatype datatype,
MPI_Comm comm,
int* size,
int * ierror)
{
*ierror = MPI_Pack_size( incount,
datatype,
comm,
size);
}

void mpi_comm_call_errhandler_f08(
MPI_Comm comm,
int errorcode,
int * ierror)
{
*ierror = MPI_Comm_call_errhandler( comm,
errorcode);
}

void mpi_comm_test_inter_f08(
MPI_Comm comm,
int* flag,
int * ierror)
{
*ierror = MPI_Comm_test_inter( comm,
flag);
}/* Skipped function MPI_Comm_create_from_groupwith conversion */


void mpi_intercomm_merge_f08(
MPI_Comm intercomm,
int* high,
MPI_Comm* newintercomm,
int * ierror)
{
*ierror = MPI_Intercomm_merge( intercomm,
high,
newintercomm);
}

void mpi_win_complete_f08(
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_complete( win);
}/* Skipped function MPI_Pack_externalwith conversion */

/* MPI_File_get_type_extent NOT IMPLEMENTED in MPC */


/* MPI_File_read_all_begin NOT IMPLEMENTED in MPC */



void mpi_type_set_attr_f08(
MPI_Datatype type,
int type_keyval,
CFI_cdesc_t* attr_val_ptr,
int * ierror)
{
void* attr_val = attr_val_ptr->base_addr;
*ierror = MPI_Type_set_attr( type,
type_keyval,
attr_val);
}/* Skipped function MPI_Info_get_valuelenwith conversion */


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
*ierror = MPI_Put( origin_addr,
origin_count,
origin_datatype,
target_rank,
target_disp,
target_count,
target_datatype,
win);
}
/* MPI_File_read_at_all NOT IMPLEMENTED in MPC */


/* MPI_File_read_ordered_end NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Isend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void mpi_type_get_envelope_f08(
MPI_Datatype type,
int* num_integers,
int* num_addresses,
int* num_datatypes,
int* combiner,
int * ierror)
{
*ierror = MPI_Type_get_envelope( type,
num_integers,
num_addresses,
num_datatypes,
combiner);
}

void mpi_group_rank_f08(
MPI_Group group,
int* rank,
int * ierror)
{
*ierror = MPI_Group_rank( group,
rank);
}

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
*ierror = MPI_Alltoall( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
comm);
}

void mpi_buffer_attach_f08(
CFI_cdesc_t* buffer_ptr,
int size,
int * ierror)
{
void* buffer = buffer_ptr->base_addr;
*ierror = MPI_Buffer_attach( buffer,
size);
}/* Skipped function MPI_Info_getwith conversion */

/* MPI_File_iwrite_at NOT IMPLEMENTED in MPC */



void mpi_group_intersection_f08(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_intersection( group1,
group2,
newgroup);
}

void mpi_type_free_keyval_f08(
int* type_keyval,
int * ierror)
{
*ierror = MPI_Type_free_keyval( type_keyval);
}

void mpi_type_create_struct_f08(
int count,
int* array_of_block_lengths,
MPI_Aint* array_of_displacements,
MPI_Datatype* array_of_types,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_struct( count,
array_of_block_lengths,
array_of_displacements,
array_of_types,
newtype);
}

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
*ierror = MPI_Type_get_contents( mtype,
max_integers,
max_addresses,
max_datatypes,
array_of_integers,
array_of_addresses,
array_of_datatypes);
}

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
*ierror = MPI_Reduce_local( inbuf,
inoutbuf,
count,
datatype,
op);
}

void mpi_group_union_f08(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_union( group1,
group2,
newgroup);
}

void mpi_type_free_f08(
MPI_Datatype* type,
int * ierror)
{
*ierror = MPI_Type_free( type);
}/* Skipped function MPI_Info_get_nthkeywith conversion */

/* MPI_File_write_at_all_begin NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Unpack_externalwith conversion */


void mpi_errhandler_set_f08(
MPI_Comm comm,
MPI_Errhandler errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_set( comm,
errhandler);
}
/* MPI_File_read_at_all_begin NOT IMPLEMENTED in MPC */



void mpi_comm_get_errhandler_f08(
MPI_Comm comm,
MPI_Errhandler* erhandler,
int * ierror)
{
*ierror = MPI_Comm_get_errhandler( comm,
erhandler);
}

void mpi_test_cancelled_f08(
MPI_Status* status,
int* flag,
int * ierror)
{
*ierror = MPI_Test_cancelled( status,
flag);
}

void mpi_win_lock_f08(
int lock_type,
int rank,
int assert,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_lock( lock_type,
rank,
assert,
win);
}

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
*ierror = MPI_Win_create( base,
size,
disp_unit,
info,
comm,
win);
}

void mpi_test_f08(
MPI_Request* request,
int* flag,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Test( request,
flag,
status);
}

void mpi_type_create_hvector_f08(
int count,
int blocklength,
MPI_Aint stride,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_hvector( count,
blocklength,
stride,
oldtype,
newtype);
}
/* MPI_File_write_all_end NOT IMPLEMENTED in MPC */



void mpi_info_get_nkeys_f08(
MPI_Info info,
int* nkeys,
int * ierror)
{
*ierror = MPI_Info_get_nkeys( info,
nkeys);
}

void mpi_win_start_f08(
MPI_Group group,
int assert,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_start( group,
assert,
win);
}
/* MPI_File_get_size NOT IMPLEMENTED in MPC */



void mpi_finalized_f08(
int* flag,
int * ierror)
{
*ierror = MPI_Finalized( flag);
}

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
*ierror = MPI_Reduce( sendbuf,
recvbuf,
count,
datatype,
op,
root,
comm);
}

void mpi_win_free_keyval_f08(
int* win_keyval,
int * ierror)
{
*ierror = MPI_Win_free_keyval( win_keyval);
}

void mpi_waitany_f08(
int count,
MPI_Request* array_of_requests,
int* index,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Waitany( count,
array_of_requests,
index,
status);
}/* Skipped function MPI_Open_portwith conversion */


void mpi_type_indexed_f08(
int count,
int* array_of_blocklengths,
int* array_of_displacements,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_indexed( count,
array_of_blocklengths,
array_of_displacements,
oldtype,
newtype);
}

void mpi_op_commutative_f08(
MPI_Op op,
int* commute,
int * ierror)
{
*ierror = MPI_Op_commutative( op,
commute);
}

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
*ierror = MPI_Gather( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
root,
comm);
}

void mpi_type_vector_f08(
int count,
int blocklength,
int stride,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_vector( count,
blocklength,
stride,
oldtype,
newtype);
}

void mpi_get_elements_f08(
MPI_Status* status,
MPI_Datatype datatype,
int* count,
int * ierror)
{
*ierror = MPI_Get_elements( status,
datatype,
count);
}
/* MPI_File_write NOT IMPLEMENTED in MPC */



void mpi_comm_get_parent_f08(
MPI_Comm* parent,
int * ierror)
{
*ierror = MPI_Comm_get_parent( parent);
}
/* MPI_File_read_at_all_end NOT IMPLEMENTED in MPC */



void mpi_probe_f08(
int source,
int tag,
MPI_Comm comm,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Probe( source,
tag,
comm,
status);
}
/* MPI_File_set_view NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Unpack( inbuf,
insize,
position,
outbuf,
outcount,
datatype,
comm);
}

void mpi_type_ub_f08(
MPI_Datatype mtype,
MPI_Aint* ub,
int * ierror)
{
*ierror = MPI_Type_ub( mtype,
ub);
}

void mpi_status_set_elements_f08(
MPI_Status* status,
MPI_Datatype datatype,
int count,
int * ierror)
{
*ierror = MPI_Status_set_elements( status,
datatype,
count);
}

void mpi_win_delete_attr_f08(
MPI_Win win,
int win_keyval,
int * ierror)
{
*ierror = MPI_Win_delete_attr( win,
win_keyval);
}

void mpi_type_hindexed_f08(
int count,
int* array_of_blocklengths,
int* array_of_displacements,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_hindexed( count,
array_of_blocklengths,
array_of_displacements,
oldtype,
newtype);
}
/* MPI_File_set_atomicity NOT IMPLEMENTED in MPC */



void mpi_op_free_f08(
MPI_Op* op,
int * ierror)
{
*ierror = MPI_Op_free( op);
}

void mpi_group_range_incl_f08(
MPI_Group group,
int n,
int** ranges,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_range_incl( group,
n,
ranges,
newgroup);
}

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
*ierror = MPI_Get( origin_addr,
origin_count,
origin_datatype,
target_rank,
target_disp,
target_count,
target_datatype,
win);
}

void mpi_iprobe_f08(
int source,
int tag,
MPI_Comm comm,
int* flag,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Iprobe( source,
tag,
comm,
flag,
status);
}
/* MPI_File_write_at_all_end NOT IMPLEMENTED in MPC */



void mpi_type_get_true_extent_f08(
MPI_Datatype datatype,
MPI_Aint* true_lb,
MPI_Aint* true_extent,
int * ierror)
{
*ierror = MPI_Type_get_true_extent( datatype,
true_lb,
true_extent);
}

void mpi_graph_get_f08(
MPI_Comm comm,
int maxindex,
int maxedges,
int* index,
int* edges,
int * ierror)
{
*ierror = MPI_Graph_get( comm,
maxindex,
maxedges,
index,
edges);
}/* Skipped function MPI_Info_deletewith conversion */


void mpi_win_get_attr_f08(
MPI_Win win,
int win_keyval,
CFI_cdesc_t* attribute_val_ptr,
int* flag,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
*ierror = MPI_Win_get_attr( win,
win_keyval,
attribute_val,
flag);
}/* Skipped function MPI_Cart_rankwith conversion */


void mpi_finalize_f08(
int * ierror)
{
*ierror = MPI_Finalize( );
}

void mpi_comm_create_f08(
MPI_Comm comm,
MPI_Group group,
MPI_Comm* newcomm,
int * ierror)
{
*ierror = MPI_Comm_create( comm,
group,
newcomm);
}/* Skipped function MPI_Pack_external_sizewith conversion */


void mpi_testall_f08(
int count,
MPI_Request* array_of_requests,
int* flag,
MPI_Status* array_of_statuses,
int * ierror)
{
*ierror = MPI_Testall( count,
array_of_requests,
flag,
array_of_statuses);
}

void mpi_comm_join_f08(
int fd,
MPI_Comm* intercomm,
int * ierror)
{
*ierror = MPI_Comm_join( fd,
intercomm);
}
/* MPI_File_read_at NOT IMPLEMENTED in MPC */



void mpi_keyval_free_f08(
int* keyval,
int * ierror)
{
*ierror = MPI_Keyval_free( keyval);
}

void mpi_win_wait_f08(
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_wait( win);
}

void mpi_alloc_mem_f08(
MPI_Aint size,
MPI_Info info,
CFI_cdesc_t* baseptr_ptr,
int * ierror)
{
void* baseptr = baseptr_ptr->base_addr;
*ierror = MPI_Alloc_mem( size,
info,
baseptr);
}

void mpi_improbe_f08(
int source,
int tag,
MPI_Comm comm,
int* flag,
MPI_Message* message,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Improbe( source,
tag,
comm,
flag,
message,
status);
}

void mpi_type_size_f08(
MPI_Datatype type,
int* size,
int * ierror)
{
*ierror = MPI_Type_size( type,
size);
}/* Skipped function MPI_Lookup_namewith conversion */

/* MPI_File_get_atomicity NOT IMPLEMENTED in MPC */



void mpi_mprobe_f08(
int source,
int tag,
MPI_Comm comm,
MPI_Message* message,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Mprobe( source,
tag,
comm,
message,
status);
}
/* MPI_File_read_shared NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Type_create_darray( size,
rank,
ndims,
gsize_array,
distrib_array,
darg_array,
psize_array,
order,
oldtype,
newtype);
}

void mpi_win_create_errhandler_f08(
MPI_Win_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Win_create_errhandler( function,
errhandler);
}

void mpi_cart_map_f08(
MPI_Comm comm,
int ndims,
int* dims,
int** periods,
int* newrank,
int * ierror)
{
*ierror = MPI_Cart_map( comm,
ndims,
dims,
periods,
newrank);
}
/* MPI_File_write_at NOT IMPLEMENTED in MPC */

/* Skipped function MPI_Comm_acceptwith conversion */
/* Skipped function MPI_Type_set_namewith conversion */
/* Skipped function MPI_Close_portwith conversion */

/* MPI_File_close NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Type_create_subarray( ndims,
size_array,
subsize_array,
start_array,
order,
oldtype,
newtype);
}
/* MPI_File_delete NOT IMPLEMENTED in MPC */


/* MPI_File_get_byte_offset NOT IMPLEMENTED in MPC */


/* MPI_File_read_all NOT IMPLEMENTED in MPC */



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
*ierror = MPI_Recv( buf,
count,
datatype,
source,
tag,
comm,
status);
}

void mpi_comm_dup_f08(
MPI_Comm comm,
MPI_Comm* newcomm,
int * ierror)
{
*ierror = MPI_Comm_dup( comm,
newcomm);
}

void mpi_waitall_f08(
int count,
MPI_Request* array_of_requests,
MPI_Status* array_of_statuses,
int * ierror)
{
*ierror = MPI_Waitall( count,
array_of_requests,
array_of_statuses);
}

void mpi_comm_delete_attr_f08(
MPI_Comm comm,
int comm_keyval,
int * ierror)
{
*ierror = MPI_Comm_delete_attr( comm,
comm_keyval);
}

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
*ierror = MPI_Ssend( buf,
count,
datatype,
dest,
tag,
comm);
}

void mpi_group_size_f08(
MPI_Group group,
int* size,
int * ierror)
{
*ierror = MPI_Group_size( group,
size);
}

void mpi_file_get_errhandler_f08(
MPI_File file,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_File_get_errhandler( file,
errhandler);
}

void mpi_attr_put_f08(
MPI_Comm comm,
int keyval,
CFI_cdesc_t* attribute_val_ptr,
int * ierror)
{
void* attribute_val = attribute_val_ptr->base_addr;
*ierror = MPI_Attr_put( comm,
keyval,
attribute_val);
}

void mpi_barrier_f08(
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Barrier( comm);
}

void mpi_win_free_f08(
MPI_Win* win,
int * ierror)
{
*ierror = MPI_Win_free( win);
}/* Skipped function MPI_Alltoallwwith conversion */
/* Skipped function MPI_Alltoallvwith conversion */


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
*ierror = MPI_Bsend_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

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
*ierror = MPI_Exscan( sendbuf,
recvbuf,
count,
datatype,
op,
comm);
}/* Skipped function MPI_Cart_subwith conversion */


void mpi_win_set_errhandler_f08(
MPI_Win win,
MPI_Errhandler errhandler,
int * ierror)
{
*ierror = MPI_Win_set_errhandler( win,
errhandler);
}

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
*ierror = MPI_Accumulate( origin_addr,
origin_count,
origin_datatype,
target_rank,
target_disp,
target_count,
target_datatype,
op,
win);
}/* Skipped function MPI_Comm_get_namewith conversion */


void mpi_file_create_errhandler_f08(
MPI_File_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_File_create_errhandler( function,
errhandler);
}
/* MPI_File_write_ordered NOT IMPLEMENTED in MPC */



void mpi_group_range_excl_f08(
MPI_Group group,
int n,
int** ranges,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_range_excl( group,
n,
ranges,
newgroup);
}

void mpi_comm_split_f08(
MPI_Comm comm,
int color,
int key,
MPI_Comm* newcomm,
int * ierror)
{
*ierror = MPI_Comm_split( comm,
color,
key,
newcomm);
}

void mpi_comm_set_errhandler_f08(
MPI_Comm comm,
MPI_Errhandler errhandler,
int * ierror)
{
*ierror = MPI_Comm_set_errhandler( comm,
errhandler);
}

void mpi_request_get_status_f08(
MPI_Request request,
int* flag,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Request_get_status( request,
flag,
status);
}

void mpi_group_free_f08(
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Group_free( group);
}

void mpi_type_create_keyval_f08(
MPI_Type_copy_attr_function* type_copy_attr_fn,
MPI_Type_delete_attr_function* type_delete_attr_fn,
int* type_keyval,
CFI_cdesc_t* extra_state_ptr,
int * ierror)
{
void* extra_state = extra_state_ptr->base_addr;
*ierror = MPI_Type_create_keyval( type_copy_attr_fn,
type_delete_attr_fn,
type_keyval,
extra_state);
}
/* MPI_File_write_ordered_begin NOT IMPLEMENTED in MPC */


/* MPI_File_read_ordered_begin NOT IMPLEMENTED in MPC */



void mpi_graphdims_get_f08(
MPI_Comm comm,
int* nnodes,
int* nedges,
int * ierror)
{
*ierror = MPI_Graphdims_get( comm,
nnodes,
nedges);
}

void mpi_comm_rank_f08(
MPI_Comm comm,
int* rank,
int * ierror)
{
*ierror = MPI_Comm_rank( comm,
rank);
}/* Skipped function MPI_Session_get_nth_psetwith conversion */


void mpi_cancel_f08(
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Cancel( request);
}

void mpi_win_fence_f08(
int assert,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_fence( assert,
win);
}

void mpi_session_init_f08(
MPI_Info info,
MPI_Errhandler errh,
MPI_Session* session,
int * ierror)
{
*ierror = MPI_Session_init( info,
errh,
session);
}

void mpi_errhandler_free_f08(
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_free( errhandler);
}

void mpi_win_test_f08(
MPI_Win win,
int* flag,
int * ierror)
{
*ierror = MPI_Win_test( win,
flag);
}

void mpi_type_match_size_f08(
int typeclass,
int size,
MPI_Datatype* type,
int * ierror)
{
*ierror = MPI_Type_match_size( typeclass,
size,
type);
}

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
*ierror = MPI_Send( buf,
count,
datatype,
dest,
tag,
comm);
}