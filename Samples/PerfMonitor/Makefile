#-----------------------------------------------------------------------------
#
#      File         :  Makefile
#      Abstract     :  The makefile for building an Application
#      Last Revision:  02-01-07
#      Usage        :  To Build Target:
#                          make
#
#                      To Cleanup Intermdiate files only:
#                          make clean
#
#                      To Cleanup All files:
#                          make cleanall
#
#-----------------------------------------------------------------------------


#=============================================================================
# Modify the following lines as needed:
#
# ImageName   = The final image name
# TGT_TYPE    = Type of Target image [App | Library | Driver]
# PLX_DEBUG   = Add/remove the comment symbol(#) to disable/enable debugging
#=============================================================================
ImageName   = PerfMon$(DBG)
TGT_TYPE    = App
#PLX_DEBUG   = 1


#=============================================================================
# Put all object files here
#=============================================================================
PLX_OBJECTS = \
	$(OBJ_DIR)/PerfMonitor.o

# Additional PLX files with shared functions
PLX_OBJECTS += $(OBJ_DIR)/ConsFunc.o


#=============================================================================
# Include shared PLX makefile
#=============================================================================
include $(PLX_SDK_DIR)/Makefiles/PlxMake.def
