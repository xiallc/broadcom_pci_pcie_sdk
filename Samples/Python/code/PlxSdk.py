from const import *
from misc import *


#Structures
class PLX_DEVICE_KEY(Structure):
	_fields_ = [ ( "IsValidTag",c_int),
                 ( "domain",c_ubyte),
                 ( "bus",c_ubyte),
                 ( "slot",c_ubyte),
                 ( "function",c_ubyte),
                 ( "VendorId",c_ushort),
                 ( "DeviceId",c_ushort),
                 ( "SubVendorId",c_ushort),
                 ( "SubDeviceId",c_ushort),
                 ( "Revision",c_ubyte),
                 ( "Padding",c_ubyte),
                 ( "PlxChip",c_ushort),
                 ( "PlxRevision",c_ubyte),
                 ( "PlxFamily",c_ubyte),
                 ( "ApiIndex",c_ubyte),
                 ( "Padding1",c_ubyte),
                 ( "DeviceNumber",c_ushort),
                 ( "ApiMode",c_ubyte),
                 ( "PlxPort",c_ubyte),
                 ( "PlxPortType",c_ubyte),
                 ( "NTPortNum",c_ubyte),
                 ( "DeviceMode",c_ubyte),
                 ( "ApiInternal1",c_uint),
                 ( "ApiInternal2",c_uint) ]
	def __init__(self):
		self.IsValidTag=-1
		self.domain=-1
		self.bus=-1
		self.slot=-1
		self.function=-1
		self.VendorId=-1
		self.DeviceId=-1
		self.SubVendorId=-1
		self.SubDeviceId=-1
		self.Revision=-1
		self.PlxChip=-1
		self.PlxRevision=-1
		self.PlxFamily=-1
		self.ApiIndex=-1
		self.DeviceNumber=-1
		self.ApiMode=-1
		self.PlxPort=-1
		self.PlxPortType=-1
		self.NTPortNum=-1
		self.DeviceMode=-1
		self.ApiInternal1=-1
		self.ApiInternal2=-1
		self.Padding = -1
		self.Padding1= -1
#Set functions
	def set_bus(self,b):
		self.bus=b
		return
	def set_slot(self,d):
		self.slot=d
		return
	def set_function(self,f):
		self.function=f
		return
	def set_VendorId(self,V):
		self.VendorId=V;
		return
	def set_DeviceId(self,D):
		self.VendorId=D;
		return

#Get Functions
	def get_bus(self):
		return self.bus
	def get_slot(self):
		return self.slot
	def get_function(self):
		return self.function
	def get_VendorId(self):
		return self.VendorId;
	def get_DeviceId(self):
		return self.DeviceId
		

class PLX_PORT_PROPERTIES(Structure):
	_fields_ = [ ( "PortType",c_ubyte),
                 ( "PortNumber",c_ubyte),
                 ( "LinkWidth", c_ubyte),
                 ( "MaxLinkWidth",c_ubyte),
                 ( "LinkSpeed",c_ubyte),
                 ( "MaxLinkSpeed",c_ubyte),
                 ( "MaxReadReqSize",c_ushort),
                 ( "MaxPayloadSize",c_ushort),
                 ( "MaxPayloadSupported",c_ushort),
                 ( "bNonPcieDevice",c_ubyte)]
	def __init__(self):
			self.PortType = 0
			self.PortNumber =0
			self.LinkWidth =0
			self.MaxLinkWidth =0
			self.LinkSpeed =0
			self.MaxLinkSpeed = 0
			self.MaxReadReqSize =0
			self.MaxPayloadSize =0
			self.MaxPayloadSupported =0
			self.bNonPcieDevice =0

class PLX_PHYSICAL_MEM(Structure):
	_fields_ = [	("UserAddr",c_ulonglong),
					("PhysicalAddr",c_ulonglong),
					("CpuPhysical",c_ulonglong),
					("Size",c_uint) ]


class PLX_DRIVER_PROP(Structure):
    _fields_ = [ ( "Version",c_uint,32),
                 ( "Name",c_char*16),
                 ( "FullName",c_char*255),
                 ( "bIsServiceDriver",c_ubyte),
                 ( "AcpiPcieEcam",c_ulonglong),
                 ( "Reserved",c_ubyte*40) ]

class PLX_PCI_BAR_PROP(Structure):
    _fields_ = [ ("BarValue",c_uint64),
                 ("Physical",c_uint64),
                 ("Size",c_uint64),
                 ("Flags",c_uint32) ]
class PLX_VERSION(Structure):
    _fields_ = [ ("ApiMode",c_uint),
				 ("ApiLibrary",c_ushort),
				 ("Software",c_ushort),
				 ("Firmware",c_ushort),
				 ("Hardware",c_ushort),
				 ("SwReqByFw",c_ushort),
				 ("FwReqBySw",c_ushort),
				 ("ApiReqBySw",c_ushort),
				 ("Features",c_uint)]
