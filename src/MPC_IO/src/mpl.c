#include "mpl.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <mpc_common_spinlock.h>

void MPL_create_pathname(char *dest_filename, const char *dirname,
                         const char *prefix, const int is_dir)
{
	if(dirname)
	{
		sprintf(dest_filename, "%s/%s-%d-%u%s", dirname, prefix, getpid(), (unsigned int)rand(), is_dir?"/":"");
	}
	else
	{
		sprintf(dest_filename, "%s-%d-%u%s", prefix, getpid(), (unsigned int)rand(), is_dir?"/":"");
	}
}

/************************************************************************/
/* Locks                                                                */
/************************************************************************/

/* Here we gathered the locks which are required by ROMIO
 * in order to guarantee writing thread safety between tasks.
 * The reason they have to be here is that in ROMIO locks would be privatized
 * as global variables and it is not what we want. */

static mpc_common_rwlock_t __io_rw_lock = SCTK_SPIN_RWLOCK_INITIALIZER;

void mpc_io_read_lock()
{
	mpc_common_spinlock_write_lock_yield(&__io_rw_lock);
}


void mpc_io_write_lock()
{
	mpc_common_spinlock_write_lock_yield(&__io_rw_lock);
}

void mpc_io_unlock()
{
	mpc_common_spinlock_write_unlock(&__io_rw_lock);
}

static mpc_common_spinlock_t __cslock = SCTK_SPINLOCK_INITIALIZER;

void mpc_io_critical_section_enter()
{
	mpc_common_spinlock_lock_yield(&__cslock);
}

void mpc_io_critical_section_leave()
{
	mpc_common_spinlock_unlock(&__cslock);
}