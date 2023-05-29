import heapq
import json
import hashlib
import math
import os
import struct
import sys
import time
import queue

import progress

from passes.cte import utils

# Number of digits to keep with times (in s.)
# Note that time are often measures with microsecond precision
FLOAT_PRECISION = 6

MAGIC         = "task"
HEADER_SIZE   = 16
BYTE_ORDER    = 'little' # or 'big' (endianess)

MPC_OMP_TASK_LABEL_MAX_LENGTH = 64

# record sizes from C's sizeof()
RECORD_GENERIC_SIZE         = 16    # 0, 1
RECORD_DEPENDENCY_SIZE      = 24    # 2
RECORD_SCHEDULE_SIZE        = 72    # 3
RECORD_CREATE_SIZE          = 112   # 4
RECORD_DELETE_SIZE          = 32    # 5
RECORD_SEND_SIZE            = 48    # 6
RECORD_RECV_SIZE            = 48    # 7
RECORD_ALLREDUCE_SIZE       = 40    # 8
RECORD_RANK_SIZE            = 24    # 9
RECORD_BLOCKED_SIZE         = 24    # 10
RECORD_UNBLOCKED_SIZE       = 24    # 11

MPC_OMP_TASK_TRACE_TYPE_BEGIN           = 0
MPC_OMP_TASK_TRACE_TYPE_END             = 1
MPC_OMP_TASK_TRACE_TYPE_DEPENDENCY      = 2
MPC_OMP_TASK_TRACE_TYPE_SCHEDULE        = 3
MPC_OMP_TASK_TRACE_TYPE_CREATE          = 4
MPC_OMP_TASK_TRACE_TYPE_DELETE          = 5
MPC_OMP_TASK_TRACE_TYPE_SEND            = 6
MPC_OMP_TASK_TRACE_TYPE_RECV            = 7
MPC_OMP_TASK_TRACE_TYPE_ALLREDUCE       = 8
MPC_OMP_TASK_TRACE_TYPE_RANK            = 9
MPC_OMP_TASK_TRACE_TYPE_BLOCKED         = 10
MPC_OMP_TASK_TRACE_TYPE_UNBLOCKED       = 11
MPC_OMP_TASK_TRACE_TYPE_COUNT           = 12

""" Hash a string to a 16-digits integer """
def hash_string(string):
    return int(hashlib.sha1(string.encode("utf-8")).hexdigest(), 16) % (2**64)

ON_TASK_CREATE      = 'on_task_create'
ON_TASK_DELETE      = 'on_task_delete'
ON_TASK_READY       = 'on_task_ready'
ON_TASK_STARTED     = 'on_task_started'
ON_TASK_RESUMED     = 'on_task_resumed'
ON_TASK_DEPENDENCY  = 'on_task_dependency'
ON_TASK_COMPLETED   = 'on_task_completed'
ON_TASK_PAUSED      = 'on_task_paused'
ON_TASK_BLOCKED     = 'on_task_blocked'
ON_TASK_UNBLOCKED   = 'on_task_unblocked'
ON_TASK_SEND        = 'on_task_send'
ON_TASK_RECV        = 'on_task_recv'
ON_TASK_ALLREDUCE   = 'on_task_allreduce'

