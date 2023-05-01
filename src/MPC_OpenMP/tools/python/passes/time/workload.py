from .. import ipass

class TimeWorkloadPass(ipass.Pass):

    NAME = 'time.workload'

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "TimeWorkloadPass(...)"

    def on_start(self, config):
        pass

    def on_end(self, config):
        pass

    def on_process_inspection_start(self, env):
        env['workload'] = {}

    def on_process_inspection_end(self, env):
        pass

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
        uid = env['record'].uid
        schedules = env['schedules']
        assert(len(schedules[uid]) % 2 == 0)
        env['workload'][uid] = sum(schedules[uid][i+1].time - schedules[uid][i].time for i in range(0, len(schedules[uid]), 2))

    def on_task_blocked(self, env):
        pass

    def on_task_unblocked(self, env):
        pass

    def on_task_paused(self, env):
        pass

    def on_task_resumed(self, env):
        pass
