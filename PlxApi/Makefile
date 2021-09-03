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
# Set default SDK path if not set
# Note: Since shared between projects, account for SDK & System SW variables
#=============================================================================
ifndef PLX_SDK_DIR
    PLX_SDK_DIR := $(shell cd ..;pwd)
endif

ifndef PLX_SYS_DIR
    PLX_SYS_DIR := $(shell cd ..;pwd)
endif

# Decide which directory to use
PLX_DIR = $(shell cd ..;pwd)
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
