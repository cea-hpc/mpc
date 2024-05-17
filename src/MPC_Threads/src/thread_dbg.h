#ifndef THREAD_DBG_H_
#define THREAD_DBG_H_


#ifndef MPC_Debugger
	#define sctk_thread_add(a, b)                     (void)(0)
	#define sctk_thread_remove(a)                     (void)(0)
	#define sctk_enable_lib_thread_db()               (void)(0)

	#define sctk_init_thread_debug(a)                 (void)(0)
	#define sctk_refresh_thread_debug(a, b)           (void)(0)
	#define sctk_refresh_thread_debug_migration(a)    (void)(0)

	#define sctk_init_idle_thread_dbg(a, b)           (void)(0)
	#define sctk_free_idle_thread_dbg(a)              (void)(0)

	#define sctk_report_creation(a)                   (void)(0)
	#define sctk_report_death(a)                      (void)(0)
#else
	#include "sctk_thread_dbg.h"
#endif

#endif /* THREAD_DBG_H_ */
