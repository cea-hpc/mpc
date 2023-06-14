#!/bin/bash

# compute environment depends on partition
PARTITION=@SLURMPARTITION@

if [ "$PARTITION" == "localhost" ];
then
	export PTL_IFACE_NAME=lo
fi

# run given command
$@
