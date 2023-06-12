"""
"   usage: python3 cte_times.py -h
"
"   This tool read a Chrome Trace Event file, and
"   print the time of the first, and the last event
"""

import json
import sys
import getopt
from utils import *
import math

def usage():
    print('usage: ' + sys.argv[0] + ' [CTE_FILE]')

def main():
    if len(sys.argv) != 2:
        return usage()

    filename = sys.argv[1]
    print("Opening {}".format(filename))
    with open(filename, 'r') as f:
        print("Parsing {}".format(filename))
        cte = json.loads(f.read())
        times = {
            'events': {
                't0': math.inf,
                'tf': 0
            },

            'creation': {
                't0': math.inf,
                'tf': 0
            }
        }
        print("Reading {} events".format(filename))
        for event in cte['traceEvents']:
            if 'ts' in event:
                t = event['ts']
                times['events']['t0'] = min(t, times['events']['t0'])
                times['events']['tf'] = max(t, times['events']['tf'])
                if 'cat' in event:
                    if event['cat'] in ('tc', 'task-create'):
                        times['creation']['t0'] = min(t, times['creation']['t0'])
                        times['creation']['tf'] = max(t, times['creation']['tf'])
        print(times)

if __name__ == "__main__":
    main()
