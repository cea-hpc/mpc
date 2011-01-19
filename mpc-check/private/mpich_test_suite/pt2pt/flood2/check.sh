#!/bin/sh
MPC_AUTO_KILL_TIMEOUT="300"
export MPC_AUTO_KILL_TIMEOUT

COMMAND="mpcrun $MPCRUN_COMMAND -n=4 ./a.out"
echo "$COMMAND"
eval "$COMMAND"
command_result="$?"
rm -rf TEMP_MPC_$$
exit $command_result
