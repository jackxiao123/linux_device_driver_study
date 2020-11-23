#!/bin/sh
module="myscull"
device="myscull"
/sbin/insmod ./$module.ko
#rm -f /dev/${device}[0-3]
major=$(awk "\\$2==\"module\" {print \\$1}" /proc/devices)
print $major
