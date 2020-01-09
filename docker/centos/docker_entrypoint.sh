#!/bin/bash
command="$@"
shift $#
. /install/mpcvars.sh
$command
