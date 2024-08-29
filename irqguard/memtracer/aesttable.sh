#!/bin/bash

PIN_ROOT=$(pwd)/pintool


# disable pmcd profiling
MEMTRACING_ACTIVE_FILE="/tmp/memtracing_active.iguard"

touch $MEMTRACING_ACTIVE_FILE

# setarch is used to disable ASLR
#LD_LIBRARY_PATH=~/pmcd-aes-ttables/self_compiled_openssl gdb --args setarch `uname -m` -R ./pintool/pin -follow_execv -t $PWD/memtracer-pin-extension/obj-intel64/pinmemtracer -- ~/pmcd-aes-ttables/profile
LD_LIBRARY_PATH=~/case-study-openssh/self_compiled_openssl setarch `uname -m` -R ./pintool/pin -follow_execv -t $PWD/memtracer-pin-extension/obj-intel64/pinmemtracer -- ~/case-study-openssh/profile

rm $MEMTRACING_ACTIVE_FILE

# minimize config
#python3 minimize_trace.py
