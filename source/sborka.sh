#!/bin/bash

make format
make
sudo insmod HW_02_hello_world.ko
cat /sys/module/HW_02_hello_world/parameters/my_str
sudo rmmod HW_02_hello_world

make check
