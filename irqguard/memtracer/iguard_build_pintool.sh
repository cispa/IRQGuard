#!/bin/bash
HERE1=$(pwd)
if [ ! -d pintool ];
then
    wget http://software.intel.com/sites/landingpage/pintool/downloads/pin-3.14-98223-gb010a12c6-gcc-linux.tar.gz -O ./pin.tar.gz
    tar xzvf pin.tar.gz
    rm pin.tar.gz
    mv pin* pintool
fi

HERE2=$(pwd)
export PIN_ROOT=$(pwd)/pintool
cd ./memtracer-pin-extension && make && cd $HERE2
cp ./memtracer-pin-extension/obj-intel64/pinmemtracer.so ./pintool/.

cd $HERE1