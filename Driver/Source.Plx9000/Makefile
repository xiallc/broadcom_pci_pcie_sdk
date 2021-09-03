#-----------------------------------------------------------------------------
#
#      File         :  Makefile
#      Abstract     :  Driver-specific makefile entries
#
#-----------------------------------------------------------------------------


#=============================================================================
# Set C_SRC to contain the list of files to compile
#=============================================================================
C_SRC = \
    ApiFunc.c       \
    Dispatch.c      \
    Driver.c        \
    Eep_9000.c      \
    ModuleVersion.c \
    PciFunc.c       \
    SuppFunc.c      \


#=============================================================================
# Set C_SRC_CHIP to contain the list of chip-specific files to compile
#
# 9000-series is special case since multiple chips share same source but also
# have chip-specific directories contain source files.
#=============================================================================
C_SRC_CHIP = \
    PlxChipApi.c    \
    PlxChipFn.c     \
    PlxInterrupt.c
