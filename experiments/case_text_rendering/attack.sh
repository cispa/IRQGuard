#!/bin/bash
CORE1=4
CORE2=0

ADDR=$(objdump -d libcaca-0.99.beta19/caca/.libs/libcaca.so.0.99.19 | grep put_figchar -A 200 | grep lfence | awk '{print "0x"$1}' | tr -d :)
ADDR="0x18100" #$(objdump -d libcaca-0.99.beta19/caca/.libs/libcaca.so.0.99.19 | grep put_figchar -A 200 | grep lfence | awk '{print "0x"$1}' | tr -d :)
echo $ADDR

taskset -c $CORE1 ./monitor libcaca-0.99.beta19/caca/.libs/libcaca.so.0.99.19 $ADDR &
sleep 2
taskset -c $CORE2 ./plot.sh $1
wait
 
