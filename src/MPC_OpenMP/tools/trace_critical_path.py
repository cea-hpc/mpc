"""
"   This tool compute the critical path from an MPI+OpenMP(tasks) trace
"""
import getopt
import sys

from utils import *

def traces_critical_path(src):
    (Too, TP, T1, P) = blumofe_leiserson_check(src)
    print('Values')
    print('    Too = {} s.'.format(Too))
    print('    TP  = {} s.'.format(TP))
    print('    T1  = {} s.'.format(T1))
    print('    P   = {} threads'.format(P))
    print('Check')
    print('    Too <= T1/P <= TP <= Too + T1/P')
    print('    {} <= {} <= {} <= {}'.format(Too, T1 / P, TP, Too + T1 / P))

def usage():
    print('usage: {} [TRACE]'.format(sys.argv[0]))

def main():
    if len(sys.argv) != 2:
        return usage()
    traces_critical_path(sys.argv[1])

if __name__ == "__main__":
    main()

""
