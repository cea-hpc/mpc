import math
from pcvs.plugins import Plugin

class MPI(Plugin):
    step = Plugin.Step.TEST_EVAL

    def run(self, *args, **kwargs):
        # this dict maps keys (it name) with values (it value)
        # returns True if the combination should be used
        comb = kwargs['combination']
        portalsrail = comb.get('portalsrails', None)
        tcprail = comb.get('tcprails', None)
        n_proc = comb.get('n_proc', None)
        n_node = comb.get('n_node', None)
        n_core = comb.get('n_core', None)

        if (n_proc == n_node) and (n_node == n_core) and (portalsrail != 0 or tcprail != 0) :
            return True
        else:
            return False

