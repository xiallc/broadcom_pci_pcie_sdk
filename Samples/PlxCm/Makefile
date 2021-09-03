#-----------------------------------------------------------------------------
#
#      File         :  Makefile
#      Abstract     :  The makefile for building an Application
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
ImageName = PlxCm$(DBG)
TGT_TYPE  = App
#PLX_DEBUG = 1


#=============================================================================
# Additional source files. Any .C files in source folder are auto-added.
#=============================================================================

# Additional shared files
C_SRC += ConsFunc.c PlxLists.c


#=============================================================================
# Set default SDK path if not set
# Note: Since shared between projects, account for SDK & System SW variables
#=============================================================================
ifndef PLX_SDK_DIR
    PLX_SDK_DIR := $(shell cd ../..;pwd)
endif

ifndef PLX_SYS_DIR
    PLX_SYS_DIR := $(shell cd ../..;pwd)
endif

# Decide which directory to use
PLX_DIR = $(shell cd ../..;pwd)
ifeq ( $PLX_DIR, $PLX_SDK_DIR )
    PLX_DIR = $PLX_SDK_DIR
endif
ifeq ( $PLX_DIR, $PLX_SYS_DIR )
    PLX_DIR = $PLX_SYS_DIR
endif


#=============================================================================
# Include shared PLX makefile
#=============================================================================
include $(PLX_DIR)/Makefiles/PlxMake.def
