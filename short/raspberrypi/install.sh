#!/bin/sh
module="myshort"
device="myshort"
mode="664"
/sbin/insmod ./$module.ko
rm -f /dev/${device}0
major=$(awk '$2=="myshort" {print $1}' /proc/devices)
echo $major
mknod /dev/${device}0 c $major 0
chmod $mode /dev/${device}0