class PLX_PERF_PROP(Structure):
    _fields_ = [ ("IsValidTag",c_uint),
				 ("PlxFamily",c_ubyte),
				 ("PortNumber",c_ubyte),
				 ("LinkWidth",c_ubyte),
				 ("LinkSpeed",c_ubyte),
				 ("Station", c_ubyte),
				 ("StationPort",c_ubyte),

				 ("IngressPostedHeader",c_uint),
				 ("IngressPostedDW",c_uint),
				 ("IngressNonpostedDW",c_uint),
				 ("IngressCplHeader",c_uint),
				 ("IngressCplDW",c_uint),
				 ("IngressDllp",c_uint),
				 ("IngressPhy",c_uint),

				 ("EgressPostedHeader",c_uint),
				 ("EgressPostedDW",c_uint),
				 ("EgressNonpostedDW",c_uint),
				 ("EgressCplHeader",c_uint),
				 ("EgressCplDW",c_uint),
				 ("EgressDllp",c_uint),
				 ("EgressPhy",c_uint),

				 ("Prev_IngressPostedHeader",c_uint),
				 ("Prev_IngressPostedDW",c_uint),
				 ("Prev_IngressNonpostedDW",c_uint),
				 ("Prev_IngressCplHeader",c_uint),
				 ("Prev_IngressCplDW",c_uint),
				 ("Prev_IngressDllp",c_uint),

				 ("Prev_IngressPhy",c_uint),
				 ("Prev_EgressPostedHeader",c_uint),
				 ("Prev_EgressPostedDW",c_uint),
				 ("Prev_EgressNonpostedDW",c_uint),
				 ("Prev_EgressCplHeader",c_uint),
				 ("Prev_EgressCplDW",c_uint),
				 ("Prev_EgressDllp",c_uint),
				 ("Prev_EgressPhy",c_uint)]

class PLX_PERF_STATS(Structure):
    _fields_ = [("IngressTotalBytes",  c_longlong ),	# Total bytes including overhead
				("IngressTotalByteRate",  c_longdouble ),	# Total byte rate
				("IngressCplAvgPerReadReq",  c_longlong ),	# Average number of completion TLPs for read requests
				("IngressCplAvgBytesPerTlp",  c_longlong ),	# Average number of bytes per completion TLPs
				("IngressPayloadReadBytes",  c_longlong ),	# Payload bytes read (Completion TLPs)
				("IngressPayloadReadBytesAvg",  c_longlong ),	# Average payload bytes for reads (Completion TLPs)
				("IngressPayloadWriteBytes",  c_longlong ),	# Payload bytes written (Posted TLPs)
				("IngressPayloadWriteBytesAvg",  c_longlong ),	# Average payload bytes for writes (Posted TLPs)
("IngressPayloadTotalBytes",  c_longlong ),	# Payload total bytes
				("IngressPayloadAvgPerTlp",  c_double ),	# Payload average size per TLP
				("IngressPayloadByteRate",  c_longdouble ),	# Payload byte rate
				("IngressLinkUtilization",  c_longdouble ),	# Total link utilization
				("EgressTotalBytes",  c_longlong ),	# Total byte including overhead
				("EgressTotalByteRate",  c_longdouble ),	# Total byte rate
				("EgressCplAvgPerReadReq",  c_longlong ),	# Average number of completion TLPs for read requests
				("EgressCplAvgBytesPerTlp",  c_longlong ),	# Average number of bytes per completion TLPs
				("EgressPayloadReadBytes",  c_longlong ),	# Payload bytes read (Completion TLPs)
				("EgressPayloadReadBytesAvg",  c_longlong ),	# Average payload bytes for reads (Completion TLPs)
				("EgressPayloadWriteBytes",  c_longlong ),	# Payload bytes written (Posted TLPs)
				("EgressPayloadWriteBytesAvg",  c_longlong ),	# Average payload bytes for writes (Posted TLPs)
				("EgressPayloadTotalBytes",  c_longlong ),	# Payload total bytes
				("EgressPayloadAvgPerTlp",  c_double ),	# Payload average size per TLP
				("EgressPayloadByteRate",  c_longdouble ),	# Payload byte rate
				("EgressLinkUtilization",  c_longdouble )]	# Total link utilization
				
def getVersion(SdkLib):
	SdkLib.PlxPci_ApiVersion.argtypes=[POINTER(c_ubyte),POINTER(c_ubyte),POINTER(c_ubyte)]
	SdkLib.PlxPci_ApiVersion.restype=c_uint
	M=c_ubyte()
	m=c_ubyte()
	r=c_ubyte()
	rc=SdkLib.PlxPci_ApiVersion(M,m,r)
	return rc,M.value,m.value,r.value

def FindDeviceByIndex(SdkLib,i):
	SdkLib.PlxPci_DeviceFind.argtypes=[POINTER(PLX_DEVICE_KEY),c_uint]
	SdkLib.PlxPci_DeviceFind.restype=c_uint
	dev = PLX_DEVICE_KEY()
	#rc=SdkLib.PlxPci_DeviceFind(byref(dev),i)
	rc=SdkLib.PlxPci_DeviceFind(dev,i)
	return rc,dev

