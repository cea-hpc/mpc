#!/bin/sh
MPC_AUTO_KILL_TIMEOUT="20"

export MPC_AUTO_KILL_TIMEOUT 
COMMAND="mpcrun $MPCRUN_COMMAND -n=4 ./a.out"
echo "$COMMAND"
eval "$COMMAND"
command_result="$?"
rm -rf MPC_$$
echo "return code $command_result"
if test "$command_result" = "0" ; then 
    exit 1
else
    exit 0
fi
