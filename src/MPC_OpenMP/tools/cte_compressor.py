import json
import sys
import getopt
from utils import *
import math

def usage():
    print('usage: {} {-h|--help} [[-i |--in=]CTE_IN] [[-o | --out=]CTE_OUT] {[-s|--skip=]SKIP_FACTOR} {[-b|--begin=]TIME_BEGIN} [{-e | --end=}TIME_END]'.format(sys.argv[0]))

def main():
    if len(sys.argv) < 2:
        return usage()

    CONFIG = {
        'in' : None,
        'out': None,
        'skip': 1.0,
        'begin': 0,
        'end': math.inf
    }

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'i:o:hs:b:e:', ['help', 'in=', 'out=', 'skip=', 'begin=', 'end='])
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
        elif opt in ('-s', '--skip'):
            CONFIG['skip'] = float(arg)
        elif opt in ('-b', '--begin'):
            CONFIG['begin'] = int(arg)
        elif opt in ('-e', '--end'):
            CONFIG['end'] = int(arg)
        else:
            usage()
            sys.exit(1)

    if CONFIG['in'] == None or CONFIG['out'] == None:
        usage()
        sys.exit(1)

    ntask_skip = int(1.0 / CONFIG['skip'])

    replacements = (
        ('DEPEND', 'D'),
        ('STARTED', 'S'),
        ('COMPLETED', 'C'),
        ('PAUSED', 'P'),
        ('task-schedule', 't-s'),
        ('communication', 'comm'),
        ('block-resume', 'b-r'),
        ('dependencies', 'dep'),
        ('properties_begin', 'p-b'),
        ('properties_end', 'p-e'),
        ('weight', 'w'),
        ('created', 'c'),
        ('uid', 'i'),
    )

    print("Opening {}".format(CONFIG['in']))
    with open(CONFIG['in'], 'r') as f:
        print("Reading {}".format(CONFIG['in']))
        data = f.read().replace('\n', '')
        print("Compressing strings")
        for replacement in replacements:
            print("Compressing strings... {}".format(replacement))
            data = data.replace(replacement[0], replacement[1])

        print("removing async events")
        cte = json.loads(data)
        data = None
        events = []
        ntask = 0
        for event in cte['traceEvents']:
            if 'cat' in event:
                if event['cat'] in ('b-r', 'comm', 'dep'):
                    continue
                if event['cat'] == 't-s':
                    ntask += 1
                    if (ntask - 1) % ntask_skip != 0:
                        continue
            if 'ts' in event:
                if not (CONFIG['begin'] <= event['ts'] <= CONFIG['end']):
                    continue
            events.append(event)

    with open(CONFIG['out'], "w") as f:
        print("writing `{}` to disk...".format(CONFIG['out']))
        json.dump({'traceEvents': events}, f)
        f.write("\n")
        print("`{}` compressed to `{}`".format(CONFIG['in'], CONFIG['out']))

if __name__ == "__main__":
    main()