def FindDeviceByDevice(SdkLib,dev):
	SdkLib.PlxPci_DeviceFind.argtypes=[POINTER(PLX_DEVICE_KEY),c_uint]
	SdkLib.PlxPci_DeviceFind.restype=c_uint
	for i in range(256):
		rc=SdkLib.PlxPci_DeviceFind(dev,i)
		if rc == PLX_STATUS_OK:
			return rc,i
	return PLX_STATUS_RSVD_LAST_ERROR,-1
def DeviceOpen(SdkLib,dev):
	SdkLib.PlxPci_DeviceOpen.argtypes=[POINTER(PLX_DEVICE_KEY),c_char_p]
	SdkLib.PlxPci_DeviceOpen.restype=c_uint
	devObject = create_string_buffer(512)
	rc = SdkLib.PlxPci_DeviceOpen(dev,devObject)
	return rc,devObject
def DeviceClose(SdkLib,devObject):
	SdkLib.PlxPci_DeviceClose.argtypes=[c_char_p]
	SdkLib.PlxPci_DeviceClose.restype=c_uint
	rc = SdkLib.PlxPci_DeviceClose(devObject)
	del devObject
	return rc
def GetPortProperties(SdkLib,devObject):
	SdkLib.PlxPci_GetPortProperties.argtypes=[c_char_p,POINTER(PLX_PORT_PROPERTIES)]
	SdkLib.PlxPci_GetPortProperties.restype=c_uint
	portprop = PLX_PORT_PROPERTIES()
	rc = SdkLib.PlxPci_GetPortProperties(devObject,portprop)
	return rc,portprop
	
def ChipTypeGet(SdkLib,devObject):
	SdkLib.PlxPci_ChipTypeGet.argtypes=[c_char_p,POINTER(c_ushort),POINTER(c_ubyte)]
	SdkLib.PlxPci_ChipTypeGet.restype=c_uint
	ChipType=c_ushort()
	Revision=c_ubyte()
	rc= SdkLib.PlxPci_ChipTypeGet(devObject,ChipType,Revision)
	return rc,ChipType.value,Revision.value
def ChipTypeSet(SdkLib,devObject,ChipType,Revision):
	SdkLib.PlxPci_ChipTypeSet.argtypes=[c_char_p,c_ushort,c_ubyte]
	SdkLib.PlxPci_ChipTypeSet.restype=c_uint
	rc= SdkLib.PlxPci_ChipTypeSet(devObject,ChipType,Revision)
	return rc
def CommonBufferProperties(SdkLib,devObject):
	SdkLib.PlxPci_CommonBufferProperties.argtypes=[c_char_p,POINTER(PLX_PHYSICAL_MEM)]
	SdkLib.PlxPci_CommonBufferProperties.restype=c_uint
	pme = PLX_PHYSICAL_MEM()
	rc=SdkLib.PlxPci_CommonBufferProperties(devObject,pme)
	return rc,pme

def CommonBufferMap(SdkLib,devObject):
	SdkLib.PlxPci_CommonBufferMap.argtypes=[c_char_p,ctypes.POINTER(c_void_p)]
	SdkLib.PlxPci_CommonBufferMap.restype=c_uint
	bufMap=ctypes.c_void_p(0)
	rc=SdkLib.PlxPci_CommonBufferMap(devObject,ctypes.byref(bufMap))
	print CommonBufferMap.__name__," is a suspect implementation"
	return rc,bufMap
def CommonBufferUnmap(SdkLib,devObject,bufMap):
	SdkLib.PlxPci_CommonBufferUnmap.argtypes=[c_char_p,ctypes.POINTER(c_void_p)]
	SdkLib.PlxPci_CommonBufferUnmap.restype=c_uint
	rc=SdkLib.PlxPci_CommonBufferUnmap(devObject,byref(bufMap))
	print CommonBufferUnmap.__name__,"is a suspect implementation"
	return rc

def DeviceReset(SdkLib,devObject):
	SdkLib.PlxPci_DeviceReset.argtypes=[c_char_p]
	SdkLib.PlxPci_DeviceReset.restype=c_uint
	rc = SdkLib.PlxPci_DeviceReset(devObject)
	return rc
def DriverProperties(SdkLib,devObject):
	SdkLib.PlxPci_DriverProperties.argtypes = [c_char_p,POINTER(PLX_DRIVER_PROP)]
	SdkLib.PlxPci_DriverProperties.restype = c_uint
	dp = PLX_DRIVER_PROP()
	rc = SdkLib.PlxPci_DriverProperties(devObject,byref(dp))
	return rc,dp
