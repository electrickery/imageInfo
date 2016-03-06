#!/bin/sh
#
TTY='/dev/ttyS0'
BAUD='38400'
IMG1='./images/px8util.vfd'
IMG2='./images/px8games.vfd'
IMG3='./images/px4util.vfd'
IMG4='./images/editor.vfd'

/bin/stty -F $TTY ospeed $BAUD ispeed $BAUD
./epspd -s $TTY -0 $IMG1 -1 $IMG2 -2 $IMG3 -3 $IMG4 $1 $2
