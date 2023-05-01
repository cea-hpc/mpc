"""
"   This pass accumulate PAPI counters per tasks
"""

"""
"   TODO : maybe use numpy to accelerate sums here
"""

import json

from . import papi_pass

MAX_CTRS = 4

class AccumulatePAPIPass(papi_pass.PAPIPass):

    NAME = 'papi.accumulate'

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "AccumulatePAPIPass(...)"

    def on_start(self, config):
        pass

    def on_end(self, config):
        pass

    def on_process_inspection_start(self, env):
        env['papi'] = {}

    def on_process_inspection_end(self, env):
        hwcounters = [0 for i in range(MAX_CTRS)]
        for uid in env['papi']:
            for i in range(MAX_CTRS):
                hwcounters[i] += env['papi'][uid]['hwcounters'][i]
        stats = {
            'hwcounters': hwcounters
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
        # if there is multiple tasks stacking on a thread,
        # then couters may be counted twice.
        # TODO : handle this in this function :-)
        # assert(len(env['bound'][env['record'].tid]) == 1)
        pass

    def on_task_completed(self, env):
        uid = env['record'].uid
        schedules = env['schedules'][uid]
        assert(len(schedules) % 2 == 0)
        lst = []
        for i in range(MAX_CTRS):
            lst.append(sum(schedules[j+1].hwcounters[i] - schedules[j].hwcounters[i] for j in range(0, len(schedules), 2)))
        env['papi'][uid] = {
            'hwcounters': lst
        }

    def on_task_blocked(self, env):
        pass

    def on_task_unblocked(self, env):
        pass

    def on_task_paused(self, env):
        pass

    def on_task_resumed(self, env):
        pass
