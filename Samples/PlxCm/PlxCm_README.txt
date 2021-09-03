
                PLX Command-line Monitor (PlxCm)

                          July 2014
    _________________________________________________________


     ******************************************************
        NOTE: This document is a work-in-progress.
     ******************************************************



CONTENTS

   I. ABSTRACT
  II. PLXCM HISTORY
 III. SUPPORTED PLATFORMS
  IV. USING PLXCM
   V. PLXCM COMMANDS



I. ABSTRACT

This file provides additional documentation for using the PlxCm application.
PlxCm is updated often and may contain additional features/options not yet
incorporated into this document.



II. PLXCM HISTORY

PlxCm was initially developed in the late 1990's to execute on embedded
CPUs incorporated into PLX local bus RDKs, such as the PLX 9656-MPC860 RDK.
The command-line was modeled similar to the PLXMon for DOS application
provided by PLX in the mid-1990's. Eventually, PlxCm was ported to MS-DOS
to provide PLX with an updated utility for newer PLX devices, such as 9056,
6000-series, & then 8000-series PCI Express devices. It was then incorporated
into the PLX SDK for use in Windows & Linux. Complete source code for the
application layer is provided in the PLX SDK.



III. SUPPORTED PLATFORMS

PlxCm is available for the following platforms:

 DOS
   PlxCm is compiled with the DJGPP DOS GCC compiler
   (http://www.delorie.com/djgpp) with native support for DOS Protected Mode
   Interface (DPMI). PlxCm will exectute in MS-DOS or equivalent (eg Dr. DOS
   or FreeDOS), as well as Windows 95/98. The application source code is taken
   directly from the PLX SDK & efforts are made to keep the DOS version
   synchronized with PLX SDK updates. A PLX custom DOS layer, implementing a
   subset of the PLX API, allows PlxCm to build as-is. The DOS layer is
   proprietary to PLX & is not publicly provided.

 Windows & Linux
   PlxCm is provided in the respective PLX SDK package for either OS. In order
   for PlxCm, or any application utilizing the PLX API, to access devices
   through PCI/PCIe, at least one PLX driver must be loaded in the system.
   The driver(s) loaded determine which devices show up in PlxCm. The PLX PCI
   Service driver allows PlxCm to access any PCI/PCIe device in the system,
   including non-PLX devices.



IV. USING PLXCM

 COMMAND-LINE
   The PlxCm command-line behaves similar to any DOS or standard
   terminal/console. Command history is supported via Up/Down arrow keys.

 INTEGRATED HELP
   There is currently a 'help' command that lists the commands PlxCm supports.
   At this time, the help system is still under development so it does not list
   additional details. A future version may allow commands like "help dl" to
   display addtional usage information.

 VIRTUAL ADDRESSES
   In a virtual memory OS, applications deal with virtual addresses. Addresses
   used by devices, such as those reported in PCI BARs, are PCI physical
   addresses. For an application like PlxCm to access PCI spaces, a driver
   must perform a mapping to a physical address & provide a valid virtual
   address for the application to use.  The actual virtual address value is
   not important & will be different than the physical address. PLX drivers
   provide this virtual mapping for PlxCm to use.

   For convenience, PlxCm automatically maps any valid PCI memory-type BARs of
   the selected device & assigns the virtual address to system variables. The
   variables V0,V1,..V5 will contain virtual addresses of BAR0,BAR1,...,BAR5,
   respectively. This assumes BARs are enabled & of type memory, otherwise the
   Vx variable will not be made available.  Additionally, the number of BARs
   also depends upon the device PCI header type, ie Type0=6 BARs, Type1=2 BARs.
   Type the 'vars' command to see the current list of variables.

   NOTE:
   PlxCm for DOS virtual & physical addresses are identical. PlxCm will assign
   these to variables, just like in a virtual OS environment.

 VALUES & OPERATORS
   In most cases, PlxCm displays values in HEX format. Some arithmetic
   operators are supported, which also work with variables. For example,
   'dl v0+1000+10 10' displays 16B from BAR 0 offset 1010h.



V. PLXCM COMMANDS

The available PlxCm commands are documented here, grouped by feature.


 GENERAL
 -------------------------------------
  * clear/cls
      Clears the terminal display.


  * screen [Num_Lines]
      Displays or sets the screen/terminal size. DOS only support standard text
      modes 25,43,& 50. Windows console supports any size. Changing the
      terminal size does not currently work properly in Linux.

      Examples:
        screen 43     - Set terminal to 43 lines
        screen        - Display current size of the terminal


  * ver
      Display PlxCm version as well PLX API & driver information used to access
      the selected device.


  * history/h [Cmd_Num]
      Display the command history or select a previous command

      Examples:
        history       - Display command history
        h 14          - Re-issue command #14


  * help
      Display the list of available commands.


  * exit/quit/q
      Exit PlxCm.



 DEVICE ACCESS
 -------------------------------------
  * dev <Dev_Num>
      Display the device list or select a specific device. Device numbers are
      listed in the 1st column.


  * i2c <USB#> <I2C_Address> <I2C_Clock>
      Probe for new devices using I2C. At this time, PLX software only
      supports TotalPhase Aardvark I2C USB device.  I2C support has only been
      tested in Windows, but should work in Linux also, provided the Aardvark
      I2C driver is installed properly.

      Parameters:
        USB#         : USB I2C device number (e.g. 0,1,2..)
        I2C_Address  : I2C bus hex address of PLX chip (0=auto-probe)
        I2C_Clock    : Decimal I2C bus clock speed in KHz. (0=auto/100KHz)

      Examples:
        i2c 0 0 0    - Auto-scan for any PLX chip at all possible addresses
        i2c 0 0 40   - Auto-scan for PLX chip & use 40KHz I2C clock
        i2c 1 68 0   - Auto-scan @ 68h using default I2C clock using USB #1


  * set_chip <Chip_Type>
      Force the selected device to a specific PLX chip type. Generally used in
      DOS when a device has been programmed with a non-default PLX ID so PlxCm
      is not able to detect it as a PLX chip. Setting the correct chip type
      then allows PlxCm to perform PLX-specific operations, such as EEPROM
      access.

      Examples:
        set_chip 9056  - Force selected device to be treated as a 9056 chip
        set_chip 0     - Force auto-detection re-attempt of the PLX chip type



 PCI/PCI EXPRESS INFORMATION
 -------------------------------------



 REGISTER ACCESS
 -------------------------------------
