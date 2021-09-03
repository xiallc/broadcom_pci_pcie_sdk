#!/usr/bin/python

import sys
import os
import sys
import getopt
import time
import datetime
import random
sys.path.insert(0,"../code")

from PlxSdk import *
from const import *
from misc import *
from bar import *

def usage(progname):
	print "progname -s [<bus>:<slot>.<func>]  -b -o offset"
	print "progname -s [<bus>:<slot>.<func>]  -w -o offset"
	print "progname -s [<bus>:<slot>.<func>]  -d -o offset"
	print " Bus,slot,function, and offset are hex numbers"
	return

def parseopts():
	offset=-1
	l=0
	if len(sys.argv) <2:
		print "Invalid Arguments"
		usage(sys.argv[0])
		sys.exit(2)
	try:
		opts,args =getopt.getopt(sys.argv[1:],"hbwdas:o:",['--help','--byte','--word','--dword','--all','--dev=','--offset='])
	except getopt.GetoptError,err:
		print "Here1"
		print err
		usage(sys.argv[0])
		sys.exit(2)
	if len(opts) == 0:
		print "Here2"
		usage(sys.argv[0])
		sys.exit(2)
	for o, a in opts:
		if o in ("-h","--help"):
			usage(sys.argv[0])
			sys.exit(0)
		elif o in("-s","--dev"):
			tmp =a
			bus,dev,fun=parsedev(tmp)
		elif o in("-o","--offset"):
			offset =int(a,16)
		elif o in ('-b','--byte'):
			l=8
		elif o in ('-w','--word'):
			l=16
		elif o in ('-d','--dword'):
			l=32
		elif o in ('-a','--all'):
			l=0
		else:
			print "Here3"
			usage(sys.argv[0])
			sys.exit(0)
	if tmp == None:
		print"Need BDF"
		usage(sys.argv[0])
		sys.exit(0)
	if offset == -1:
		print "Here4"
		usage(sys.argv[0])
		sys.exit(0)
	return bus,dev,fun,offset,l

def MemoryRead(SdkLib,dp,offset,l):
	B=BarSpace()
	#Assume Bar0
	rc = B.initBarProperties(SdkLib,dp,0)
	if rc != PLX_STATUS_OK:	
		print "Could not Initialize the BAR",rc
		sys.exit(0)
	if l == 8 or l ==0 :
		print "Offset = ",offset,"Size = ",l," Value =",hex((B.MemoryRead8(offset)))
	if l == 16 or l == 0 :
		print "Offset = ",offset,"Size = ",l," Value =",hex((B.MemoryRead16(offset)))
	if l == 32 or l ==0 :
		print "Offset = ",offset,"Size = ",l," Value =",hex((B.MemoryRead32(offset)))
	

def main():
	SdkLib = LoadLibrary()
	if SdkLib == None:
		print "Library Not loaded"
		sys.exit(0)
	bus,dev,fun,offset,l = parseopts()
	pkey=PLX_DEVICE_KEY()
	pkey.set_bus(bus)
	pkey.set_slot(dev)
	pkey.set_function(fun)
	rc,idx=FindDeviceByDevice(SdkLib,pkey)
	if rc != PLX_STATUS_OK:	
		print "Could not find the device",rc
		sys.exit(0)
	rc,dp=DeviceOpen(SdkLib,pkey)
	if rc !=PLX_STATUS_OK:
		print "Open Failed",rc
		sys.exit(0)
	MemoryRead(SdkLib,dp,offset,l)
	DeviceClose(SdkLib,dp)

if __name__ == "__main__":
    main()