def DriverScheduleRescan(SdkLib,devObject):
	SdkLib.PlxPci_DriverScheduleRescan.argtypes=[c_char_p]
	SdkLib.PlxPci_DriverScheduleRescan.restype=c_uint
	rc = SdkLib.PlxPci_DriverScheduleRescan(devObject)
	return rc
def DriverVersion(SdkLib,devObject):
	SdkLib.PlxPci_DriverVersion.argtypes=[c_char_p,POINTER(c_ubyte),POINTER(c_ubyte),POINTER(c_ubyte)]
	SdkLib.PlxPci_DriverVersion.restype=c_uint
	M=c_ubyte()
	m=c_ubyte()
	r=c_ubyte()
	rc = SdkLib.PlxPci_DriverVersion(devObject,M,m,r)
	return rc,M.value,m.value,r.value
def EepromPresent(SdkLib,devObject):
	SdkLib.PlxPci_EepromPresent.argtypes=[c_char_p,POINTER(c_uint)]
	SdkLib.PlxPci_EepromPresent.restype=c_uint
	rc=c_uint()
	res = SdkLib.PlxPci_EepromPresent(devObject,rc)
	return rc.value,res
def EepromProbe(SdkLib,devObject):
	SdkLib.PlxPci_EepromProbe.argtypes=[c_char_p,POINTER(c_uint)]
	SdkLib.PlxPci_EepromProbe.restype=c_uint
	rc=c_uint()
	res = SdkLib.PlxPci_EepromProbe(devObject,rc)
	return rc.value,res
def EepromCrcGet(SdkLib,devObject):
	SdkLib.PlxPci_EepromCrcGet.argtypes=[c_char_p,POINTER(c_uint),POINTER(c_uint)]
	SdkLib.PlxPci_EepromCrcGet.restype=c_uint
	crcstat=c_uint()
	crcval=c_uint()
	rc = SdkLib.PlxPci_EepromCrcGet(devObject,crcval,crcstat)
	return rc,crcval.value,crcstat.value
	
def EepromCrcUpdate(SdkLib,devObject,update):
	SdkLib.PlxPci_EepromCrcUpdate.argtypes=[c_char_p,POINTER(c_uint),c_uint]
	SdkLib.PlxPci_EepromCrcUpdate.restype=c_uint
	newcrc = c_uint()
	rc = SdkLib.PlxPci_EepromCrcUpdate(devObject,newcrc,update)
	return rc,newcrc.value
def EepromGetAddressWidth(SdkLib,devObject):
	SdkLib.PlxPci_EepromGetAddressWidth.argtypes=[c_char_p,POINTER(c_ubyte)]
	SdkLib.PlxPci_EepromGetAddressWidth.restypes=c_uint
	
	width=c_ubyte()
	rc=SdkLib.PlxPci_EepromGetAddressWidth(devObject,width)
	return rc,width.value
def EepromSetAddressWidth(SdkLib,devObject):
	SdkLib.PlxPci_EepromSetAddressWidth.argtypes=[c_char_p,c_ubyte]
	SdkLib.PlxPci_EepromSetAddressWidth.restypes=c_uint
	rc=SdkLib.PlxPci_EepromGetAddressWidth(devObject,width)
	return rc
def EepromReadByOffset(SdkLib,devObject,offset):
	SdkLib.PlxPci_EepromReadByOffset.argtypes=[c_char_p,c_uint,POINTER(c_uint)]
	SdkLib.PlxPci_EepromReadByOffset.restype=c_uint
	Value=c_uint()
	rc=SdkLib.PlxPci_EepromReadByOffset(devObject,offset,Value)
	return rc,Value.value
def EepromWriteByOffset(SdkLib,devObject,offset,Value):
	SdkLib.PlxPci_EepromWriteByOffset.argtypes=[c_char_p,c_uint,c_uint]
	SdkLib.PlxPci_EepromWriteByOffset.restype=c_uint
	rc=SdkLib.PlxPci_EepromWriteByOffset(devObject,offset,Value)
	return rc
def EepromReadByOffset_16(SdkLib,devObject,offset):
	SdkLib.PlxPci_EepromReadByOffset_16.argtypes=[c_char_p,c_uint,POINTER(c_ushort)]
	SdkLib.PlxPci_EepromReadByOffset_16.restype=c_uint
	Value=c_ushort()
	rc=SdkLib.PlxPci_EepromReadByOffset_16(devObject,offset,Value)
	return rc,Value.value
def EepromWriteByOffset_16(SdkLib,devObject,offset,Value):
	SdkLib.PlxPci_EepromWriteByOffset_16.argtypes=[c_char_p,c_uint,c_ushort]
	SdkLib.PlxPci_EepromWriteByOffset_16.restype=c_uint
	rc=SdkLib.PlxPci_EepromWriteByOffset_16(devObject,offset,Value)
	return rc
def I2cGetPorts(SdkLib,ApiMode):
	SdkLib.PlxPci_I2cGetPorts.argtypes=[c_uint,POINTER(c_uint)]
	SdkLib.PlxPci_I2cGetPorts.restype=c_uint
	n_ports=c_uint()
	rc = SdkLib.PlxPci_I2cGetPorts(ApiMode,n_ports)
	return rc,n_ports
