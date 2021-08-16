import json
import sys
from utils import *

def main():
    if len(sys.argv) != 3:
        print("error usage : {} [TRACE_DIR_SRC] [CTE_FILE_DST]".format(sys.argv[0]))
        return

    src = sys.argv[1]
    dst = sys.argv[2]
    with open(dst, "w") as f:
        (_, cte, gg) = parse_traces(src)
        print("writing `{}` to disk...".format(dst))
        json.dump(cte, f)
        f.write("\n")
        print("`{}` converted to `{}`".format(src, dst))

if __name__ == "__main__":
    main()


