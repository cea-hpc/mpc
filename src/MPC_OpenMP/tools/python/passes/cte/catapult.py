import hashlib
import random

allcolors = [
    "thread_state_uninterruptible",
    "thread_state_iowait",
    "thread_state_running",
    "thread_state_runnable",
    "thread_state_unknown",
    "background_memory_dump",
    "light_memory_dump",
    "detailed_memory_dump",
    "vsync_highlight_color",
    "generic_work",
    "good",
    "bad",
    "terrible",
    "black",
    "grey",
    "white",
    "yellow",
    "olive",
    "rail_response",
    "rail_animation",
    "rail_idle",
    "rail_load",
    "startup",
    "heap_dump_stack_frame",
    "heap_dump_child_node_arrow",
    "cq_build_running",
    "cq_build_passed",
    "cq_build_failed",
    "cq_build_abandoned",
    # "cq_build_attempt_running"
    # some version of chrome has a buggy name 'cq_build_attempt_runnig'
    "cq_build_attempt_passed",
    "cq_build_attempt_failed",
]

colors = allcolors.copy()

s = "Oui mais j'aime pas les harengs !"
seed = int(7 * hashlib.sha1(s.encode("utf-8")).hexdigest(), 16) % ((1 << 32) - 1)
random.seed(seed)
random.shuffle(colors)
