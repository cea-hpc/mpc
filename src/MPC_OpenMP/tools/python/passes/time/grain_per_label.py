from .. import ipass

from operator import add

class GrainPerLabelPass(ipass.Pass):

    DEPENDENCIES = ["time.workload", "papi.accumulate"]
    NAME = 'time.grain_per_label'

    def __init__(self):
        pass

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "GrainPerLabelPass(...)"

    def on_start(self, config):
        pass

    def on_end(self, config):
        pass

    def on_process_inspection_start(self, env):
        pass

# {'create': RecordCreate(pid=3088297, tid=5, time=1686226591146338, uid=19217, persistent_uid=0, properties/flags=['DEPEND'], label=EvalEOSForElems4, color=0, parent_uid=4, omp_priority=0, send=False, recv=False, allreduce=False), 'created': 1, 'delete': None, 'schedules': [RecordSchedule(pid=3088297, tid=1, time=1686226592623489, uid=19217, priority=0, properties/flags=['DEPEND', 'SCHEDULED'], schedule_id=19950, hwcounters=[0, 0, 0, 0]), RecordSchedule(pid=3088297, tid=1, time=1686226592623563, uid=19217, priority=0, properties/flags=['DEPEND', 'EXECUTED'], schedule_id=19950, hwcounters=[0, 0, 0, 0])], 'successors': [19627, 19504, 19422, 19340, 19299], 'predecessors': [19094, 19176], 'sends': [], 'recvs': [], 'allreduces': [], 'ref_predecessors': 0, 'npredecessors': 2, 'workload': 74}

    def on_process_inspection_end(self, env):
        tasks_per_label = {}
        for uid in env['tasks']:
            task = env['tasks'][uid]
            if task['create'] == None or len(task['create'].label) == 0:
                continue
            label = task['create'].label
            if label not in tasks_per_label:
                tasks_per_label[label] = []
            tasks_per_label[label].append(task)

        print("label time(us.) hwctr1 hwctr2 hwctr3 hwctr4")
        for label in tasks_per_label:
            grain = 0
            hwcounters = [0, 0, 0, 0]
            for task in tasks_per_label[label]:
                grain += task['workload']
                papi = env['papi'][task['create'].uid]['hwcounters']
                hwcounters = list(map(add, hwcounters, papi))
            grain = grain / len(tasks_per_label[label])
            hwcounters = [str(int(ctrs / len(tasks_per_label[label]))) for ctrs in hwcounters]
            print("{} {} {}".format(label, grain, ' '.join(hwcounters)))

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
