#!/usr/bin/python

import sys
import os
import sys
import getopt
import time
import datetime
import signal
sys.path.insert(0,"../code")
from PlxSdk import *
from const import *
from misc import *

SdkLib=None
dp=None
raw=0
def Cleanup(signal,frame):
	global SdkLib,dp
	print "Cleaning up"
	PerformanceMonitorControl(SdkLib,dp,PLX_PERF_CMD_STOP)
	DeviceClose(SdkLib,dp)
	sys.exit(0)
	return
def DisplayRawData(PerfProp):
	print "FMLY:{0:2d} PN:{1:02d} LW:{2:02d} LS:{3:04d} ST:{4:02d} STP:{5:02d}".format(PerfProp.PlxFamily,
		PerfProp.PortNumber,
		PerfProp.LinkWidth,
		PerfProp.LinkSpeed,
		PerfProp.Station,
		PerfProp.StationPort)

	print "Ing: {0:04d} {1:04d} {2:04d} {3:04d} {4:04d} {5:04d} {6:04d}".format(PerfProp.IngressPostedHeader,
		PerfProp.IngressPostedDW,
		PerfProp.IngressNonpostedDW,
		PerfProp.IngressCplHeader,
		PerfProp.IngressCplDW,
		PerfProp.IngressDllp,
		PerfProp.IngressPhy)
	print "Egr: {0:04d} {1:04d} {2:04d} {3:04d} {4:04d} {5:04d} {6:04d}".format(PerfProp.EgressPostedHeader,
		PerfProp.EgressPostedDW,
		PerfProp.EgressNonpostedDW,
		PerfProp.EgressCplHeader,
		PerfProp.EgressCplDW,
		PerfProp.EgressDllp,
		PerfProp.EgressPhy)
		
	return
def DisplayStat(PerfProp,pStat):
	print "{0:02d} Ing:{1:6.2f} {2:6.2f} {3:06.2f} {4:6.2f} {5:6.2f} {6:6.2f} ".format(PerfProp.PortNumber,
					pStat.IngressLinkUtilization,
					FormatInteger(pStat.IngressTotalBytes),
					FormatInteger(pStat.IngressTotalByteRate),
					FormatInteger(pStat.IngressPayloadTotalBytes),
					FormatInteger(pStat.IngressPayloadByteRate),
					pStat.IngressPayloadAvgPerTlp)
	print "{0:02d} Egr:{1:6.2f} {2:6.2f} {3:06.2f} {4:6.2f} {5:6.2f} {6:6.2f} ".format(PerfProp.PortNumber,
					pStat.EgressLinkUtilization,
					FormatInteger(pStat.EgressTotalBytes),
					FormatInteger(pStat.EgressTotalByteRate),
					FormatInteger(pStat.EgressPayloadTotalBytes),
					FormatInteger(pStat.EgressPayloadByteRate),
					pStat.EgressPayloadAvgPerTlp)

def CheckDevice(SdkLib,dKey):
	ChipSets={0x2300,0x3300,0x8600,0x8700,0x9700}
	PortTypes={PLX_PORT_UPSTREAM,PLX_PORT_DOWNSTREAM,PLX_PORT_ENDPOINT,PLX_PORT_LEGACY_ENDPOINT}
	rc,dp=DeviceOpen(SdkLib,dKey)
	if rc !=PLX_STATUS_OK:
		print "Could not open the device",rc
		return False
	rc,ct,rev=ChipTypeGet(SdkLib,dp)
	if rc !=PLX_STATUS_OK:
		print "Could not get Chip Type",rc
		DeviceClose(SdkLib,dp)
		return False
	PlxChip = (ct & 0xFF00)
	if PlxChip not in ChipSets:
		print "Chip",hex(PlxChip),"not supported for Performance Measurement"
		DeviceClose(SdkLib,dp)
		return False
	rc,pp=GetPortProperties(SdkLib,dp)
	if pp.PortType not in PortTypes:
		print "Unsupported Port Type",pp
		DeviceClose(SdkLib,dp)
		return False
#Avoid MIRA annd NT_VIRTUAL checks for now. TODO
	rc,driverprop = DriverProperties(SdkLib,dp)
	if driverprop.bIsServiceDriver == False:
		print "This is not a Service driver"
		DeviceClose(SdkLib,dp)
		return False
	DeviceClose(SdkLib,dp)
	return True

def usage(progname):
	print "progname -s [<bus>:<slot>.<func>] -r "
	print "use -r to print raw data"
	return


def parseopts():
	global raw
	if len(sys.argv) <2:
		print "Invalid Arguments"
		usage(sys.argv[0])
		sys.exit(2)
	try:
		opts,args =getopt.getopt(sys.argv[1:],"hrs:",['--help','--raw','--dev='])
	except getopt.GetoptError,err:
			usage(sys.argv[0])
			sys.exit(2)
	if len(opts) == 0:
		usage(sys.argv[0])
		sys.exit(2)
	for o, a in opts:
		if o in ("-h","--help"):
			usage(sys.argv[0])
			sys.exit(0)
		elif o in("-r","--raw"):
			raw=1
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

def PerformTest(SdkLib,dp):
	global raw
	rc,PerfProp = PerformanceInitializeProperties(SdkLib,dp)
	if rc !=PLX_STATUS_OK:
		print "Cannot Iniialize performance",rc,
		return
	# Start the performance monitors
	PerformanceMonitorControl(SdkLib,dp,PLX_PERF_CMD_START)
	PerformanceResetCounters(SdkLib,dp,PerfProp,1)
	pTime=int(time.time()*1000)   #Crude attempt to get a millisecond resolution
	
	while True:
		time.sleep(1)	#The C code sleeps in milliseconds
		PerformanceGetCounters(SdkLib,dp,PerfProp,1)
		cTime=int(time.time()*1000)   #Crude attempt to get a millisecond resolution
		ElapsedTime=cTime-pTime
		pTime=cTime
		pStat=PerformanceCalcStatistics(SdkLib,PerfProp,ElapsedTime)
		if raw ==0:
			DisplayStat(PerfProp,pStat)
		else:
			DisplayRawData(PerfProp)
def main():
	global SdkLib,dp
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
	if CheckDevice(SdkLib,pkey) == False:
		print "Device not supported for Performance Management",bus,dev,fun
		sys.exit(0)
	rc,dp=DeviceOpen(SdkLib,pkey)
	if rc !=PLX_STATUS_OK:
		print "Open Failed",rc
		sys.exit(0)
	signal.signal(signal.SIGINT,Cleanup)
	PerformTest(SdkLib,dp)
	DeviceClose(SdkLib,dp)

if __name__ == "__main__":
    main()
