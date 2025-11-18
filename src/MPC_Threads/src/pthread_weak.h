#pragma once

#include "mpc_thread.h" // For symbol declaration (not mandatory)

#pragma weak sched_yield = mpc_thread_yield
#pragma weak raise       = mpc_thread_raise
#ifndef SCTK_DONOT_REDEFINE_KILL
#pragma weak kill        = mpc_thread_process_kill
#endif

#pragma weak sigpending  = mpc_thread_sigpending
#pragma weak sigsuspend  = mpc_thread_sigsuspend
#pragma weak sigwait     = mpc_thread_sigwait

#pragma weak sleep       = mpc_thread_sleep
#pragma weak usleep      = mpc_thread_usleep
#pragma weak nanosleep   = mpc_thread_nanosleep
