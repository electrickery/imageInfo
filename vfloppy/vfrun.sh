#!/bin/sh
# vfrun.sh - start script for epspd from the vfloppy 1.1 package

TTY='/dev/ttyS0'
BAUD='38400'
IMG1='./px_disk'
IMG2='./disk1'
IMG3='./test.disk.clean'
IMG4=''

/bin/stty -F $TTY ospeed $BAUD ispeed $BAUD
./epspd $TTY $IMG1 $IMG2 $IMG3 $IMG4
