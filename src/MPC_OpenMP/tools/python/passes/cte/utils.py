import hashlib

MPC_OMP_TASK_PROP = [
    'UNDEFERRED',
    'UNTIED',
    'EXPLICIT',
    'IMPLICIT',
    'INITIAL',
    'INCLUDED',
    'FINAL',
    'MERGED',
    'MERGEABLE',
    'DEPEND',
    'PRIORITY',
    'UP',
    'GRAINSIZE',
    'IF',
    'NOGROUP',
    'HAS_FIBER',
    'PERSISTENT',
    'CONTROL_FLOW',
    'DETACH',
]

MPC_OMP_TASK_FLAGS = [
    'BLOCKING',
    'BLOCKED',
    'UNBLOCKED',
    'IN_BLOCKED_LIST',
    'CANCELLED',
    'DIRECT_SUCCESSOR',
]

MPC_OMP_TASK_STATE = [
    'UNITIALIZED',
    'NOT_QUEUABLE',
    'QUEUABLE',
    'QUEUED',
    'SCHEDULED',
    'SUSPENDED',
    'EXECUTED',
    'RESOLVING',
    'DETACHED',
    'RESOLVED',
    'DEINITIALIZED',
]

def record_to_properties_and_flags_and_state(record):
    lst = []

    # task properties
    for i in range(len(MPC_OMP_TASK_PROP)):
        if record.properties & (1 << i):
            lst.append(MPC_OMP_TASK_PROP[i])

    # task flags
    for i in range(len(MPC_OMP_TASK_FLAGS)):
        if record.flags & (1 << i):
            lst.append(MPC_OMP_TASK_FLAGS[i])

    # task state
    if hasattr(record, 'state'):
        lst.append(MPC_OMP_TASK_STATE[record.state])

    return lst

""" Hash a string to a 16-digits integer """
def hash_string(string):
    return int(hashlib.sha1(string.encode("utf-8")).hexdigest(), 16) % (2**64)
