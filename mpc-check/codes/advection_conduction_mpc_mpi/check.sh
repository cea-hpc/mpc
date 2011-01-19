#!/bin/sh

COMMAND="mpcrun $MPCRUN_COMMAND -n=4 "
echo "$COMMAND"
eval "$COMMAND ./advection" && eval "$COMMAND ./conduction"
command_result="$?"
rm -rf TEMP_MPC_$$
exit $command_result
