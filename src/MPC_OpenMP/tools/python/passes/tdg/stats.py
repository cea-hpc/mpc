import json

from .. import ipass

class TDGStatsPass(ipass.Pass):

    NAME = "tdg.stats"

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "TDGStatsPass(...)"

    def on_start(self, config):
        pass

    def on_end(self, config):
        pass

    def on_process_inspection_start(self, env):
        self.ntasks = 0
        self.narcs = 0

    def on_process_inspection_end(self, env):
        print(json.dumps({'tasks': self.ntasks, 'arcs': self.narcs}, indent=4))

    def on_task_create(self, env):
        self.ntasks += 1

    def on_task_delete(self, env):
        pass

    def on_task_dependency(self, env):
        self.narcs += 1

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
