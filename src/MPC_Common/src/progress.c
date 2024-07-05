/* ############################# MPC License ############################## */
/* # Thu May  6 10:26:16 CEST 2021                                        # */
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
/* # Maintainers:                                                         # */
/* # - CARRIBAULT Patrick patrick.carribault@cea.fr                       # */
/* # - JAEGER Julien julien.jaeger@cea.fr                                 # */
/* # - PERACHE Marc marc.perache@cea.fr                                   # */
/* # - ROUSSEL Adrien adrien.roussel@cea.fr                               # */
/* # - TABOADA Hugo hugo.taboada@cea.fr                                   # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* # - CANAT Paul pcanat@paratools.fr                                     # */
/* # - BESNARD Jean-Baptiste jbbesnard@paratools.com                      # */
/* # - MOREAU Gilles gilles.moreau@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include "mpc_common_debug.h"
#include <mpc_common_progress.h>
#include <mpc_common_spinlock.h>

#define COMMON_PROGRESS_CALLBACK_MAX 32
#define COMMON_PROGRESS_FUNC_NOTFOUND -1

static mpc_common_spinlock_t progress_lock = MPC_COMMON_SPINLOCK_INITIALIZER;

static int num_callbacks = 0;
static progress_callback_func_t callbacks[COMMON_PROGRESS_CALLBACK_MAX];


static int _mpc_common_find_callback(progress_callback_func_t func) 
{
        int i;
        for (i = 0; i < num_callbacks; i++) {
                if (callbacks[i] == func) {
                        return i;
                }
        }
        return COMMON_PROGRESS_FUNC_NOTFOUND;
}

int mpc_common_progress_register(progress_callback_func_t func) 
{
        mpc_common_spinlock_lock(&progress_lock);
        if (_mpc_common_find_callback(func) != COMMON_PROGRESS_FUNC_NOTFOUND) {
                goto unlock;
        }

        if (num_callbacks >= COMMON_PROGRESS_CALLBACK_MAX) {
                mpc_common_debug_fatal("COMMON PROGRESS: reach progress "
                                       "callback limit.");
                goto unlock;
        }

        callbacks[num_callbacks++] = func;

unlock:
        mpc_common_spinlock_unlock(&progress_lock);

        return 0;
}

int mpc_common_progress_unregister(progress_callback_func_t func)
{
        int i, callback_idx;
        
        callback_idx = _mpc_common_find_callback(func);
        if (callback_idx == COMMON_PROGRESS_FUNC_NOTFOUND) {
                mpc_common_debug_error("COMMON PROGRESS: unregister unknown callback");
                return 1;
        }

        mpc_common_spinlock_lock(&progress_lock);

        //FIXME: there is a vulnerability here since another thread might be
        //       polling at the same time. Callback pointer should be loaded
        //       atomically.
        for (i = callback_idx; i < num_callbacks - 1; i++) {
                callbacks[i] = callbacks[i + 1];
        }
        num_callbacks--;

        mpc_common_spinlock_unlock(&progress_lock);

        return 0;
}

int mpc_common_progress()
{
        int i, rc = 0;

        for (i = 0; i < num_callbacks; i++) {
                rc |= callbacks[i]();
        }

        return rc; 
}
