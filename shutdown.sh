#!/bin/sh

export DISPLAY=:0
killall power_manager
killall -9 udevd
#killall -9 system_server
killall -9 xinit

rm -rf /tmp/vip/*                                                               
rm -rf /tmp/permanent/* 

echo "Run Shutdown animation..."
nice -n -15 /usr/bin/boot-animation --offmsg " " --clear &
sleep 1

echo "Stopping indicator..."
killall -9 indicator

# factory resetting...
# flag set by factory-reset.sh
if [ -f /opt/.factoryreset ]; then
	/usr/bin/run-factory-reset.sh
fi


