#!/bin/sh
mode="664"

module1="myscull1"
device1="myscull1"
/sbin/insmod ./$module1.ko
rm -f /dev/${device1}
major=$(awk '$2=="myscull1" {print $1}' /proc/devices)
echo $major
mknod /dev/${device1} c $major 0
chmod $mode /dev/${device1}

module2="myscull2"
device2="myscull2"
/sbin/insmod ./$module2.ko
rm -f /dev/${device2}
major=$(awk '$2=="myscull2" {print $1}' /proc/devices)
echo $major
mknod /dev/${device2} c $major 0
chmod $mode /dev/${device2}

module3="myscull3"
device3="myscull3"
/sbin/insmod ./$module3.ko
rm -f /dev/${device3}
major=$(awk '$2=="myscull3" {print $1}' /proc/devices)
echo $major
mknod /dev/${device3} c $major 0
chmod $mode /dev/${device3}