def I2cVersion(SdkLib,I2cPort):
	SdkLib.PlxPci_I2cVersion.argtypes=[c_ushort,POINTER(PLX_VERSION)]
	SdkLib.PlxPci_I2cVersion.restype=u_int
	plxv=PLX_VERSION()
	rc=SdkLib.PlxPci_I2cVersion(I2cPort,plxv)
	return rc,plxv
def IoPortRead(SdkLib,devObject,port,AccessType):
	SdkLib.PlxPci_IoPortRead.argtypes=[c_char_p,c_longlong,POINTER(c_void_p),c_uint,c_uint]
	SdkLib.PlxPci_IoPortRead.restype=c_uint
	Buffer=c_void_p()
	rc=SdkLib.PlxPci_IoPortRead(devObject,port,Buffer,ByteCount,AccessTYpe)
	return rc,Buffer,ByteCount
def IoPortWrite(SdkLib,devObject,port,Buffer,ByteCount,AccessType):
	SdkLib.PlxPci_IoPortWrite.argtypes=[c_char_p,c_longlong,POINTER(c_void_p),c_uint,c_uint]
	SdkLib.PlxPci_IoPortWrite.restype=c_uint
	Buffer=c_void_p()
	rc=SdkLib.PlxPci_IoPortWrite(devObject,port,Buffer,ByteCount,AccessTYpe)
	return rc
def InterruptEnable(SdkLib,devObject):
	print "Not Implemented"
	return PLX_STATUS_FAILED
def InterruptDisable(SdkLib,devObject):
	print "Not Implemented"
	return PLX_STATUS_FAILED
def BarSpaceRead(SdkLib,devObject,BarIndex,offset,Buffer,ByteCount,AccessType,bOffsetAsLocalAddr):
	SdkLib.PlxPci_PciBarSpaceRead.argtypes=[c_char_p,c_ubyte,c_uint,POINTER(c_void_p),c_uint,c_uint]
	SdkLib.PlxPci_PciBarSpaceRead.restype=c_uint
	rc = SdkLib.PlxPci_PciBarSpaceRead(devObject,BarIndex,offset,Buffer,ByteCount,AccessType,bOffsetAsLocalAddr)
	return rc
def BarSpaceWrite(SdkLib,devObject,BarIndex,offset,Buffer,ByteCount,AccessType,bOffsetAsLocalAddr):
	SdkLib.PlxPci_PciBarSpaceWrite.argtypes=[c_char_p,c_ubyte,c_uint,POINTER(c_void_p),c_uint,c_uint]
	SdkLib.PlxPci_PciBarSpaceWrite.restype=c_uint
	rc = SdkLib.PlxPci_PciBarSpaceWrite(devObject,BarIndex,offset,Buffer,ByteCount,AccessType,bOffsetAsLocalAddr)
	return rc
#Casting example
#mydblPtr = cast(myVoidVal, POINTER(c_double))
def PciBarMap(SdkLib,devObject,BarIndex):
	SdkLib.PlxPci_PciBarMap.argtypes=[c_char_p,c_ubyte,POINTER(POINTER(c_ubyte))]
	SdkLib.PlxPci_PciBarMap.restype=c_uint
	Buffer= POINTER(c_ubyte)()
	rc = SdkLib.PlxPci_PciBarMap(devObject,BarIndex,byref(Buffer))
	return rc,Buffer
def PciBarUnmap(SdkLib,devObject,Buffer):
	SdkLib.PlxPci_PciBarUnmap.argtypes=[c_char_p,POINTER(POINTER(c_ubyte))]
	SdkLib.PlxPci_PciBarUnmap.restype=c_uint
	rc = SdkLib.PlxPci_PciBarUnmap(devObject,byref(Buffer))
	return rc
def PciBarProperties(SdkLib,devObject,BarIndex):
	SdkLib.PlxPci_PciBarProperties.argtypes=[c_char_p,c_uint,POINTER(PLX_PCI_BAR_PROP)]
	SdkLib.PlxPci_PciBarProperties.restype=c_uint
	bp=PLX_PCI_BAR_PROP()
	rc = SdkLib.PlxPci_PciBarProperties(devObject,BarIndex,bp)
	return rc,bp
def PciRegisterRead(SdkLib,bus,slot,function,offset):
	SdkLib.PlxPci_PciRegisterRead.argtypes=[c_ubyte,c_ubyte,c_ubyte,c_uint,POINTER(c_uint)]
	SdkLib.PlxPci_PciRegisterRead.restype=c_uint
	rc=c_uint()
	data=SdkLib.PlxPci_PciRegisterRead(bus,slot,function,offset,rc)
	return rc,data
