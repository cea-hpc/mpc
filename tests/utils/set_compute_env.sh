#!/bin/bash

# TOBEFIXED: if several default, take last one
export PTL_IFACE_NAME=$(ip -o -4 route show to default | awk 'END{print $5}')

# run given command
$@
