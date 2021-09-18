import heapq
import json
import math
import os
import struct
import sys
import time

import queue

MAGIC         = "task"
HEADER_SIZE   = 16
BYTE_ORDER    = 'little' # or 'big' (endianess)

MPC_OMP_TASK_PROP = {
        'UNDEFERRED':   (1 << 0),
        'UNTIED':       (1 << 1),
        'EXPLICIT':     (1 << 2),
        'IMPLICIT':     (1 << 3),
        'INITIAL':      (1 << 4),
        'INCLUDED':     (1 << 5),
        'FINAL':        (1 << 6),
        'MERGED':       (1 << 7),
        'MERGEABLE':    (1 << 8),
        'DEPEND':       (1 << 9),
        'PRIORITY':     (1 << 10),
        'UP':           (1 << 11),
        'GRAINSIZE':    (1 << 12),
        'IF':           (1 << 13),
        'NOGROUP':      (1 << 14),
        'HAS_FIBER':    (1 << 15)
}

MPC_OMP_TASK_STATUSES = [
    'STARTED',
    'COMPLETED',
    'BLOCKING',
    'BLOCKED',
    'UNBLOCKED',
    'IN_BLOCKED_LIST'
]

MPC_OMP_TASK_LABEL_MAX_LENGTH = 64

# record sizes from C's sizeof()
RECORD_GENERIC_SIZE         = 16
RECORD_DEPENDENCY_SIZE      = 24
RECORD_SCHEDULE_SIZE        = 112
RECORD_ASYNC_SIZE           = 24
RECORD_FAMINE_OVERLAP_SIZE  = 24

MPC_OMP_TASK_TRACE_TYPE_DEPENDENCY      = 0
MPC_OMP_TASK_TRACE_TYPE_SCHEDULE        = 1
MPC_OMP_TASK_TRACE_TYPE_CREATE          = 2
MPC_OMP_TASK_TRACE_TYPE_FAMINE_OVERLAP  = 3
MPC_OMP_TASK_TRACE_TYPE_ASYNC           = 4
MPC_OMP_TASK_TRACE_TYPE_COUNT           = 5

def records_to_communications(records):
    communications = {}
    for pid in records:
        for record in records[pid]:
            if isinstance(record, RecordSchedule):
                if record.label.startswith('send') or record.label.startswith('recv'):
                    data = record.label.split('-')
                    name = data[0][:4:]
                    src = data[1]
                    dst = data[2]
                    tag = data[3]
                    if name not in communications:
                        communications[name] = {}
                    if src not in communications[name]:
                        communications[name][src] = {}
                    if dst not in communications[name][src]:
                        communications[name][src][dst] = {}
                    if tag not in communications[name][src][dst]:
                        communications[name][src][dst][tag] = []
                    communications[name][src][dst][tag].append(record)
    return communications

