#!/bin/sh

COMMAND="mpcrun $MPCRUN_COMMAND -n=4 ./a.out 4"
echo "$COMMAND"
eval "$COMMAND"
command_result="$?"
rm -rf MPC_$$
exit $command_result
