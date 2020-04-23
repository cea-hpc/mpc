
#include <mpi.h>
#include <sctk_alloc.h>

static inline char *sctk_char_fortran_to_c(char *buf, int size, char **free_ptr)
{
	char *   tmp;
	long int i;

	tmp = sctk_malloc(size + 1);
	assume(tmp != NULL);
	*free_ptr = tmp;

	for(i = 0; i < size; i++)
	{
		tmp[i] = buf[i];
	}
	tmp[i] = ' ';

	/* Trim */

	while(*tmp == ' ')
	{
		tmp++;
	}

	size_t len = strlen(tmp);

	char *begin = tmp;

	while( (tmp[len - 1] == ' ') && (&tmp[len] != begin) )
	{
		tmp[len - 1] = ' ';
		len--;
	}

	return tmp;
}

static inline void sctk_char_c_to_fortran(char *buf, int size)
{
	size_t i;

	for(i = strlen(buf); i < size; i++)
	{
		buf[i] = ' ';
	}
}
#if defined(USE_CHAR_MIXED)
#define CHAR_END(thename)
#define CHAR_MIXED(thename) , long int thename
#else
#define CHAR_END(thename) long int thename
#define CHAR_MIXED(thename)
#endif




 /*********MPI_Start**************/



void pmpi_start_(
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Start( request);
}

void pmpi_start__(
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Start( request);
}

 /*********MPI_Win_post**************/



void pmpi_win_post_(
MPI_Group group,
int assert,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_post( group,
assert,
win);
}

void pmpi_win_post__(
MPI_Group group,
int assert,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_post( group,
assert,
win);
}

 /*********MPI_Win_get_errhandler**************/



void pmpi_win_get_errhandler_(
MPI_Win win,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Win_get_errhandler( win,
errhandler);
}

void pmpi_win_get_errhandler__(
MPI_Win win,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Win_get_errhandler( win,
errhandler);
}

 /*********MPI_Sendrecv**************/