""" compute the number of predecessors of each tasks """
def compute_predecessors(records):
    print("Computing tasks predecessors/successors relationship...")

    def ensure_task(tasks, uid):
        if uid not in tasks:
            tasks[uid] = {
                'create': None,
                'created': 0,
                'delete': None,
                'schedules':    [],
                'successors':   [],
                'predecessors': [],
                'sends':        [],
                'recvs':        [],
                'allreduces':   [],
                'ref_predecessors': 0,
                'npredecessors': 0
            }

    def record_order(record):
        orders = {
            RecordRank:        -1,
            RecordIgnore:       0,
            RecordCreate:       0,
            RecordDependency:   1,
            RecordSchedule:     2,
            RecordBlocked:      3,
            RecordUnblocked:    4,
            RecordSend:         5,
            RecordRecv:         6,
            RecordAllreduce:    7,
            RecordDelete:       8,
        }
        schedule_id = 0
        if hasattr(record, 'schedule_id'):
            schedule_id = record.schedule_id
        return (record.time, orders[type(record)], schedule_id)

    progress.init(sum(len(records[pid]) for pid in records))
    tasks_per_pid = {}

    for pid in records:
        tasks = {}
        records[pid] = sorted(records[pid], key=record_order)
        for record in records[pid]:
            progress.update()
            if isinstance(record, RecordCreate):
                ensure_task(tasks, record.uid)
                tasks[record.uid]['create'] = record
            elif isinstance(record, RecordDelete):
                ensure_task(tasks, record.uid)
                tasks[record.uid]['delete'] = record
            elif isinstance(record, RecordDependency):
                ensure_task(tasks, record.in_uid)
                ensure_task(tasks, record.out_uid)
                tasks[record.in_uid]['predecessors'].append(record.out_uid)
                tasks[record.in_uid]['ref_predecessors'] += 1
                tasks[record.in_uid]['npredecessors'] += 1
                tasks[record.out_uid]['successors'].append(record.in_uid)
            elif isinstance(record, RecordSchedule):
                ensure_task(tasks, record.uid)
                tasks[record.uid]['schedules'].append(record)
        tasks_per_pid[pid] = tasks
    progress.deinit()
    return tasks_per_pid

