#!/usr/bin/python
import sys
import os
import sys
import getopt
import time
import datetime
sys.path.insert(0,"../code")
from PlxSdk import *
from const import *
from misc import *

details=0
def getPortType(PortType):
    if PortType ==PLX_PORT_UNKNOWN:
        return "Unknown?"
    elif PortType == PLX_PORT_ENDPOINT:
        return  "Endpoint or NT port"
    elif PortType == PLX_PORT_UPSTREAM:
        return "Upstream"
    elif PortType == PLX_PORT_DOWNSTREAM:
        return "Downstream"
    elif PortType == PLX_PORT_LEGACY_ENDPOINT:
        return "Endpoint"
    elif PortType == PLX_PORT_ROOT_PORT:
        return "Root Port"
    elif PortType == PLX_PORT_PCIE_TO_PCI_BRIDGE:
        return "PCIe-to-PCI Bridge"
    elif PortType == PLX_PORT_PCI_TO_PCIE_BRIDGE:
        return "PCI-to-PCIe Bridge"
    elif PortType == PLX_PORT_ROOT_ENDPOINT:
        return "Root Complex Endpoint"
    elif PortType == PLX_PORT_ROOT_EVENT_COLL:
        return "Root Complex Endpoint"
    else:
        return "N/A"



def main():
	SdkLib = LoadLibrary()
	if SdkLib == None:
		print "Library Not loaded"
		sys.exit(0)
	#find device by index
	if details == 0:
		print "{5:3s} {0:4s} {1:4s} {2:2s} {3:2s} {4:2s}".format(" VN "," DV ","BS","ST","FN","IDX")
	else:
		print "{5:3s} {0:4s} {1:4s} {2:2s} {3:2s} {4:2s} {6:2s} {7:2s} {8:3s} {9:10s}".format(" VN "," DV ","BS","ST","FN","IDX","PN","LW","  LS","Port Type")
		
	for i in range(256):
		rc,dev= FindDeviceByIndex(SdkLib,i)
		if rc != PLX_STATUS_OK:	
			continue
		if details ==0:
			print "{5:3d} {0:04x} {1:04x} {2:02x} {3:02x} {4:02x}".format(dev.get_VendorId(),dev.get_DeviceId(),dev.get_bus(),dev.get_slot(),dev.get_function(),i)
			continue
		rc,dp=DeviceOpen(SdkLib,dev)
		if rc !=PLX_STATUS_OK:
			print "Open Failed",rc
			continue
		rc,pp = GetPortProperties(SdkLib,dp)
		if rc != PLX_STATUS_OK:	
			continue
		print "{0:3d} {1:04x} {2:04x} {3:02x} {4:02x} {5:02x} {7:02x} x{8:02d} {9:3d} {6:10s}".format(i,dev.VendorId, dev.DeviceId,dev.bus,dev.slot,dev.function,getPortType(pp.PortType),pp.PortNumber,pp.LinkWidth,pp.LinkSpeed)

		DeviceClose(SdkLib,dp)
				

def usage(progname):
	print "Usage: ",progname,"-a"
	return


if __name__ == "__main__":
	print "Length = ",len(sys.argv)
	details=0
	if len(sys.argv) >=2:
		if sys.argv[1] == '-a':
			details =1
		else:
			usage(sys.argv[0])
			sys.exit(0)
	print "Details = ",details
	main()
