import json
import sys

from . import catapult
from . import utils
from .. import ipass

# TODO: réimplementé les comms. MPI

CONFIG = {
    'color': not True,
    'schedule': True,
    'create': not True,
    'delete': not True,
    'control': not True,
    'dependences': True,
}

class GoogleCtePass(ipass.Pass):

    NAME = "gantt.cte"

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "GoogleCtePass(...)"

    def on_start(self, config):
        self.cte = {
            'traceEvents': []
        }

    def on_end(self, config):
        src = config['input']
        dst = "{}.json".format(config['output'])
        with open(dst, "w") as f:
            print("writing `{}` to disk...".format(dst))
            json.dump(self.cte, f)
            f.write("\n")
            print("`{}` converted to `{}`".format(src, dst))

    # trace task creation
    def trace_task_create(self, env, task):
        if CONFIG['create']:
            create = task['create']
            event = {
                'name': create.label,
                'cat':  'task-create',
                'ph':   'X',
                'ts':   create.time,
                'dur':  1,
                'pid':  env['rank'],
                'tid':  create.tid,
                'args': {
                    'uid': create.uid,
                    'ts': create.time,
                    'omp_priority': create.omp_priority,
                    'properties': utils.record_to_properties_and_statuses(create)
               }
            }
            self.cte['traceEvents'].append(event)

        # creation
        if CONFIG['control']:
            if create.parent_uid in env['tasks']:
                parent = env['tasks'][create.parent_uid]['create']
                cat = 'task-create-control'
                identifier = "{}-{}-{}".format(cat, create.uid, parent.uid)
                event = {
                    'name' : identifier,
                    'cat': cat,
                    'ph': 's',
                    'ts': parent.time + 0.5,
                    'pid': pid_to_rank(processes, parent.pid),
                    'tid': parent.tid,
                    'id': hash_string(identifier)
                }
                cte['traceEvents'].append(event)
                event = {
                    'name' : identifier,
                    'cat': cat,
                    'ph': 't',
                    'ts': create.time + 0.5,
                    'pid': pid_to_rank(processes, create.pid),
                    'tid': create.tid,
                    'id': hash_string(identifier)
                }
                cte['traceEvents'].append(event)


    # trace task deletion
    def trace_task_delete(self, env, task):
        create = task['create']
        delete = task['delete']
        event = {
            'name': create.label,
            'cat':  'task-delete',
            'ph':   'X',
            'ts':   delete.time,
            'dur':  1,
            'pid':  env['rank'],
            'tid':  delete.tid,
            'args': {
                'uid': delete.uid,
                'ts': delete.time,
                'priority': delete.priority,
                'properties': utils.record_to_properties_and_statuses(delete)
           }
        }
        self.cte['traceEvents'].append(event)

    # trace a task schedule between the two given records
    def trace_task_schedule(self, env, uid):
        assert(uid in env['tasks'])
        schedules = env['schedules'][uid]
        assert(len(schedules) % 2 == 0)
        for i in range(0, len(schedules), 2):
            r1 = schedules[i + 0]
            r2 = schedules[i + 1]
            assert(r1.uid == r2.uid)
            task = env['tasks'][r1.uid]['create']
            event = {
                'name': task.label,
                'cat':  'task-schedule',
                'ph':   'X',
                'ts':   r1.time,
                'dur':  r2.time - r1.time,
                #'dur':  max(r2.time - r1.time, 1),
                'pid':  env['rank'],
                'tid':  r1.tid,
                'args': {
                    'uid': r1.uid,
                    'ts': r1.time,
                    'created': task.time,
                    'priority': r1.priority,
                    'properties_begin': utils.record_to_properties_and_statuses(r1),
                    'properties_end': utils.record_to_properties_and_statuses(r2),
                    'color': task.color,
                    'hwcounters_begin': r1.hwcounters,
                    'hwcounters_end': r2.hwcounters,
                }
            }
            if CONFIG['color']:
                event['cname'] = catapult.colors[task.color % len(catapult.colors)]
            if task.recv or task.send or task.allreduce:
                event['args']['mpi'] = []
                if task.recv:
                    event['args']['mpi'].append('recv')
                if task.send:
                    event['args']['mpi'].append('send')
                if task.allreduce:
                    event['args']['mpi'].append('allreduce')
            self.cte['traceEvents'].append(event)

    def trace_task_dependences(self, env, succ_uid):
        if succ_uid in env['predecessors']:
            schedules = env['schedules']
            predecessors = env['predecessors'][succ_uid]
            pid = env['rank']
            for pred_uid in predecessors:
                r1 = schedules[pred_uid][-1]
                r2 = schedules[succ_uid][0] # TODO : this may fail is task with 'uid2' was cancelled
                identifier = "dependency-{}-{}-{}-{}-{}".format(pid, r1.tid, r2.tid, pred_uid, succ_uid)
                event = {
                    'name' : identifier,
                    'cat': 'dependencies',
                    'ph': 's',
                    'ts': r1.time,
                    'pid': pid,
                    'tid': r1.tid,
                    'id': utils.hash_string(identifier)
                }
                self.cte['traceEvents'].append(event)
                event = {
                    'name' : identifier,
                    'cat': 'dependencies',
                    'ph': 't',
                    'ts': r2.time + 0.000001,
                    'pid': pid,
                    'tid': r2.tid,
                    'id': utils.hash_string(identifier)
                }
                self.cte['traceEvents'].append(event)

    def on_process_inspection_start(self, env):
        for tid in env['bound']:
            self.cte["traceEvents"].append({
                'name': "thread_name",
                'ph': "M",
                'pid': env['rank'],
                'tid': tid,
                'args': {
                    'name': "omp thread {}".format(tid)
                }
            })

    def on_process_inspection_end(self, env):
        for uid in env['tasks']:
            task = env['tasks'][uid]
            self.trace_task_create(env, task)
            self.trace_task_delete(env, task)

        for uid in env['schedules']:
            self.trace_task_schedule(env, uid)

        for uid in env['predecessors']:
            self.trace_task_dependences(env, uid)

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
