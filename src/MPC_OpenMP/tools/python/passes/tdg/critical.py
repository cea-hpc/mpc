import json
import sys

from .. import ipass
from . import tdg

class TDGCriticalPass(ipass.Pass):

    NAME = "tdg.critical"

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "TDGCriticalPass(...)"

    def on_start(self, config):
        pass

    def on_end(self, config):
        pass

    def on_process_inspection_start(self, env):
        pass

    def on_process_inspection_end(self, env):
        gg = tdg.GlobalTDG()
        g = tdg.TDG(env)
        gg.add_tdg(env['pid'], g)

        # compute critical path
        critical_path = gg.compute_critical()
        Too = 0.
        for index, node in enumerate(critical_path):
            node.set_critical(index)
            Too += node.time
        Too = 1e-6 * Too
        stats = {}
        stats['Too (critical-path)'] = round(Too, 6)

        # TODO: depends on TimingPass
        if 'time' in env:
            P = env['time']['n-threads']
            T1 = env['time']['in-tasks']
            TS =  env['time']['makespan'] - env['time']['overhead'] / P
            stats['P'] = P
            stats['T1'] = round(T1, 6)
            stats['T1/P'] = round(T1/P, 6)
            stats['T(S) (makespan with no overheads)'] = round(TS, 6)
            stats['T1/P+Too'] = round(T1/P + Too, 6)
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
