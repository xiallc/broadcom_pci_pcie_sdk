from const import *
from misc import *
from PlxSdk import *

#ATLAS_OFFSET=0x800000
ATLAS_OFFSET=0

class BarSpace:
	SdkLib=None
	devObject = None
	barid = -1
	barvalue=0
	barphysicaladdress=0
	barmappedaddress=0
	barsize=0
	barflags=0
	Buffer = None
	
	def initBarProperties(self,SdkLib,devObject,barid):
		self.SdkLib=SdkLib
		self.devObject = devObject
		self.barid=barid

		rc,bp = PciBarProperties(SdkLib,devObject,barid)
		if rc != PLX_STATUS_OK:
			return -1

		self.barvalue=bp.BarValue
		self.barphysicaladdress = bp.Physical
		self.barsize= bp.Size
		self.barflags=bp.Flags
		
		rc,self.Buffer=PciBarMap(SdkLib,devObject,barid);
		return rc


	def delBarProperties(self):
		if  self.Buffer == None:
			return
		rc = PciBarUnmap(self.SdkLib, self.devObject, self.Buffer)
		return
		
	def getBarValue(self):
		return self.barvalue
	def getBarPhysical(self):
		return self.barphysicaladdress
	def getBarSize(self):
		return self.barsize
	def getBarFlags(self):
		return self.barflags
		
	def MemoryRead8(self,offset):
		offset+=ATLAS_OFFSET
		B8=cast(self.Buffer,POINTER(c_ubyte))
		return B8[offset]
	def MemoryRead16(self,offset):
		offset+=ATLAS_OFFSET
		B16=cast(self.Buffer,POINTER(c_ushort))
		return B16[offset/2]
	def MemoryRead32(self,offset):
		offset+=ATLAS_OFFSET
		B32=cast(self.Buffer,POINTER(c_uint))
		return B32[offset/4]
		

	def MemoryWrite8(self,offset,value):
		offset+= ATLAS_OFFSET
		B8=cast(self.Buffer,POINTER(c_ubyte))
		B8[offset]=(value&0xFF)
		return

	def MemoryWrite16(self,offset,value):
		offset+=ATLAS_OFFSET
		B16=cast(self.Buffer,POINTER(c_ushort))
		B16[offset/2]=(value&0xFFFF)
		return
	def MemoryWrite32(self,offset,value):
		offset+=ATLAS_OFFSET
		B32=cast(self.Buffer,POINTER(c_uint))
		B32[offset/4]=(value&0xFFFFFFFF)
	
	def __del__(self):
		self.barid = -1
		self.barvalue=0
		self.barphysicaladdress=0
		self.barmappedaddress=0
		self.barsize=0
		self.barflags=0
		self.Buffer=None