#!/bin/sh
#
IMG0='./images/archs.d88'
IMG1='./images/px8_eps-tf20.d88'
IMG2='./images/px4UtilityDisk.d88'
IMG3='./images/px4wand.d88'


./epspdv3 -s $TTY -0 $IMG0 -1 $IMG1 -2 $IMG2 -3 $IMG3
