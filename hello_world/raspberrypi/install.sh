#!/bin/sh
module="hello"
device="myhello"
mode="664"
/sbin/insmod ./$module.ko
rm -f /dev/${device}0
major=$(awk '$2=="myhello" {print $1}' /proc/devices)
echo $major
mknod /dev/${device}0 c $major 0
chmod $mode /dev/${device}0
