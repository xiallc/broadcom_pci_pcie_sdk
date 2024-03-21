#class Constants:
PLX_STATUS_START=0x200
PLX_STATUS_OK = PLX_STATUS_START
PLX_STATUS_FAILED =PLX_STATUS_START+ 1
PLX_STATUS_NULL_PARAM =PLX_STATUS_START+ 2
PLX_STATUS_UNSUPPORTED =PLX_STATUS_START+ 3
PLX_STATUS_NO_DRIVER =PLX_STATUS_START+ 4
PLX_STATUS_INVALID_OBJECT =PLX_STATUS_START+ 5
PLX_STATUS_VER_MISMATCH =PLX_STATUS_START+ 6
PLX_STATUS_INVALID_OFFSET =PLX_STATUS_START+ 7
PLX_STATUS_INVALID_DATA =PLX_STATUS_START+ 8
PLX_STATUS_INVALID_SIZE =PLX_STATUS_START+ 9
PLX_STATUS_INVALID_ADDR =PLX_STATUS_START+ 10
PLX_STATUS_INVALID_ACCESS =PLX_STATUS_START+ 11
PLX_STATUS_INSUFFICIENT_RES =PLX_STATUS_START+ 12
PLX_STATUS_TIMEOUT =PLX_STATUS_START+ 13
PLX_STATUS_CANCELED =PLX_STATUS_START+ 14
PLX_STATUS_COMPLETE =PLX_STATUS_START+ 15
PLX_STATUS_PAUSED =PLX_STATUS_START+ 16
PLX_STATUS_IN_PROGRESS =PLX_STATUS_START+ 17
PLX_STATUS_PAGE_GET_ERROR =PLX_STATUS_START+ 18
PLX_STATUS_PAGE_LOCK_ERROR =PLX_STATUS_START+ 19
PLX_STATUS_LOW_POWER =PLX_STATUS_START+ 20
PLX_STATUS_IN_USE =PLX_STATUS_START+ 21
PLX_STATUS_DISABLED =PLX_STATUS_START+ 22
PLX_STATUS_PENDING =PLX_STATUS_START+ 23
PLX_STATUS_NOT_FOUND =PLX_STATUS_START+ 24
PLX_STATUS_INVALID_STATE =PLX_STATUS_START+ 25
PLX_STATUS_BUFF_TOO_SMALL =PLX_STATUS_START+ 26
PLX_STATUS_RSVD_LAST_ERROR     =PLX_STATUS_START+ 27

PLX_FLAG_PORT_NT_LINK_1     = 63
PLX_FLAG_PORT_NT_LINK_0     = 62
PLX_FLAG_PORT_NT_VIRTUAL_1  = 61
PLX_FLAG_PORT_NT_VIRTUAL_0  = 60
PLX_FLAG_PORT_NT_DS_P2P     = 59
PLX_FLAG_PORT_DMA_RAM       = 58
PLX_FLAG_PORT_DMA_3         = 57
PLX_FLAG_PORT_DMA_2         = 56
PLX_FLAG_PORT_DMA_1         = 55
PLX_FLAG_PORT_DMA_0         = 54
PLX_FLAG_PORT_PCIE_TO_USB   = 53
PLX_FLAG_PORT_USB           = 52
PLX_FLAG_PORT_ALUT_3        = 51
PLX_FLAG_PORT_ALUT_2        = 50
PLX_FLAG_PORT_ALUT_1        = 49
PLX_FLAG_PORT_ALUT_0        = 48
PLX_FLAG_PORT_STN_REGS_S5   = 47
PLX_FLAG_PORT_STN_REGS_S4   = 46
_PLX_FLAG_PORT_STN_REGS_S3  = 45
PLX_FLAG_PORT_STN_REGS_S2   = 44
PLX_FLAG_PORT_STN_REGS_S1   = 43
PLX_FLAG_PORT_STN_REGS_S0   = 42
PLX_FLAG_PORT_MAX           = 41
PLX_LINK_SPEED_2_5_GBPS     = 1
PLX_LINK_SPEED_5_0_GBPS     = 2
PLX_LINK_SPEED_8_0_GBPS     = 3
PLX_LINK_SPEED_16_0_GBPS     = 4
PLX_PCIE_GEN_1_0            = PLX_LINK_SPEED_2_5_GBPS
PLX_PCIE_GEN_2_0            = PLX_LINK_SPEED_5_0_GBPS
PLX_PCIE_GEN_3_0            = PLX_LINK_SPEED_8_0_GBPS
PLX_PCIE_GEN_4_0            = PLX_LINK_SPEED_16_0_GBPS

#class PLX_EEPROM_STATUS_CONSTANTS:
PLX_EEPROM_STATUS_NONE=0
PLX_EEPROM_STATUS_VALID=1
PLX_EEPROM_STATUS_INVALID_DATA=2
PLX_EEPROM_STATUS_BLANK=2
PLX_EEPROM_STATUS_CRC_ERROR=2

