"""
"   usage: python3 cte_compressed.py -h
"
"   This tool compresses a Chrome Trace Engine (CTE) trace file (.json),
"   since Chrome may only accept traces of size < 400 Mo
"""

import json
import sys
import getopt
from utils import *
import math

def usage():
    print('usage: ' + sys.argv[0] + ' {-h|--help} [[-i |--in=]CTE_IN] [[-o | --out=]CTE_OUT] {-s|--strings} {[-b|--begin=]TIME_BEGIN} {[-e | --end=]TIME_END} {-t | --time} {-d | --dependencies}')

def main():
    if len(sys.argv) < 2:
        return usage()

    CONFIG = {
        'in' : None,
        'out': None,
        'strings': False,
        'begin': 0,
        'end': math.inf,
        'time': False,
        'dependencies': False
    }

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'i:o:hsb:e:td', ['help', 'in=', 'out=', 'strings', 'begin=', 'end=', 'time', 'dependencies'])
    except getopt.GetoptError:
        usage()
        sys.exit(1)

    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage()
            sys.exit(0)
        elif opt in ('-i', '--in'):
            CONFIG['in'] = arg
        elif opt in ('-o', '--out'):
            CONFIG['out'] = arg
        elif opt in ('-s', '--strings'):
            CONFIG['strings'] = True
        elif opt in ('-b', '--begin'):
            CONFIG['begin'] = int(arg)
        elif opt in ('-e', '--end'):
            CONFIG['end'] = int(arg)
        elif opt in ('-t', '--time'):
            CONFIG['time'] = True
        elif opt in ('-d', '--dependencies'):
            CONFIG['dependencies'] = True
        else:
            usage()
            sys.exit(1)

    if CONFIG['in'] == None or CONFIG['out'] == None:
        usage()
        sys.exit(1)

    replacements = (
        ('DEPEND', 'D'),
        ('STARTED', 'S'),
        ('COMPLETED', 'C'),
        ('PERSISTENT', 'PER'),
        ('BLOCKED', 'BKED'),
        ('BLOCKING', 'BKING'),
        ('UNBLOCKED', 'UBKED'),
        ('PAUSED', 'P'),
        ('task-schedule', 't-s'),
        ('communication', 'comm'),
        ('task-create-control', 'tcc'),
        ('task-delete-control', 'tdc'),
        ('task-create', 'tc'),
        ('task-delete', 'td'),
        ('block-resume', 'b-r'),
        ('dependencies', 'dep'),
        ('properties', 'p'),
        ('properties_begin', 'p-b'),
        ('properties_end', 'p-e'),
        ('weight', 'w'),
        ('created', 'c'),
        ('uid', 'i'),
        ('recv', 'r'),
        ('send', 's'),
        ('allreduce', 'a')
    )

    print("Opening {}".format(CONFIG['in']))
    with open(CONFIG['in'], 'r') as f:
        print("Reading {}".format(CONFIG['in']))
        data = f.read().replace('\n', '')

        if CONFIG['strings']:
            print("Compressing strings")
            for replacement in replacements:
                print("Compressing strings... {}".format(replacement))
                data = data.replace(replacement[0], replacement[1])

        print("Removing async. events")
        cte = json.loads(data)
        data = None
        events = []
        tmin = math.inf
        for event in cte['traceEvents']:
            if 'cat' in event:
                if event['cat'] in ('dep') and CONFIG['dependencies']:
                    continue
            if 'ts' in event:
                if not (CONFIG['begin'] <= event['ts'] <= CONFIG['end']):
                    continue
                if CONFIG['time'] and event['ts'] < tmin:
                    tmin = event['ts']
            events.append(event)

        if CONFIG['time']:
            print("Compressing times")
            for event in events:
                if 'ts' in event:
                    event['ts'] -= tmin
                if 'args' in event:
                    if 'ts' in event['args']:
                        event['args']['ts'] -= tmin
                    if 'c' in event['args']:
                        event['args']['c'] -= tmin

    with open(CONFIG['out'], "w") as f:
        print("writing `{}` to disk...".format(CONFIG['out']))
        json.dump({'traceEvents': events}, f)
        f.write("\n")
        print("`{}` compressed to `{}`".format(CONFIG['in'], CONFIG['out']))

    def human_bytes(size, decimal_places=2):
        for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
            if size < 1024.0 or unit == 'TB':
                break
            size /= 1024.0
        return f"{size:.{decimal_places}f} {unit}"
    s1 = os.path.getsize(CONFIG['in'])
    s2 = os.path.getsize(CONFIG['out'])
    print("Compressed from {} to {}. {}% memory gained".format(human_bytes(s1), human_bytes(s2), int((1.0 - s2 / s1) * 100.0)))

if __name__ == "__main__":
    main()
