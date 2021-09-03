#-----------------------------------------------------------------------------
#
#      File         :  Makefile
#      Abstract     :  The makefile for building the API library
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
# DEBUG       = Add/remove the comment symbol(#) to disable/enable debugging.
#=============================================================================
ImageName   = PlxApi$(DBG)
TGT_TYPE    = Library
#DEBUG       = 1


#=============================================================================
# Put all object files here
#=============================================================================
PLX_OBJECTS = \
	$(OBJ_DIR)/Aardvark.o    \
	$(OBJ_DIR)/Eep_8000.o    \
	$(OBJ_DIR)/PlxApi.o      \
	$(OBJ_DIR)/PlxApiI2cAa.o


#=============================================================================
# Include shared PLX makefile
#=============================================================================
include $(PLX_SDK_DIR)/Makefiles/PlxMake.def
