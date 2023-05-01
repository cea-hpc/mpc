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
RECORD_GENERIC_SIZE         = 16
RECORD_DEPENDENCY_SIZE      = 24
RECORD_SCHEDULE_SIZE        = 72
RECORD_CREATE_SIZE          = 112
RECORD_DELETE_SIZE          = 32
RECORD_SEND_SIZE            = 48
RECORD_RECV_SIZE            = 48
RECORD_ALLREDUCE_SIZE       = 40
RECORD_RANK_SIZE            = 24
RECORD_BLOCKED_SIZE         = 24
RECORD_UNBLOCKED_SIZE       = 24

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
def records_fix(records):
    print("Fixing records timing...")
    progress.init(sum(len(records[pid]) for pid in records))
    for pid in records:
        records_create = {}
        records_dependences = []
        for record in records[pid]:
            progress.update()
            if isinstance(record, RecordCreate):
                records_create[record.uid] = record
            elif isinstance(record, RecordDependency):
                records_dependences.append(record)
        for record in records_dependences:
            assert(record.in_uid in records_create)
            assert(record.out_uid in records_create)
            records_create[record.in_uid].npredecessors += 1
        # TODO: maybe also sort by record type
        records[pid] = sorted(records[pid], key=lambda r: r.time)
    progress.deinit()

def parse_traces(traces, show_progress, inspectors):

    progress.switch(show_progress)

    # parse records (TODO : maybe this can be optimized)
    records = traces_to_records(traces)
    records_fix(records)

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

        # the tasks
        tasks = {}

        # the number of dependences that have been
        # resolved before the 'task create' event was raised
        tasks_dependences_before_create = {}

        # per-task schedules (x2, contains start, pause, resume, complete)
        schedules = {}

        # TODO : maybe remove one of the 2 lists for memory consumptions
        # successors[uid] => [uid1, uid2, ..., uidn] : successors of 'uid'
        successors = {}

        # predecessors[uid] => [uid1, uid2, ..., uidn] : predecessors of 'uid'
        predecessors = {}

        # npredecessors[uid] => n : number of remaining predecessors
        npredecessors = {}

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
        env['schedules']     = schedules
        env['readyqueue']    = readyqueue
        env['blocked']       = blockedlist
        env['pid']           = pid
        env['rank']          = ptr[pid]
        env['successors']    = successors
        env['predecessors']  = predecessors
        env['npredecessors'] = npredecessors

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
                assert(record.uid not in tasks)
                tasks[record.uid] = {'create': record, 'delete': None}
                if record.uid not in npredecessors:
                    npredecessors[record.uid] = 0
                    predecessors[record.uid] = {}
                npredecessors[record.uid] += record.npredecessors
                properties = utils.record_to_properties_and_statuses(record)
                if (npredecessors[record.uid] == 0) and ('INITIAL' not in properties):
                    readyqueue[record.uid] = True
                    events.append(ON_TASK_READY)
                events.append(ON_TASK_CREATE)

            # deleting a task
            elif isinstance(record, RecordDelete):
                assert(record.uid in tasks)
                properties = utils.record_to_properties_and_statuses(record)

                # task whether got cancelled or was the last instance of a persistent task that was not reschedule
                if record.uid in readyqueue:
                    del readyqueue[record.uid]
                if record.uid in npredecessors:
                    del npredecessors[record.uid]
                tasks[record.uid]['delete'] = record
                events.append(ON_TASK_DELETE)

            # dependency completion
            elif isinstance(record, RecordDependency):

                assert(record.in_uid in tasks)
                assert(record.out_uid in tasks)
                assert(record.in_uid not in readyqueue)

                if record.in_uid not in npredecessors:
                    npredecessors[record.in_uid] = 0
                    predecessors[record.in_uid] = {}
                npredecessors[record.in_uid] -= 1
                predecessors[record.in_uid][record.out_uid] = True

                if npredecessors[record.in_uid] == 0:
                    readyqueue[record.in_uid] = True
                    events.append(ON_TASK_READY)

                # add successors
                if record.out_uid not in successors:
                    successors[record.out_uid] = {}
                successors[record.out_uid][record.in_uid] = True
                events.append(ON_TASK_DEPENDENCY)

            # task scheduling
            elif isinstance(record, RecordSchedule):

                # update various lists depending on the schedule details
                properties = utils.record_to_properties_and_statuses(record)
                event = None

                # a task completed
                if 'COMPLETED' in properties:
                    assert(len(bound[record.tid]) > 0)
                    #assert(record.uid == bound[record.tid][-1].uid)
                    del bound[record.tid][-1]
                    events.append(ON_TASK_COMPLETED)

                # a task unblocked and resumed
                elif 'UNBLOCKED' in properties:
                    #assert(record.uid in readyqueue)
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
                    assert(record.uid not in npredecessors or npredecessors[record.uid] <= 0)
                    assert(record.uid not in schedules)
                    if record.uid in readyqueue:
                        del readyqueue[record.uid]
                    bound[record.tid].append(record)
                    events.append(ON_TASK_STARTED)

                # register task scheduling
                if record.uid not in schedules:
                    schedules[record.uid] = []
                schedules[record.uid].append(record)

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
                # TODO : theoritically, a task could block several times, and blocking events
                # could be recorded/raised out of order... this can raise issues here
                # but this doesn't happen in current mpi/omp interop.

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
        for uid in npredecessors:
            assert(npredecessors[uid] == 0)
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