def parse_traces(traces, show_progress, inspectors):

    progress.switch(show_progress)

    # parse records (TODO : this can be optimized, currently, conversion from trace files to CPython objects takes ages)
    records = traces_to_records(traces)
    tasks_per_pid = compute_predecessors(records)

    env = {}
    def inspect(name):
        for inspector in inspectors:
            getattr(inspector, name)(env)

    # number of threads (total)
    n_threads = 0

    # pid to rank converter
    ptr = {}

    # TODO : add 'standard' passes that fill the 'env'
    # so the user can decide which information it needs to compute

    # replay the scheduling to compute stats
    for pid in records:

        # if no rank is found, then set the pid as the rank
        ptr[pid] = pid

        # bound[tid] => [uid1, uid1, uid2, uid2, ..., uidn, uidn] : per-thread schedules
        print("Discovering threads and converting PID to rank")
        progress.init(len(records[pid]))
        bound = {}
        for record in records[pid]:
            progress.update()
            if record.tid not in bound:
                bound[record.tid] = []
            if isinstance(record, RecordRank):
                ptr[pid] = record.rank
        progress.deinit()

    for pid in records:

        print("Scheduling process {} of rank {}".format(pid, ptr[pid]))

        # all tasks
        tasks = tasks_per_pid[pid]

        # the number of dependences that have been
        # resolved before the 'task create' event was raised
        tasks_dependences_before_create = {}

        # tasks ready to be scheduled
        readyqueue = {}

        # blocked tasks
        blockedlist = {}

        # generate new env for the process
        env = {}
        env['t0']            = +math.inf        # first record time
        env['tf']            = -math.inf        # last record time
        env['records']       = records[pid]
        env['tasks']         = tasks
        env['bound']         = bound
        env['readyqueue']    = readyqueue
        env['blocked']       = blockedlist
        env['pid']           = pid
        env['rank']          = ptr[pid]

        progress.init(len(records[pid]))
        inspect('on_process_inspection_start')

        # for each record on this process
        for i in range(len(records[pid])):
            progress.update()
            record = records[pid][i]
            events = []

            # timing
            env['t0'] = min(env['t0'], record.time)
            env['tf'] = max(env['tf'], record.time)

            # creating a new task
            if isinstance(record, RecordCreate):
                tasks[record.uid]['created'] = 1
                properties = utils.record_to_properties_and_flags_and_state(record)
                if (tasks[record.uid]['ref_predecessors'] == 0) and ('INITIAL' not in properties):
                    readyqueue[record.uid] = True
                    events.append(ON_TASK_READY)
                events.append(ON_TASK_CREATE)

            # deleting a task
            elif isinstance(record, RecordDelete):
                if record.uid in readyqueue:
                    print(tasks[record.uid])
                assert(record.uid not in readyqueue)
                events.append(ON_TASK_DELETE)

            # dependency completion
            elif isinstance(record, RecordDependency):
                events.append(ON_TASK_DEPENDENCY)
                assert(record.in_uid not in readyqueue)
                tasks[record.in_uid]['ref_predecessors'] -= 1
                if tasks[record.in_uid]['ref_predecessors'] == 0 and tasks[record.in_uid]['created']:
                    readyqueue[record.in_uid] = True
                    events.append(ON_TASK_READY)

            # task scheduling
            elif isinstance(record, RecordSchedule):

                # update various lists depending on the schedule details
                properties = utils.record_to_properties_and_flags_and_state(record)
                event = None

                # a task completed
                if 'EXECUTED' in properties:
                    assert(len(bound[record.tid]) > 0)
                    #assert(record.uid == bound[record.tid][-1].uid)
                    del bound[record.tid][-1]
                    events.append(ON_TASK_COMPLETED)

                else:
                    assert('SCHEDULED' in properties)

                    # a task unblocked and resumed
                    if 'UNBLOCKED' in properties:
                        assert(record.uid in readyqueue)
                        assert(record.uid not in blockedlist)
                        if record.uid in readyqueue:
                            del readyqueue[record.uid]
                        bound[record.tid].append(record)
                        events.append(ON_TASK_RESUMED)

                    # a task blocked and paused
                    elif 'BLOCKING' in properties:
                        assert(record.uid == bound[record.tid][-1].uid)
                        del bound[record.tid][-1]
                        events.append(ON_TASK_PAUSED)

                    # a task started
                    elif 'BLOCKED' not in properties:
                        task = tasks[record.uid]
                        if record.uid in readyqueue: # TODO: this is an assert ?
                            del readyqueue[record.uid]
                        bound[record.tid].append(record)
                        events.append(ON_TASK_STARTED)

            # mark send tasks
            elif isinstance(record, RecordSend):
                if record.uid in tasks:
                    tasks[record.uid]['create'].send = True
                events.append(ON_TASK_SEND)

            # mark recv tasks
            elif isinstance(record, RecordRecv):
                if record.uid in tasks:
                    tasks[record.uid]['create'].recv = True
                events.append(ON_TASK_RECV)

            # mark allreduce tasks
            elif isinstance(record, RecordAllreduce):
                if record.uid in tasks:
                    tasks[record.uid]['create'].allreduce = True
                events.append(ON_TASK_ALLREDUCE)

            # task block
            elif isinstance(record, RecordBlocked):
                # TODO: this assume a task only blocks/unblocks once
                # task unblocked before blocking
                if record.uid in readyqueue: # task unblocked before blocking
                    blockedlist[record.uid] = True
                events.append(ON_TASK_BLOCKED)

            # task unblock
            elif isinstance(record, RecordUnblocked):
                if record.uid in blockedlist: # task blocked before unblocking
                    del blockedlist[record.uid]
                events.append(ON_TASK_UNBLOCKED)

            # process events
            env['record'] = record
            for event in events:
                inspect(event)

        progress.deinit()

        # increment number of thread counter
        n_threads += len(bound)

        # coherency check - TODO : this is no longer guaranteed with persistent tasks
        for uid in tasks:
            task = tasks[uid]
            assert(task['ref_predecessors'] == 0)
        for k in readyqueue:
            print(tasks[k])
        assert(len(readyqueue) == 0)
        inspect('on_process_inspection_end')

    # 'pid' loop ends here

    # (stats, cte, gg, g, records, blocked_tasks_over_time_per_pid)
    return records

