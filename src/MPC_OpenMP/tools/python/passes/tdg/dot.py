import json
import math
import queue
import sys
import time

from . import tdg
from .. import ipass

CONFIG = {
    'gradient': True,
    'label': True,
    'priority': False, #True,
    'omp_priority': False, #True,
    'critical': True, # TODO
    'time': not True, # TODO
}

class TDGDotPass(ipass.Pass):

    NAME = 'tdg.dot'

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "TDGDotPass(...)"

    def on_start(self, config):
        self.gg = tdg.GlobalTDG()

    def on_end(self, config):
        # compute critical path
        if CONFIG['critical']:
            critical_path = self.gg.compute_critical()
            for index, node in enumerate(critical_path):
                node.set_critical(index)

        # dump tdg
        src = config['input']
        dst = "{}.dot".format(config['output'])
        with open(dst, "w") as f:
            print("writing `{}` to disk...".format(dst))
            self.gg.dump(CONFIG, f)
            f.write("\n")
            print("`{}` converted to `{}`".format(src, dst))

    def on_process_inspection_start(self, env):
        pass

    def on_process_inspection_end(self, env):
        g = tdg.TDG(env)
        self.gg.add_tdg(env['pid'], g)

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
