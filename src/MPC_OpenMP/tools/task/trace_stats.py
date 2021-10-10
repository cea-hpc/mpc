"""
"   usage: python3 trace_stats.py -h
"
"   This tool gather statistics of an hybrid MPI+OpenMP(tasks) trace
"""
import json
import sys
from utils import *

def main():
    if len(sys.argv) != 3:
        print("error usage : {} [TRACE_DIR_SRC] [TRACE_STATS_DST]".format(sys.argv[0]))
        return
    src = sys.argv[1]
    dst = sys.argv[2]
    with open(dst, 'w') as f:
        (stats, _, _, _) = parse_traces(sys.argv[1])
        print("writing `{}` to disk...".format(dst))
        json.dump(stats, f, indent=4)
        f.write("\n")

if __name__ == "__main__":
    main()