void pmpi_sendrecv_(
void* sendbuf,
int sendcount,
MPI_Datatype sendtype,
int dest,
int sendtag,
void* recvbuf,
int recvcount,
MPI_Datatype recvtype,
int source,
int recvtag,
MPI_Comm comm,
MPI_Status* status,
int * ierror)
{
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

void pmpi_sendrecv__(
void* sendbuf,
int sendcount,
MPI_Datatype sendtype,
int dest,
int sendtag,
void* recvbuf,
int recvcount,
MPI_Datatype recvtype,
int source,
int recvtag,
MPI_Comm comm,
MPI_Status* status,
int * ierror)
{
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

 /*********MPI_Scan**************/



void pmpi_scan_(
void* sendbuf,
void* recvbuf,
int count,
MPI_Datatype datatype,
MPI_Op op,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Scan( sendbuf,
recvbuf,
count,
datatype,
op,
comm);
}

void pmpi_scan__(
void* sendbuf,
void* recvbuf,
int count,
MPI_Datatype datatype,
MPI_Op op,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Scan( sendbuf,
recvbuf,
count,
datatype,
op,
comm);
}

 /*********MPI_Startall**************/



void pmpi_startall_(
int count,
MPI_Request* array_of_requests,
int * ierror)
{
*ierror = MPI_Startall( count,
array_of_requests);
}

void pmpi_startall__(
int count,
MPI_Request* array_of_requests,
int * ierror)
{
*ierror = MPI_Startall( count,
array_of_requests);
}

 /*********MPI_Attr_delete**************/



void pmpi_attr_delete_(
MPI_Comm comm,
int keyval,
int * ierror)
{
*ierror = MPI_Attr_delete( comm,
keyval);
}

void pmpi_attr_delete__(
MPI_Comm comm,
int keyval,
int * ierror)
{
*ierror = MPI_Attr_delete( comm,
keyval);
}

 /*********MPI_Comm_get_attr**************/



void pmpi_comm_get_attr_(
MPI_Comm comm,
int comm_keyval,
void* attribute_val,
int* flag,
int * ierror)
{
*ierror = MPI_Comm_get_attr( comm,
comm_keyval,
attribute_val,
flag);
}

void pmpi_comm_get_attr__(
MPI_Comm comm,
int comm_keyval,
void* attribute_val,
int* flag,
int * ierror)
{
*ierror = MPI_Comm_get_attr( comm,
comm_keyval,
attribute_val,
flag);
}

 /*********MPI_Type_delete_attr**************/



void pmpi_type_delete_attr_(
MPI_Datatype type,
int type_keyval,
int * ierror)
{
*ierror = MPI_Type_delete_attr( type,
type_keyval);
}

void pmpi_type_delete_attr__(
MPI_Datatype type,
int type_keyval,
int * ierror)
{
*ierror = MPI_Type_delete_attr( type,
type_keyval);
}

 /*********MPI_Error_class**************/



void pmpi_error_class_(
int errorcode,
int* errorclass,
int * ierror)
{
*ierror = MPI_Error_class( errorcode,
errorclass);
}

void pmpi_error_class__(
int errorcode,
int* errorclass,
int * ierror)
{
*ierror = MPI_Error_class( errorcode,
errorclass);
}

 /*********MPI_Free_mem**************/



void pmpi_free_mem_(
void* base,
int * ierror)
{
*ierror = MPI_Free_mem( base);
}

void pmpi_free_mem__(
void* base,
int * ierror)
{
*ierror = MPI_Free_mem( base);
}

 /*********MPI_Info_dup**************/



void pmpi_info_dup_(
MPI_Info info,
MPI_Info* newinfo,
int * ierror)
{
*ierror = MPI_Info_dup( info,
newinfo);
}

void pmpi_info_dup__(
MPI_Info info,
MPI_Info* newinfo,
int * ierror)
{
*ierror = MPI_Info_dup( info,
newinfo);
}

 /*********MPI_Type_lb**************/



void pmpi_type_lb_(
MPI_Datatype type,
MPI_Aint* lb,
int * ierror)
{
*ierror = MPI_Type_lb( type,
lb);
}

void pmpi_type_lb__(
MPI_Datatype type,
MPI_Aint* lb,
int * ierror)
{
*ierror = MPI_Type_lb( type,
lb);
}

 /*********MPI_Cart_get**************/



void pmpi_cart_get_(
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

void pmpi_cart_get__(
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

 /*********MPI_Add_error_class**************/



void pmpi_add_error_class_(
int* errorclass,
int * ierror)
{
*ierror = MPI_Add_error_class( errorclass);
}

void pmpi_add_error_class__(
int* errorclass,
int * ierror)
{
*ierror = MPI_Add_error_class( errorclass);
}

 /*********MPI_Buffer_detach**************/



void pmpi_buffer_detach_(
void* buffer,
int* size,
int * ierror)
{
*ierror = MPI_Buffer_detach( buffer,
size);
}

void pmpi_buffer_detach__(
void* buffer,
int* size,
int * ierror)
{
*ierror = MPI_Buffer_detach( buffer,
size);
}

 /*********MPI_Intercomm_create**************/



void pmpi_intercomm_create_(
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
}

void pmpi_intercomm_create__(
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
}

 /*********MPI_Allreduce**************/



void pmpi_allreduce_(
void* sendbuf,
void* recvbuf,
int count,
MPI_Datatype datatype,
MPI_Op op,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Allreduce( sendbuf,
recvbuf,
count,
datatype,
op,
comm);
}

void pmpi_allreduce__(
void* sendbuf,
void* recvbuf,
int count,
MPI_Datatype datatype,
MPI_Op op,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Allreduce( sendbuf,
recvbuf,
count,
datatype,
op,
comm);
}

 /*********MPI_Comm_create_keyval**************/



void pmpi_comm_create_keyval_(
MPI_Comm_copy_attr_function* comm_copy_attr_fn,
MPI_Comm_delete_attr_function* comm_delete_attr_fn,
int* comm_keyval,
void* extra_state,
int * ierror)
{
*ierror = MPI_Comm_create_keyval( comm_copy_attr_fn,
comm_delete_attr_fn,
comm_keyval,
extra_state);
}

void pmpi_comm_create_keyval__(
MPI_Comm_copy_attr_function* comm_copy_attr_fn,
MPI_Comm_delete_attr_function* comm_delete_attr_fn,
int* comm_keyval,
void* extra_state,
int * ierror)
{
*ierror = MPI_Comm_create_keyval( comm_copy_attr_fn,
comm_delete_attr_fn,
comm_keyval,
extra_state);
}

 /*********MPI_Ibsend**************/



void pmpi_ibsend_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Ibsend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void pmpi_ibsend__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Ibsend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

 /*********MPI_Comm_remote_size**************/



void pmpi_comm_remote_size_(
MPI_Comm comm,
int* size,
int * ierror)
{
*ierror = MPI_Comm_remote_size( comm,
size);
}

void pmpi_comm_remote_size__(
MPI_Comm comm,
int* size,
int * ierror)
{
*ierror = MPI_Comm_remote_size( comm,
size);
}

 /*********MPI_Type_contiguous**************/



void pmpi_type_contiguous_(
int count,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_contiguous( count,
oldtype,
newtype);
}

void pmpi_type_contiguous__(
int count,
MPI_Datatype oldtype,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_contiguous( count,
oldtype,
newtype);
}

 /*********MPI_Send_init**************/



void pmpi_send_init_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Send_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void pmpi_send_init__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Send_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

 /*********MPI_Type_get_name**************/



void pmpi_type_get_name_(
MPI_Datatype type,
char* type_name CHAR_MIXED(size_type_name),
int* resultlen,
int * ierror, CHAR_END(size_type_name))
{
*ierror = MPI_Type_get_name( type,
type_name,
resultlen);
sctk_char_c_to_fortran(type_name, size_type_name);
}

void pmpi_type_get_name__(
MPI_Datatype type,
char* type_name CHAR_MIXED(size_type_name),
int* resultlen,
int * ierror, CHAR_END(size_type_name))
{
*ierror = MPI_Type_get_name( type,
type_name,
resultlen);
sctk_char_c_to_fortran(type_name, size_type_name);
}

 /*********MPI_Reduce**************/



void pmpi_reduce_(
void* sendbuf,
void* recvbuf,
int count,
MPI_Datatype datatype,
MPI_Op op,
int root,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Reduce( sendbuf,
recvbuf,
count,
datatype,
op,
root,
comm);
}

void pmpi_reduce__(
void* sendbuf,
void* recvbuf,
int count,
MPI_Datatype datatype,
MPI_Op op,
int root,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Reduce( sendbuf,
recvbuf,
count,
datatype,
op,
root,
comm);
}

 /*********MPI_Type_commit**************/



void pmpi_type_commit_(
MPI_Datatype* type,
int * ierror)
{
*ierror = MPI_Type_commit( type);
}

void pmpi_type_commit__(
MPI_Datatype* type,
int * ierror)
{
*ierror = MPI_Type_commit( type);
}

 /*********MPI_Comm_get_parent**************/



void pmpi_comm_get_parent_(
MPI_Comm* parent,
int * ierror)
{
*ierror = MPI_Comm_get_parent( parent);
}

void pmpi_comm_get_parent__(
MPI_Comm* parent,
int * ierror)
{
*ierror = MPI_Comm_get_parent( parent);
}

 /*********MPI_Type_create_f90_integer**************/



void pmpi_type_create_f90_integer_(
int r,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_f90_integer( r,
newtype);
}

void pmpi_type_create_f90_integer__(
int r,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_f90_integer( r,
newtype);
}

 /*********MPI_Testany**************/



void pmpi_testany_(
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

void pmpi_testany__(
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

 /*********MPI_Type_extent**************/



void pmpi_type_extent_(
MPI_Datatype type,
MPI_Aint* extent,
int * ierror)
{
*ierror = MPI_Type_extent( type,
extent);
}

void pmpi_type_extent__(
MPI_Datatype type,
MPI_Aint* extent,
int * ierror)
{
*ierror = MPI_Type_extent( type,
extent);
}

 /*********MPI_Sendrecv_replace**************/



void pmpi_sendrecv_replace_(
void* buf,
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

void pmpi_sendrecv_replace__(
void* buf,
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

 /*********MPI_Type_get_extent**************/



void pmpi_type_get_extent_(
MPI_Datatype type,
MPI_Aint* lb,
MPI_Aint* extent,
int * ierror)
{
*ierror = MPI_Type_get_extent( type,
lb,
extent);
}

void pmpi_type_get_extent__(
MPI_Datatype type,
MPI_Aint* lb,
MPI_Aint* extent,
int * ierror)
{
*ierror = MPI_Type_get_extent( type,
lb,
extent);
}

 /*********MPI_Keyval_create**************/



void pmpi_keyval_create_(
MPI_Copy_function* copy_fn,
MPI_Delete_function* delete_fn,
int* keyval,
void* extra_state,
int * ierror)
{
*ierror = MPI_Keyval_create( copy_fn,
delete_fn,
keyval,
extra_state);
}

void pmpi_keyval_create__(
MPI_Copy_function* copy_fn,
MPI_Delete_function* delete_fn,
int* keyval,
void* extra_state,
int * ierror)
{
*ierror = MPI_Keyval_create( copy_fn,
delete_fn,
keyval,
extra_state);
}

 /*********MPI_Mrecv**************/



void pmpi_mrecv_(
void* buf,
int count,
MPI_Datatype datatype,
MPI_Message* message,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Mrecv( buf,
count,
datatype,
message,
status);
}

void pmpi_mrecv__(
void* buf,
int count,
MPI_Datatype datatype,
MPI_Message* message,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Mrecv( buf,
count,
datatype,
message,
status);
}

 /*********MPI_Type_hvector**************/



void pmpi_type_hvector_(
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

void pmpi_type_hvector__(
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

 /*********MPI_Group_translate_ranks**************/



void pmpi_group_translate_ranks_(
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
}

void pmpi_group_translate_ranks__(
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
}

 /*********MPI_Testsome**************/

/* Skipped function MPI_Testsomewith conversion */
/* Skipped function MPI_Testsomewith conversion */


 /*********MPI_Recv_init**************/



void pmpi_recv_init_(
void* buf,
int count,
MPI_Datatype datatype,
int source,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Recv_init( buf,
count,
datatype,
source,
tag,
comm,
request);
}

void pmpi_recv_init__(
void* buf,
int count,
MPI_Datatype datatype,
int source,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Recv_init( buf,
count,
datatype,
source,
tag,
comm,
request);
}

 /*********MPI_Win_set_name**************/



void pmpi_win_set_name_(
MPI_Win win,
char* win_name CHAR_MIXED(size_win_name),
int * ierror, CHAR_END(size_win_name))
{
char *tmp_win_name, *ptr_win_name;
tmp_win_name = sctk_char_fortran_to_c(win_name, size_win_name, &ptr_win_name);
*ierror = MPI_Win_set_name( win,
win_name);
sctk_free(ptr_win_name);
}

void pmpi_win_set_name__(
MPI_Win win,
char* win_name CHAR_MIXED(size_win_name),
int * ierror, CHAR_END(size_win_name))
{
char *tmp_win_name, *ptr_win_name;
tmp_win_name = sctk_char_fortran_to_c(win_name, size_win_name, &ptr_win_name);
*ierror = MPI_Win_set_name( win,
win_name);
sctk_free(ptr_win_name);
}

 /*********MPI_Type_dup**************/



void pmpi_type_dup_(
MPI_Datatype type,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_dup( type,
newtype);
}

void pmpi_type_dup__(
MPI_Datatype type,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_dup( type,
newtype);
}

 /*********MPI_Query_thread**************/



void pmpi_query_thread_(
int* provided,
int * ierror)
{
*ierror = MPI_Query_thread( provided);
}

void pmpi_query_thread__(
int* provided,
int * ierror)
{
*ierror = MPI_Query_thread( provided);
}

 /*********MPI_Comm_group**************/



void pmpi_comm_group_(
MPI_Comm comm,
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Comm_group( comm,
group);
}

void pmpi_comm_group__(
MPI_Comm comm,
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Comm_group( comm,
group);
}

 /*********MPI_Add_error_code**************/



void pmpi_add_error_code_(
int errorclass,
int* errorcode,
int * ierror)
{
*ierror = MPI_Add_error_code( errorclass,
errorcode);
}

void pmpi_add_error_code__(
int errorclass,
int* errorcode,
int * ierror)
{
*ierror = MPI_Add_error_code( errorclass,
errorcode);
}

 /*********MPI_Type_create_resized**************/



void pmpi_type_create_resized_(
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

void pmpi_type_create_resized__(
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

 /*********MPI_Unpublish_name**************/



void pmpi_unpublish_name_(
char* service_name CHAR_MIXED(size_service_name),
MPI_Info info,
char* port_name CHAR_MIXED(size_port_name),
int * ierror, CHAR_END(size_service_name), CHAR_END(size_port_name))
{
char *tmp_service_name, *ptr_service_name;
tmp_service_name = sctk_char_fortran_to_c(service_name, size_service_name, &ptr_service_name);
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Unpublish_name( service_name,
info,
port_name);
sctk_free(ptr_service_name);
sctk_free(ptr_port_name);
}

void pmpi_unpublish_name__(
char* service_name CHAR_MIXED(size_service_name),
MPI_Info info,
char* port_name CHAR_MIXED(size_port_name),
int * ierror, CHAR_END(size_service_name), CHAR_END(size_port_name))
{
char *tmp_service_name, *ptr_service_name;
tmp_service_name = sctk_char_fortran_to_c(service_name, size_service_name, &ptr_service_name);
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Unpublish_name( service_name,
info,
port_name);
sctk_free(ptr_service_name);
sctk_free(ptr_port_name);
}

 /*********MPI_Get_address**************/



void pmpi_get_address_(
void* location,
MPI_Aint* address,
int * ierror)
{
*ierror = MPI_Get_address( location,
address);
}

void pmpi_get_address__(
void* location,
MPI_Aint* address,
int * ierror)
{
*ierror = MPI_Get_address( location,
address);
}

 /*********MPI_Is_thread_main**************/



void pmpi_is_thread_main_(
int* flag,
int * ierror)
{
*ierror = MPI_Is_thread_main( flag);
}

void pmpi_is_thread_main__(
int* flag,
int * ierror)
{
*ierror = MPI_Is_thread_main( flag);
}

 /*********MPI_Irecv**************/



void pmpi_irecv_(
void* buf,
int count,
MPI_Datatype datatype,
int source,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Irecv( buf,
count,
datatype,
source,
tag,
comm,
request);
}

void pmpi_irecv__(
void* buf,
int count,
MPI_Datatype datatype,
int source,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Irecv( buf,
count,
datatype,
source,
tag,
comm,
request);
}

 /*********MPI_Type_create_indexed_block**************/



void pmpi_type_create_indexed_block_(
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

void pmpi_type_create_indexed_block__(
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

 /*********MPI_Initialized**************/



void pmpi_initialized_(
int* flag,
int * ierror)
{
*ierror = MPI_Initialized( flag);
}

void pmpi_initialized__(
int* flag,
int * ierror)
{
*ierror = MPI_Initialized( flag);
}

 /*********MPI_Bsend**************/



void pmpi_bsend_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Bsend( buf,
count,
datatype,
dest,
tag,
comm);
}

void pmpi_bsend__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Bsend( buf,
count,
datatype,
dest,
tag,
comm);
}

 /*********MPI_Group_excl**************/



void pmpi_group_excl_(
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

void pmpi_group_excl__(
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

 /*********MPI_Get_count**************/



void pmpi_get_count_(
MPI_Status* status,
MPI_Datatype datatype,
int* count,
int * ierror)
{
*ierror = MPI_Get_count( status,
datatype,
count);
}

void pmpi_get_count__(
MPI_Status* status,
MPI_Datatype datatype,
int* count,
int * ierror)
{
*ierror = MPI_Get_count( status,
datatype,
count);
}

 /*********MPI_Error_string**************/



void pmpi_error_string_(
int errorcode,
char* string CHAR_MIXED(size_string),
int* resultlen,
int * ierror, CHAR_END(size_string))
{
*ierror = MPI_Error_string( errorcode,
string,
resultlen);
sctk_char_c_to_fortran(string, size_string);
}

void pmpi_error_string__(
int errorcode,
char* string CHAR_MIXED(size_string),
int* resultlen,
int * ierror, CHAR_END(size_string))
{
*ierror = MPI_Error_string( errorcode,
string,
resultlen);
sctk_char_c_to_fortran(string, size_string);
}

 /*********MPI_Grequest_start**************/



void pmpi_grequest_start_(
MPI_Grequest_query_function* query_fn,
MPI_Grequest_free_function* free_fn,
MPI_Grequest_cancel_function* cancel_fn,
void* extra_state,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Grequest_start( query_fn,
free_fn,
cancel_fn,
extra_state,
request);
}

void pmpi_grequest_start__(
MPI_Grequest_query_function* query_fn,
MPI_Grequest_free_function* free_fn,
MPI_Grequest_cancel_function* cancel_fn,
void* extra_state,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Grequest_start( query_fn,
free_fn,
cancel_fn,
extra_state,
request);
}

 /*********MPI_Cartdim_get**************/



void pmpi_cartdim_get_(
MPI_Comm comm,
int* ndims,
int * ierror)
{
*ierror = MPI_Cartdim_get( comm,
ndims);
}

void pmpi_cartdim_get__(
MPI_Comm comm,
int* ndims,
int * ierror)
{
*ierror = MPI_Cartdim_get( comm,
ndims);
}

 /*********MPI_Allgather**************/



void pmpi_allgather_(
void* sendbuf,
int sendcount,
MPI_Datatype sendtype,
void* recvbuf,
int recvcount,
MPI_Datatype recvtype,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Allgather( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
comm);
}

void pmpi_allgather__(
void* sendbuf,
int sendcount,
MPI_Datatype sendtype,
void* recvbuf,
int recvcount,
MPI_Datatype recvtype,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Allgather( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
comm);
}

 /*********MPI_Cart_coords**************/



void pmpi_cart_coords_(
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
}

void pmpi_cart_coords__(
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
}

 /*********MPI_Scatterv**************/

/* Skipped function MPI_Scattervwith conversion */
/* Skipped function MPI_Scattervwith conversion */


 /*********MPI_Issend**************/



void pmpi_issend_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Issend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void pmpi_issend__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Issend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

 /*********MPI_Rsend**************/



void pmpi_rsend_(
void* ibuf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Rsend( ibuf,
count,
datatype,
dest,
tag,
comm);
}

void pmpi_rsend__(
void* ibuf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Rsend( ibuf,
count,
datatype,
dest,
tag,
comm);
}

 /*********MPI_Abort**************/



void pmpi_abort_(
MPI_Comm comm,
int errorcode,
int * ierror)
{
*ierror = MPI_Abort( comm,
errorcode);
}

void pmpi_abort__(
MPI_Comm comm,
int errorcode,
int * ierror)
{
*ierror = MPI_Abort( comm,
errorcode);
}

 /*********MPI_Grequest_complete**************/



void pmpi_grequest_complete_(
MPI_Request request,
int * ierror)
{
*ierror = MPI_Grequest_complete( request);
}

void pmpi_grequest_complete__(
MPI_Request request,
int * ierror)
{
*ierror = MPI_Grequest_complete( request);
}

 /*********MPI_Pack**************/



void pmpi_pack_(
void* inbuf,
int incount,
MPI_Datatype datatype,
void* outbuf,
int outsize,
int* position,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Pack( inbuf,
incount,
datatype,
outbuf,
outsize,
position,
comm);
}

void pmpi_pack__(
void* inbuf,
int incount,
MPI_Datatype datatype,
void* outbuf,
int outsize,
int* position,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Pack( inbuf,
incount,
datatype,
outbuf,
outsize,
position,
comm);
}

 /*********MPI_Win_set_attr**************/



void pmpi_win_set_attr_(
MPI_Win win,
int win_keyval,
void* attribute_val,
int * ierror)
{
*ierror = MPI_Win_set_attr( win,
win_keyval,
attribute_val);
}

void pmpi_win_set_attr__(
MPI_Win win,
int win_keyval,
void* attribute_val,
int * ierror)
{
*ierror = MPI_Win_set_attr( win,
win_keyval,
attribute_val);
}

 /*********MPI_Info_create**************/



void pmpi_info_create_(
MPI_Info* info,
int * ierror)
{
*ierror = MPI_Info_create( info);
}

void pmpi_info_create__(
MPI_Info* info,
int * ierror)
{
*ierror = MPI_Info_create( info);
}

 /*********MPI_Type_create_f90_complex**************/



void pmpi_type_create_f90_complex_(
int p,
int r,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_f90_complex( p,
r,
newtype);
}

void pmpi_type_create_f90_complex__(
int p,
int r,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_f90_complex( p,
r,
newtype);
}

 /*********MPI_Gatherv**************/

/* Skipped function MPI_Gathervwith conversion */
/* Skipped function MPI_Gathervwith conversion */


 /*********MPI_Type_get_attr**************/



void pmpi_type_get_attr_(
MPI_Datatype type,
int type_keyval,
void* attribute_val,
int* flag,
int * ierror)
{
*ierror = MPI_Type_get_attr( type,
type_keyval,
attribute_val,
flag);
}

void pmpi_type_get_attr__(
MPI_Datatype type,
int type_keyval,
void* attribute_val,
int* flag,
int * ierror)
{
*ierror = MPI_Type_get_attr( type,
type_keyval,
attribute_val,
flag);
}

 /*********MPI_Comm_set_name**************/



void pmpi_comm_set_name_(
MPI_Comm comm,
char* comm_name CHAR_MIXED(size_comm_name),
int * ierror, CHAR_END(size_comm_name))
{
char *tmp_comm_name, *ptr_comm_name;
tmp_comm_name = sctk_char_fortran_to_c(comm_name, size_comm_name, &ptr_comm_name);
*ierror = MPI_Comm_set_name( comm,
comm_name);
sctk_free(ptr_comm_name);
}

void pmpi_comm_set_name__(
MPI_Comm comm,
char* comm_name CHAR_MIXED(size_comm_name),
int * ierror, CHAR_END(size_comm_name))
{
char *tmp_comm_name, *ptr_comm_name;
tmp_comm_name = sctk_char_fortran_to_c(comm_name, size_comm_name, &ptr_comm_name);
*ierror = MPI_Comm_set_name( comm,
comm_name);
sctk_free(ptr_comm_name);
}

 /*********MPI_Comm_disconnect**************/



void pmpi_comm_disconnect_(
MPI_Comm* comm,
int * ierror)
{
*ierror = MPI_Comm_disconnect( comm);
}

void pmpi_comm_disconnect__(
MPI_Comm* comm,
int * ierror)
{
*ierror = MPI_Comm_disconnect( comm);
}

 /*********MPI_Comm_remote_group**************/



void pmpi_comm_remote_group_(
MPI_Comm comm,
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Comm_remote_group( comm,
group);
}

void pmpi_comm_remote_group__(
MPI_Comm comm,
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Comm_remote_group( comm,
group);
}

 /*********MPI_Cart_shift**************/



void pmpi_cart_shift_(
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

void pmpi_cart_shift__(
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

 /*********MPI_Group_incl**************/



void pmpi_group_incl_(
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

void pmpi_group_incl__(
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

 /*********MPI_Comm_size**************/



void pmpi_comm_size_(
MPI_Comm comm,
int* size,
int * ierror)
{
*ierror = MPI_Comm_size( comm,
size);
}

void pmpi_comm_size__(
MPI_Comm comm,
int* size,
int * ierror)
{
*ierror = MPI_Comm_size( comm,
size);
}

 /*********MPI_Type_create_hindexed**************/



void pmpi_type_create_hindexed_(
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

void pmpi_type_create_hindexed__(
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

 /*********MPI_Register_datarep**************/



void pmpi_register_datarep_(
char* datarep CHAR_MIXED(size_datarep),
MPI_Datarep_conversion_function* read_conversion_fn,
MPI_Datarep_conversion_function* write_conversion_fn,
MPI_Datarep_extent_function* dtype_file_extent_fn,
void* extra_state,
int * ierror, CHAR_END(size_datarep))
{
char *tmp_datarep, *ptr_datarep;
tmp_datarep = sctk_char_fortran_to_c(datarep, size_datarep, &ptr_datarep);
*ierror = MPI_Register_datarep( datarep,
read_conversion_fn,
write_conversion_fn,
dtype_file_extent_fn,
extra_state);
sctk_free(ptr_datarep);
}

void pmpi_register_datarep__(
char* datarep CHAR_MIXED(size_datarep),
MPI_Datarep_conversion_function* read_conversion_fn,
MPI_Datarep_conversion_function* write_conversion_fn,
MPI_Datarep_extent_function* dtype_file_extent_fn,
void* extra_state,
int * ierror, CHAR_END(size_datarep))
{
char *tmp_datarep, *ptr_datarep;
tmp_datarep = sctk_char_fortran_to_c(datarep, size_datarep, &ptr_datarep);
*ierror = MPI_Register_datarep( datarep,
read_conversion_fn,
write_conversion_fn,
dtype_file_extent_fn,
extra_state);
sctk_free(ptr_datarep);
}

 /*********MPI_Waitsome**************/

/* Skipped function MPI_Waitsomewith conversion */
/* Skipped function MPI_Waitsomewith conversion */


 /*********MPI_Group_difference**************/



void pmpi_group_difference_(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_difference( group1,
group2,
newgroup);
}

void pmpi_group_difference__(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_difference( group1,
group2,
newgroup);
}

 /*********MPI_Attr_get**************/



void pmpi_attr_get_(
MPI_Comm comm,
int keyval,
void* attribute_val,
int* flag,
int * ierror)
{
*ierror = MPI_Attr_get( comm,
keyval,
attribute_val,
flag);
}

void pmpi_attr_get__(
MPI_Comm comm,
int keyval,
void* attribute_val,
int* flag,
int * ierror)
{
*ierror = MPI_Attr_get( comm,
keyval,
attribute_val,
flag);
}

 /*********MPI_Reduce_scatter**************/

/* Skipped function MPI_Reduce_scatterwith conversion */
/* Skipped function MPI_Reduce_scatterwith conversion */


 /*********MPI_Win_get_name**************/



void pmpi_win_get_name_(
MPI_Win win,
char* win_name CHAR_MIXED(size_win_name),
int* resultlen,
int * ierror, CHAR_END(size_win_name))
{
*ierror = MPI_Win_get_name( win,
win_name,
resultlen);
sctk_char_c_to_fortran(win_name, size_win_name);
}

void pmpi_win_get_name__(
MPI_Win win,
char* win_name CHAR_MIXED(size_win_name),
int* resultlen,
int * ierror, CHAR_END(size_win_name))
{
*ierror = MPI_Win_get_name( win,
win_name,
resultlen);
sctk_char_c_to_fortran(win_name, size_win_name);
}

 /*********MPI_Type_create_f90_real**************/



void pmpi_type_create_f90_real_(
int p,
int r,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_f90_real( p,
r,
newtype);
}

void pmpi_type_create_f90_real__(
int p,
int r,
MPI_Datatype* newtype,
int * ierror)
{
*ierror = MPI_Type_create_f90_real( p,
r,
newtype);
}

 /*********MPI_Imrecv**************/



void pmpi_imrecv_(
void* buf,
int count,
MPI_Datatype datatype,
MPI_Message* message,
MPI_Request* status,
int * ierror)
{
*ierror = MPI_Imrecv( buf,
count,
datatype,
message,
status);
}

void pmpi_imrecv__(
void* buf,
int count,
MPI_Datatype datatype,
MPI_Message* message,
MPI_Request* status,
int * ierror)
{
*ierror = MPI_Imrecv( buf,
count,
datatype,
message,
status);
}

 /*********MPI_Wait**************/



void pmpi_wait_(
MPI_Request* request,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Wait( request,
status);
}

void pmpi_wait__(
MPI_Request* request,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Wait( request,
status);
}

 /*********MPI_Testall**************/



void pmpi_testall_(
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

void pmpi_testall__(
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

 /*********MPI_Irsend**************/



void pmpi_irsend_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Irsend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void pmpi_irsend__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Irsend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

 /*********MPI_Get_version**************/



void pmpi_get_version_(
int* version,
int* subversion,
int * ierror)
{
*ierror = MPI_Get_version( version,
subversion);
}

void pmpi_get_version__(
int* version,
int* subversion,
int * ierror)
{
*ierror = MPI_Get_version( version,
subversion);
}

 /*********MPI_Comm_create_errhandler**************/



void pmpi_comm_create_errhandler_(
MPI_Comm_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Comm_create_errhandler( function,
errhandler);
}

void pmpi_comm_create_errhandler__(
MPI_Comm_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Comm_create_errhandler( function,
errhandler);
}

 /*********MPI_Comm_connect**************/



void pmpi_comm_connect_(
char* port_name CHAR_MIXED(size_port_name),
MPI_Info info,
int root,
MPI_Comm comm,
MPI_Comm* newcomm,
int * ierror, CHAR_END(size_port_name))
{
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Comm_connect( port_name,
info,
root,
comm,
newcomm);
sctk_free(ptr_port_name);
}

void pmpi_comm_connect__(
char* port_name CHAR_MIXED(size_port_name),
MPI_Info info,
int root,
MPI_Comm comm,
MPI_Comm* newcomm,
int * ierror, CHAR_END(size_port_name))
{
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Comm_connect( port_name,
info,
root,
comm,
newcomm);
sctk_free(ptr_port_name);
}

 /*********MPI_Group_compare**************/



void pmpi_group_compare_(
MPI_Group group1,
MPI_Group group2,
int* result,
int * ierror)
{
*ierror = MPI_Group_compare( group1,
group2,
result);
}

void pmpi_group_compare__(
MPI_Group group1,
MPI_Group group2,
int* result,
int * ierror)
{
*ierror = MPI_Group_compare( group1,
group2,
result);
}

 /*********MPI_Address**************/



void pmpi_address_(
void* location,
MPI_Aint* address,
int * ierror)
{
*ierror = MPI_Address( location,
address);
}

void pmpi_address__(
void* location,
MPI_Aint* address,
int * ierror)
{
*ierror = MPI_Address( location,
address);
}

 /*********MPI_Comm_compare**************/



void pmpi_comm_compare_(
MPI_Comm comm1,
MPI_Comm comm2,
int* result,
int * ierror)
{
*ierror = MPI_Comm_compare( comm1,
comm2,
result);
}

void pmpi_comm_compare__(
MPI_Comm comm1,
MPI_Comm comm2,
int* result,
int * ierror)
{
*ierror = MPI_Comm_compare( comm1,
comm2,
result);
}

 /*********MPI_Win_unlock**************/



void pmpi_win_unlock_(
int rank,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_unlock( rank,
win);
}

void pmpi_win_unlock__(
int rank,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_unlock( rank,
win);
}

 /*********MPI_Graph_map**************/

/* Skipped function MPI_Graph_mapwith conversion */
/* Skipped function MPI_Graph_mapwith conversion */


 /*********MPI_Request_free**************/



void pmpi_request_free_(
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Request_free( request);
}

void pmpi_request_free__(
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Request_free( request);
}

 /*********MPI_Topo_test**************/



void pmpi_topo_test_(
MPI_Comm comm,
int* status,
int * ierror)
{
*ierror = MPI_Topo_test( comm,
status);
}

void pmpi_topo_test__(
MPI_Comm comm,
int* status,
int * ierror)
{
*ierror = MPI_Topo_test( comm,
status);
}

 /*********MPI_Buffer_attach**************/



void pmpi_buffer_attach_(
void* buffer,
int size,
int * ierror)
{
*ierror = MPI_Buffer_attach( buffer,
size);
}

void pmpi_buffer_attach__(
void* buffer,
int size,
int * ierror)
{
*ierror = MPI_Buffer_attach( buffer,
size);
}

 /*********MPI_Win_call_errhandler**************/



void pmpi_win_call_errhandler_(
MPI_Win win,
int errorcode,
int * ierror)
{
*ierror = MPI_Win_call_errhandler( win,
errorcode);
}

void pmpi_win_call_errhandler__(
MPI_Win win,
int errorcode,
int * ierror)
{
*ierror = MPI_Win_call_errhandler( win,
errorcode);
}

 /*********MPI_Win_get_group**************/



void pmpi_win_get_group_(
MPI_Win win,
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Win_get_group( win,
group);
}

void pmpi_win_get_group__(
MPI_Win win,
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Win_get_group( win,
group);
}

 /*********MPI_Errhandler_create**************/



void pmpi_errhandler_create_(
MPI_Handler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_create( function,
errhandler);
}

void pmpi_errhandler_create__(
MPI_Handler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_create( function,
errhandler);
}

 /*********MPI_Cart_create**************/



void pmpi_cart_create_(
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

void pmpi_cart_create__(
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

 /*********MPI_Status_set_cancelled**************/



void pmpi_status_set_cancelled_(
MPI_Status* status,
int* flag,
int * ierror)
{
*ierror = MPI_Status_set_cancelled( status,
flag);
}

void pmpi_status_set_cancelled__(
MPI_Status* status,
int* flag,
int * ierror)
{
*ierror = MPI_Status_set_cancelled( status,
flag);
}

 /*********MPI_Status_f2c**************/


/* MPI_Status_f2c NOT IMPLEMENTED in MPC */


/* MPI_Status_f2c NOT IMPLEMENTED in MPC */



 /*********MPI_Type_struct**************/



void pmpi_type_struct_(
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

void pmpi_type_struct__(
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

 /*********MPI_Graph_neighbors_count**************/



void pmpi_graph_neighbors_count_(
MPI_Comm comm,
int rank,
int* nneighbors,
int * ierror)
{
*ierror = MPI_Graph_neighbors_count( comm,
rank,
nneighbors);
}

void pmpi_graph_neighbors_count__(
MPI_Comm comm,
int rank,
int* nneighbors,
int * ierror)
{
*ierror = MPI_Graph_neighbors_count( comm,
rank,
nneighbors);
}

 /*********MPI_Allgatherv**************/

/* Skipped function MPI_Allgathervwith conversion */
/* Skipped function MPI_Allgathervwith conversion */


 /*********MPI_Graph_neighbors**************/



void pmpi_graph_neighbors_(
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

void pmpi_graph_neighbors__(
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

 /*********MPI_Dims_create**************/



void pmpi_dims_create_(
int nnodes,
int ndims,
int* dims,
int * ierror)
{
*ierror = MPI_Dims_create( nnodes,
ndims,
dims);
}

void pmpi_dims_create__(
int nnodes,
int ndims,
int* dims,
int * ierror)
{
*ierror = MPI_Dims_create( nnodes,
ndims,
dims);
}

 /*********MPI_Scatter**************/



void pmpi_scatter_(
void* sendbuf,
int sendcount,
MPI_Datatype sendtype,
void* recvbuf,
int recvcount,
MPI_Datatype recvtype,
int root,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Scatter( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
root,
comm);
}

void pmpi_scatter__(
void* sendbuf,
int sendcount,
MPI_Datatype sendtype,
void* recvbuf,
int recvcount,
MPI_Datatype recvtype,
int root,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Scatter( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
root,
comm);
}

 /*********MPI_Comm_free_keyval**************/



void pmpi_comm_free_keyval_(
int* comm_keyval,
int * ierror)
{
*ierror = MPI_Comm_free_keyval( comm_keyval);
}

void pmpi_comm_free_keyval__(
int* comm_keyval,
int * ierror)
{
*ierror = MPI_Comm_free_keyval( comm_keyval);
}

 /*********MPI_Op_create**************/



void pmpi_op_create_(
MPI_User_function* function,
int* commute,
MPI_Op* op,
int * ierror)
{
*ierror = MPI_Op_create( function,
commute,
op);
}

void pmpi_op_create__(
MPI_User_function* function,
int* commute,
MPI_Op* op,
int * ierror)
{
*ierror = MPI_Op_create( function,
commute,
op);
}

 /*********MPI_Add_error_string**************/



void pmpi_add_error_string_(
int errorcode,
char* string CHAR_MIXED(size_string),
int * ierror, CHAR_END(size_string))
{
char *tmp_string, *ptr_string;
tmp_string = sctk_char_fortran_to_c(string, size_string, &ptr_string);
*ierror = MPI_Add_error_string( errorcode,
string);
sctk_free(ptr_string);
}

void pmpi_add_error_string__(
int errorcode,
char* string CHAR_MIXED(size_string),
int * ierror, CHAR_END(size_string))
{
char *tmp_string, *ptr_string;
tmp_string = sctk_char_fortran_to_c(string, size_string, &ptr_string);
*ierror = MPI_Add_error_string( errorcode,
string);
sctk_free(ptr_string);
}

 /*********MPI_Mprobe**************/



void pmpi_mprobe_(
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

void pmpi_mprobe__(
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

 /*********MPI_Ssend_init**************/



void pmpi_ssend_init_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Ssend_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void pmpi_ssend_init__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Ssend_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

 /*********MPI_Rsend_init**************/



void pmpi_rsend_init_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Rsend_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void pmpi_rsend_init__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Rsend_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

 /*********MPI_Info_free**************/



void pmpi_info_free_(
MPI_Info* info,
int * ierror)
{
*ierror = MPI_Info_free( info);
}

void pmpi_info_free__(
MPI_Info* info,
int * ierror)
{
*ierror = MPI_Info_free( info);
}

 /*********MPI_Publish_name**************/



void pmpi_publish_name_(
char* service_name CHAR_MIXED(size_service_name),
MPI_Info info,
char* port_name CHAR_MIXED(size_port_name),
int * ierror, CHAR_END(size_service_name), CHAR_END(size_port_name))
{
char *tmp_service_name, *ptr_service_name;
tmp_service_name = sctk_char_fortran_to_c(service_name, size_service_name, &ptr_service_name);
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Publish_name( service_name,
info,
port_name);
sctk_free(ptr_service_name);
sctk_free(ptr_port_name);
}

void pmpi_publish_name__(
char* service_name CHAR_MIXED(size_service_name),
MPI_Info info,
char* port_name CHAR_MIXED(size_port_name),
int * ierror, CHAR_END(size_service_name), CHAR_END(size_port_name))
{
char *tmp_service_name, *ptr_service_name;
tmp_service_name = sctk_char_fortran_to_c(service_name, size_service_name, &ptr_service_name);
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Publish_name( service_name,
info,
port_name);
sctk_free(ptr_service_name);
sctk_free(ptr_port_name);
}

 /*********MPI_Bcast**************/



void pmpi_bcast_(
void* buffer,
int count,
MPI_Datatype datatype,
int root,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Bcast( buffer,
count,
datatype,
root,
comm);
}

void pmpi_bcast__(
void* buffer,
int count,
MPI_Datatype datatype,
int root,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Bcast( buffer,
count,
datatype,
root,
comm);
}

 /*********MPI_Get_processor_name**************/



void pmpi_get_processor_name_(
char* name CHAR_MIXED(size_name),
int* resultlen,
int * ierror, CHAR_END(size_name))
{
*ierror = MPI_Get_processor_name( name,
resultlen);
sctk_char_c_to_fortran(name, size_name);
}

void pmpi_get_processor_name__(
char* name CHAR_MIXED(size_name),
int* resultlen,
int * ierror, CHAR_END(size_name))
{
*ierror = MPI_Get_processor_name( name,
resultlen);
sctk_char_c_to_fortran(name, size_name);
}

 /*********MPI_Info_set**************/



void pmpi_info_set_(
MPI_Info info,
char* key CHAR_MIXED(size_key),
char* value CHAR_MIXED(size_value),
int * ierror, CHAR_END(size_key), CHAR_END(size_value))
{
char *tmp_key, *ptr_key;
tmp_key = sctk_char_fortran_to_c(key, size_key, &ptr_key);
char *tmp_value, *ptr_value;
tmp_value = sctk_char_fortran_to_c(value, size_value, &ptr_value);
*ierror = MPI_Info_set( info,
key,
value);
sctk_free(ptr_key);
sctk_free(ptr_value);
}

void pmpi_info_set__(
MPI_Info info,
char* key CHAR_MIXED(size_key),
char* value CHAR_MIXED(size_value),
int * ierror, CHAR_END(size_key), CHAR_END(size_value))
{
char *tmp_key, *ptr_key;
tmp_key = sctk_char_fortran_to_c(key, size_key, &ptr_key);
char *tmp_value, *ptr_value;
tmp_value = sctk_char_fortran_to_c(value, size_value, &ptr_value);
*ierror = MPI_Info_set( info,
key,
value);
sctk_free(ptr_key);
sctk_free(ptr_value);
}

 /*********MPI_Graph_create**************/

/* Skipped function MPI_Graph_createwith conversion */
/* Skipped function MPI_Graph_createwith conversion */


 /*********MPI_Comm_free**************/



void pmpi_comm_free_(
MPI_Comm* comm,
int * ierror)
{
*ierror = MPI_Comm_free( comm);
}

void pmpi_comm_free__(
MPI_Comm* comm,
int * ierror)
{
*ierror = MPI_Comm_free( comm);
}

 /*********MPI_Errhandler_get**************/



void pmpi_errhandler_get_(
MPI_Comm comm,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_get( comm,
errhandler);
}

void pmpi_errhandler_get__(
MPI_Comm comm,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_get( comm,
errhandler);
}

 /*********MPI_Pack_size**************/



void pmpi_pack_size_(
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

void pmpi_pack_size__(
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

 /*********MPI_Comm_call_errhandler**************/



void pmpi_comm_call_errhandler_(
MPI_Comm comm,
int errorcode,
int * ierror)
{
*ierror = MPI_Comm_call_errhandler( comm,
errorcode);
}

void pmpi_comm_call_errhandler__(
MPI_Comm comm,
int errorcode,
int * ierror)
{
*ierror = MPI_Comm_call_errhandler( comm,
errorcode);
}

 /*********MPI_Comm_test_inter**************/



void pmpi_comm_test_inter_(
MPI_Comm comm,
int* flag,
int * ierror)
{
*ierror = MPI_Comm_test_inter( comm,
flag);
}

void pmpi_comm_test_inter__(
MPI_Comm comm,
int* flag,
int * ierror)
{
*ierror = MPI_Comm_test_inter( comm,
flag);
}

 /*********MPI_Intercomm_merge**************/



void pmpi_intercomm_merge_(
MPI_Comm intercomm,
int* high,
MPI_Comm* newintercomm,
int * ierror)
{
*ierror = MPI_Intercomm_merge( intercomm,
high,
newintercomm);
}

void pmpi_intercomm_merge__(
MPI_Comm intercomm,
int* high,
MPI_Comm* newintercomm,
int * ierror)
{
*ierror = MPI_Intercomm_merge( intercomm,
high,
newintercomm);
}

 /*********MPI_Win_complete**************/



void pmpi_win_complete_(
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_complete( win);
}

void pmpi_win_complete__(
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_complete( win);
}

 /*********MPI_Pack_external**************/



void pmpi_pack_external_(
char* datarep CHAR_MIXED(size_datarep),
void* inbuf,
int incount,
MPI_Datatype datatype,
void* outbuf,
MPI_Aint outsize,
MPI_Aint* position,
int * ierror, CHAR_END(size_datarep))
{
char *tmp_datarep, *ptr_datarep;
tmp_datarep = sctk_char_fortran_to_c(datarep, size_datarep, &ptr_datarep);
*ierror = MPI_Pack_external( datarep,
inbuf,
incount,
datatype,
outbuf,
outsize,
position);
sctk_free(ptr_datarep);
}

void pmpi_pack_external__(
char* datarep CHAR_MIXED(size_datarep),
void* inbuf,
int incount,
MPI_Datatype datatype,
void* outbuf,
MPI_Aint outsize,
MPI_Aint* position,
int * ierror, CHAR_END(size_datarep))
{
char *tmp_datarep, *ptr_datarep;
tmp_datarep = sctk_char_fortran_to_c(datarep, size_datarep, &ptr_datarep);
*ierror = MPI_Pack_external( datarep,
inbuf,
incount,
datatype,
outbuf,
outsize,
position);
sctk_free(ptr_datarep);
}

 /*********MPI_Type_set_attr**************/



void pmpi_type_set_attr_(
MPI_Datatype type,
int type_keyval,
void* attr_val,
int * ierror)
{
*ierror = MPI_Type_set_attr( type,
type_keyval,
attr_val);
}

void pmpi_type_set_attr__(
MPI_Datatype type,
int type_keyval,
void* attr_val,
int * ierror)
{
*ierror = MPI_Type_set_attr( type,
type_keyval,
attr_val);
}

 /*********MPI_Info_get_valuelen**************/



void pmpi_info_get_valuelen_(
MPI_Info info,
char* key CHAR_MIXED(size_key),
int* valuelen,
int* flag,
int * ierror, CHAR_END(size_key))
{
char *tmp_key, *ptr_key;
tmp_key = sctk_char_fortran_to_c(key, size_key, &ptr_key);
*ierror = MPI_Info_get_valuelen( info,
key,
valuelen,
flag);
sctk_free(ptr_key);
}

void pmpi_info_get_valuelen__(
MPI_Info info,
char* key CHAR_MIXED(size_key),
int* valuelen,
int* flag,
int * ierror, CHAR_END(size_key))
{
char *tmp_key, *ptr_key;
tmp_key = sctk_char_fortran_to_c(key, size_key, &ptr_key);
*ierror = MPI_Info_get_valuelen( info,
key,
valuelen,
flag);
sctk_free(ptr_key);
}

 /*********MPI_Put**************/



void pmpi_put_(
void* origin_addr,
int origin_count,
MPI_Datatype origin_datatype,
int target_rank,
MPI_Aint target_disp,
int target_count,
MPI_Datatype target_datatype,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Put( origin_addr,
origin_count,
origin_datatype,
target_rank,
target_disp,
target_count,
target_datatype,
win);
}

void pmpi_put__(
void* origin_addr,
int origin_count,
MPI_Datatype origin_datatype,
int target_rank,
MPI_Aint target_disp,
int target_count,
MPI_Datatype target_datatype,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Put( origin_addr,
origin_count,
origin_datatype,
target_rank,
target_disp,
target_count,
target_datatype,
win);
}

 /*********MPI_Isend**************/



void pmpi_isend_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Isend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void pmpi_isend__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Isend( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

 /*********MPI_Type_get_envelope**************/



void pmpi_type_get_envelope_(
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

void pmpi_type_get_envelope__(
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

 /*********MPI_Group_rank**************/



void pmpi_group_rank_(
MPI_Group group,
int* rank,
int * ierror)
{
*ierror = MPI_Group_rank( group,
rank);
}

void pmpi_group_rank__(
MPI_Group group,
int* rank,
int * ierror)
{
*ierror = MPI_Group_rank( group,
rank);
}

 /*********MPI_Alltoall**************/



void pmpi_alltoall_(
void* sendbuf,
int sendcount,
MPI_Datatype sendtype,
void* recvbuf,
int recvcount,
MPI_Datatype recvtype,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Alltoall( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
comm);
}

void pmpi_alltoall__(
void* sendbuf,
int sendcount,
MPI_Datatype sendtype,
void* recvbuf,
int recvcount,
MPI_Datatype recvtype,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Alltoall( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
comm);
}

 /*********MPI_Info_get**************/



void pmpi_info_get_(
MPI_Info info,
char* key CHAR_MIXED(size_key),
int valuelen,
char* value CHAR_MIXED(size_value),
int* flag,
int * ierror, CHAR_END(size_key), CHAR_END(size_value))
{
char *tmp_key, *ptr_key;
tmp_key = sctk_char_fortran_to_c(key, size_key, &ptr_key);
*ierror = MPI_Info_get( info,
key,
valuelen,
value,
flag);
sctk_free(ptr_key);
sctk_char_c_to_fortran(value, size_value);
}

void pmpi_info_get__(
MPI_Info info,
char* key CHAR_MIXED(size_key),
int valuelen,
char* value CHAR_MIXED(size_value),
int* flag,
int * ierror, CHAR_END(size_key), CHAR_END(size_value))
{
char *tmp_key, *ptr_key;
tmp_key = sctk_char_fortran_to_c(key, size_key, &ptr_key);
*ierror = MPI_Info_get( info,
key,
valuelen,
value,
flag);
sctk_free(ptr_key);
sctk_char_c_to_fortran(value, size_value);
}

 /*********MPI_Group_intersection**************/



void pmpi_group_intersection_(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_intersection( group1,
group2,
newgroup);
}

void pmpi_group_intersection__(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_intersection( group1,
group2,
newgroup);
}

 /*********MPI_Type_free_keyval**************/



void pmpi_type_free_keyval_(
int* type_keyval,
int * ierror)
{
*ierror = MPI_Type_free_keyval( type_keyval);
}

void pmpi_type_free_keyval__(
int* type_keyval,
int * ierror)
{
*ierror = MPI_Type_free_keyval( type_keyval);
}

 /*********MPI_Type_create_struct**************/



void pmpi_type_create_struct_(
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

void pmpi_type_create_struct__(
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

 /*********MPI_Type_get_contents**************/



void pmpi_type_get_contents_(
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

void pmpi_type_get_contents__(
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

 /*********MPI_Reduce_local**************/



void pmpi_reduce_local_(
void* inbuf,
void* inoutbuf,
int count,
MPI_Datatype datatype,
MPI_Op op,
int * ierror)
{
*ierror = MPI_Reduce_local( inbuf,
inoutbuf,
count,
datatype,
op);
}

void pmpi_reduce_local__(
void* inbuf,
void* inoutbuf,
int count,
MPI_Datatype datatype,
MPI_Op op,
int * ierror)
{
*ierror = MPI_Reduce_local( inbuf,
inoutbuf,
count,
datatype,
op);
}

 /*********MPI_Group_union**************/



void pmpi_group_union_(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_union( group1,
group2,
newgroup);
}

void pmpi_group_union__(
MPI_Group group1,
MPI_Group group2,
MPI_Group* newgroup,
int * ierror)
{
*ierror = MPI_Group_union( group1,
group2,
newgroup);
}

 /*********MPI_Type_free**************/



void pmpi_type_free_(
MPI_Datatype* type,
int * ierror)
{
*ierror = MPI_Type_free( type);
}

void pmpi_type_free__(
MPI_Datatype* type,
int * ierror)
{
*ierror = MPI_Type_free( type);
}

 /*********MPI_Info_get_nthkey**************/



void pmpi_info_get_nthkey_(
MPI_Info info,
int n,
char* key CHAR_MIXED(size_key),
int * ierror, CHAR_END(size_key))
{
*ierror = MPI_Info_get_nthkey( info,
n,
key);
sctk_char_c_to_fortran(key, size_key);
}

void pmpi_info_get_nthkey__(
MPI_Info info,
int n,
char* key CHAR_MIXED(size_key),
int * ierror, CHAR_END(size_key))
{
*ierror = MPI_Info_get_nthkey( info,
n,
key);
sctk_char_c_to_fortran(key, size_key);
}

 /*********MPI_Unpack_external**************/



void pmpi_unpack_external_(
char* datarep CHAR_MIXED(size_datarep),
void* inbuf,
MPI_Aint insize,
MPI_Aint* position,
void* outbuf,
int outcount,
MPI_Datatype datatype,
int * ierror, CHAR_END(size_datarep))
{
char *tmp_datarep, *ptr_datarep;
tmp_datarep = sctk_char_fortran_to_c(datarep, size_datarep, &ptr_datarep);
*ierror = MPI_Unpack_external( datarep,
inbuf,
insize,
position,
outbuf,
outcount,
datatype);
sctk_free(ptr_datarep);
}

void pmpi_unpack_external__(
char* datarep CHAR_MIXED(size_datarep),
void* inbuf,
MPI_Aint insize,
MPI_Aint* position,
void* outbuf,
int outcount,
MPI_Datatype datatype,
int * ierror, CHAR_END(size_datarep))
{
char *tmp_datarep, *ptr_datarep;
tmp_datarep = sctk_char_fortran_to_c(datarep, size_datarep, &ptr_datarep);
*ierror = MPI_Unpack_external( datarep,
inbuf,
insize,
position,
outbuf,
outcount,
datatype);
sctk_free(ptr_datarep);
}

 /*********MPI_Errhandler_set**************/



void pmpi_errhandler_set_(
MPI_Comm comm,
MPI_Errhandler errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_set( comm,
errhandler);
}

void pmpi_errhandler_set__(
MPI_Comm comm,
MPI_Errhandler errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_set( comm,
errhandler);
}

 /*********MPI_Comm_get_errhandler**************/



void pmpi_comm_get_errhandler_(
MPI_Comm comm,
MPI_Errhandler* erhandler,
int * ierror)
{
*ierror = MPI_Comm_get_errhandler( comm,
erhandler);
}

void pmpi_comm_get_errhandler__(
MPI_Comm comm,
MPI_Errhandler* erhandler,
int * ierror)
{
*ierror = MPI_Comm_get_errhandler( comm,
erhandler);
}

 /*********MPI_Test_cancelled**************/



void pmpi_test_cancelled_(
MPI_Status* status,
int* flag,
int * ierror)
{
*ierror = MPI_Test_cancelled( status,
flag);
}

void pmpi_test_cancelled__(
MPI_Status* status,
int* flag,
int * ierror)
{
*ierror = MPI_Test_cancelled( status,
flag);
}

 /*********MPI_Win_lock**************/



void pmpi_win_lock_(
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

void pmpi_win_lock__(
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

 /*********MPI_Win_create**************/



void pmpi_win_create_(
void* base,
MPI_Aint size,
int disp_unit,
MPI_Info info,
MPI_Comm comm,
MPI_Win* win,
int * ierror)
{
*ierror = MPI_Win_create( base,
size,
disp_unit,
info,
comm,
win);
}

void pmpi_win_create__(
void* base,
MPI_Aint size,
int disp_unit,
MPI_Info info,
MPI_Comm comm,
MPI_Win* win,
int * ierror)
{
*ierror = MPI_Win_create( base,
size,
disp_unit,
info,
comm,
win);
}

 /*********MPI_Test**************/



void pmpi_test_(
MPI_Request* request,
int* flag,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Test( request,
flag,
status);
}

void pmpi_test__(
MPI_Request* request,
int* flag,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Test( request,
flag,
status);
}

 /*********MPI_Type_create_hvector**************/



void pmpi_type_create_hvector_(
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

void pmpi_type_create_hvector__(
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

 /*********MPI_Info_get_nkeys**************/



void pmpi_info_get_nkeys_(
MPI_Info info,
int* nkeys,
int * ierror)
{
*ierror = MPI_Info_get_nkeys( info,
nkeys);
}

void pmpi_info_get_nkeys__(
MPI_Info info,
int* nkeys,
int * ierror)
{
*ierror = MPI_Info_get_nkeys( info,
nkeys);
}

 /*********MPI_Win_start**************/



void pmpi_win_start_(
MPI_Group group,
int assert,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_start( group,
assert,
win);
}

void pmpi_win_start__(
MPI_Group group,
int assert,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_start( group,
assert,
win);
}

 /*********MPI_Finalized**************/



void pmpi_finalized_(
int* flag,
int * ierror)
{
*ierror = MPI_Finalized( flag);
}

void pmpi_finalized__(
int* flag,
int * ierror)
{
*ierror = MPI_Finalized( flag);
}

 /*********MPI_Win_free_keyval**************/



void pmpi_win_free_keyval_(
int* win_keyval,
int * ierror)
{
*ierror = MPI_Win_free_keyval( win_keyval);
}

void pmpi_win_free_keyval__(
int* win_keyval,
int * ierror)
{
*ierror = MPI_Win_free_keyval( win_keyval);
}

 /*********MPI_Waitany**************/



void pmpi_waitany_(
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
}

void pmpi_waitany__(
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
}

 /*********MPI_Open_port**************/



void pmpi_open_port_(
MPI_Info info,
char* port_name CHAR_MIXED(size_port_name),
int * ierror, CHAR_END(size_port_name))
{
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Open_port( info,
port_name);
sctk_free(ptr_port_name);
}

void pmpi_open_port__(
MPI_Info info,
char* port_name CHAR_MIXED(size_port_name),
int * ierror, CHAR_END(size_port_name))
{
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Open_port( info,
port_name);
sctk_free(ptr_port_name);
}

 /*********MPI_Type_indexed**************/



void pmpi_type_indexed_(
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

void pmpi_type_indexed__(
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

 /*********MPI_Op_commutative**************/



void pmpi_op_commutative_(
MPI_Op op,
int* commute,
int * ierror)
{
*ierror = MPI_Op_commutative( op,
commute);
}

void pmpi_op_commutative__(
MPI_Op op,
int* commute,
int * ierror)
{
*ierror = MPI_Op_commutative( op,
commute);
}

 /*********MPI_Gather**************/



void pmpi_gather_(
void* sendbuf,
int sendcount,
MPI_Datatype sendtype,
void* recvbuf,
int recvcount,
MPI_Datatype recvtype,
int root,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Gather( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
root,
comm);
}

void pmpi_gather__(
void* sendbuf,
int sendcount,
MPI_Datatype sendtype,
void* recvbuf,
int recvcount,
MPI_Datatype recvtype,
int root,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Gather( sendbuf,
sendcount,
sendtype,
recvbuf,
recvcount,
recvtype,
root,
comm);
}

 /*********MPI_Type_vector**************/



void pmpi_type_vector_(
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

void pmpi_type_vector__(
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

 /*********MPI_Get_elements**************/



void pmpi_get_elements_(
MPI_Status* status,
MPI_Datatype datatype,
int* count,
int * ierror)
{
*ierror = MPI_Get_elements( status,
datatype,
count);
}

void pmpi_get_elements__(
MPI_Status* status,
MPI_Datatype datatype,
int* count,
int * ierror)
{
*ierror = MPI_Get_elements( status,
datatype,
count);
}

 /*********MPI_Probe**************/



void pmpi_probe_(
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

void pmpi_probe__(
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

 /*********MPI_Unpack**************/



void pmpi_unpack_(
void* inbuf,
int insize,
int* position,
void* outbuf,
int outcount,
MPI_Datatype datatype,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Unpack( inbuf,
insize,
position,
outbuf,
outcount,
datatype,
comm);
}

void pmpi_unpack__(
void* inbuf,
int insize,
int* position,
void* outbuf,
int outcount,
MPI_Datatype datatype,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Unpack( inbuf,
insize,
position,
outbuf,
outcount,
datatype,
comm);
}

 /*********MPI_Type_ub**************/



void pmpi_type_ub_(
MPI_Datatype mtype,
MPI_Aint* ub,
int * ierror)
{
*ierror = MPI_Type_ub( mtype,
ub);
}

void pmpi_type_ub__(
MPI_Datatype mtype,
MPI_Aint* ub,
int * ierror)
{
*ierror = MPI_Type_ub( mtype,
ub);
}

 /*********MPI_Status_set_elements**************/



void pmpi_status_set_elements_(
MPI_Status* status,
MPI_Datatype datatype,
int count,
int * ierror)
{
*ierror = MPI_Status_set_elements( status,
datatype,
count);
}

void pmpi_status_set_elements__(
MPI_Status* status,
MPI_Datatype datatype,
int count,
int * ierror)
{
*ierror = MPI_Status_set_elements( status,
datatype,
count);
}

 /*********MPI_Win_delete_attr**************/



void pmpi_win_delete_attr_(
MPI_Win win,
int win_keyval,
int * ierror)
{
*ierror = MPI_Win_delete_attr( win,
win_keyval);
}

void pmpi_win_delete_attr__(
MPI_Win win,
int win_keyval,
int * ierror)
{
*ierror = MPI_Win_delete_attr( win,
win_keyval);
}

 /*********MPI_Type_hindexed**************/



void pmpi_type_hindexed_(
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

void pmpi_type_hindexed__(
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

 /*********MPI_Group_range_incl**************/



void pmpi_group_range_incl_(
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

void pmpi_group_range_incl__(
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

 /*********MPI_Get**************/



void pmpi_get_(
void* origin_addr,
int origin_count,
MPI_Datatype origin_datatype,
int target_rank,
MPI_Aint target_disp,
int target_count,
MPI_Datatype target_datatype,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Get( origin_addr,
origin_count,
origin_datatype,
target_rank,
target_disp,
target_count,
target_datatype,
win);
}

void pmpi_get__(
void* origin_addr,
int origin_count,
MPI_Datatype origin_datatype,
int target_rank,
MPI_Aint target_disp,
int target_count,
MPI_Datatype target_datatype,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Get( origin_addr,
origin_count,
origin_datatype,
target_rank,
target_disp,
target_count,
target_datatype,
win);
}

 /*********MPI_Iprobe**************/



void pmpi_iprobe_(
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

void pmpi_iprobe__(
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

 /*********MPI_Type_get_true_extent**************/



void pmpi_type_get_true_extent_(
MPI_Datatype datatype,
MPI_Aint* true_lb,
MPI_Aint* true_extent,
int * ierror)
{
*ierror = MPI_Type_get_true_extent( datatype,
true_lb,
true_extent);
}

void pmpi_type_get_true_extent__(
MPI_Datatype datatype,
MPI_Aint* true_lb,
MPI_Aint* true_extent,
int * ierror)
{
*ierror = MPI_Type_get_true_extent( datatype,
true_lb,
true_extent);
}

 /*********MPI_Graph_get**************/



void pmpi_graph_get_(
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
}

void pmpi_graph_get__(
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
}

 /*********MPI_Info_delete**************/



void pmpi_info_delete_(
MPI_Info info,
char* key CHAR_MIXED(size_key),
int * ierror, CHAR_END(size_key))
{
char *tmp_key, *ptr_key;
tmp_key = sctk_char_fortran_to_c(key, size_key, &ptr_key);
*ierror = MPI_Info_delete( info,
key);
sctk_free(ptr_key);
}

void pmpi_info_delete__(
MPI_Info info,
char* key CHAR_MIXED(size_key),
int * ierror, CHAR_END(size_key))
{
char *tmp_key, *ptr_key;
tmp_key = sctk_char_fortran_to_c(key, size_key, &ptr_key);
*ierror = MPI_Info_delete( info,
key);
sctk_free(ptr_key);
}

 /*********MPI_Win_get_attr**************/



void pmpi_win_get_attr_(
MPI_Win win,
int win_keyval,
void* attribute_val,
int* flag,
int * ierror)
{
*ierror = MPI_Win_get_attr( win,
win_keyval,
attribute_val,
flag);
}

void pmpi_win_get_attr__(
MPI_Win win,
int win_keyval,
void* attribute_val,
int* flag,
int * ierror)
{
*ierror = MPI_Win_get_attr( win,
win_keyval,
attribute_val,
flag);
}

 /*********MPI_Cart_rank**************/

/* Skipped function MPI_Cart_rankwith conversion */
/* Skipped function MPI_Cart_rankwith conversion */


 /*********MPI_Finalize**************/



void pmpi_finalize_(
int * ierror)
{
*ierror = MPI_Finalize( );
}

void pmpi_finalize__(
int * ierror)
{
*ierror = MPI_Finalize( );
}

 /*********MPI_Comm_create**************/



void pmpi_comm_create_(
MPI_Comm comm,
MPI_Group group,
MPI_Comm* newcomm,
int * ierror)
{
*ierror = MPI_Comm_create( comm,
group,
newcomm);
}

void pmpi_comm_create__(
MPI_Comm comm,
MPI_Group group,
MPI_Comm* newcomm,
int * ierror)
{
*ierror = MPI_Comm_create( comm,
group,
newcomm);
}

 /*********MPI_Pack_external_size**************/



void pmpi_pack_external_size_(
char* datarep CHAR_MIXED(size_datarep),
int incount,
MPI_Datatype datatype,
MPI_Aint* size,
int * ierror, CHAR_END(size_datarep))
{
char *tmp_datarep, *ptr_datarep;
tmp_datarep = sctk_char_fortran_to_c(datarep, size_datarep, &ptr_datarep);
*ierror = MPI_Pack_external_size( datarep,
incount,
datatype,
size);
sctk_free(ptr_datarep);
}

void pmpi_pack_external_size__(
char* datarep CHAR_MIXED(size_datarep),
int incount,
MPI_Datatype datatype,
MPI_Aint* size,
int * ierror, CHAR_END(size_datarep))
{
char *tmp_datarep, *ptr_datarep;
tmp_datarep = sctk_char_fortran_to_c(datarep, size_datarep, &ptr_datarep);
*ierror = MPI_Pack_external_size( datarep,
incount,
datatype,
size);
sctk_free(ptr_datarep);
}

 /*********MPI_Comm_join**************/



void pmpi_comm_join_(
int fd,
MPI_Comm* intercomm,
int * ierror)
{
*ierror = MPI_Comm_join( fd,
intercomm);
}

void pmpi_comm_join__(
int fd,
MPI_Comm* intercomm,
int * ierror)
{
*ierror = MPI_Comm_join( fd,
intercomm);
}

 /*********MPI_Keyval_free**************/



void pmpi_keyval_free_(
int* keyval,
int * ierror)
{
*ierror = MPI_Keyval_free( keyval);
}

void pmpi_keyval_free__(
int* keyval,
int * ierror)
{
*ierror = MPI_Keyval_free( keyval);
}

 /*********MPI_Win_wait**************/



void pmpi_win_wait_(
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_wait( win);
}

void pmpi_win_wait__(
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_wait( win);
}

 /*********MPI_Alloc_mem**************/



void pmpi_alloc_mem_(
MPI_Aint size,
MPI_Info info,
void* baseptr,
int * ierror)
{
*ierror = MPI_Alloc_mem( size,
info,
baseptr);
}

void pmpi_alloc_mem__(
MPI_Aint size,
MPI_Info info,
void* baseptr,
int * ierror)
{
*ierror = MPI_Alloc_mem( size,
info,
baseptr);
}

 /*********MPI_Improbe**************/



void pmpi_improbe_(
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

void pmpi_improbe__(
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

 /*********MPI_Type_size**************/



void pmpi_type_size_(
MPI_Datatype type,
int* size,
int * ierror)
{
*ierror = MPI_Type_size( type,
size);
}

void pmpi_type_size__(
MPI_Datatype type,
int* size,
int * ierror)
{
*ierror = MPI_Type_size( type,
size);
}

 /*********MPI_Lookup_name**************/



void pmpi_lookup_name_(
char* service_name CHAR_MIXED(size_service_name),
MPI_Info info,
char* port_name CHAR_MIXED(size_port_name),
int * ierror, CHAR_END(size_service_name), CHAR_END(size_port_name))
{
char *tmp_service_name, *ptr_service_name;
tmp_service_name = sctk_char_fortran_to_c(service_name, size_service_name, &ptr_service_name);
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Lookup_name( service_name,
info,
port_name);
sctk_free(ptr_service_name);
sctk_free(ptr_port_name);
}

void pmpi_lookup_name__(
char* service_name CHAR_MIXED(size_service_name),
MPI_Info info,
char* port_name CHAR_MIXED(size_port_name),
int * ierror, CHAR_END(size_service_name), CHAR_END(size_port_name))
{
char *tmp_service_name, *ptr_service_name;
tmp_service_name = sctk_char_fortran_to_c(service_name, size_service_name, &ptr_service_name);
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Lookup_name( service_name,
info,
port_name);
sctk_free(ptr_service_name);
sctk_free(ptr_port_name);
}

 /*********MPI_Type_create_darray**************/



void pmpi_type_create_darray_(
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

void pmpi_type_create_darray__(
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

 /*********MPI_Win_create_errhandler**************/



void pmpi_win_create_errhandler_(
MPI_Win_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Win_create_errhandler( function,
errhandler);
}

void pmpi_win_create_errhandler__(
MPI_Win_errhandler_function* function,
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Win_create_errhandler( function,
errhandler);
}

 /*********MPI_Cart_map**************/



void pmpi_cart_map_(
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

void pmpi_cart_map__(
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

 /*********MPI_Comm_accept**************/



void pmpi_comm_accept_(
char* port_name CHAR_MIXED(size_port_name),
MPI_Info info,
int root,
MPI_Comm comm,
MPI_Comm* newcomm,
int * ierror, CHAR_END(size_port_name))
{
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Comm_accept( port_name,
info,
root,
comm,
newcomm);
sctk_free(ptr_port_name);
}

void pmpi_comm_accept__(
char* port_name CHAR_MIXED(size_port_name),
MPI_Info info,
int root,
MPI_Comm comm,
MPI_Comm* newcomm,
int * ierror, CHAR_END(size_port_name))
{
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Comm_accept( port_name,
info,
root,
comm,
newcomm);
sctk_free(ptr_port_name);
}

 /*********MPI_Type_set_name**************/



void pmpi_type_set_name_(
MPI_Datatype type,
char* type_name CHAR_MIXED(size_type_name),
int * ierror, CHAR_END(size_type_name))
{
char *tmp_type_name, *ptr_type_name;
tmp_type_name = sctk_char_fortran_to_c(type_name, size_type_name, &ptr_type_name);
*ierror = MPI_Type_set_name( type,
type_name);
sctk_free(ptr_type_name);
}

void pmpi_type_set_name__(
MPI_Datatype type,
char* type_name CHAR_MIXED(size_type_name),
int * ierror, CHAR_END(size_type_name))
{
char *tmp_type_name, *ptr_type_name;
tmp_type_name = sctk_char_fortran_to_c(type_name, size_type_name, &ptr_type_name);
*ierror = MPI_Type_set_name( type,
type_name);
sctk_free(ptr_type_name);
}

 /*********MPI_Close_port**************/



void pmpi_close_port_(
char* port_name CHAR_MIXED(size_port_name),
int * ierror, CHAR_END(size_port_name))
{
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Close_port( port_name);
sctk_free(ptr_port_name);
}

void pmpi_close_port__(
char* port_name CHAR_MIXED(size_port_name),
int * ierror, CHAR_END(size_port_name))
{
char *tmp_port_name, *ptr_port_name;
tmp_port_name = sctk_char_fortran_to_c(port_name, size_port_name, &ptr_port_name);
*ierror = MPI_Close_port( port_name);
sctk_free(ptr_port_name);
}

 /*********MPI_Type_create_subarray**************/



void pmpi_type_create_subarray_(
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

void pmpi_type_create_subarray__(
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

 /*********MPI_Comm_set_attr**************/



void pmpi_comm_set_attr_(
MPI_Comm comm,
int comm_keyval,
void* attribute_val,
int * ierror)
{
*ierror = MPI_Comm_set_attr( comm,
comm_keyval,
attribute_val);
}

void pmpi_comm_set_attr__(
MPI_Comm comm,
int comm_keyval,
void* attribute_val,
int * ierror)
{
*ierror = MPI_Comm_set_attr( comm,
comm_keyval,
attribute_val);
}

 /*********MPI_Recv**************/



void pmpi_recv_(
void* buf,
int count,
MPI_Datatype datatype,
int source,
int tag,
MPI_Comm comm,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Recv( buf,
count,
datatype,
source,
tag,
comm,
status);
}

void pmpi_recv__(
void* buf,
int count,
MPI_Datatype datatype,
int source,
int tag,
MPI_Comm comm,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Recv( buf,
count,
datatype,
source,
tag,
comm,
status);
}

 /*********MPI_Comm_dup**************/



void pmpi_comm_dup_(
MPI_Comm comm,
MPI_Comm* newcomm,
int * ierror)
{
*ierror = MPI_Comm_dup( comm,
newcomm);
}

void pmpi_comm_dup__(
MPI_Comm comm,
MPI_Comm* newcomm,
int * ierror)
{
*ierror = MPI_Comm_dup( comm,
newcomm);
}

 /*********MPI_Waitall**************/



void pmpi_waitall_(
int count,
MPI_Request* array_of_requests,
MPI_Status* array_of_statuses,
int * ierror)
{
*ierror = MPI_Waitall( count,
array_of_requests,
array_of_statuses);
}

void pmpi_waitall__(
int count,
MPI_Request* array_of_requests,
MPI_Status* array_of_statuses,
int * ierror)
{
*ierror = MPI_Waitall( count,
array_of_requests,
array_of_statuses);
}

 /*********MPI_Comm_delete_attr**************/



void pmpi_comm_delete_attr_(
MPI_Comm comm,
int comm_keyval,
int * ierror)
{
*ierror = MPI_Comm_delete_attr( comm,
comm_keyval);
}

void pmpi_comm_delete_attr__(
MPI_Comm comm,
int comm_keyval,
int * ierror)
{
*ierror = MPI_Comm_delete_attr( comm,
comm_keyval);
}

 /*********MPI_Ssend**************/



void pmpi_ssend_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Ssend( buf,
count,
datatype,
dest,
tag,
comm);
}

void pmpi_ssend__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Ssend( buf,
count,
datatype,
dest,
tag,
comm);
}

 /*********MPI_Group_size**************/



void pmpi_group_size_(
MPI_Group group,
int* size,
int * ierror)
{
*ierror = MPI_Group_size( group,
size);
}

void pmpi_group_size__(
MPI_Group group,
int* size,
int * ierror)
{
*ierror = MPI_Group_size( group,
size);
}

 /*********MPI_Attr_put**************/



void pmpi_attr_put_(
MPI_Comm comm,
int keyval,
void* attribute_val,
int * ierror)
{
*ierror = MPI_Attr_put( comm,
keyval,
attribute_val);
}

void pmpi_attr_put__(
MPI_Comm comm,
int keyval,
void* attribute_val,
int * ierror)
{
*ierror = MPI_Attr_put( comm,
keyval,
attribute_val);
}

 /*********MPI_Barrier**************/



void pmpi_barrier_(
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Barrier( comm);
}

void pmpi_barrier__(
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Barrier( comm);
}

 /*********MPI_Win_free**************/



void pmpi_win_free_(
MPI_Win* win,
int * ierror)
{
*ierror = MPI_Win_free( win);
}

void pmpi_win_free__(
MPI_Win* win,
int * ierror)
{
*ierror = MPI_Win_free( win);
}

 /*********MPI_Alltoallw**************/

/* Skipped function MPI_Alltoallwwith conversion */
/* Skipped function MPI_Alltoallwwith conversion */


 /*********MPI_Alltoallv**************/

/* Skipped function MPI_Alltoallvwith conversion */
/* Skipped function MPI_Alltoallvwith conversion */


 /*********MPI_Bsend_init**************/



void pmpi_bsend_init_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Bsend_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

void pmpi_bsend_init__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Bsend_init( buf,
count,
datatype,
dest,
tag,
comm,
request);
}

 /*********MPI_Exscan**************/



void pmpi_exscan_(
void* sendbuf,
void* recvbuf,
int count,
MPI_Datatype datatype,
MPI_Op op,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Exscan( sendbuf,
recvbuf,
count,
datatype,
op,
comm);
}

void pmpi_exscan__(
void* sendbuf,
void* recvbuf,
int count,
MPI_Datatype datatype,
MPI_Op op,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Exscan( sendbuf,
recvbuf,
count,
datatype,
op,
comm);
}

 /*********MPI_Op_free**************/



void pmpi_op_free_(
MPI_Op* op,
int * ierror)
{
*ierror = MPI_Op_free( op);
}

void pmpi_op_free__(
MPI_Op* op,
int * ierror)
{
*ierror = MPI_Op_free( op);
}

 /*********MPI_Win_set_errhandler**************/



void pmpi_win_set_errhandler_(
MPI_Win win,
MPI_Errhandler errhandler,
int * ierror)
{
*ierror = MPI_Win_set_errhandler( win,
errhandler);
}

void pmpi_win_set_errhandler__(
MPI_Win win,
MPI_Errhandler errhandler,
int * ierror)
{
*ierror = MPI_Win_set_errhandler( win,
errhandler);
}

 /*********MPI_Accumulate**************/



void pmpi_accumulate_(
void* origin_addr,
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
*ierror = MPI_Accumulate( origin_addr,
origin_count,
origin_datatype,
target_rank,
target_disp,
target_count,
target_datatype,
op,
win);
}

void pmpi_accumulate__(
void* origin_addr,
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
*ierror = MPI_Accumulate( origin_addr,
origin_count,
origin_datatype,
target_rank,
target_disp,
target_count,
target_datatype,
op,
win);
}

 /*********MPI_Comm_get_name**************/



void pmpi_comm_get_name_(
MPI_Comm comm,
char* comm_name CHAR_MIXED(size_comm_name),
int* resultlen,
int * ierror, CHAR_END(size_comm_name))
{
*ierror = MPI_Comm_get_name( comm,
comm_name,
resultlen);
sctk_char_c_to_fortran(comm_name, size_comm_name);
}

void pmpi_comm_get_name__(
MPI_Comm comm,
char* comm_name CHAR_MIXED(size_comm_name),
int* resultlen,
int * ierror, CHAR_END(size_comm_name))
{
*ierror = MPI_Comm_get_name( comm,
comm_name,
resultlen);
sctk_char_c_to_fortran(comm_name, size_comm_name);
}

 /*********MPI_Cart_sub**************/

/* Skipped function MPI_Cart_subwith conversion */
/* Skipped function MPI_Cart_subwith conversion */


 /*********MPI_Group_range_excl**************/



void pmpi_group_range_excl_(
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

void pmpi_group_range_excl__(
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

 /*********MPI_Comm_split**************/



void pmpi_comm_split_(
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

void pmpi_comm_split__(
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

 /*********MPI_Comm_set_errhandler**************/



void pmpi_comm_set_errhandler_(
MPI_Comm comm,
MPI_Errhandler errhandler,
int * ierror)
{
*ierror = MPI_Comm_set_errhandler( comm,
errhandler);
}

void pmpi_comm_set_errhandler__(
MPI_Comm comm,
MPI_Errhandler errhandler,
int * ierror)
{
*ierror = MPI_Comm_set_errhandler( comm,
errhandler);
}

 /*********MPI_Request_get_status**************/



void pmpi_request_get_status_(
MPI_Request request,
int* flag,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Request_get_status( request,
flag,
status);
}

void pmpi_request_get_status__(
MPI_Request request,
int* flag,
MPI_Status* status,
int * ierror)
{
*ierror = MPI_Request_get_status( request,
flag,
status);
}

 /*********MPI_Group_free**************/



void pmpi_group_free_(
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Group_free( group);
}

void pmpi_group_free__(
MPI_Group* group,
int * ierror)
{
*ierror = MPI_Group_free( group);
}

 /*********MPI_Type_create_keyval**************/



void pmpi_type_create_keyval_(
MPI_Type_copy_attr_function* type_copy_attr_fn,
MPI_Type_delete_attr_function* type_delete_attr_fn,
int* type_keyval,
void* extra_state,
int * ierror)
{
*ierror = MPI_Type_create_keyval( type_copy_attr_fn,
type_delete_attr_fn,
type_keyval,
extra_state);
}

void pmpi_type_create_keyval__(
MPI_Type_copy_attr_function* type_copy_attr_fn,
MPI_Type_delete_attr_function* type_delete_attr_fn,
int* type_keyval,
void* extra_state,
int * ierror)
{
*ierror = MPI_Type_create_keyval( type_copy_attr_fn,
type_delete_attr_fn,
type_keyval,
extra_state);
}

 /*********MPI_Graphdims_get**************/



void pmpi_graphdims_get_(
MPI_Comm comm,
int* nnodes,
int* nedges,
int * ierror)
{
*ierror = MPI_Graphdims_get( comm,
nnodes,
nedges);
}

void pmpi_graphdims_get__(
MPI_Comm comm,
int* nnodes,
int* nedges,
int * ierror)
{
*ierror = MPI_Graphdims_get( comm,
nnodes,
nedges);
}

 /*********MPI_Comm_rank**************/



void pmpi_comm_rank_(
MPI_Comm comm,
int* rank,
int * ierror)
{
*ierror = MPI_Comm_rank( comm,
rank);
}

void pmpi_comm_rank__(
MPI_Comm comm,
int* rank,
int * ierror)
{
*ierror = MPI_Comm_rank( comm,
rank);
}

 /*********MPI_Cancel**************/



void pmpi_cancel_(
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Cancel( request);
}

void pmpi_cancel__(
MPI_Request* request,
int * ierror)
{
*ierror = MPI_Cancel( request);
}

 /*********MPI_Win_fence**************/



void pmpi_win_fence_(
int assert,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_fence( assert,
win);
}

void pmpi_win_fence__(
int assert,
MPI_Win win,
int * ierror)
{
*ierror = MPI_Win_fence( assert,
win);
}

 /*********MPI_Win_create_keyval**************/



void pmpi_win_create_keyval_(
MPI_Win_copy_attr_function* win_copy_attr_fn,
MPI_Win_delete_attr_function* win_delete_attr_fn,
int* win_keyval,
void* extra_state,
int * ierror)
{
*ierror = MPI_Win_create_keyval( win_copy_attr_fn,
win_delete_attr_fn,
win_keyval,
extra_state);
}

void pmpi_win_create_keyval__(
MPI_Win_copy_attr_function* win_copy_attr_fn,
MPI_Win_delete_attr_function* win_delete_attr_fn,
int* win_keyval,
void* extra_state,
int * ierror)
{
*ierror = MPI_Win_create_keyval( win_copy_attr_fn,
win_delete_attr_fn,
win_keyval,
extra_state);
}

 /*********MPI_Errhandler_free**************/



void pmpi_errhandler_free_(
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_free( errhandler);
}

void pmpi_errhandler_free__(
MPI_Errhandler* errhandler,
int * ierror)
{
*ierror = MPI_Errhandler_free( errhandler);
}

 /*********MPI_Win_test**************/



void pmpi_win_test_(
MPI_Win win,
int* flag,
int * ierror)
{
*ierror = MPI_Win_test( win,
flag);
}

void pmpi_win_test__(
MPI_Win win,
int* flag,
int * ierror)
{
*ierror = MPI_Win_test( win,
flag);
}

 /*********MPI_Type_match_size**************/



void pmpi_type_match_size_(
int typeclass,
int size,
MPI_Datatype* type,
int * ierror)
{
*ierror = MPI_Type_match_size( typeclass,
size,
type);
}

void pmpi_type_match_size__(
int typeclass,
int size,
MPI_Datatype* type,
int * ierror)
{
*ierror = MPI_Type_match_size( typeclass,
size,
type);
}

 /*********MPI_Send**************/



void pmpi_send_(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Send( buf,
count,
datatype,
dest,
tag,
comm);
}

void pmpi_send__(
void* buf,
int count,
MPI_Datatype datatype,
int dest,
int tag,
MPI_Comm comm,
int * ierror)
{
*ierror = MPI_Send( buf,
count,
datatype,
dest,
tag,
comm);
}