#!/bin/python
# addressReduce, converts an octal dump file to something that streams into a PDP-11 ODT prompt.
# the octal dump lines should look like "000004/000226". Consecutive addresses are stripped. 
# Lines starting with a "#" are ignored.
# usage: python addressReduce.py testfile.txt > /dev/ttyUSB0

# This is address Reduce 0.1, tested with Python 2.7.12 and a KDF-11-A/DBV-J-11 set in a Minc-11 at 9600 Baud
# fjkraan@xs4all.nl, 2017-10-29

from __future__ import print_function
import sys
import time

fileName = sys.argv[1]
file = open(fileName, "r") 

oldAddress = 0o0

for line in file: 
    if line.startswith('#') or not line.strip():
        continue
    slashIndex = line.find('/')
    
    address = int(line[:slashIndex], 8)
    data    = line[slashIndex+1:].strip()
#    print str(int(address, 8)) + "==" + str(oldAddress)
    if not (address == oldAddress):
        print ("\r")
        print (oct(address), end='')
        print ("/", end='')
        sys.stdout.flush() 
        time.sleep(0.5)
    oldAddress = address + 2
    print (data)
    time.sleep(0.2)
print ("\r")