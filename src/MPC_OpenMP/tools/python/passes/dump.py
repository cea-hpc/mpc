from . import ipass

class DumpPass(ipass.Pass):

    NAME = "dump"

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
        print("Starting inspection of {}".format(env['rank']))

    def on_process_inspection_end(self, env):
        print("Finishing inspection of {}".format(env['rank']))

    def on_task_create(self, env):
        print("created {}".format(env['record'].uid))

    def on_task_delete(self, env):
        print("delete {}".format(env['record'].uid))

    def on_task_dependency(self, env):
        print("dependency {} -> {}".format(env['record'].out_uid, env['record'].in_uid))

    def on_task_ready(self, env):
        print(env['record'])
        record = env['record']
        if hasattr(record, "out_uid"):
            uid = record.out_uid
        else:
            uid = record.uid
        print("now ready {}".format(uid))

    def on_task_started(self, env):
        print("started {}".format(env['record'].uid))

    def on_task_completed(self, env):
        print("completed {}".format(env['record'].uid))

    def on_task_blocked(self, env):
        print("blocked {}".format(env['record'].uid))

    def on_task_unblocked(self, env):
        print("unblocked {}".format(env['record'].uid))

    def on_task_paused(self, env):
        print("paused {}".format(env['record'].uid))

    def on_task_resumed(self, env):
        print("resumed {}".format(env['record'].uid))
