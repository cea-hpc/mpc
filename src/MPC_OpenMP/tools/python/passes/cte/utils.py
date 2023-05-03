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
]

MPC_OMP_TASK_STATUSES = [
    'STARTED',
    'COMPLETED',
    'BLOCKING',
    'BLOCKED',
    'UNBLOCKED',
    'IN_BLOCKED_LIST',
    'CANCELLED',
    'DIRECT_SUCCESSOR',
]

def record_to_properties_and_statuses(record):
    lst = []

    # task properties
    for i in range(len(MPC_OMP_TASK_PROP)):
        if record.properties & (1 << i):
            lst.append(MPC_OMP_TASK_PROP[i])

    # task statuses
    for i in range(len(MPC_OMP_TASK_STATUSES)):
        if record.statuses & (1 << i):
            lst.append(MPC_OMP_TASK_STATUSES[i])

    return lst

""" Hash a string to a 16-digits integer """
def hash_string(string):
    return int(hashlib.sha1(string.encode("utf-8")).hexdigest(), 16) % (2**64)
