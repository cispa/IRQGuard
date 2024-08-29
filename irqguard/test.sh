#! /bin/sh

rm ./dummy
make dummy
make kmodule

sudo rmmod iguard
sudo insmod iguard.ko
sudo ./dummy

sudo rmmod iguard
sudo insmod iguard.ko
sudo ./dummy

sudo rmmod iguard

echo "\n[=] You should see 2 incoming signals"
