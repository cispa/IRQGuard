#!/bin/bash

PIN_ROOT=$(pwd)/pintool


# disable iguard profiling
MEMTRACING_ACTIVE_FILE="/tmp/memtracing_active.iguard"

touch $MEMTRACING_ACTIVE_FILE

# setarch is used to disable ASLR
setarch `uname -m` -R ./pintool/pin -follow_execv -t $PWD/memtracer-pin-extension/obj-intel64/pinmemtracer -- $@

rm $MEMTRACING_ACTIVE_FILE

# minimize config
python3 minimize_trace.py
