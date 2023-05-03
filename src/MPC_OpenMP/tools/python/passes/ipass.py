class Pass():

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "Pass(...)"

    def on_start(self, config):
        pass

    def on_end(self, config):
        pass

    def on_process_inspection_start(self, env):
        pass

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
        pass

    def on_task_blocked(self, env):
        pass

    def on_task_unblocked(self, env):
        pass

    def on_task_paused(self, env):
        pass

    def on_task_resumed(self, env):
        pass

    def on_task_recv(self, env):
        pass

    def on_task_send(self, env):
        pass

    def on_task_allreduce(self, env):
        pass
