import time
import sys

ENABLED = True
PROGRESS_HEADER = " -> "
CURRENT = 0
TOTAL = 1
PREV = 0

DO_TIME = 1
BGN = 0
END = 0

def switch(enabled):
    global ENABLED
    ENABLED = enabled

def init(total):
    if not ENABLED:
        return

    global CURRENT, TOTAL, BGN, END
    CURRENT = 0
    TOTAL = total
    sys.stdout.write(PROGRESS_HEADER + "  0%")
    sys.stdout.flush()
    sys.stdout.write("\b" * 4)
    if DO_TIME:
        BGN = time.time()

def update():
    if not ENABLED:
        return

    global CURRENT, TOTAL, PREV
    CURRENT = CURRENT + 1
    percent = CURRENT / float(TOTAL)
    percent = max(0, min(percent, 1.0))
    percent = int(100 * percent)
    nspaces = 0
    if 0 <= percent < 10:
        nspaces = 2
    elif 10 <= percent < 100:
        nspaces = 1
    if percent != PREV:
        PREV = percent
        sys.stdout.write(" " * nspaces + str(percent) + "%")
        sys.stdout.flush()
        sys.stdout.write("\b" * 4)

def deinit():
    if not ENABLED:
        return

    sys.stdout.write("100%")
    if DO_TIME:
        END = time.time()
        print(" - Took {} s.\n".format(round(END - BGN, 6)))
    else:
        sys.stdout.write("\n\n")