def PciRegisterWrite(SdkLib,bus,slot,function,offset,value):
	SdkLib.PlxPci_PciRegisterWrite.argtypes=[c_ubyte,c_ubyte,c_ubyte,c_uint,c_uint]
	SdkLib.PlxPci_PciRegisterWrite.restype=c_uint
	rc=SdkLib.PlxPci_PciRegisterRead(bus,slot,function,offset,value)
	return rc
def PciRegisterReadFast(SdkLib,devObject,offset):
	SdkLib.PlxPci_PciRegisterReadFast.argtypes=[c_char_p,c_uint,POINTER(c_uint)]	
	SdkLib.PlxPci_PciRegisterReadFast.restype=c_uint
	rc=c_uint()
	data=SdkLib.PlxPci_PciRegisterReadFast(devObject,offset,rc)
	return rc,data

def PciRegisterWriteFast(SdkLib,devObject,offset,value):
	SdkLib.PlxPci_PciRegisterWriteFast.argtypes=[c_char_p,c_uint,c_uint]
	SdkLib.PlxPci_PciRegisterWriteFast.restype=c_uint
	rc = SdkLib.PlxPci_PciRegisterWriteFast(devObject,offset,value)
	return rc
def PciRegisterRead_BypassOS(SdkLib,bus,slot,function,offset):
	SdkLib.PlxPci_PciRegisterRead_BypassOS.argtypes=[c_ubyte,c_ubyte,c_ubyte,c_uint,POINTER(c_uint)]
	SdkLib.PlxPci_PciRegisterRead_BypassOS.restype=c_uint
	rc=c_uint()
	data=SdkLib.PlxPci_PciRegisterRead_BypassOS(bus,slot,function,offset,rc)
	return rc,data
def PciRegisterWrite_BypassOS(SdkLib,bus,slot,function,offset,value):
	SdkLib.PlxPci_PciRegisterWrite_BypassOS.argtypes=[c_ubyte,c_ubyte,c_ubyte,c_uint,c_uint]
	SdkLib.PlxPci_PciRegisterWrite_BypassOS.restype=c_uint
	rc=SdkLib.PlxPci_PciRegisterWrite_BypassOS(bus,slot,function,offset,value)
	return rc
def PlxRegisterRead(SdkLib,devObject,offset):
	SdkLib.PlxPci_PlxRegisterRead.argtypes=[c_char_p,c_uint,POINTER(c_uint)]	
	SdkLib.PlxPci_PlxRegisterRead.restype=c_uint
	rc=c_uint()
	data=SdkLib.PlxPci_PlxRegisterRead(devObject,offset,rc)
	return rc,data

def PlxRegisterWrite(SdkLib,devObject,offset,value):
	SdkLib.PlxPci_PlxRegisterWrite.argtypes=[c_char_p,c_uint,c_uint]
	SdkLib.PlxPci_PlxRegisterWrite.restype=c_uint
	rc = SdkLib.PlxPci_PlxRegisterWrite(devObject,offset,value)
	return rc
def PlxMappedRegisterRead(SdkLib,devObject,offset):
	SdkLib.PlxPci_PlxMappedRegisterRead.argtypes=[c_char_p,c_uint,POINTER(c_uint)]	
	SdkLib.PlxPci_PlxMappedRegisterRead.restype=c_uint
	rc=c_uint()
	data=SdkLib.PlxPci_PlxMappedRegisterRead(devObject,offset,rc)
	return rc,data

def PlxMappedRegisterWrite(SdkLib,devObject,offset,value):
	SdkLib.PlxPci_PlxMappedRegisterWrite.argtypes=[c_char_p,c_uint,c_uint]
	SdkLib.PlxPci_PlxMappedRegisterWrite.restype=c_uint
	rc = SdkLib.PlxPci_PlxMappedRegisterWrite(devObject,offset,value)
	return rc

def PerformanceInitializeProperties(SdkLib,devObject):
	SdkLib.PlxPci_PerformanceInitializeProperties.argtypes=[c_char_p,POINTER(PLX_PERF_PROP)]
	SdkLib.PlxPci_PerformanceInitializeProperties.restype=c_uint
	pPerfProp = PLX_PERF_PROP()
	rc = SdkLib.PlxPci_PerformanceInitializeProperties(devObject,byref(pPerfProp))
	return rc,pPerfProp
def PerformanceMonitorControl(SdkLib,devObject,cmd):
	SdkLib.PlxPci_PerformanceMonitorControl.argtypes=[c_char_p,c_uint] #the second argument is actually an enum
	SdkLib.PlxPci_PerformanceMonitorControl.restype=c_uint
	rc = SdkLib.PlxPci_PerformanceMonitorControl(devObject,cmd)
	return rc
	
def PerformanceResetCounters(SdkLib,devObject,pPerfProps,num):
	SdkLib.PlxPci_PerformanceResetCounters.argtypes=[c_char_p,POINTER(PLX_PERF_PROP),c_uint]
	SdkLib.PlxPci_PerformanceResetCounters.restype=c_uint
	rc=SdkLib.PlxPci_PerformanceResetCounters(devObject,pPerfProps,num)
	return rc
