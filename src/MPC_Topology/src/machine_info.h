
#ifndef MPC_COMMON_MACHINE_INFO_H_
#define MPC_COMMON_MACHINE_INFO_H_

#include <mpc_config.h>

#include <sched.h>

#if defined( HP_UX_SYS )
#include <sys/param.h>
#include <sys/pstat.h>
#endif


#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#else
struct utsname
{
	char *sysname;
	char *nodename;
	char *release;
	char *version;
	char *machine;
};

static int
uname( struct utsname *buf )
{
	buf->sysname = "";
	buf->nodename = "";
	buf->release = "";
	buf->version = "";
	buf->machine = "";
	return 0;
}

#endif


#endif /* MPC_COMMON_MACHINE_INFO_H_ */
