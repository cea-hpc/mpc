/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - CARRIBAULT Patrick patrick.carribault@cea.fr                     # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef ___MPC___GLOBAL_RENAMING__HEADER___FILE___
#define ___MPC___GLOBAL_RENAMING__HEADER___FILE___

#ifdef MPC_THREAD_ENABLED

	/* atexit forward declaration is needed by mpc.h.
	stdlib is not included since it would prevent it
	from recognizing _GNU_SOURCE when defined by users.
	This is due to mpc.h being included with -include
	and thus included before any other file and especially
	 user sources containing _GNU_SOURCE definitions.*/

	extern int atexit (void (*__func) (void));

	#define atexit mpc_thread_atexit
#endif

#ifndef MPC_NO_AUTO_MAIN_REDEF
	#undef main
	#ifdef __cplusplus
		#define main(...) long mpc_user_main_dummy__ (); extern "C" int mpc_user_main__(__VA_ARGS__)
	#else
		#define main(...) mpc_user_main__(__VA_ARGS__)
		#if defined(MPC_TLS_DLWRAP) && defined(MPC_USE_EXTLS)
			#define dlopen extls_dlopen
			#define dlsym extls_dlsym
			#define dlclose extls_dlclose
		#endif
	#endif
#endif

#endif
