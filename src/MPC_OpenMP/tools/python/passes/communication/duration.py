from .. import ipass
import json

"""
"   An pass to trace communication duration
"""

class CommunicationDurationPass(ipass.Pass):

    NAME = "communication.time"

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "CommunicationDurationPass(...)"

    def on_start(self, config):
        pass

    def on_end(self, config):
        pass

    def on_process_inspection_start(self, env):
        self.windows = {}
        self.sends      = 0.0
        self.recvs      = 0.0
        self.allreduces = 0.0

    def on_process_inspection_end(self, env):
        stats = {
            'send-duration': round(self.sends, 6),
            'recv-duration': round(self.recvs, 6),
            'allreduce-duration': round(self.allreduces, 6),
        }
        print(json.dumps(stats, indent=4))

    def on_comm(self, env):
        record = env['record']
        if env['record'].completed:
            assert(record.uid in self.windows)
            dt = (record.time - self.windows[record.uid].time) * 1e-6
            env['tasks'][record.uid]['comm-duration'] = dt
            if env['tasks'][record.uid]['create'].send:
                self.sends += dt
            elif env['tasks'][record.uid]['create'].recv:
                self.recvs += dt
            elif env['tasks'][record.uid]['create'].allreduce:
                self.allreduces += dt
            del self.windows[record.uid]
        else:
            # if task is unblocked before it is blocked
            assert(record.uid not in self.windows)
            self.windows[record.uid] = record

    def on_task_recv(self, env):
        pass    # don't track recv overlap windows

    def on_task_send(self, env):
        self.on_comm(env)

    def on_task_allreduce(self, env):
        self.on_comm(env)

    def on_task_recv(self, env):
        self.on_comm(env)
