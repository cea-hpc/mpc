import sys
import json
import math

if len(sys.argv) != 4:
    print("usage: {} [A] [B] [OUT]".format(sys.argv[0]))
    sys.exit(1)

events = []

def append(name):
    global events
    cte = {}
    with open(name, 'r') as f:
        cte = json.loads(f.read().replace('\n', ''))
        m = math.inf
        for event in cte['traceEvents']:
            if 'ts' in event:
                if 'cat' in event and event['cat'] == 'task-schedule':
                    m = min(m, event['ts'])
    return (m, cte['traceEvents'])

(m1, cte1) = append(sys.argv[1])
(m2, cte2) = append(sys.argv[2])
if m1 > m2:
    cte1, cte2 = cte2, cte1
    m1, m2 = m2, m1
assert(m1 < m2)
dt = abs(m2 - m1)

for event in cte2:
    if 'ts' in event:
        if 'cat' in event and event['cat'] == 'task-schedule':
            event['ts'] -= dt

with open(sys.argv[3], "w") as f:
    json.dump({'traceEvents': cte1 + cte2}, f)
    f.write("\n")