class Header:

    def __init__(self, buff):
        # see omptg_record.h (omptg_trace_file_header_t)
        self.magic   = buff[0:4].decode("ascii")
        self.version = int.from_bytes(buff[4:8],    byteorder=BYTE_ORDER, signed=False)
        self.pid     = int.from_bytes(buff[8:12],   byteorder=BYTE_ORDER, signed=False)
        self.tid     = int.from_bytes(buff[12:16],  byteorder=BYTE_ORDER, signed=False)
        assert(self.magic == MAGIC)

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "Header(magic={}, version={}, pid={}, tid={})".format(self.magic, self.version, self.pid, self.tid)

# see mpcomp_task_trace.h
class Record:

    def __init__(self, pid, tid, t, typ, f):
        self.pid = pid
        self.tid = tid
        self.time = t
        self.typ  = typ
        self.parse(f.read(self.size() - RECORD_GENERIC_SIZE))

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
        return "{}(pid={}, tid={}, time={}, {})".format(type(self).__name__, self.pid, self.tid, self.time, self.attributes())

class RecordIgnore(Record):

    def parse(self, f):
        pass

    def attributes(self):
        return "time={}".format(self.time)

    def size(self):
        return RECORD_GENERIC_SIZE

class RecordDependency(Record):

    def parse(self, buff):
        self.out_uid    = int.from_bytes(buff[0:4], byteorder=BYTE_ORDER, signed=False)
        self.in_uid     = int.from_bytes(buff[4:8], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "out_uid={}, in_uid={}".format(self.out_uid, self.in_uid)

    def size(self):
        return RECORD_DEPENDENCY_SIZE


class RecordSchedule(Record):

    def parse(self, buff):
        self.uid                = int.from_bytes(buff[0:4],     byteorder=BYTE_ORDER, signed=False)
        self.priority           = int.from_bytes(buff[4:8],     byteorder=BYTE_ORDER, signed=False)
        self.properties         = int.from_bytes(buff[8:12],    byteorder=BYTE_ORDER, signed=False)
        self.schedule_id        = int.from_bytes(buff[12:16],   byteorder=BYTE_ORDER, signed=False)
        self.flags              = int.from_bytes(buff[16:20],   byteorder=BYTE_ORDER, signed=False)
        self.state              = int.from_bytes(buff[20:24],   byteorder=BYTE_ORDER, signed=False)
        self.hwcounters = []
        self.hwcounters.append(   int.from_bytes(buff[24:32],   byteorder=BYTE_ORDER, signed=False))
        self.hwcounters.append(   int.from_bytes(buff[32:40],   byteorder=BYTE_ORDER, signed=False))
        self.hwcounters.append(   int.from_bytes(buff[40:48],   byteorder=BYTE_ORDER, signed=False))
        self.hwcounters.append(   int.from_bytes(buff[48:56],   byteorder=BYTE_ORDER, signed=False))

    def attributes(self):
        return "uid={}, priority={}, properties/flags={}, schedule_id={}, hwcounters={}".format(
            self.uid,
            self.priority,
            utils.record_to_properties_and_flags_and_state(self),
            self.schedule_id,
            self.hwcounters
        )

    def size(self):
        return RECORD_SCHEDULE_SIZE

class RecordCreate(Record):

    def parse(self, buff):
        self.send = False
        self.recv = False
        self.allreduce = False

        self.uid                = int.from_bytes(buff[0:4],     byteorder=BYTE_ORDER, signed=False)
        self.persistent_uid     = int.from_bytes(buff[4:8],     byteorder=BYTE_ORDER, signed=False)
        self.properties         = int.from_bytes(buff[8:12],    byteorder=BYTE_ORDER, signed=False)
        self.flags              = int.from_bytes(buff[12:16],   byteorder=BYTE_ORDER, signed=False)

        x = 16
        y = MPC_OMP_TASK_LABEL_MAX_LENGTH;
        self.label              = buff[x:x+y].decode("ascii").replace('\0', '')
        self.color              = int.from_bytes(buff[x+y+0:x+y+4],   byteorder=BYTE_ORDER, signed=False)
        self.parent_uid         = int.from_bytes(buff[x+y+4:x+y+8],   byteorder=BYTE_ORDER, signed=False)
        self.omp_priority       = int.from_bytes(buff[x+y+8:x+y+12],  byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}, persistent_uid={}, properties/flags={}, label={}, color={}, parent_uid={}, omp_priority={}, send={}, recv={}, allreduce={}".format(
            self.uid,
            self.persistent_uid,
            utils.record_to_properties_and_flags_and_state(self),
            self.label,
            self.color,
            self.parent_uid,
            self.omp_priority,
            self.send,
            self.recv,
            self.allreduce)

    def size(self):
        return RECORD_CREATE_SIZE

