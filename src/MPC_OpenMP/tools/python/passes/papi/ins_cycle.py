"""
"   This pass computes the instruction / cycle per tasks.
"   The run must have been traced using
"
"       MPCFRAMEWORK_OMP_TASK_TRACEPAPIEVENTS=PAPI_TOT_INS,PAPI_TOT_CYC
"""

from . import papi_pass

CTR_INS = 0
CTR_CYC = 1

class InsCyclePAPIPass(papi_pass.PAPIPass):

    NAME = "papi.ins_per_cycle"

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "InsCyclePAPIPass(...)"

    def on_start(self, config):
        pass

    def on_end(self, config):
        pass

    def on_process_inspection_start(self, env):
        pass

    def on_process_inspection_end(self, env):
        for uid in env['papi']:
            ins = env['papi'][uid]['hwcounters'][CTR_INS]
            cyc = env['papi'][uid]['hwcounters'][CTR_CYC]
            ins_p_cyc = 0
            if cyc != 0:
                ins_p_cyc = round(ins / cyc, 3)
            env['papi'][uid]['ins/cyc'] = ins_p_cyc

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
