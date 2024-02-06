from .. import ipass
import queue

"""
"   An pass to trace the amount of computation
"   leading to each communication
"""

class CommunicationPredecessorsPass(ipass.Pass):

    NAME = "communication.predecessors"

    def __init__(self):
        self.envs = {}
        self.communication_computations = {}

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "Pass(...)"

    def on_start(self, config):
        pass

    def on_end(self, config):
        fname="{}.communication-workload.csv".format(config['output'])
        with open(fname, 'w') as f:
            f.write("color label uid workload comm-duration\n")
            for pid in self.envs:
                env = self.envs[pid]
                comm = self.communication_computations[pid]
                for uid in comm:
                    task = env['tasks'][uid]['create']
                    q = queue.Queue()
                    q.put(uid)
                    visited = {uid: True}
                    work = 0
                    while not q.empty():
                        suid = q.get()
                        for puid in env['predecessors'][suid]:
                            if puid in visited:
                                continue
                            visited[puid] = True
                            predecessor = env['tasks'][puid]['create']

                            # breaking condition : don't propagate on preceding communications of the same type
                            if      task.send == predecessor.send               \
                                and task.recv == predecessor.recv               \
                                and task.allreduce == predecessor.allreduce:
                                continue

                            # breaking condition : don't propagate on other tasks colors
                            if task.color != predecessor.color:
                                continue
                            work += env['workload'][puid]
                            q.put(puid)

                    work = round(1e-6 * work, 6)

                    if   task.send:
                        label = 'send'
                    elif task.recv:
                        label = 'recv'
                    elif task.allreduce:
                        label = 'allreduce'
                    else:
                        label = 'ERROR'

                    if 'comm-duration' not in env['tasks'][uid]:
                        comm = 0
                    else:
                        comm = env['tasks'][uid]['comm-duration']

                    f.write('{} "{}" {} {} {}\n'.format(task.color, label, task.uid, work, comm))

            print("exported to '{}'".format(fname))

    def on_process_inspection_start(self, env):
        self.communication_computations[env['pid']] = []

    def on_process_inspection_end(self, env):
        self.envs[env['pid']] = env

    def on_task_completed(self, env):
        record = env['record']
        if hasattr(record, 'in_uid'):
            uid = record.in_uid
        elif hasattr(record, 'uid'):
            uid = record.uid
        else:
            assert(0)
        predecessors = env['predecessors'][uid]
        task = env['tasks'][uid]['create']
        if task.send or task.allreduce: # don't trace recv
            self.communication_computations[env['pid']].append(uid)
