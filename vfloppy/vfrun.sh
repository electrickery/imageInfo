#!/bin/sh
#
TTY='/dev/ttyS0'
BAUD='38400'
IMG1='./px8util.vfd'
IMG2='./px8games.vfd'
IMG3='./px4util.vfd'

/bin/stty -F $TTY ospeed $BAUD ispeed $BAUD
./epspd $TTY $IMG1 $IMG2 $IMG3 $IMG4
