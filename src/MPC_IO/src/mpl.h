#ifndef MPL_H_
#define MPL_H_

#include <dirent.h>
#include <string.h>

#include <mpc_mpi.h>

#include <mpc_common_debug.h>
#include <mpc_common_helper.h>

#define HAVE_MPIIO_CONST

#define MPL_UNREFERENCED_ARG(arg) (void)(arg)

#define MPIO_Request_c2f MPI_Request_c2f
#define MPIO_Request_f2c MPI_Request_f2c

#define MPL_malloc(a, b) malloc(a)
#define MPL_calloc(a, b, c) calloc(a ,b)
#define MPL_realloc(a,b,c) realloc(a, b)
#define MPL_free(a) free(a)

#define MPL_VG_MEM_INIT(addr_,len_)                 do {} while (0)

#define MPL_MIN(a,b) mpc_common_min(a,b)
#define MPL_MAX(a,b) mpc_common_max(a,b)

#define MPL_strnapp strncat
#define MPL_snprintf snprintf

void MPL_create_pathname(char *dest_filename, const char *dirname,
                         const char *prefix, const int is_dir);

/*********
 * LOCKS *
 *********/

void mpc_io_read_lock();
void mpc_io_write_lock();
void mpc_io_unlock();

void mpc_rwlock_read();
void mpc_rwlock_read_unlock();

void mpc_rwlock_write();
void mpc_rwlock_write_unlock();


void mpc_rwlock_strided_read();
void mpc_rwlock_strided_read_unlock();

void mpc_rwlock_strided_write();
void mpc_rwlock_strided_write_unlock();


#endif /* MPL_H_ */