class RecordDelete(Record):

    def parse(self, buff):
        self.uid            = int.from_bytes(buff[0:4],     byteorder=BYTE_ORDER, signed=False)
        self.priority       = int.from_bytes(buff[4:8],     byteorder=BYTE_ORDER, signed=False)
        self.properties     = int.from_bytes(buff[8:12],    byteorder=BYTE_ORDER, signed=False)
        self.flags          = int.from_bytes(buff[12:16],   byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}, priority={}, properties={}".format(
            self.uid, self.priority, utils.record_to_properties_and_flags_and_state(self))

    def size(self):
        return RECORD_DELETE_SIZE

class RecordSend(Record):

    def parse(self, buff):
        self.uid        = int.from_bytes(buff[0:4],   byteorder=BYTE_ORDER, signed=False)
        self.count      = int.from_bytes(buff[4:8],   byteorder=BYTE_ORDER, signed=False)
        self.dtype      = int.from_bytes(buff[8:12],  byteorder=BYTE_ORDER, signed=False)
        self.dst        = int.from_bytes(buff[12:16], byteorder=BYTE_ORDER, signed=False)
        self.tag        = int.from_bytes(buff[16:20], byteorder=BYTE_ORDER, signed=False)
        self.comm       = int.from_bytes(buff[20:24], byteorder=BYTE_ORDER, signed=False)
        self.completed  = int.from_bytes(buff[24:28], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}, count={}, dtype={}, dst={}, tag={}, comm={}, completed={}".format(self.uid, self.count, self.dtype, self.dst, self.tag, self.comm, self.completed)

    def size(self):
        return RECORD_SEND_SIZE

class RecordRecv(Record):

    def parse(self, buff):
        self.uid        = int.from_bytes(buff[0:4],   byteorder=BYTE_ORDER, signed=False)
        self.count      = int.from_bytes(buff[4:8],   byteorder=BYTE_ORDER, signed=False)
        self.dtype      = int.from_bytes(buff[8:12],  byteorder=BYTE_ORDER, signed=False)
        self.src        = int.from_bytes(buff[12:16], byteorder=BYTE_ORDER, signed=False)
        self.tag        = int.from_bytes(buff[16:20], byteorder=BYTE_ORDER, signed=False)
        self.comm       = int.from_bytes(buff[20:24], byteorder=BYTE_ORDER, signed=False)
        self.completed  = int.from_bytes(buff[24:28], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}, count={}, dtype={}, src={}, tag={}, comm={}, completed={}".format(self.uid, self.count, self.dtype, self.src, self.tag, self.comm, self.completed)

    def size(self):
        return RECORD_SEND_SIZE

