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
	l=0
	offset=-1
	value=-1
	if len(sys.argv) <2:
		print "Invalid Arguments"
		usage(sys.argv[0])
		sys.exit(2)
	try:
		opts,args =getopt.getopt(sys.argv[1:],"hbwds:o:v:",['--help','--byte','--word','--dword','--dev=','--offset=','--value='])
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
		elif o in("-o","--offset"):
			offset =int(a,16)
		elif o in ('-b','--byte'):
			l=8
		elif o in ('-w','--word'):
			l=16
		elif o in ('-d','--dword'):
			l=32
		elif o in("-v","--value"):
			value =int(a)
			
		else:
			usage(sys.argv[0])
			sys.exit(0)
	if tmp == None:
		print"Need BDF"
		usage(sys.argv[0])
		sys.exit(0)
	if offset == -1:
		usage(sys.argv[0])
		sys.exit(0)
	if value == -1:
		print "Invalid value to write",value
		usage(sys.argv[0])
		sys.exit(0)
	return bus,dev,fun,offset,l,value

def MemoryWrite(SdkLib,dp,offset,l,value):
	B=BarSpace()
	#Assume Bar0
	rc = B.initBarProperties(SdkLib,dp,0)
	if rc != PLX_STATUS_OK:	
		print "Could not Initialize the BAR",rc
		sys.exit(0)
	if l == 8:
		b=B.MemoryRead8(offset)
		B.MemoryWrite8(offset,(value & 0xFF))
		a=B.MemoryRead8(offset)
		print "Before = ",hex(b),"Value=",hex(value),"After = ",hex(a)
		return
	if l == 16:
		b=B.MemoryRead16(offset)
		B.MemoryWrite16(offset,(value & 0xFFFF))
		a=B.MemoryRead16(offset)
		print "Before = ",hex(b),"Value=",hex(value),"After = ",hex(a)
		return
	if l == 32:
		b=B.MemoryRead32(offset)
		B.MemoryWrite32(offset,(value & 0xFFFFFFFF))
		a=B.MemoryRead32(offset)
		print "Before = ",hex(b),"Value=",hex(value),"After = ",hex(a)
		return
	else:
		print "Unknown size"
	return
		
	

def main():
	SdkLib = LoadLibrary()
	if SdkLib == None:
		print "Library Not loaded"
		sys.exit(0)
	bus,dev,fun,offset,l,value = parseopts()
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
	MemoryWrite(SdkLib,dp,offset,l,value)
	DeviceClose(SdkLib,dp)

if __name__ == "__main__":
    main()
