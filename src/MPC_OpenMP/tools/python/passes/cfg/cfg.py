print("TODO")

def dotprint(f, depth, s):
    f.write('{}{}\n'.format(' ' * depth * 4, s))

def task_uuid(pid, uid):
    return 'Tx{}x{}'.format(pid, uid)

class CFG:
    def __init__(self, tasks):
        pass

    def dump(self, config, f):
        pass
