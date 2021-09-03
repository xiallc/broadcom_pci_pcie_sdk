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
	print "progname -s [<bus>:<slot>.<func>] "
	return

def parseopts():
	if len(sys.argv) <2:
		print "Invalid Arguments"
		usage(sys.argv[0])
		sys.exit(2)
	try:
		opts,args =getopt.getopt(sys.argv[1:],"hs:",['--help','--dev='])
	except getopt.GetoptError,err:
		print err
		usage(sys.argv[0])
		sys.exit(2)
	if len(opts) == 0:
		usage(sys.argv[0])
		sys.exit(2)
	for o, a in opts:
		if o in ("-h","--help"):
			usage(sys.argv[0])
			sys.exit(0)
		elif o in("-s","--dev"):
			tmp =a
			bus,dev,fun=parsedev(tmp)
		else:
			usage(sys.argv[0])
			sys.exit(0)
	if tmp == None:
		print"Need BDF"
		usage(sys.argv[0])
		sys.exit(0)
	return bus,dev,fun

def PrintBarProperties(SdkLib,dp):
	B=BarSpace()
	print "{0:^2s} {1:^8s} {2:^8s} {3:^8s} {4:^4s}".format("ID","Value   ","Physical","  Size  ","Flags")
	for i in range(7):
		rc = B.initBarProperties(SdkLib,dp,i)
		if rc != PLX_STATUS_OK:	
			#print "Could not Initialize the BAR",rc
			continue
		print "{0:2d} {1:08x} {2:08x} {3:8x} {4:04x}".format(i,B.getBarValue(),B.getBarPhysical(),B.getBarSize(),B.getBarFlags())

def main():
	SdkLib = LoadLibrary()
	if SdkLib == None:
		print "Library Not loaded"
		sys.exit(0)
	bus,dev,fun = parseopts()
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
	PrintBarProperties(SdkLib,dp)
	DeviceClose(SdkLib,dp)

if __name__ == "__main__":
    main()