#class PLX_PORT_TYPE:
PLX_PORT_UNKNOWN            =0xFF
PLX_PORT_ENDPOINT           = 0
PLX_PORT_LEGACY_ENDPOINT    = 1
PLX_PORT_ROOT_PORT          = 4
PLX_PORT_UPSTREAM           = 5
PLX_PORT_DOWNSTREAM         = 6
PLX_PORT_PCIE_TO_PCI_BRIDGE = 7
PLX_PORT_PCI_TO_PCIE_BRIDGE = 8
PLX_PORT_ROOT_ENDPOINT      = 9
PLX_PORT_ROOT_EVENT_COLL    = 10

#PLX_API_MODE - It is an Enum
PLX_API_MODE_PCI            = 0
PLX_API_MODE_AARDVARK       = PLX_API_MODE_PCI+1
PLX_API_MODE_MDIO_SPLICE    = PLX_API_MODE_PCI+2
PLX_API_MODE_SDB			= PLX_API_MODE_PCI+3
PLX_API_MODE_TCP            = PLX_API_MODE_PCI+4

#Performance Monitor Control ( It is an Enum)
PLX_PERF_CMD_START          = 0 
PLX_PERF_CMD_STOP			= 1


# Used for performance counter calculations$
Sizeof_32		= 4
PERF_MAX_PORTS          = 96  # Max # ports in a switch
PERF_COUNTERS_PER_PORT  = 14  # Number of counters per port
PERF_TLP_OH_DW          = 2   # Overhead DW per TLP
PERF_TLP_DW             = (3 + PERF_TLP_OH_DW)  # DW per TLP
PERF_TLP_SIZE           = (PERF_TLP_DW * Sizeof_32) # TLP header bytes with overhead
PERF_TLP_SIZE_NO_OH     = (3 * Sizeof_32)  # TLP header bytes w/o overhead
PERF_DLLP_SIZE          = (2 * Sizeof_32)  # Bytes per DLLP
PERF_MAX_BPS_GEN_1_0    = (250000000)   # 250 MB/s (2.5 Gbps * 80%)
PERF_MAX_BPS_GEN_2_0    = (500000000)   # 500 MB/s (5 Gbps * 80%)
PERF_MAX_BPS_GEN_3_0    = (1000000000)  #   1 GB/s (8 Gbps)
PERF_MAX_BPS_GEN_4_0    = (2000000000)  #   2 GB/s (16 Gbps)


#PLX Chip Familes

PLX_FAMILY_NONE = 0
PLX_FAMILY_UNKNOWN=PLX_FAMILY_NONE+1
PLX_FAMILY_BRIDGE_P2L=PLX_FAMILY_NONE+2         # 9000 series & 8311
PLX_FAMILY_BRIDGE_PCI_P2P=PLX_FAMILY_NONE+3     # 6000 series
PLX_FAMILY_BRIDGE_PCIE_P2P=PLX_FAMILY_NONE+4    # 8111,8112,8114
PLX_FAMILY_ALTAIR = PLX_FAMILY_NONE+5           # 8525,8533,8547,8548
PLX_FAMILY_ALTAIR_XL =PLX_FAMILY_NONE+6         # 8505,8509
PLX_FAMILY_VEGA =PLX_FAMILY_NONE+7              # 8516,8524,8532
PLX_FAMILY_VEGA_LITE =PLX_FAMILY_NONE+8         # 8508,8512,8517,8518
PLX_FAMILY_DENEB =PLX_FAMILY_NONE+9             # 8612,8616,8624,8632,8647,8648
PLX_FAMILY_SIRIUS =PLX_FAMILY_NONE+10           # 8604,8606,8608,8609,8613,8614,8615  8617,8618,8619
PLX_FAMILY_CYGNUS=PLX_FAMILY_NONE+11            # 8625,8636,8649,8664,8680,8696
PLX_FAMILY_SCOUT =PLX_FAMILY_NONE+12            # 8700
PLX_FAMILY_DRACO_1 =PLX_FAMILY_NONE+1           # 8712,8716,8724,8732,8747,8748,8749
PLX_FAMILY_DRACO_2 =PLX_FAMILY_NONE+14          # 8713,8717,8725,8733 + [Draco 1 rev BA]
PLX_FAMILY_MIRA =PLX_FAMILY_NONE+15             # 2380,3380,3382,8603,8605
PLX_FAMILY_CAPELLA_1 =PLX_FAMILY_NONE+16        # 8714,8718,8734,8750,8764,8780,8796
PLX_FAMILY_CAPELLA_2 =PLX_FAMILY_NONE+17        # 9712,9713,9716,9717,9733,9734,9749  9750,9765,9766,9781,9782,9797,9798
PLX_FAMILY_ATLAS =PLX_FAMILY_NONE+18            # C010,C011,C012
PLX_FAMILY_ATLAS_2 =PLX_FAMILY_NONE+19          # C030
PLX_FAMILY_ATLAS2_LLC =PLX_FAMILY_NONE+20          # C034
PLX_FAMILY_LAST_ENTRY=PLX_FAMILY_NONE+20        # -- Must be final entry --

