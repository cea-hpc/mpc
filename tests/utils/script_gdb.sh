#!/bin/bash 
#export PTL_DEGUB=1
#export PTL_LOG_LEVEL=3 
export PTL_IFACE_NAME=lo
gdb --command=./gdbscript.gdb -e $1 
