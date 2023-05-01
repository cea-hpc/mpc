from .. import ipass
import json

"""
"   An pass to trace overlap ratio over time on blocked tasks
"""

class CommunicationOverlapPass(ipass.Pass):

    NAME = "communication.overlap"

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "Pass(...)"

    def on_end(self, config):
        fname = "{}.overlap.csv".format(config['output'])
        with open(fname, 'w') as f:
            f.write("time overlap n_pending\n")
            for (time, overlap, n_pending) in self.overlap_over_time:
                f.write("{} {} {}\n".format(time, round(100 * overlap, 2), n_pending))
        print("exported to '{}'".format(fname))

        overlapable = 0
        i = 0
        j = 0
        while i < len(self.overlap_over_time) and j < len(self.overlap_over_time):
            (t0, _, n0) = self.overlap_over_time[i]
            j = i + 1
            while j < len(self.overlap_over_time):
                (t1, _, n1) = self.overlap_over_time[j]
                if n0 != n1:
                    dt = t1 - t0
                    overlapable += dt * n0
                    i = j
                    break
                j = j + 1

        stats = {
            'overlaped (no recv.)':  round(self.overlaped, 6),
            'overlapable (per core)':  round(overlapable, 6),       # this should be equals to the comm. duration
        }
        print(json.dumps(stats, indent=4))

    def on_process_inspection_start(self, env):
        self.n_pending = 0                          # current number of pendings (= task block)
        self.windows = {}                           # current overlapping windows
        self.overlap_over_time = [(0.0, 0.0, 0)]    # % overlap over time
        self.overlaped = 0.0                        # total overlaped time

    def update(self, env, pending_variation):

        def overlap():
            n_window  = len(self.windows)
            if n_window == 0:
                return 0
            n_tasks   = 0
            n_threads = len(env['bound'])
            for tid in env['bound']:
                if len(env['bound'][tid]) > 0:
                    n_tasks += 1
            return n_tasks / n_threads

        time = round((env['record'].time - env['records'][0].time) * 1e-6, 6)
        self.n_pending += pending_variation
        overlap_ratio = overlap()
        if len(self.overlap_over_time) == 0 or self.overlap_over_time[-1][1] != self.n_pending or self.overlap_over_time[-1][2] != overlap_ratio:
            self.overlap_over_time.append((time, overlap_ratio, self.n_pending))
            if len(self.overlap_over_time) >= 2:
                dt = self.overlap_over_time[-1][0] - self.overlap_over_time[-2][0]
                ov = self.overlap_over_time[-2][1] * len(env['bound'])
                self.overlaped += ov * dt

    def on_task_started(self, env):
        self.update(env, 0)

    def on_task_completed(self, env):
        self.update(env, 0)

    def on_task_paused(self, env):
        self.update(env, 0)

    def on_task_resumed(self, env):
        self.update(env, 0)

    def on_comm(self, env):
        record = env['record']
        if env['record'].completed:
            assert(record.uid in self.windows)
            del self.windows[record.uid]
            self.update(env, -1)
        else:
            # if task is unblocked before it is blocked
            assert(record.uid not in self.windows)
            self.windows[record.uid] = record
            self.update(env, +1)

    def on_task_recv(self, env):
        pass    # don't track recv overlap windows

    def on_task_send(self, env):
        self.on_comm(env)

    def on_task_allreduce(self, env):
        self.on_comm(env)
