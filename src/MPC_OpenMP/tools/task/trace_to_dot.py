"""
"   This tool generate a .dot graph file from an MPI+OpenMp(tasks) trace
"
"   WARNING:    to generate inter-process edges that corresponds to MPI communications,
"               OpenMP tasks that contains communications should be labeled this way :

                on the sender rank :
"                   send-%d-%d-%d, sender_rank, receiver_rank, tag

                on the sender rank :
"                   recv-%d-%d-%d, receiver_rank, sender_rank, tag
"""
import getopt
import sys

from utils import *

def traces_to_dot(src, show_priority=False, show_omp_priority=False, show_critical=False, show_time=False, gradient=False):
    gg = traces_to_gg(src)

    if show_critical:
        print('computing critical...', file=sys.stderr)
        critical_path = gg.compute_critical()
        critical_time = 0.
        for index, node in enumerate(critical_path):
            node.set_critical(index)
            critical_time += node.time
        print('global critical path time = {}'.format(critical_time / 1000000.0), file=sys.stderr)

    gg.dump(show_priority=show_priority, show_omp_priority=show_omp_priority, show_time=show_time, gradient=gradient)

def usage():
    print('usage: {} [[-f |--file=]TRACE] {-h|--help} {-w|--priority} {-p|--omp_priority} {-c|--critical} {-t|--time} {-g|--gradient}'.format(sys.argv[0]))

def main():
    if len(sys.argv) < 2:
        return usage()

    CONFIG = {
        'file' : None,
        'priority': False,
        'omp_priority': False,
        'critical': False,
        'time': False,
        'gradient': False
    }

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'f:hwpctg', ['file=', 'help', 'priority', 'omp_priority', 'critical', 'time', 'gradient'])
    except getopt.GetoptError:
        usage()
        sys.exit(1)

    for opt, arg in opts:
        if opt in ('-f', '--file'):
            CONFIG['file'] = arg
        elif opt in ('-h', '--help'):
            usage()
            sys.exit(0)
        elif opt in ('-w', '--priority'):
            CONFIG['priority'] = True
        elif opt in ('-p', '--omp_priority'):
            CONFIG['omp_priority'] = True
        elif opt in ('-c', '--critical'):
            CONFIG['critical'] = True
        elif opt in ('-t', '--time'):
            CONFIG['time'] = True
        elif opt in ('-g', '--gradient'):
            CONFIG['gradient'] = True
        else:
            usage()
            sys.exit(1)

    if CONFIG['file']:
        traces_to_dot(
            CONFIG['file'],
            show_priority=CONFIG['priority'],
            show_omp_priority=CONFIG['omp_priority'],
            show_critical=CONFIG['critical'],
            show_time=CONFIG['time'],
            gradient=CONFIG['gradient'])
    else:
        usage()

if __name__ == "__main__":
    main()

""
