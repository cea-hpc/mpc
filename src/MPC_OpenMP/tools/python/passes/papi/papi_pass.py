from .. import ipass

class PAPIPass(ipass.Pass):

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "PAPIPass(...)"

    def on_start(self, config):
        assert(0)

    def on_end(self, config):
        assert(0)

    def on_process_inspection_start(self, env):
        assert(0)

    def on_process_inspection_end(self, env):
        assert(0)

    def on_task_create(self, env):
        assert(0)

    def on_task_delete(self, env):
        assert(0)

    def on_task_dependency(self, env):
        assert(0)

    def on_task_ready(self, env):
        assert(0)

    def on_task_started(self, env):
        assert(0)

    def on_task_completed(self, env):
        assert(0)

    def on_task_blocked(self, env):
        assert(0)

    def on_task_unblocked(self, env):
        assert(0)

    def on_task_paused(self, env):
        assert(0)

    def on_task_resumed(self, env):
        assert(0)
