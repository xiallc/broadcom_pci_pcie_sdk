# Broadcom PCI/PCIe SDK

[![Build Status](https://www.travis-ci.com/xiallc/broadcom_pci_pcie_sdk.svg?branch=master)](https://www.travis-ci.com/xiallc/broadcom_pci_pcie_sdk)

XIA LLC develops a number of products using the PLX 9054 chipset. We maintain this repository as a
way to provide CI support for our software. From time to time we may need to restructure this
repository to meet the current needs of our internal processes.

For the most part, we will stick with the vanilla Broadcom releases. Since we're a small team, we
cannot support every combination of OS, kernel, compiler that may be encountered in the wild. If you
run into issues we'll do our best to help, but may recommend that you contact Broadcom's support.

Users can obtain the original source code directly
from [Broadcom](https://www.broadcom.com/products/pcie-switches-bridges/software-dev-kits).

We've provided some simple [installation instructions](xia/doc/install.md). They're not meant to be
comprehensive. The specific process may be different on your system.

## Operating Systems
A quick note about kernels and operating systems. We've evaluated the **compilation** of the kernel
and library on various Linux operating systems. In cases where the compilation of the driver failed, 
we attempted to find a solution to the issue with minor modifications. We only consider the most 
recent release in the 7.X and 8.X lineages. 

### 7.25

| OS | Driver | Library | Kernel | Notes |
|:---:|:---:|:---:|:---:|:---:|
| CentOS 7 | Yes | Yes | 3.10.0-1160.41.1.el7.x86_64 |  |
| CentOS 8 | No | Yes | 4.18.0-305.12.1.el8_4.x86_64 | Driver issue with `mm_segment_t` and `access_ok`. |
| Debian 08 | Yes | Yes | 3.16.0-11-amd64 |  |
| Debian 09 | No | Yes | 4.9.0-16-amd64 | Driver issue with `get_user_pages` |
| Debian 10 | No | Yes | 4.19.0-17-cloud-amd64 | Driver issue with `mm_segment_t`, `copy_to_user`, and `copy_from_user`. |
| Ubuntu 18.04 | No | Yes | 5.4.0-1055-aws | Driver issue with `mm_segment_t` and `access_ok`.  |
| Ubuntu 20.04 | No | Yes | 5.11.0-1016-aws | Driver issue with `access_ok`.  |

### 8.23

| OS | Driver | Library | Kernel | Notes |
|:---:|:---:|:---:|:---:|:---:|
| CentOS 7 | Yes | Yes | 3.10.0-1160.41.1.el7.x86_64 |  |
| CentOS 8 | Sorta | Yes | 4.18.0-305.12.1.el8_4.x86_64 | Driver needs a fix to `Plx_sysdep.h`, see centos8/8.23 branch. |
| Debian 08 | Yes | Yes | 3.16.0-11-amd64 |  |
| Debian 09 | Yes | Yes | 4.9.0-16-amd64 |  |
| Debian 10 | Yes | Yes | 4.19.0-17-cloud-amd64 |  |
| Ubuntu 18.04 | Yes | Yes | 5.4.0-1055-aws |  |
| Ubuntu 20.04 | No | Yes | 5.11.0-1016-aws | Driver issues with `vermagic.h` being included. Fixing that leads to issue with `mmap_sem`. |

## Disclaimer

Broadcom in no way supports or endorses this version of the PCI/PCIe SDK.

## License

We have maintained the same licensing as provided by Broadcom. You can find that information in the
[COPYING](COPYING) file.

## Warranty

This software is provided by "as is" and any express or implied warranties, including, but not
limited to, the implied warranties of merchantability and fitness for a particular purpose are
disclaimed. In no event shall the copyright owner or contributors be liable for any direct,
indirect, incidental, special, exemplary, or consequential damages (including, but not limited to,
procurement of substitute goods or services; loss of use, data, or profits; or business
interruption) however caused and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of the use of this software, even
if advised of the possibility of such damage.