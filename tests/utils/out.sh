#!/bin/sh

OUT="$(hostname).$$.out"
"$@" 2>&1 | tee ${OUT}
