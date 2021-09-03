#-----------------------------------------------------------------------------
#
#      File         :  Makefile
#      Abstract     :  Driver-specific makefile entries
#
#-----------------------------------------------------------------------------


#=============================================================================
# Set driver name
#
# ImageName = The final image name
#=============================================================================
ImageName = Plx$(PLX_CHIP)$(DBG)



#=============================================================================
# Put all object files here
#=============================================================================

# Set source folder
SRC_DIR      = $(COMMON_DIR)
SRC_CHIP_DIR = $(PLX_CHIP_DIR)

# Files to build
PLX_OBJECTS = \
    $(SRC_DIR)/ApiFunc.o       \
    $(SRC_DIR)/Dispatch.o      \
    $(SRC_DIR)/Driver.o        \
    $(SRC_DIR)/Eep_9000.o      \
    $(SRC_DIR)/ModuleVersion.o \
    $(SRC_DIR)/PciFunc.o       \
    $(SRC_DIR)/SuppFunc.o      \

# Add chip-specific files
PLX_OBJECTS += \
    $(SRC_CHIP_DIR)/PlxChipApi.o    \
    $(SRC_CHIP_DIR)/PlxChipFn.o     \
    $(SRC_CHIP_DIR)/PlxInterrupt.o