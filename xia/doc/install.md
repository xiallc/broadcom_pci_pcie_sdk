# Installation
These installation instructions cover the basics of building the library and driver. 

### Prerequisites
* Linux Kernel headers
* GNU make

### Instructions
We'll use the variable `<version>` to represent one of the versions listed above. For example, if
you want to download version 8.00 then replace `<version>` with 8.00 in the instructions below.

1. Create the installation directory
   ```shell
   sudo mkdir /usr/local/broadcom
   ```
1. Clone the repository from GitHub, replacing `<version>` with the version you need. Ex. `8.00`
   ```shell
   git clone https://github.com/xiallc/broadcom_pci_pcie_sdk.git --single-branch --branch <version> <version>
   ```
1. Move the cloned repository into the installation directory and link it.
   ```shell script
   sudo mv <version> /usr/local/broadcom/<version>
   sudo ln -s /usr/local/broadcom/<version> /usr/local/broadcom/current
   ```
2. Export the `PLX_SDK_DIR` environment variable in `/etc/profile.d/broadcom.sh`
   ```shell script
   echo -e "export PLX_SDK_DIR=/usr/local/broadcom/current\n" | sudo tee /etc/profile.d/broadcom.sh > /dev/null
   source /etc/profile.d/broadcom.sh
   ```
2. Build the SDK and 9054 kernel driver
   ```shell script
   sudo make -C ${PLX_SDK_DIR}/PlxApi/ PLX_NO_CLEAR_SCREEN=1
   sudo -E PLX_CHIP=9054 make -C ${PLX_SDK_DIR}/Driver/ PLX_NO_CLEAR_SCREEN=1
   ```
4. Load the kernel driver
    ```shell script
    sudo -E ${PLX_SDK_DIR}/Bin/Plx_load 9054
    ```
**NOTE:** If you do not have a PLX PCI/PCIe 9054 device attached and discoverable by the system then
loading the kernel module will fail.

**NOTE:** If you're using a system that has `selinux` enabled. Then you'll need to temporarily
set `selinux` into permissive mode before attempting to load the driver.
----
## Systemd service
We provide with this repository a systemd script that allows users to automatically load the drivers
when at system boot. This script is not included with versions 6.40-8.23 by default. It still works
with those versions you'll just have to download it from the repo directly.

### Installing the service
1. Copy the correct version for your Linux flavor into `/etc/systemd/system/broadcom.service`
2. Enable and start the service
   ```shell
   sudo systemctl daemon-reload
   sudo systemctl enable broadcom
   ```
3. Reboot your system
4. Verify that the driver loaded properly
   ```shell
   sudo systemctl status broadcom
   ```
