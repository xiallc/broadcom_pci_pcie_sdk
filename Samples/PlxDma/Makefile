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
ImageName   = PlxDma$(DBG)
TGT_TYPE    = App
#PLX_DEBUG   = 1


#=============================================================================
# Additional source files. Any .C files in source folder are auto-added.
#=============================================================================

# Additional shared files
C_SRC += ConsFunc.c PlxInit.c


#=============================================================================
# Set default SDK path if not set
#=============================================================================
ifndef PLX_SDK_DIR
    PLX_SDK_DIR := $(shell cd ../..;pwd)
endif


#=============================================================================
# Include shared PLX makefile
#=============================================================================
include $(PLX_SDK_DIR)/Makefiles/PlxMake.def
