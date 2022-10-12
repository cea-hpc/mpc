#!/bin/bash

# compute environment depends on partition
PARTITION=@SLURMPARTITION@

if [ "$PARTITION" == "localhost" ];
then
	export PTL_IFACE_NAME=lo
else
	# TOBEFIXED: if several default, take last one
	export PTL_IFACE_NAME=$(ip -o -4 route show to default | awk 'END{print $5}')
fi

# run given command
$@
