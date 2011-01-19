#!/bin/sh

COMMAND="mpcrun $MPCRUN_COMMAND -n=4 ./a.out"
echo "$COMMAND"
cat <<EOF | eval "$COMMAND"
1
EOF
command_result="$?"
rm -rf TEMP_MPC_$$
exit $command_result
