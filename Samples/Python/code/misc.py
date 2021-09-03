import os
import sys
from ctypes import *
import ctypes

def getLibPrefix():
	if os.name == "nt":
		return ""
	try:
		sdkdir=os.environ['PLX_SDK_DIR']
	except:
		print "Fatal Error. Environment Variable PLX_SDK_DIR Not set"
		return None
	return sdkdir+"/PlxApi/Library/"
def LoadLibrary():
	LibNames=['PlxApi.so',"PlxApi720_x64.dll","PlxApi800_x64.dll","PlxApi800.dll"]
	lp = getLibPrefix()
	lt=None
	if lp == None:
		print "On Linux Systems, PLX_SDK_DIR needs to be set"
		return None
	for lib in LibNames:
		try:
			print "Trying to Load ",lp+lib
			cdll.LoadLibrary(lp+lib)
			lt=CDLL(lp+lib)
			break
		except:
			#print "Fatal Error. Cannot find", lp+lib
			continue
	if lt == None:
		print "Fatal Error: Cannot find SDK Library"
		return None

	else:
		print "Library Loaded"
		return lt

def parsedev(str):
        s1=str.split(':')
        bus=int(s1[0],16)
        dev=int(s1[1].split('.')[0],16)
        fun=int(s1[1].split('.')[1],16)
        return bus,dev,fun
