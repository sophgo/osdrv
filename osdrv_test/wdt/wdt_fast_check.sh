#!/bin/sh


if [ -e /sys/kernel/debug/27000000.cv-wd/registers ]; then
	echo "PASSED"
else
	echo "FAILED"
fi

reg_value=`cat /sys/kernel/debug/27000000.cv-wd/registers`
echo $reg_value