def ensure_task(tasks, uid):
    if uid not in tasks:
        tasks[uid] = {
            'create': None,
            'delete': None,
            'schedules':    [],
            'successors':   [],
            'predecessors': [],
            'sends':        [],
            'recvs':        [],
            'allreduces':   []
        }

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

    def store(self, processes, pid):
        pass

    def size(self):
        return RECORD_GENERIC_SIZE

class RecordDependency(Record):

    def parse(self, buff):
        self.out_uid    = int.from_bytes(buff[0:4], byteorder=BYTE_ORDER, signed=False)
        self.in_uid     = int.from_bytes(buff[4:8], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "out_uid={}, in_uid={}".format(self.out_uid, self.in_uid)

    def store(self, processes, pid):
        tasks = processes['by-pids'][pid]['tasks']
        ensure_task(tasks, self.in_uid)
        ensure_task(tasks, self.out_uid)
        tasks[self.in_uid]['predecessors'].append(self.out_uid)
        tasks[self.out_uid]['successors'].append(self.in_uid)

    def size(self):
        return RECORD_DEPENDENCY_SIZE


class RecordSchedule(Record):

    def parse(self, buff):
        self.uid                = int.from_bytes(buff[0:4],     byteorder=BYTE_ORDER, signed=False)
        self.priority           = int.from_bytes(buff[4:8],     byteorder=BYTE_ORDER, signed=False)
        self.properties         = int.from_bytes(buff[8:12],    byteorder=BYTE_ORDER, signed=False)
        self.schedule_id        = int.from_bytes(buff[12:16],   byteorder=BYTE_ORDER, signed=False)
        self.statuses           = int.from_bytes(buff[16:20],   byteorder=BYTE_ORDER, signed=False)
        self.hwcounters = []
        self.hwcounters.append(   int.from_bytes(buff[24:32],   byteorder=BYTE_ORDER, signed=False))
        self.hwcounters.append(   int.from_bytes(buff[32:40],   byteorder=BYTE_ORDER, signed=False))
        self.hwcounters.append(   int.from_bytes(buff[40:48],   byteorder=BYTE_ORDER, signed=False))
        self.hwcounters.append(   int.from_bytes(buff[48:56],   byteorder=BYTE_ORDER, signed=False))

    def attributes(self):
        return "uid={}, priority={}, properties/statuses={}, schedule_id={}, hwcounters={}".format(
            self.uid,
            self.priority,
            utils.record_to_properties_and_statuses(self),
            self.schedule_id,
            self.hwcounters
        )

    def store(self, processes, pid):
        tasks = processes['by-pids'][pid]['tasks']
        ensure_task(tasks, self.uid)
        tasks[self.uid]['schedules'].append(self)

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
        self.statuses           = int.from_bytes(buff[12:16],   byteorder=BYTE_ORDER, signed=False)

        self.npredecessors      = 0

        x = 16
        y = MPC_OMP_TASK_LABEL_MAX_LENGTH;
        self.label              = buff[x:x+y].decode("ascii").replace('\0', '')
        self.color              = int.from_bytes(buff[x+y+0:x+y+4],   byteorder=BYTE_ORDER, signed=False)
        self.parent_uid         = int.from_bytes(buff[x+y+4:x+y+8],   byteorder=BYTE_ORDER, signed=False)
        self.omp_priority       = int.from_bytes(buff[x+y+8:x+y+12],  byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}, persistent_uid={}, properties/statuses={}, npredecessors={}, label={}, color={}, parent_uid={}, omp_priority={}, send={}, recv={}, allreduce={}".format(
            self.uid,
            self.persistent_uid,
            utils.record_to_properties_and_statuses(self),
            self.npredecessors,
            self.label,
            self.color,
            self.parent_uid,
            self.omp_priority,
            self.send,
            self.recv,
            self.allreduce)

    def store(self, processes, pid):
        tasks = processes['by-pids'][pid]['tasks']
        ensure_task(tasks, self.uid)
        tasks[self.uid]['create'] = self

    def size(self):
        return RECORD_CREATE_SIZE