def PerformanceGetCounters(SdkLib,devObject,pPerfProps,num):
	SdkLib.PlxPci_PerformanceGetCounters.argtypes=[c_char_p,POINTER(PLX_PERF_PROP),c_uint]
	SdkLib.PlxPci_PerformanceGetCounters.restype=c_uint
	rc=SdkLib.PlxPci_PerformanceGetCounters(devObject,pPerfProps,num)
	return rc
#Format a decimal Integer to make it more readable
def FormatInteger(value):
	if value < (1<<10):
		#return value,"B"
		return value
	if value < (1<<20):
		#return float(value)/(1<<10),"KB"
		return float(value)/(1<<10)
	if value < (1<<30):
		return (float)(value)/(1<<20)
	if value < (1<<40):
		return (float)(value)/(1<<30)
	else:
		return (float)(value)/(1<<40)
		
# Handle Wrap Around Issues
def GetCounterValue(current,prev):
	if current < prev:
		current+=1<<32
	return current - prev
		
def PerformanceCalcStatistics(SdkLib,pPerfProp,ElapsedTime_ms):
	pPerfStats=PLX_PERF_STATS(0)

	#Verify elapsed time and link is up
	if ElapsedTime_ms == 0 or  pPerfProp.LinkWidth == 0:
		return PLX_STATUS_INVALID_DATA,None
	#Determine theoretical max link rate for 1 second (Gen1 bps * link_width * 2^(link_speed - 1) )
	MaxLinkRate = PERF_MAX_BPS_GEN_1_0*pPerfProp.LinkWidth*pow(2,(PerfProp.Linkspeed -1))

	#print "LinkWidth = ",pPerfProp.LinkWidth,"GEN 3",PERF_MAX_BPS_GEN_3_0
	MaxLinkRate=(float)(MaxLinkRate*ElapsedTime_ms)/1000.0
	#print "MaxLinkRate =",MaxLinkRate


	Counter_PostedHeader = GetCounterValue(pPerfProp.IngressPostedHeader,pPerfProp.Prev_IngressPostedHeader)
	Counter_PostedDW = GetCounterValue(pPerfProp.IngressPostedDW,pPerfProp.Prev_IngressPostedDW)
	Counter_NonpostedDW = GetCounterValue(pPerfProp.IngressNonpostedDW,pPerfProp.Prev_IngressNonpostedDW)
	Counter_CplHeader = GetCounterValue(pPerfProp.IngressCplHeader,pPerfProp.Prev_IngressCplHeader)
	Counter_CplDW = GetCounterValue(pPerfProp.IngressCplDW,pPerfProp.Prev_IngressCplDW)
	Counter_Dllp=GetCounterValue(pPerfProp.IngressDllp,pPerfProp.Prev_IngressDllp)
	#print "Counter_PostedHeader",Counter_PostedHeader
	#print "Counter_PostedDW",Counter_PostedDW
	#print "Counter_NonpostedDW",Counter_NonpostedDW
	#print "Counter_CplHeader",Counter_CplHeader
	#print "Counter_CplDW",Counter_CplDW
	#print "Counter_Dllp",Counter_Dllp
	
	#Ingress Statistics
	
	if ( Counter_PostedHeader *PERF_TLP_DW > Counter_PostedDW):
		Counter_PostedHeader = Counter_PostedDW/(PERF_TLP_DW+1)
	
	#Posted Payload bytes ((P_DW * size(DW) - (P_TLP * size(P_TLP))
	pPerfStats.IngressPayloadWriteBytes = (Counter_PostedDW*4) - (Counter_PostedHeader *PERF_TLP_SIZE)

	# Completion Payload ((CPL_DW * size(DW) - (CPL_TLP * size(TLP))
	pPerfStats.IngressPayloadReadBytes = (Counter_CplDW*4) - (Counter_CplHeader *PERF_TLP_SIZE)

	# Total Paylod
	pPerfStats.IngressPayloadTotalBytes = pPerfStats.IngressPayloadWriteBytes+pPerfStats.IngressPayloadReadBytes

	# Average payload size (Payload / (P_TLP + CPL_TLP))
	PayLoadAvg = Counter_PostedHeader + Counter_CplHeader

	if PayLoadAvg != 0 :
		pPerfStats.IngressPayloadAvgPerTlp = float(pPerfStats.IngressPayloadTotalBytes) /float(PayLoadAvg)
	else:
		 pPerfStats.IngressPayloadAvgPerTlp =0.0
	
    # Total number of TLP data ((P_DW + NP_DW + CPL_DW) * size(DW))
	TotalBytes = (Counter_PostedDW+Counter_NonpostedDW+Counter_CplDW)*Sizeof_32 
	
	TotalBytes+=(Counter_Dllp *PERF_DLLP_SIZE)

	#TotalBytes
	pPerfStats.IngressTotalBytes = TotalBytes

	#Total Byte Rate
	pPerfStats.IngressTotalByteRate = (TotalBytes *1000)/ElapsedTime_ms

	#Payload Rate
	pPerfStats.IngressPayloadByteRate = (pPerfStats.IngressPayloadTotalBytes*1000)/ElapsedTime_ms


	#Link Utilization
	if MaxLinkRate == 0:
		pPerfStats.IngressLinkUtilization =0
	else:
		pPerfStats.IngressLinkUtilization = ( TotalBytes*100)/MaxLinkRate
	if pPerfStats.IngressLinkUtilization > 100:
		pPerfStats.IngressLinkUtilization = 100
	#
	# Calculate Egress actual counters, adjusting for counter wrapping
	#
	Counter_PostedHeader=GetCounterValue(pPerfProp.EgressPostedHeader,pPerfProp.Prev_EgressPostedHeader)
	Counter_PostedDW = GetCounterValue(pPerfProp.EgressPostedDW,pPerfProp.Prev_EgressPostedDW)
	Counter_NonpostedDW =GetCounterValue(pPerfProp.EgressNonpostedDW,pPerfProp.Prev_EgressNonpostedDW)
	Counter_CplHeader=GetCounterValue(pPerfProp.EgressCplHeader,pPerfProp.Prev_EgressCplHeader)
	Counter_CplDW=GetCounterValue(pPerfProp.EgressCplDW,pPerfProp.Prev_EgressCplDW)
	Counter_Dllp=GetCounterValue(pPerfProp.EgressDllp,pPerfProp.Prev_EgressDllp)


	# Capella-2 does not count the 2DW overhead for egress DW. 
	# The DW counts are adjusted by adding 2DW per TLP to account 
	# for the overhead.

	if pPerfProp.PlxFamily == PLX_FAMILY_CAPELLA_2:
		Counter_PostedDW += (Counter_PostedHeader *PERF_TLP_OH_DW)
		Counter_CplDW += (Counter_CplHeader*PERF_TLP_OH_DW)

		#No TLP count is provided for non-posted, unable to adjust
		# C code has the following pointless code. 
		# We will simbly ignore
		# Counter_NonpostedDW+=0

	# Now doing Egress Statistics
	if (Counter_PostedHeader *PERF_TLP_DW) > Counter_PostedDW:
		Counter_PostedHeader = Counter_PostedDW /(PERF_TLP_DW+1)

	# Posted Payload bytes ((P_DW * size(DW) - (P_TLP * size(P_TLP))
	pPerfStats.EgressPayloadWriteBytes =  \
		((Counter_PostedDW*Sizeof_32) - 
		(Counter_PostedHeader*PERF_TLP_SIZE))

	# Completion Payload ((CPL_DW * size(DW) - (CPL_TLP * size(CPL_TLP))
	pPerfStats.EgressPayloadReadBytes = \
		((Counter_CplDW *Sizeof_32) - 
		(Counter_CplHeader *PERF_TLP_SIZE))

	# Total Payload
	pPerfStats.EgressPayloadTotalBytes = \
		(pPerfStats.EgressPayloadWriteBytes+
		 pPerfStats.EgressPayloadReadBytes)

	# Average payload size (Payload / (P_TLP + CPL_TLP))
	PayloadAvg = Counter_PostedHeader+Counter_CplHeader
	
	if PayloadAvg != 0:
		pPerfStats.EgressPayloadAvgPerTlp = \
			float(pPerfStats.EgressPayloadTotalBytes)/float(PayloadAvg)
	else:
		pPerfStats.EgressPayloadAvgPerTlp = 0.0

	
    # Total number of TLP data ((P_DW + NP_DW + CPL_DW) * size(DW))
	TotalBytes = (Counter_PostedDW + Counter_NonpostedDW + Counter_CplDW)*Sizeof_32

	# Add DLLPs to total bytes
	TotalBytes += (Counter_Dllp * PERF_DLLP_SIZE)
	

	pPerfStats.EgressTotalBytes = TotalBytes
	
	#TotalByte Rate
	pPerfStats.EgressTotalByteRate = \
		float(TotalBytes*1000)/ElapsedTime_ms

	# Payload rate
	pPerfStats.EgressPayloadByteRate = \
		float(pPerfStats.EgressPayloadTotalBytes*1000)/ElapsedTime_ms
	
	if MaxLinkRate == 0:
		pPerfStats.EgressLinkUtilization=0.0
	else:
		pPerfStats.EgressLinkUtilization = \
			(float)(TotalBytes*100)/MaxLinkRate
		if pPerfStats.EgressLinkUtilization > 100.0:
			pPerfStats.EgressLinkUtilization=100.0
	return pPerfStats

def ConfigRead(SdkLib,dp,offset):
	rc,data = PciRegisterReadFast(SdkLib,dp,offset)
	return rc,data
def ConfigWrite(SdkLib,dp,offset,value):
	rc=PciRegisterWriteFast(SdkLib,dp,offset,value)
	return rc