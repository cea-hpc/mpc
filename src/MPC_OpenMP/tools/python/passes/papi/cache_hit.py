"""
"   This pass computes the cache hit ratio per tasks.
"   The run must have been traced using one of the following configuration
"
"       MPCFRAMEWORK_OMP_TASK_TRACEPAPIEVENTS=PAPI_X,PAPI_LST_INS
"
"   Where 'PAPI_X' is an hardware counter measuring the number of cache misses
"   (for instance : PAPI_L1_DCM, PAPI_L1_TCM, PAPI_L2_DCM, PAPI_L3_TCM...)
"
"""

import json

from . import papi_pass

CTR_MISS = 0
CTR_LST  = 1

class CacheHitPAPIPass(papi_pass.PAPIPass):

    NAME = "papi.cache_hit"

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "CacheHitPAPIPass(...)"

    def on_start(self, config):
        pass

    def on_end(self, config):
        pass

    def on_process_inspection_start(self, env):
        pass

    def on_process_inspection_end(self, env):
        total_miss = 0
        total_lst  = 0
        for uid in env['papi']:
            miss = env['papi'][uid]['hwcounters'][CTR_MISS]
            lst  = env['papi'][uid]['hwcounters'][CTR_LST]
            if lst != 0:
                env['papi'][uid]['cache hit'] = round(1.0 - miss/lst, 3)
            else:
                env['papi'][uid]['cache hit'] = 0
            total_miss += miss
            total_lst  += lst
        stats = {
            'cache_hit': round(1.0 - total_miss/total_lst, 3)
        }
        print(json.dumps(stats, indent=4))

    def on_task_create(self, env):
        pass

    def on_task_delete(self, env):
        pass

    def on_task_dependency(self, env):
        pass

    def on_task_ready(self, env):
        pass

    def on_task_started(self, env):
        pass

    def on_task_completed(self, env):
        pass

    def on_task_blocked(self, env):
        pass

    def on_task_unblocked(self, env):
        pass

    def on_task_paused(self, env):
        pass

    def on_task_resumed(self, env):
        pass
