#!/usr/bin/python

import sys
sys.path.insert(0,"../code")
from PlxSdk import *
from const import *
from misc import *

SdkLib = LoadLibrary()
if SdkLib == None:
	print "Library Not loaded"
	sys.exit(0)
def PrintBuffer(Buffer):
#mydblPtr = cast(myVoidVal, POINTER(c_double))
	myptr=cast(Buffer,POINTER(c_ubyte))
	if Buffer == None:
		print "Buffer is Null"
		return
	l=16
	rsize=16
	for i in range(16):
		#start=(i*rsize)+0x800000
		start=(i*rsize)
		end=start+rsize
		print "{0:02x}:".format(start),' '.join(chr(x).encode('hex') for x in Buffer[start:end])
	

# Version
rc,M,m,r = getVersion(SdkLib)
print rc,M,m,r

#find device by index
for i in range(256):
	rc,dev= FindDeviceByIndex(SdkLib,i)
	if rc == PLX_STATUS_OK:	
		print "{0:04x} {1:04x} {2:02x} {3:02x} {4:02x}".format(dev.get_VendorId(),dev.get_DeviceId(),dev.get_bus(),dev.get_slot(),dev.get_function())

#find device by param
pkey=PLX_DEVICE_KEY()

#find the first Intel device
pkey.set_VendorId(0x8086)
rc,idx=FindDeviceByDevice(SdkLib,pkey)
if rc == PLX_STATUS_OK:	
	print "PLX: {0:04x} {1:04x} {2:02x} {3:02x} {4:02x}".format(pkey.get_VendorId(),pkey.get_DeviceId(),pkey.get_bus(),pkey.get_slot(),pkey.get_function())

#find using bdf
pkey=PLX_DEVICE_KEY()
pkey.set_bus(0x2)
pkey.set_slot(0x0)
pkey.set_function(0x0)
rc,idx=FindDeviceByDevice(SdkLib,pkey)
if rc != PLX_STATUS_OK:	
	print "Could not find the device",rc
	sys.exit(0)

print "BDF: {0:04x} {1:04x} {2:02x} {3:02x} {4:02x}".format(pkey.get_VendorId(),pkey.get_DeviceId(),pkey.get_bus(),pkey.get_slot(),pkey.get_function())


#Open
rc,dp=DeviceOpen(SdkLib,pkey)
if rc !=PLX_STATUS_OK:
	print "Open Failed"
	sys.exit(0)
else:
	print "Open Succeeded"

#Get Port Properties
rc,pp=GetPortProperties(SdkLib,dp)
if rc ==PLX_STATUS_OK:
	print "PN={0:02x} LW={1:02x} MLW={2:02x} LS={3:02x} MLS={4:02x} MRR={5:02x} MPS={6:02x} MPSS={7:02x} PCIe={8:02x}".format(pp.PortType,pp.PortNumber,pp.LinkWidth,pp.MaxLinkWidth,pp.LinkSpeed,pp.MaxLinkSpeed,pp.MaxReadReqSize,pp.MaxReadReqSize,pp.MaxPayloadSize,pp.MaxPayloadSupported,pp.bNonPcieDevice)
else:
	print "Failed to get Port Properties",rc

#Get ChipType
rc,ct,rev=ChipTypeGet(SdkLib,dp)
if rc ==PLX_STATUS_OK:
	print "ChipType=",hex(ct),"Revision=",hex(rev)
else:
	print "Failed to get Chip Properties"

#Set Chip Type
# Not Tested

#Common Buffer Properties
#rc,pme=CommonBufferProperties(SdkLib,dp)
#if rc == PLX_STATUS_OK:
	#print "Common Buffer Props: {0:08x} {1:08x} {2:08x} {3:d}".format(pme.UserAddr,pme.PhysicalAddr,pme.CpuPhysical,pme.Size)
#else:
	#print "Common Buffer Properties Failed",rc

#Map
rc,bufMap = CommonBufferMap(SdkLib,dp)
if rc == PLX_STATUS_OK:
	print "Common Buffer Map Success"
else:
	print "Common Buffer Map Fail",rc

#UnMap
if bufMap != None:
	rc = CommonBufferUnmap(SdkLib,dp,bufMap)

#DeviceReset
	print "Not Testing Device Reset"
	#rc=DevicevReset(SdkLib,dp)

#Driver Properties
rc,drProp  = DriverProperties(SdkLib,dp)
if rc == PLX_STATUS_OK:
	print "{0:04x} {1:16s} {2:32s}".format(drProp.Version,drProp.Name,drProp.FullName)


#DriverScheduleRescan
	print "Not Testing DriverScheduleRescan"
	#rc=DriverScheduleRescan(SdkLib,dp)
#Driver Version
rc,M,m,r = DriverVersion(SdkLib,dp)
print rc,M,m,r

#Test Eeprom present
rc,eep=EepromPresent(SdkLib,dp)
print rc,eep

#Test Eeprom Probe
rc,eep=EepromProbe(SdkLib,dp)
print rc,eep
rc,crcstat,crcval=EepromCrcGet(SdkLib,dp)
print rc,hex(crcval),hex(crcstat)
	
for i in range(6):
	rc,bi=PciBarProperties(SdkLib,dp,i)
	print "{0:<3d}  0x{1:^08X} 0x{2:^08X} 0x{3:^06x} 0x{4:^04X}".format(i,bi.BarValue,bi.Physical,bi.Size,bi.Flags)
	
rc,Buffer=PciBarMap(SdkLib,dp,0);
if rc !=PLX_STATUS_OK:
	print "Cannot print Bar",rc
else:
	PrintBuffer(Buffer)


#Close

rc = DeviceClose(SdkLib,dp)
if rc !=PLX_STATUS_OK:
	print "Close Failed"
else:
	print "Close Succeeded"