class RecordAllreduce(Record):

    def parse(self, buff):
        self.uid        = int.from_bytes(buff[0:4],   byteorder=BYTE_ORDER, signed=False)
        self.count      = int.from_bytes(buff[4:8],   byteorder=BYTE_ORDER, signed=False)
        self.dtype      = int.from_bytes(buff[8:12],  byteorder=BYTE_ORDER, signed=False)
        self.op         = int.from_bytes(buff[12:16], byteorder=BYTE_ORDER, signed=False)
        self.comm       = int.from_bytes(buff[16:20], byteorder=BYTE_ORDER, signed=False)
        self.completed  = int.from_bytes(buff[20:24], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}, count={}, dtype={}, op={}, comm={}, completed={}".format(self.uid, self.count, self.dtype, self.op, self.comm, self.completed)

    def size(self):
        return RECORD_ALLREDUCE_SIZE

class RecordRank(Record):

    def parse(self, buff):
        self.comm = int.from_bytes(buff[0:4],   byteorder=BYTE_ORDER, signed=False)
        self.rank = int.from_bytes(buff[4:8],   byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "comm={}, rank={}".format(self.comm, self.rank)

    def size(self):
        return RECORD_RANK_SIZE

class RecordBlocked(Record):

    def parse(self, buff):
        self.uid = int.from_bytes(buff[0:4], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}".format(self.uid)

    def size(self):
        return RECORD_BLOCKED_SIZE

class RecordUnblocked(Record):

    def parse(self, buff):
        self.uid = int.from_bytes(buff[0:4], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}".format(self.uid)

    def size(self):
        return RECORD_UNBLOCKED_SIZE

RECORD_CLASS = {
    MPC_OMP_TASK_TRACE_TYPE_BEGIN:          RecordIgnore,
    MPC_OMP_TASK_TRACE_TYPE_END:            RecordIgnore,
    MPC_OMP_TASK_TRACE_TYPE_DEPENDENCY:     RecordDependency,
    MPC_OMP_TASK_TRACE_TYPE_SCHEDULE:       RecordSchedule,
    MPC_OMP_TASK_TRACE_TYPE_CREATE:         RecordCreate,
    MPC_OMP_TASK_TRACE_TYPE_DELETE:         RecordDelete,
    MPC_OMP_TASK_TRACE_TYPE_SEND:           RecordSend,
    MPC_OMP_TASK_TRACE_TYPE_RECV:           RecordRecv,
    MPC_OMP_TASK_TRACE_TYPE_ALLREDUCE:      RecordAllreduce,
    MPC_OMP_TASK_TRACE_TYPE_BLOCKED:        RecordBlocked,
    MPC_OMP_TASK_TRACE_TYPE_UNBLOCKED:      RecordUnblocked,
    MPC_OMP_TASK_TRACE_TYPE_RANK:           RecordRank,
}

# ---------------------------------------------------- #

def trace_to_records(records, path):
    total_size = os.path.getsize(path)
    with open(path, "rb") as f:
        buff = f.read(HEADER_SIZE)
        assert(len(buff) == HEADER_SIZE)
        header = Header(buff)
        if header.pid not in records:
            records[header.pid] = []
        while True:
            buff = f.read(RECORD_GENERIC_SIZE)
            if buff == b'':
                break
            assert(len(buff) == RECORD_GENERIC_SIZE)
            t   = int.from_bytes(buff[0:8],     byteorder=BYTE_ORDER, signed=False)
            typ = int.from_bytes(buff[8:12],    byteorder=BYTE_ORDER, signed=False)
            assert(typ in RECORD_CLASS)
            record = RECORD_CLASS[typ](header.pid, header.tid, t, typ, f)
            records[header.pid].append(record)

# TODO : how can I parallelize this with python ? :-(
def traces_to_records(src):
    records = {}
    num_files = len(os.listdir(src))
    print("Converting raw trace to records...")
    progress.init(num_files)
    for f in os.listdir(src):
        progress.update()
        trace_to_records(records, "{}/{}".format(src, f))
    progress.deinit()
    return records