def parse_traces(traces):
    # parse records (TODO : maybe this can be optimized)
    records = traces_to_records(traces)

    # cte trace format
    cte = {
        'traceEvents': []
    }

    # inter-node communications
    communications = records_to_communications(records)

    # stats dict
    stats = {}

    # number of threads (total)
    n_threads = 0

    # timers
    total_time      = 0
    intask_time     = 0
    outtask_time    = 0
    blocked_time    = 0
    famine_time     = 0
    recv_time       = 0
    send_time       = 0

    # granularity
    granularities = {}

    max_process_total_time = -math.inf

    # return logical order of events
    def record_order(record):
        orders = {
            RecordCreate: 0,
            RecordDependency: 1,
            RecordSchedule: 2,
            RecordAsync: 3,
            RecordFamineOverlap: 4
        }
        return orders[type(record)]

    # replay the scheduling to compute stats
    for pid in records:

        # sort records by time
        records[pid] = sorted(records[pid], key=lambda x: (x.time, record_order(x)))

        # find first and last time a thread worked
        ti = records[pid][0].time
        tf = records[pid][-1].time

        # tasks[uid] => record : RecordCreate of task with given id
        tasks = {}

        # readyqueue[uid] => `True` : the task is ready for schedule
        readyqueue = {}

        # per-task schedules
        schedules = {}

        # blocked[uid] => `True | False` if the task with 'uid' is blocked
        blocked = {}

        # successors[uid] => [uid1, uid2, ..., uidn] : successors of 'uid'
        successors = {}

        # predecessors[uid] => n : number of remaining predecessors
        predecessors = {}

        # bind[tid] => [uid1, uid1, uid2, uid2, ..., uidn, uidn] : per-thread schedules
        bind = {}
        for record in records[pid]:
            if record.tid not in bind:
                bind[record.tid] = [] 

        # for each record on this process
        for i in range(len(records[pid])):
            record = records[pid][i]

            # creating a new task
            if isinstance(record, RecordCreate):
                tasks[record.uid] = record
                predecessors[record.uid] = record.npredecessors
                if record.npredecessors == 0:
                    readyqueue[record.uid] = True

            # dependency completion
            elif isinstance(record, RecordDependency):
                assert(record.in_uid in tasks)
                assert(record.out_uid in tasks)
                assert(record.in_uid not in readyqueue)
                # update predecessors
                predecessors[record.in_uid] -= 1
                if predecessors[record.in_uid] == 0:
                    readyqueue[record.in_uid] = True
                # add successors
                if record.out_uid not in successors:
                    successors[record.out_uid] = []
                successors[record.out_uid].append(record.in_uid)

            # task scheduling
            elif isinstance(record, RecordSchedule):
                # append schedule time to the granularity tracker list
                uuid = '{}-{}'.format(pid, record.uid)
                if uuid not in granularities:
                    granularities[uuid] = []
                granularities[uuid].append(record.time)

                # update lists
                properties = record_to_properties_and_statuses(record)

                # a task completed
                if 'COMPLETED' in properties:
                    assert(record.tid in bind)
