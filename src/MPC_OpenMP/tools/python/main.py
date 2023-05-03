"""
"   usage: python3 main.py -h
"
"   This tool gather statistics of an hybrid MPI+OpenMP(tasks) trace
"""
import json
import getopt
import sys
import difflib

# TODO: le systeme de dépendances a été implémenté, il reste maintenant à implémenter les dépendances dans chaque pass (c.f TimeBreakdownPass et TimeWorkloadPass qui sont implémentés)

from passes.cte.google_cte              import GoogleCtePass

from passes.communication.duration      import CommunicationDurationPass
from passes.communication.predecessors  import CommunicationPredecessorsPass
from passes.communication.overlap       import CommunicationOverlapPass
from passes.dump                        import DumpPass
from passes.tdg.dot                     import TDGDotPass
from passes.tdg.stats                   import TDGStatsPass
from passes.tdg.critical                import TDGCriticalPass
from passes.time.breakdown              import TimeBreakdownPass
from passes.time.workload               import TimeWorkloadPass

from passes.papi.accumulate             import AccumulatePAPIPass
from passes.papi.cache_hit              import CacheHitPAPIPass
from passes.papi.ins_cycle              import InsCyclePAPIPass

from scheduler import *

# map pass classes by their name
PASS_CLASSES = [
    TimeWorkloadPass,
    TimeBreakdownPass,
    AccumulatePAPIPass,
    CacheHitPAPIPass,
    InsCyclePAPIPass,
    DumpPass,
    CommunicationDurationPass,
    CommunicationOverlapPass,
    CommunicationPredecessorsPass,
    TDGCriticalPass,
    TDGStatsPass,
    TDGDotPass,
    GoogleCtePass,
]
PASSES_NAME = []
PASS_CLASSES_BY_NAME = {}
for p in PASS_CLASSES:
    PASS_CLASSES_BY_NAME[p.NAME] = p
    PASSES_NAME.append(p.NAME)

# Generate dependency tree from passes
PASSES_RANK = {}
for p in PASS_CLASSES:
    PASSES_RANK[p.NAME] = 0

def pass_get_closest_name(pass_name):
    global PASSES_NAME
    return sorted([(x, difflib.SequenceMatcher(None, pass_name, x).ratio()) for x in PASSES_NAME], key=lambda r: -r[1])[0][0]

changed = True
while changed:
    changed = False
    for p in PASS_CLASSES:
        if hasattr(p, 'DEPENDENCIES'):
            for dependency_name in p.DEPENDENCIES:
                if dependency_name not in PASS_CLASSES_BY_NAME:
                    print("The pass '{}' depends on '{}' which does not exist. Did you meant '{}' ?".format(p.NAME, dependency_name, pass_get_closest_name(dependency_name)), file=sys.stderr)
                    sys.exit(1)
                if PASSES_RANK[p.NAME] <= PASSES_RANK[dependency_name]:
                    PASSES_RANK[p.NAME] = PASSES_RANK[dependency_name] + 1
                    changed = True

# print usage
def usage():
    print('usage: ' + sys.argv[0] + ' {-h|--help} [[-i|--input=]TRACE] {-o|--output=OUTPUT} {-p|--progress} {-l|--list=pass1,..., passn}')
    print("Available passes: '{}'".format(", ".join(PASSES_NAME)))

def main():

    # default config
    config = {
        'input':  'traces',
        'output': 'traces',
        'progress': False,
        'passes': ['time.workload', 'time.breakdown', 'tdg.stats', 'papi.accumulate']
    }

    # user config
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hi:o:pl', ['help', 'input=', 'output=', 'process', 'list='])
    except getopt.GetoptError:
        usage()
        sys.exit(1)

    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage()
            sys.exit(0)
        elif opt in ('-i', '--input'):
            config['input'] = arg
        elif opt in ('-o', '--output'):
            config['output'] = arg
        elif opt in ('-p', '--progress'):
            config['progress'] = True
        elif opt in ('-l', '--list'):
            config['passes'] = arg.split(",")
        else:
            usage()
            sys.exit(1)

    # filter out undefined passes
    passes_name = []
    passes_added = {}
    for pass_name in config['passes']:
        if pass_name not in PASS_CLASSES_BY_NAME:
            print("Pass '{}' does not exists. Did you meant '{}' ? Ignoring".format(pass_name, pass_get_closest_name(pass_name)), file=sys.stderr)
        elif pass_name not in passes_added:
            passes_name.append(pass_name)
            passes_added[pass_name] = True

    # add dependencies
    changed = True
    while changed:
        changed = False
        for pass_name in passes_name:
            p = PASS_CLASSES_BY_NAME[pass_name]
            if hasattr(p, 'DEPENDENCIES'):
                for dependency_name in p.DEPENDENCIES:
                    if dependency_name not in passes_added:
                        passes_name.append(dependency_name)
                        passes_added[dependency_name] = True
                        changed = True

    # sort by dependencies
    passes_name = sorted(passes_name, key=lambda pass_name: PASSES_RANK[pass_name])
    print("Using passes in order: {}".format(passes_name))

    # instanciate passes
    passes = []
    for pass_name in passes_name:
        passes.append(PASS_CLASSES_BY_NAME[pass_name]())

    # start inspecting trace
    for inspector in passes:
        inspector.on_start(config)
    parse_traces(config['input'], config['progress'], passes)
    for inspector in passes:
        inspector.on_end(config)

if __name__ == "__main__":
    main()


