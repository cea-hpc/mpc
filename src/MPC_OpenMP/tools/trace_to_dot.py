"""
"   This tool generate a .dot graph file from an MPI+OpenMp(tasks) trace
"   WARNING: must be using MPC to generate inter-process edges
"""
import getopt
import sys

from utils import *

def traces_to_dot(config):
    gg = traces_to_gg(config['file'])

    if config['critical']:
        print('computing critical...', file=sys.stderr)
        critical_path = gg.compute_critical()
        critical_time = 0.
        for index, node in enumerate(critical_path):
            node.set_critical(index)
            critical_time += node.time
        print('global critical path time = {}'.format(critical_time / 1000000.0), file=sys.stderr)

    gg.dump(show_label=config['label'],
            show_priority=config['priority'],
            show_omp_priority=config['omp_priority'],
            show_time=config['time'],
            gradient=config['gradient'])

def usage():
    print('usage: {} [[-f |--file=]TRACE] {-h|--help} {-w|--priority} {-p|--omp_priority} {-c|--critical} {-t|--time} {-g|--gradient}'.format(sys.argv[0]))

def main():
    if len(sys.argv) < 2:
        return usage()

    config = {
        'file' : None,
        'label': False,
        'priority': False,
        'omp_priority': False,
        'critical': False,
        'time': False,
        'gradient': False
    }

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'f:hwpctgl', ['file=', 'help', 'priority', 'omp_priority', 'critical', 'time', 'gradient', 'label'])
    except getopt.GetoptError:
        usage()
        sys.exit(1)

    for opt, arg in opts:
        if opt in ('-f', '--file'):
            config['file'] = arg
        elif opt in ('-h', '--help'):
            usage()
            sys.exit(0)
        elif opt in ('-l', '--label'):
            config['label'] = True
        elif opt in ('-w', '--priority'):
            config['priority'] = True
        elif opt in ('-p', '--omp_priority'):
            config['omp_priority'] = True
        elif opt in ('-c', '--critical'):
            config['critical'] = True
        elif opt in ('-t', '--time'):
            config['time'] = True
        elif opt in ('-g', '--gradient'):
            config['gradient'] = True
        else:
            usage()
            sys.exit(1)

    if config['file']:
        traces_to_dot(config)
    else:
        usage()

if __name__ == "__main__":
    main()