# for the check bellow, use a stack instead, in case of recursive tasking
#                    assert(record.uid == bind[record.tid][-1].uid)

                # a task unblocked and resumed
                elif 'UNBLOCKED' in properties:
                    assert(record.uid in blocked)
                    del blocked[record.uid]

                # a task blocked and paused
                elif 'BLOCKING' in properties:
                    assert(record.tid in bind)
                    assert(record.uid == bind[record.tid][-1].uid)
                    blocked[record.uid] = record

                # a task started
                elif 'BLOCKED' not in properties:
                    assert(record.uid not in schedules)
                    assert(record.uid in readyqueue)
                    del readyqueue[record.uid]

                # register task scheduling
                if record.uid not in schedules:
                    schedules[record.uid] = []
                schedules[record.uid].append(record)

                # save thread bind task
                bind[record.tid].append(record)

                # CHECK FAMINES

                # if there is no other tasks ready
                if len(readyqueue) == 0:
                    # and if we just ended a task
                    if ('COMPLETED' in properties) or (('BLOCKING' in properties) and ('UNBLOCKED' not in properties)):
                        # find famine duration
                        nrecord = None
                        for j in range(i + 1, len(records[pid])):
                            nrecord = records[pid][j]
                            if isinstance(nrecord, RecordSchedule) and nrecord.tid == record.tid:
                                break
                            nrecord = None
                        if nrecord is None:
                            duration = tf - record.time
                        else:
                            duration = nrecord.time - record.time
                        famine_time += duration
                        if len(blocked) > 0:
                            blocked_time += duration

        # compute in-task and out-task time
        process_total_time      = (tf - ti) * len(bind)
        process_intask_time     = 0
        process_send_time       = 0
        process_recv_time       = 0
        process_outtask_time    = 0

        max_process_total_time  = max(max_process_total_time, tf - ti)

        # for each tasks
        for uid in schedules:
            # get schedules
            lst = schedules[uid]
            assert(len(lst) % 2 == 0)
            for i in range(0, len(lst), 2):
                r1 = lst[i + 0]
                r2 = lst[i + 1]
                assert(r1.tid == r2.tid)
                assert(r1.time <= r2.time)

                # statistics in-task time
                dt = r2.time - r1.time
                process_intask_time += dt
                if r1.label.startswith('send'):
                    process_send_time += dt
                elif r1.label.startswith('recv'):
                    process_recv_time += dt

                # cte : task schedule
                event = {
                    'name': r1.label,
                    'cat':  'task-schedule',
                    'ph':   'X',
                    'ts':   r1.time,
                    'dur':  max(r2.time - r1.time, 1),
                    'pid':  pid,
                    'tid':  r1.tid,
                    'args': {
                        'uid': uid,
                        'ts': r1.time,
                        'created': tasks[uid].time,
                        'priority': r1.priority,
                        'properties_begin': record_to_properties_and_statuses(r1),
                        'properties_end': record_to_properties_and_statuses(r2)
                   }
                }
                cte['traceEvents'].append(event)

            # cte : task block-resume events
            if len(lst) > 2:
                for i in range(0, len(lst) - 2, 2):
                    r0 = lst[i + 0]
                    r1 = lst[i + 1]
                    r2 = lst[i + 2]
                    r3 = lst[i + 3]
                    p1 = record_to_properties_and_statuses(r1)
                    p2 = record_to_properties_and_statuses(r2)
                    assert('BLOCKING' in p1)
                    assert('UNBLOCKED' in p2)
                    cat = 'block-resume'
                    identifier = "block-resume-{}-{}-{}-{}".format(pid, r1.tid, r2.tid, uid)
                    event = {
                        'name' : identifier,
                        'cat': cat,
                        'ph': 's',
                        'ts': r1.time - min(0.01, 0.01 * (r1.time - r0.time)),
                        'pid': pid,
                        'tid': r1.tid,
                        'id': identifier
                    }
                    cte['traceEvents'].append(event)
                    event = {
                        'name' : identifier,
                        'cat': cat,
                        'ph': 't',
                        'ts': r2.time + min(0.01, 0.01 * (r3.time - r2.time)),
                        'pid': pid,
                        'tid': r2.tid,
                        'id': identifier
                    }
                    cte['traceEvents'].append(event)

        # time spent outside tasks
        process_outtask_time = process_total_time - process_intask_time

        # append to process timers to global ones
        total_time      += process_total_time
        intask_time     += process_intask_time
        send_time       += process_send_time
        recv_time       += process_recv_time
        outtask_time    += process_outtask_time

        # increment number of thread counter
        n_threads += len(bind)

        # coherency check
        for uid in predecessors:
            assert(predecessors[uid] == 0)
        assert(len(readyqueue) == 0)
        assert(len(blocked) == 0)

        # cte dependencies
        for uid1 in successors:
            lst = successors[uid1]
            for uid2 in lst:
                r1 = schedules[uid1][-1]
                r2 = schedules[uid2][0]
                identifier = "dependency-{}-{}-{}-{}-{}".format(pid, r1.tid, r2.tid, uid1, uid2)
                event = {
                    'name' : identifier,
                    'cat': 'dependencies',
                    'ph': 's',
                    'ts': r1.time,
                    'pid': pid,
                    'tid': r1.tid,
                    'id': identifier
                }
                cte['traceEvents'].append(event)

                event = {
                    'name' : identifier,
                    'cat': 'dependencies',
                    'ph': 't',
                    'ts': r2.time,
                    'pid': pid,
                    'tid': r2.tid,
                    'id': identifier
                }
                cte['traceEvents'].append(event)

        # cte thread
        for tid in bind:
            cte["traceEvents"].append({
                "name": "thread_name",
                "ph": "M",
                "pid": pid,
                "tid": tid,
                "args": {
                    "name": "omp thread {}".format(tid)
                }
            })

        # for each task creation, create event
        '''
        for uid in tasks:
            record = tasks[uid]
            event = {
                'name': record.label,
                'cat':  'task-create',
                'ph':   'X',
                'ts':   record.time,
                'dur':  0.1,
                'pid':  pid,
                'tid':  record.tid,
                'args': {
                    'uid': uid,
                    'priority': record.priority,
                    'omp_priority': record.omp_priority,
                    'properties': record_to_properties_and_statuses(record),
               }
            }
            cte['traceEvents'].append(event)
            '''


    # cte communications 
    if 'send' in communications:
        for src in communications['send']:
            for dst in communications['send'][src]:
                for tag in communications['send'][src][dst]:
                    if 'recv' in communications:
                        if dst in communications['recv']:
                            if src in communications['recv'][dst]:
                                if tag in communications['recv'][dst][src]:
                                    r = communications['recv'][dst][src][tag]
                                    s = communications['send'][src][dst][tag]
                                    for ri, si in ((i, j) for i in range(len(r) - 2, -1, -2) for j in range(len(s) - 2, -1, -2)):
                                        if s[si].time < r[ri].time:
                                            identifier = "communication-{}-{}-{}-{}-{}".format(src, dst, tag, si, ri)
                                            event = {
                                                'name' : identifier,
                                                'cat': 'communication',
                                                'ph': 's',
                                                'ts': min(s[si + 1].time, s[si].time + (r[ri].time - s[si].time) * 0.5),
                                                'pid': s[si].pid,
                                                'tid': s[si].tid,
                                                'id': identifier
                                            }
                                            cte['traceEvents'].append(event)

                                            event = {
                                                'name' : identifier,
                                                'cat': 'communication',
                                                'ph': 't',
                                                'ts': r[ri].time,
                                                'pid': r[ri].pid,
                                                'tid': r[ri].tid,
                                                'id': identifier
                                            }
                                            cte['traceEvents'].append(event)
                                            break

    # compute ranularities
    assert(all(len(granularities[uuid]) % 2 == 0 for uuid in granularities))
    g = sorted([sum(granularities[uuid][i+1] - granularities[uuid][i] for i in range(0, len(granularities[uuid]) - 1, 2)) / 1000000.0 for uuid in granularities])

    stats['about'] = {
        'n-processes': len(records),
        'n-threads-total': n_threads
    }

    # graph stats
    gg = records_to_gg(records)
    nsend = sum(node.label.startswith('send') for node in gg.nodes.values()) 
    nrecv = sum(node.label.startswith('recv') for node in gg.nodes.values()) 
    narcs = sum(len(node.successors) for node in gg.nodes.values())
    stats['graph'] = {
        'n-tasks': {
            'total': len(gg.nodes),
            'n-compute': len(gg.nodes) - nsend - nrecv,
            'n-communication': {
                'total': nsend + nrecv,
                'n-send': nsend,
                'n-recv': nrecv
            }
        },
        'n-arcs': {
            'total': narcs,
            'dependencies': narcs - nrecv,
            'communications': nrecv
        },
        'granularity (s.)': {
            'max':  g[-1],
            'min':  g[0],
            'med':  g[len(g) // 2],
            'mean': sum(g) / len(g)
        }
    }

    stats['time'] = {
        'flat (s.)': {
            'total': total_time / 1000000.0,
            'max-process-time': max_process_total_time / 1000000.0,
            'in-task': {
                'total': intask_time / 1000000.0,
                'compute': (intask_time - recv_time - send_time) / 1000000.0,
                'communication': {
                    'total': (recv_time + send_time) / 1000000.0,
                    'recv': recv_time / 1000000.0,
                    'send': send_time / 1000000.0
                }
            },
            'out-task': {
                'total': outtask_time / 1000000.0,
                'overhead': (outtask_time - famine_time) / 1000000.0,
                'famine': {
                    'total': famine_time / 1000000.0,
                    'blocked': blocked_time / 1000000.0,
                    'unblocked': (famine_time - blocked_time) / 1000000.0
                }
            }
        },
        'proportion (%)': {
            'in-task': {
                'total': intask_time / total_time * 100.0,
                'compute': (intask_time - recv_time - send_time) / total_time * 100.0,
                'communication': {
                    'total': (recv_time + send_time) / total_time * 100.0,
                    'recv': recv_time / total_time * 100.0,
                    'send': send_time / total_time * 100.0
                }
            },
            'out-task': {
                'total': outtask_time / total_time * 100.0,
                'overhead': (outtask_time - famine_time) / total_time * 100.0,
                'famine': {
                    'total': famine_time / total_time * 100.0,
                    'blocked': blocked_time / total_time * 100.0,
                    'unblocked': (famine_time - blocked_time) / total_time * 100.0
                }
            }
        }
    }
    return (stats, cte, gg, g)


class Header:

    def __init__(self, buff):
        # see omptg_record.h (omptg_trace_file_header_t)
        self.magic   = buff[0:4].decode("ascii")
        self.version = int.from_bytes(buff[4:8],   byteorder=BYTE_ORDER, signed=False)
        self.pid     = int.from_bytes(buff[8:12], byteorder=BYTE_ORDER, signed=False)
        self.tid     = int.from_bytes(buff[12:16], byteorder=BYTE_ORDER, signed=False)
        assert(self.magic == MAGIC)

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "Header(magic={}, version={}, pid={}, tid={})".format(self.magic, self.version, self.pid, self.tid)

def ensure_task(tasks, uid):
    if uid not in tasks:
        tasks[uid] = {
            'created': None,
            'schedules':    [],
            'successors':   [],
            'predecessors': []
        }

# see mpcomp_task_trace.h
class Record:

    def __init__(self, pid, tid, t, typ, f):
        self.pid = pid
        self.tid = tid
        self.time = t
        self.typ  = typ
        self.parse(f.read(self.size()))

    def parse(self, f):
        print("`{}` should implement `parse` method".format(type(self).__name__), file=stderr)
        assert(0)

    def attributes(self):
        print("`{}` should implement `attributes` method".format(type(self).__name__), file=stderr)
        assert(0)

    def size(self):
        print("`{}` should implement `size` method".format(type(self).__name__), file=stderr)
        assert(0)

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "{}(pid={}, tid={}, type={}, time={}, {})".format(type(self).__name__, self.pid, self.tid, self.typ, self.time, self.attributes())

class RecordAsync(Record):

    def parse(self, buff):
        # TODO : parse 'when'
        self.status = int.from_bytes(buff[0:4], byteorder=BYTE_ORDER, signed=False)
        self.when   = int.from_bytes(buff[4:8], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "status={}, when={}".format(self.status, self.when)

    def store(self, tasks):
        # TODO : store
        pass

    def size(self):
        return RECORD_ASYNC_SIZE - RECORD_GENERIC_SIZE

class RecordFamineOverlap(Record):

    def parse(self, buff):
        self.status = int.from_bytes(buff[0:4], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "status={}".format(self.status)

    def store(self, tasks):
        # TODO : store this event
        pass

    def size(self):
        return RECORD_FAMINE_OVERLAP_SIZE - RECORD_GENERIC_SIZE

class RecordDependency(Record):

    def parse(self, buff):
        self.out_uid    = int.from_bytes(buff[0:4], byteorder=BYTE_ORDER, signed=False)
        self.in_uid     = int.from_bytes(buff[4:8], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "out={}, in={}".format(self.out_uid, self.in_uid)

    def store(self, tasks):
        ensure_task(tasks, self.in_uid)
        ensure_task(tasks, self.out_uid)
        tasks[self.in_uid]['predecessors'].append(self.out_uid)
        tasks[self.out_uid]['successors'].append(self.in_uid)

    def size(self):
        return RECORD_DEPENDENCY_SIZE - RECORD_GENERIC_SIZE



class RecordSchedule(Record):

    def parse(self, buff):
        x = MPC_OMP_TASK_LABEL_MAX_LENGTH
        self.label          = buff[0:x].decode("ascii").replace('\0', '')
        self.uid            = int.from_bytes(buff[x:x+4],       byteorder=BYTE_ORDER, signed=False)
        self.priority       = int.from_bytes(buff[x+4:x+8],     byteorder=BYTE_ORDER, signed=False)
        self.omp_priority   = int.from_bytes(buff[x+8:x+12],    byteorder=BYTE_ORDER, signed=False)
        self.properties     = int.from_bytes(buff[x+12:x+16],   byteorder=BYTE_ORDER, signed=False)
        self.npredecessors  = int.from_bytes(buff[x+16:x+20],   byteorder=BYTE_ORDER, signed=False)
        self.schedule_id    = int.from_bytes(buff[x+20:x+24],   byteorder=BYTE_ORDER, signed=False)
        self.statuses       = buff[x+24:x+24+len(MPC_OMP_TASK_STATUSES)]

    def attributes(self):
        return "label={}, uid={}, priority={}, omp_priority={}, properties={}, npredecessors={}, schedule_id={}".format(
            self.label, self.uid, self.priority, self.omp_priority, record_to_properties_and_statuses(self), self.npredecessors, self.schedule_id)

    def store(self, tasks):
        ensure_task(tasks, self.uid)
        tasks[self.uid]['schedules'].append(self)

    def size(self):
        return RECORD_SCHEDULE_SIZE - RECORD_GENERIC_SIZE

class RecordCreate(RecordSchedule):

    def store(self, tasks):
        ensure_task(tasks, self.uid)
        tasks[self.uid]['created'] = self


def record_to_properties_and_statuses(record):
    lst = []

    # task properties
    for p in MPC_OMP_TASK_PROP:
        flag = MPC_OMP_TASK_PROP[p]
        if (record.properties & flag) == flag:
            lst.append(p)

    # task statuses
    for i in range(len(MPC_OMP_TASK_STATUSES)):
        status = MPC_OMP_TASK_STATUSES[i]
        if record.statuses[i]:
            lst.append(status)

    return lst

def trace_to_records(records, path):
    total_size = os.path.getsize(path)
    read_size  = 0
    t0 = time.time()
    with open(path, "rb") as f:
        buff = f.read(HEADER_SIZE)
        assert(len(buff) == HEADER_SIZE)
        header = Header(buff)
        if header.pid not in records:
            records[header.pid] = []
        read_size += HEADER_SIZE
        while True:
            buff = f.read(RECORD_GENERIC_SIZE)
            if buff == b'':
                break
            assert(len(buff) == RECORD_GENERIC_SIZE)
            read_size += RECORD_GENERIC_SIZE
            t   = int(struct.unpack('d', buff[0:8])[0])
            typ = int.from_bytes(buff[8:12], byteorder=BYTE_ORDER, signed=False)
            clss = {
                MPC_OMP_TASK_TRACE_TYPE_DEPENDENCY:     RecordDependency,
                MPC_OMP_TASK_TRACE_TYPE_SCHEDULE:       RecordSchedule,
                MPC_OMP_TASK_TRACE_TYPE_CREATE:         RecordCreate,
                MPC_OMP_TASK_TRACE_TYPE_FAMINE_OVERLAP: RecordFamineOverlap,
                MPC_OMP_TASK_TRACE_TYPE_ASYNC:          RecordAsync

            }
            assert(typ in clss)
            record = clss[typ](header.pid, header.tid, t, typ, f)
            records[header.pid].append(record)
            read_size += record.size()
            t1 = time.time()
            if t1 - t0 > 0.2:
                print("{}%".format(int(read_size / total_size * 100.0)), file=sys.stderr)
                t0 = t1

def traces_to_records(src):
    records = {}
    num_files = len(os.listdir(src))
    num_file  = 1
    for f in os.listdir(src):
        print("file {}/{}".format(num_file, num_files), file=sys.stderr)
        trace_to_records(records, "{}/{}".format(src, f))
        num_file += 1
    return records

def records_to_processes(records):
    processes = {}
    for pid in records:
        processes[pid] = {'threads': [], 'tasks': {}}
        process = processes[pid]
        for record in records[pid]:
            if record.tid not in process['threads']:
                process['threads'].append(record.tid)
            record.store(process['tasks'])
    return processes

def records_to_cte(records):
    cte = {}
    return cte

def trace_to_cte(src):
    return records_to_cte(traces_to_records(src))

def dotprint(depth, s):
    print('{}{}'.format(' ' * depth * 4, s))

##### GRAPH DATA STRUCTURES #####

COLOR_GRADIENT_1 = [180, 0, 0]
COLOR_GRADIENT_2 = [255, 255, 255]

# a task node
class Node:
    def __init__(self, graph, uid, schedule_id, task):
        self.graph = graph
        self.critical = False
        self.critical_index = -1
        self.uuid = 'Tx{}x{}'.format(graph.pid, uid)
        self.uid = uid
        self.successors = task['successors']
        self.predecessors = task['predecessors']
        self.label = task['schedules'][0].label
        self.priority = task['schedules'][0].priority
        self.omp_priority = task['schedules'][0].omp_priority
        self.time = sum([task['schedules'][i + 1].time - task['schedules'][i].time for i in range(0, len(task['schedules']), 2)])
        self.schedule_id = schedule_id

    def dump(self, gg, g, dumped, show_priority=False, show_omp_priority=False, show_time=False, gradient=False):
        if self.uuid in dumped:
            return
        dumped[self.uuid] = True
        s = self.label #self.label.split('-')[0]
        if show_priority:
            s += "\\n{}".format(self.priority)
        if show_omp_priority:
            s += "\\n{}".format(self.omp_priority)
        if show_time:
            s += "\\n{}".format(self.time)
        if gradient:
            s = ''  # TODO - no name in gradient mode
            f = self.schedule_id / float(self.graph.last_schedule_id)   # linear
            f = f**2
            rgb = tuple(int((1.0 - f) * component[0] + f * component[1]) for component in zip(COLOR_GRADIENT_1, COLOR_GRADIENT_2))
            color = '#%02x%02x%02x' % rgb
            shape = 'circle'
            if self.label.startswith('send'):
                shape = 'diamond'
            dotprint(2, '{} [style=filled, fillcolor="{}", label="{}", shape="{}"];'.format(self.uuid, color, s, shape))
        else:
            dotprint(2, '{} [label="{}"];'.format(self.uuid, s))
        for suid in self.successors:
            snode = g.nodes[suid]
            snode.dump(gg, g, dumped, show_priority=show_priority, show_omp_priority=show_omp_priority, show_time=show_time, gradient=gradient)
            s = '{} -> {}'.format(self.uuid, snode.uuid)
            if self.critical and snode.critical and abs(self.critical_index - snode.critical_index) == 1:
                s += ' [color=red]'
            s += ';'
            dotprint(2, s)

    def set_critical(self, index):
        self.critical = True
        self.critical_index = index

    def __str__(self):
        return 'Node(label={}, time={}, uid={}, pid={})'.format(self.label, self.time, self.uid, self.graph.pid)

    def __repr__(self):
        return str(self)

# a process task graph
class Graph:
    def __init__(self, pid, process):
        self.pid = pid
        self.nodes = {}
        self.roots = []
        self.leaves = []
        self.last_schedule_id = 0

        # find roots
        tasks = process['tasks']
        for uid in tasks:
            task = tasks[uid]
            schedule_id = min(x.schedule_id for x in task['schedules'])
            self.last_schedule_id = max(schedule_id, self.last_schedule_id)
            node = Node(self, uid, schedule_id, task)
            self.nodes[uid] = node
            if len(node.successors) == 0:
                self.leaves.append(node)
            if len(node.predecessors) == 0:
                self.roots.append(node)

    def dump(self, gg, show_priority=False, show_omp_priority=False, show_time=False, gradient=False):
        dotprint(1, 'subgraph cluster_P{}'.format(self.pid))
        dotprint(1, '{')
        
        dotprint(2, 'label="Process {}";'.format(self.pid))
        dotprint(2, 'color="#aaaaaa";')

        dumped = {}
        for node in self.roots:
            node.dump(gg, self, dumped, show_priority=show_priority, show_omp_priority=show_omp_priority, show_time=show_time, gradient=gradient)

        dotprint(1, '}')

# the global graph
class GlobalGraph:
    def __init__(self):
        self.graphs = {}
        self.nodes = {}
        self.send_to_recv = {}
        self.recv_to_send = {}
        self.roots = []
        self.leaves = []

    def add_process(self, pid, process):
        graph = Graph(pid, process)
        self.graphs[pid] = graph
        for uid in graph.nodes:
            node = graph.nodes[uid]
            self.nodes[node.uuid] = node

    def set_communications(self, communications):
        if 'send' in communications:
            for src in communications['send']:
                for dst in communications['send'][src]:
                    for tag in communications['send'][src][dst]:
                        if 'recv' in communications:
                            if dst in communications['recv']:
                                if src in communications['recv'][dst]:
                                    if tag in communications['recv'][dst][src]:
                                        spid = int(src)
                                        rpid = int(dst)

                                        suid = communications['send'][src][dst][tag][0].uid
                                        ruid = communications['recv'][dst][src][tag][0].uid

                                        snode = self.graphs[spid].nodes[suid]
                                        rnode = self.graphs[rpid].nodes[ruid]

                                        self.send_to_recv[snode] = rnode
                                        self.recv_to_send[rnode] = snode

    def finalize(self):
        for pid in self.graphs:
            graph = self.graphs[pid]
            for leaf in graph.leaves:
                if leaf not in self.send_to_recv:
                    self.leaves.append(leaf)
            for root in graph.roots:
                if root not in self.recv_to_send:
                    self.roots.append(root)

    # compute local process critical path
    def compute_critical(self):
        predecessors = {}
        d = {uuid: -math.inf for uuid in self.nodes}
        q = queue.Queue()

        for pid in self.graphs:
            graph = self.graphs[pid]
            for node in graph.roots:
                d[node.uuid] = node.time
                q.put(node)
                while not q.empty():
                    node = q.get()
                    successors = [node.graph.nodes[uid] for uid in node.successors]
                    if node in self.send_to_recv:
                        successors += [self.send_to_recv[node]]
                    for snode in successors:
                        if d[snode.uuid] < d[node.uuid] + snode.time:
                            d[snode.uuid] = d[node.uuid] + snode.time
                            predecessors[snode.uuid] = node.uuid
                            q.put(snode)

        path = []
        node = max(self.leaves, key=lambda leaf: d[leaf.uuid])
        while True:
            path.insert(0, node)
            if node.uuid in predecessors:
                node = self.nodes[predecessors[node.uuid]]
            else:
                break

        return path

    def dump(self, show_priority=False, show_omp_priority=False, show_time=False, gradient=False):
        dotprint(0, 'digraph G')
        dotprint(0, '{')

        if not gradient:
            # print legend
            dotprint(1, 'subgraph cluster_LEGEND')
            dotprint(1, '{')

            legend = 'LABEL'
            if show_priority:
                legend += '\\nWEIGHT'
            if show_omp_priority:
                legend += '\\nOMP_PRIORITY'
            if show_time:
                legend += '\\nTIME'
            dotprint(2, 'LEGEND [label="{}"];'.format(legend))
       
            dotprint(2, 'label="Legend";')
            dotprint(2, 'color="#aaaaaa";')
            dotprint(1, '}')

        # print subgraphs
        for pid in self.graphs:
            graph = self.graphs[pid]
            graph.dump(self, show_priority=show_priority, show_omp_priority=show_omp_priority, show_time=show_time, gradient=gradient)

        # print inter-graph dependencies
        for snode in self.send_to_recv:
            rnode = self.send_to_recv[snode]
            s = '{} -> {}'.format(snode.uuid, rnode.uuid)
            attr = []
            attr.append('style=dotted')
            if snode.critical and rnode.critical:
                attr.append('color=red')
            s += ' [{}] ;'.format(','.join(attr))
            dotprint(1, s)
        dotprint(0, '}')

def records_to_gg(records):
    gg = GlobalGraph()
    processes = records_to_processes(records)
    for pid in processes:
        gg.add_process(pid, processes[pid])
    gg.set_communications(records_to_communications(records))
    gg.finalize()
    return gg

def traces_to_gg(src):
    return records_to_gg(traces_to_records(src))

def blumofe_leiserson_check(trace):
    (stats, _, gg) = parse_traces(trace)

    # T1
    T1 = 0.
    for node in gg.nodes.values():
        T1 += node.time
    T1 = T1 / 1000000.0

    # Too
    cpath = gg.compute_critical()
    Too = 0.
    for node in cpath:
        Too += node.time
    Too = Too / 1000000.0

    # P
    P = stats['about']['n-threads-total']

    # TP
    TP = stats['time']['flat (s.)']['max-process-time']

    return (Too, TP, T1, P)

