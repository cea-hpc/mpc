# TODO: make this a pass, and have other TDG passes depends on it

import json
import math
import queue
import sys
import time

from passes.cte import utils

COLOR_GRADIENT_1 = [180, 0, 0]
COLOR_GRADIENT_2 = [255, 255, 255]

def dotprint(f, depth, s):
    f.write('{}{}\n'.format(' ' * depth * 4, s))

def task_uuid(pid, uid):
    return 'Tx{}x{}'.format(pid, uid)

# a task node
class Node:
    def __init__(self, uid, env):
        self.critical = False
        self.critical_index = -1
        self.uuid = task_uuid(env['pid'], uid)
        self.uid = uid

        properties = utils.record_to_properties_and_statuses(env['tasks'][uid]['create'])
        self.is_control_flow = 'CONTROL_FLOW' in properties

        # retrieve first schedule id for this task
        schedules = env['schedules'][uid]

        self.successors = env['successors'][uid] if uid in env['successors'] else {}
        self.predecessors = env['predecessors'][uid] if uid in env['predecessors'] else {}

        task = env['tasks'][uid]
        self.label = task['create'].label if task['create'] is not None else ""
        self.priority = schedules[0].priority
        self.omp_priority = task['create'].omp_priority if task['create'] is not None else 0
        self.time = sum([schedules[i + 1].time - schedules[i].time for i in range(0, len(schedules), 2)])
        self.schedule_id = min(x.schedule_id for x in schedules)
        self.last_sched = schedules[-2]

    def dump(self, config, f, g, dumped):
        if self.uuid in dumped:
            return
        dumped[self.uuid] = True
        s = ''
        if config['label']:
            s = self.label
        if config['priority']:
            s += "\\n{}".format(self.priority)
        if config['omp_priority']:
            s += "\\n{}".format(self.omp_priority)
        if config['time']:
            s += "\\n{}".format(self.time / 1e6)
        if config['gradient']:
            if g.last_schedule_id != 0:
                c = self.schedule_id / float(g.last_schedule_id)   # linear
                c = c**2
            else:
                c = 1
            rgb = tuple(int((1.0 - c) * component[0] + c * component[1]) for component in zip(COLOR_GRADIENT_1, COLOR_GRADIENT_2))
            color = '#%02x%02x%02x' % rgb
            shape = 'circle'
            if self.is_control_flow:
                shape = 'diamond'
            dotprint(f, 2, '{} [style=filled, fillcolor="{}", label="{}", shape="{}"];'.format(self.uuid, color, s, shape))
        else:
            dotprint(f, 2, '{} [label="{}"];'.format(self.uuid, s))
        for suid in self.successors:
            snode = g.nodes[suid]
            snode.dump(config, f, g, dumped)
            s = '{} -> {}'.format(self.uuid, snode.uuid)
            if self.critical and snode.critical and abs(self.critical_index - snode.critical_index) == 1:
                s += ' [color=red]'
            s += ';'
            dotprint(f, 2, s)

    def set_critical(self, index):
        self.critical = True
        self.critical_index = index

    def __str__(self):
        return 'Node(label={}, time={}, uid={}, pid={})'.format(self.label, self.time, self.uid, self.tdg.pid)

    def __repr__(self):
        return str(self)

# a process task tdg
class TDG:
    def __init__(self, env):
        self.pid = env['pid']
        self.nodes = {}
        self.roots = []
        self.leaves = []
        self.last_schedule_id = 0

        # for each task of the process
        tasks = env['tasks']
        for uid in tasks:
            task = tasks[uid]
            if uid not in env['schedules'] or len(env['schedules'][uid]) == 0:
                continue
            schedules = env['schedules'][uid]

            # ignore initial tasks
            if (task['create'] and task['create'].parent_uid == 4294967295):
                continue

            # create tdg node
            node = Node(uid, env)
            self.nodes[uid] = node
            if len(node.successors) == 0:
                self.leaves.append(node)
            if len(node.predecessors) == 0:
                self.roots.append(node)

            # set the last schedule id of the tdg
            self.last_schedule_id = max(node.schedule_id, self.last_schedule_id)

    def dump(self, config, f):
        dotprint(f, 1, 'subtdg cluster_P{}'.format(self.pid))
        dotprint(f, 1, '{')

        dotprint(f, 2, 'label="Process {}";'.format(self.pid))
        dotprint(f, 2, 'color="#aaaaaa";')

        dumped = {}
        for node in self.roots:
            node.dump(config, f, self, dumped)

        dotprint(f, 1, '}')

# global task tdg
class GlobalTDG:
    def __init__(self):
        self.tdgs = {}
        self.nodes = {}

    def add_tdg(self, pid, tdg):
        self.tdgs[pid] = tdg
        for uid in tdg.nodes:
            node = tdg.nodes[uid]
            self.nodes[node.uuid] = node

    def dump(self, config, f):
        dotprint(f, 0, 'digraph G')
        dotprint(f, 0, '{')

        # print legend
        dotprint(f, 1, 'subgraph cluster_LEGEND')
        dotprint(f, 1, '{')

        legend = 'LABEL'
        if config['priority']:
            legend += '\\nPRIORITY (internal)'
        if config['omp_priority']:
            legend += '\\nOMP_PRIORITY'
        if config['time']:
            legend += '\\nTIME (s.)'
        dotprint(f, 2, 'LEGEND [label="{}"];'.format(legend))
        dotprint(f, 2, 'label="Legend";')
        dotprint(f, 2, 'color="#aaaaaa";')
        dotprint(f, 1, '}')

        # print subtdgs
        for pid in self.tdgs:
            self.tdgs[pid].dump(config, f)

        dotprint(f, 0, '}')

    # compute local process critical path
    def compute_critical(self):
        print("computing critical path...",  file=sys.stderr)
        predecessors = {}
        d = {uuid: -math.inf for uuid in self.nodes}
        q = queue.Queue()
        last_time_dump = time.time()

        for pid in self.tdgs:
            tdg = self.tdgs[pid]
            for node in tdg.roots:
                d[node.uuid] = node.time
                q.put(node)
                while not q.empty():
                    node = q.get()
                    # TODO: use this when reimplementing global tdg
                    #successors = [node.tdg.nodes[uid] for uid in node.successors]
                    successors = [tdg.nodes[uid] for uid in node.successors]
                    #if node in self.send_to_recv:
                    #    successors += self.send_to_recv[node]
                    for snode in successors:
                        if d[snode.uuid] < d[node.uuid] + snode.time:
                            d[snode.uuid] = d[node.uuid] + snode.time
                            predecessors[snode.uuid] = node.uuid
                            q.put(snode)
        # get leaf
        node = None
        for pid in self.tdgs:
            tdg = self.tdgs[pid]
            leaf = max(tdg.leaves, key=lambda leaf: d[leaf.uuid])
            if node == None or d[node.uuid] < leaf[node.uuid]:
                node = leaf

        path = []
        while True:
            path.insert(0, node)
            if node.uuid in predecessors:
                node = self.nodes[predecessors[node.uuid]]
            else:
                break
        return path
