#!/bin/sh

COMMAND="mpcrun $MPCRUN_COMMAND -n=4 ./a.out -n"
echo "$COMMAND"
eval "$COMMAND"
command_result="$?"
rm -rf TEMP_MPC_$$
exit $command_result