class RecordDelete(Record):

    def parse(self, buff):
        self.uid            = int.from_bytes(buff[0:4],     byteorder=BYTE_ORDER, signed=False)
        self.priority       = int.from_bytes(buff[4:8],     byteorder=BYTE_ORDER, signed=False)
        self.properties     = int.from_bytes(buff[8:12],    byteorder=BYTE_ORDER, signed=False)
        self.statuses       = int.from_bytes(buff[12:16],   byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}, priority={}, properties={}".format(
            self.uid, self.priority, utils.record_to_properties_and_statuses(self))

    def store(self, processes, pid):
        tasks = processes['by-pids'][pid]['tasks']
        ensure_task(tasks, self.uid)
        tasks[self.uid]['delete'] = self

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

    def store(self, processes, pid):
        tasks = processes['by-pids'][pid]['tasks']
        assert(self.uid in tasks)
        tasks[self.uid]['sends'].append(self)

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

    def store(self, processes, pid):
        tasks = processes['by-pids'][pid]['tasks']
        assert(self.uid in tasks)
        tasks[self.uid]['recvs'].append(self)

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

    def store(self, processes, pid):
        tasks = processes['by-pids'][pid]['tasks']
        assert(self.uid in tasks)
        tasks[self.uid]['allreduces'].append(self)

    def size(self):
        return RECORD_ALLREDUCE_SIZE

class RecordRank(Record):

    def parse(self, buff):
        self.comm = int.from_bytes(buff[0:4],   byteorder=BYTE_ORDER, signed=False)
        self.rank = int.from_bytes(buff[4:8],   byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "comm={}, rank={}".format(self.comm, self.rank)

    def store(self, processes, pid):
        convert = processes['convert']
        if "p2c2r" not in convert:
            convert["p2c2r"] = {}
            convert["c2r2p"] = {}
        if pid not in convert["p2c2r"]:
            convert["p2c2r"][pid] = {}
        if self.comm not in convert["p2c2r"][pid]:
            convert["p2c2r"][pid][self.comm] = self.rank
        if self.comm not in convert["c2r2p"]:
            convert["c2r2p"][self.comm] = {}
        if self.rank not in convert["c2r2p"][self.comm]:
            convert["c2r2p"][self.comm][self.rank] = pid

    def size(self):
        return RECORD_RANK_SIZE

class RecordBlocked(Record):

    def parse(self, buff):
        self.uid = int.from_bytes(buff[0:4], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}".format(self.uid)

    def store(self, processes, pid):
        pass

    def size(self):
        return RECORD_BLOCKED_SIZE

class RecordUnblocked(Record):

    def parse(self, buff):
        self.uid = int.from_bytes(buff[0:4], byteorder=BYTE_ORDER, signed=False)

    def attributes(self):
        return "uid={}".format(self.uid)

    def store(self, processes, pid):
        pass

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
