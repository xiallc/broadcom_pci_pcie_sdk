#ifndef _PLX_SYSDEP_H_
#define _PLX_SYSDEP_H_

/*******************************************************************************
 * Copyright 2013-2016 Avago Technologies
 * Copyright (c) 2009 to 2012 PLX Technology Inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directorY of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

/******************************************************************************
 *
 * File Name:
 *
 *      Plx_sysdep.h
 *
 * Description:
 *
 *      This file is provided to support compatible code between different
 *      Linux kernel versions.
 *
 * Revision History:
 *
 *      09-01-16 : PLX SDK v7.25
 *
 *****************************************************************************/


#ifndef LINUX_VERSION_CODE
    #include <linux/version.h>
#endif


// Only allow 2.6 and higher kernels
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    #error "ERROR: Linux kernel versions prior to v2.6 not supported"
#endif




/***********************************************************
 * This definition is used by the PLX driver's PCI functions
 * implementation to decide whether to default to using Linux
 * built-in PCIe configuration access functions. Kernel versions
 * prior to this will using the ACPI ECAM access. Kernels
 * equal to or newer will pass PCIe accesses to kernel functions.
 **********************************************************/
#define PLX_KER_VER_PCIE_USE_OS         KERNEL_VERSION(2,6,18)




/***********************************************************
 * IORESOURCE_MEM_64
 *
 * This flag specifies whether a PCI BAR space is 64-bit. The
 * definition wasn't added until 2.6.31.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
    #define IORESOURCE_MEM_64           0x00100000
#endif




/***********************************************************
 * VM_RESERVED
 *
 * This flag was removed starting with 3.7. The recommended
 * replacement is a combination of two flags.
 **********************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0))
    #define VM_RESERVED                 (VM_DONTEXPAND | VM_DONTDUMP)
#endif




/***********************************************************
 * INIT_WORK & INIT_DELAYED_WORK
 *
 * This macro initializes a work structure with the function
 * to call.  In kernel 2.6.20, the 3rd parameter was removed.
 * It used to be the parameter to the function, but now, the
 * function is called with a pointer to the work_struct itself.
 * INIT_DELAYED_WORK was introduced to init the delayed_work struct.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
    #define PLX_INIT_WORK                     INIT_WORK
    // Red Hat pre-defines this
    #if !defined(RED_HAT_LINUX_KERNEL)
        #define INIT_DELAYED_WORK             INIT_WORK
    #endif
#else
    #define PLX_INIT_WORK(work, func, data)   INIT_WORK((work), (func))
#endif




/***********************************************************
 * ioremap_prot
 *
 * This function is supported after 2.6.27 on x86. PLX drivers
 * use it for probing ACPI tables. In newer kernels, calls
 * to ioremap() for ACPI locations may report errors if the
 * default flags conflict with kernel mappings.
 **********************************************************/
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)) || \
     (!defined(__i386__) && !defined(__x86_64__)))
    #define ioremap_prot(addr,size,flags)     ioremap((addr), (size))
#endif




/***********************************************************
 * pgprot_writecombine
 *
 * This function is not supported on all platforms. There is
 * a standard definition for it starting with 2.6.29.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29))
    #define pgprot_writecombine              pgprot_noncached
#endif




/***********************************************************
 * sema_init / init_MUTEX
 *
 * init_MUTEX replaced by sema_init starting with 2.6.26
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
    #define Plx_sema_init(sem, n)             init_MUTEX( (sem) )
#else
    #define Plx_sema_init                     sema_init
#endif




/***********************************************************
 * flush_work
 *
 * Flush work queue function not added until 2.6.27
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
    #define Plx_flush_work(x)
#else
    #define Plx_flush_work                    flush_work
#endif




/***********************************************************
 * PLX_DPC_PARAM
 *
 * In kernel 2.6.20, the parameter to a work queue function
 * was made to always be a pointer to the work_struct itself.
 * In previous kernels, this was always a VOID*.  Since
 * PLX drivers use work queue functions for the DPC/bottom-half
 * processing, the parameter had to be changed.  For cleaner
 * source code, the definition PLX_DPC_PARAM is used and is
 * defined below.  This also allows 2.4.x compatible source code.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
    #define PLX_DPC_PARAM      VOID
#else
    #define PLX_DPC_PARAM      struct work_struct
#endif




/***********************************************************
 * SA_SHIRQ / IRQF_SHARED
 *
 * In kernel 2.6.18, the IRQ flag SA_SHIRQ was renamed to
 * IRQF_SHARED.  SA_SHIRQ was deprecated but remained in the
 * the kernel headers to support older drivers until 2.6.24.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18))
    #define IRQF_SHARED         SA_SHIRQ
#endif




/***********************************************************
 * pci_get_domain_bus_and_slot not added until 2.6.33
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33))
    #define Plx_pci_get_domain_bus_and_slot(d,b,df)    pci_get_bus_and_slot( b, df )
#else
    #define Plx_pci_get_domain_bus_and_slot            pci_get_domain_bus_and_slot
#endif




/***********************************************************
 * pci_enable_msi  --> pci_enable_msi_exact
 * pci_enable_msix --> pci_enable_msix_exact
 *
 * A new set of pci_enable_msi* functions were added in 3.14
 * to replace existing functions.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,14,0))
    #define Plx_pci_enable_msi_exact(dev,nvec)         pci_enable_msi( (dev) )
    #define Plx_pci_enable_msix_exact                  pci_enable_msix
#else
    #define Plx_pci_enable_msi_exact                   pci_enable_msi_exact
    #define Plx_pci_enable_msix_exact                  pci_enable_msix_exact
#endif




/***********************************************************
 * kmap_atomic/kunmap_atomic - 2nd parameter removed in 2.6.37
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37))
    #define Plx_kmap_atomic                  kmap_atomic
    #define Plx_kunmap_atomic                kunmap_atomic
#else
    #define Plx_kmap_atomic(page,kmtype)     kmap_atomic( (page) )
    #define Plx_kunmap_atomic(page,kmtype)   kunmap_atomic( (page) )
#endif




/***********************************************************
 * 'secondary' & 'subordinate' fields removed from 'struct pci_bus'
 * in 3.6 & replaced with start/end fields in a 'struct resource'.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0))
    #define Plx_STRUCT_PCI_BUS_SEC(pbus)   (pbus)->secondary
    #define Plx_STRUCT_PCI_BUS_SUB(pbus)   (pbus)->subordinate
#else
    #define Plx_STRUCT_PCI_BUS_SEC(pbus)   (pbus)->busn_res.start
    #define Plx_STRUCT_PCI_BUS_SUB(pbus)   (pbus)->busn_res.end
#endif




/***********************************************************
 * skb_frag_struct "page" field changed from pointer to
 * a page pointer within a structure in 3.2.0
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0))
    #define Plx_SKB_FRAG_STRUCT_PAGE(frag)   (frag)->page
#else
    #define Plx_SKB_FRAG_STRUCT_PAGE(frag)   (frag)->page.p
#endif




/***********************************************************
 * pcie_cap field in pci_dev   - Added by 2.6.38
 *
 * The pci_dev structure has a pcie_cap field to report the
 * device's PCIe capability (10h) starting offset.
 * This field is was also added to updated RedHat kernels.
 **********************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)) || \
    ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)) && defined(RED_HAT_LINUX_KERNEL))
    #define Plx_pcie_cap(pDev)      ((pDev)->pcie_cap)
#else
    #define Plx_pcie_cap(pDev)      pci_find_capability( (pDev), 0x10 )
#endif




/***********************************************************
 * is_virtfn field in pci_dev   - Added in 2.6.30
 * pci_physfn                   - Added by 2.6.35
 *
 * For SR-IOV devices, there is an 'is_virtfn' field in the
 * pci_dev structure to report whether the device is a VF. This
 * field is was also added to updated RedHat kernels.
 *
 * pci_physfn returns the parent PF of a VF; otherwise, returns
 * the passed device.
 ***********************************************************/
#if defined(CONFIG_PCI_IOV) && \
     ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)) || \
     ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)) && defined(RED_HAT_LINUX_KERNEL)))
    #define Plx_pcie_is_virtfn(pDev)    ((pDev)->is_virtfn)

    #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
        #define Plx_pci_physfn(pDev)       \
            ({                             \
               struct pci_dev *_pDev;      \
                                           \
               if (pDev->is_virtfn)        \
                   _pDev = (pDev->physfn); \
               else                        \
                   _pDev = pDev;           \
               _pDev;                      \
            })
    #else
        #define Plx_pci_physfn          pci_physfn
    #endif
#else
    #define Plx_pcie_is_virtfn(pDev)    (FALSE)
    #define Plx_pci_physfn(pDev)        (pDev)
#endif




/***********************************************************
 * readq / writeq 
 *
 * These functions are used to perform 64-bit accesses to
 * I/O memory.  They are not defined for all architectures.
 * For x86 32-bit, they were added in 2.6.29.
 *
 * NOTE: This is still incomplete for non-x86 32-bit architectures
 *       where readq/writeq are not yet defined.
 **********************************************************/
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)) && defined(CONFIG_X86_32))
    // x86 64-bit I/O access functions
    static inline __u64 readq(const volatile void __iomem *addr)
    {
        const volatile u32 __iomem *p = addr;
        u64 value;

        value  = readl(p);
        value += ((u64)readl(p + 1) << 32);

        return value;
    }

    static inline void writeq(__u64 val, volatile void __iomem *addr)
    {
        writel(val, addr);
        writel(val >> 32, addr+4);
    }

#elif !defined(CONFIG_64BIT) && !defined(CONFIG_X86_32)
    // Revert to 32-bit accesses for non-x86/x64 platforms
    #define readq                   readl
    #define writeq                  writel
#endif




/***********************************************************
 * dma_set_mask / pci_set_dma_mask
 *
 * This function is used to set the mask for DMA addresses
 * for the device.  In 2.4 kernels, the function is pci_set_dma_mask
 * and in 2.6 kernels, it has been change to dma_set_mask.
 * In addition to the name change, the first parameter has
 * been changed from the PCI device structure in 2.4, to
 * the device structure found in the PCI device structure.
 **********************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
    #define Plx_dma_set_mask(pdx, mask) \
            (                           \
                pci_set_dma_mask(       \
                    (pdx)->pPciDevice,  \
                    (mask)              \
                    )                   \
            )
#else
    #define Plx_dma_set_mask(pdx, mask)        \
            (                                  \
                dma_set_mask(                  \
                    &((pdx)->pPciDevice->dev), \
                    (mask)                     \
                    )                          \
            )
#endif




/***********************************************************
 * dma_set_coherent_mask / pci_set_consistent_dma_mask
 *
 * This function is used to set the mask for coherent/consistent
 * DMA buffer allocations.  Prior to 2.6.34, the function is
 * pci_set_consistent_dma_mask.  It is now dma_set_coherent_mask.
 * The first parameter has also been changed from pci_dev
 * structure to the dev structure found in pci_dev.
 **********************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34)
    #define Plx_dma_set_coherent_mask(pdx, mask) \
            (                                    \
                pci_set_consistent_dma_mask(     \
                    (pdx)->pPciDevice,           \
                    (mask)                       \
                    )                            \
            )
#else
    #define Plx_dma_set_coherent_mask(pdx, mask) \
            (                                    \
                dma_set_coherent_mask(           \
                    &((pdx)->pPciDevice->dev),   \
                    (mask)                       \
                    )                            \
            )
#endif




/***********************************************************
 * DMA_BIT_MASK
 *
 * This macro is used to specify bit masks (e.g dma_set_mask).
 * It was introduced in 2.6.24
 **********************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
    #define PLX_DMA_BIT_MASK(n)         (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#else
    #define PLX_DMA_BIT_MASK            DMA_BIT_MASK
#endif




/***********************************************************
 * dma_alloc_coherent & dma_free_coherent
 *
 * These functions are used to allocate and map DMA buffers.
 * In 2.4 kernels, the functions are pci_alloc/free_consistent
 * and in 2.6 kernels, they have been changed to
 * dma_alloc/free_coherent.  In addition to the name changes,
 * the first parameter has been changed from the PCI device
 * structure in 2.4, to the device structure found in the PCI
 * device structure.
 **********************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
    #define Plx_dma_alloc_coherent(pdx, size, dma_handle, flag) \
                pci_alloc_consistent(  \
                    (pdx)->pPciDevice, \
                    (size),            \
                    (dma_handle)       \
                    )

    #define Plx_dma_free_coherent(pdx, size, cpu_addr, dma_handle) \
                pci_free_consistent(   \
                    (pdx)->pPciDevice, \
                    (size),            \
                    (cpu_addr),        \
                    (dma_handle)       \
                    )
#else
    #define Plx_dma_alloc_coherent(pdx, size, dma_handle, flag) \
                dma_alloc_coherent(            \
                    &((pdx)->pPciDevice->dev), \
                    (size),                    \
                    (dma_handle),              \
                    (flag)                     \
                    )

    #define Plx_dma_free_coherent(pdx, size, cpu_addr, dma_handle) \
                dma_free_coherent(             \
                    &((pdx)->pPciDevice->dev), \
                    (size),                    \
                    (cpu_addr),                \
                    (dma_handle)               \
                    )
#endif




/***********************************************************
 * dma_map_page & dma_unmap_page
 *
 * These functions are used to map a single user buffer page
 * in order to get a valid bus address for the page. In 2.4
 * kernels, the functions are pci_map/unmap_page and in 2.6
 * kernels, they have been changed to dma_map/unmap_page.
 * In addition to the name changes, the first parameter has
 * been changed from the PCI device structure in 2.4, to the
 * device structure found in the PCI device structure.
 **********************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
    #define Plx_dma_map_page(pdx, page, offset, size, direction) \
                pci_map_page(          \
                    (pdx)->pPciDevice, \
                    (page),            \
                    (offset),          \
                    (size),            \
                    (direction)        \
                    )

    #define Plx_dma_unmap_page(pdx, dma_address, size, direction) \
                pci_unmap_page(        \
                    (pdx)->pPciDevice, \
                    (dma_address),     \
                    (size),            \
                    (direction)        \
                    )
#else
    #define Plx_dma_map_page(pdx, page, offset, size, direction) \
                dma_map_page(                  \
                    &((pdx)->pPciDevice->dev), \
                    (page),                    \
                    (offset),                  \
                    (size),                    \
                    (direction)                \
                    )

    #define Plx_dma_unmap_page(pdx, dma_address, size, direction) \
                dma_unmap_page(                \
                    &((pdx)->pPciDevice->dev), \
                    (dma_address),             \
                    (size),                    \
                    (direction)                \
                    )
#endif




/***********************************************************
 * remap_pfn_range & remap_page_range
 *
 * The remap_pfn_range() function was added in kernel 2.6 and
 * does not exist in previous kernels.  For older kernels,
 * remap_page_range can be used, as it is the same function
 * except the Page Frame Number (pfn) parameter should
 * actually be a physical address.  For that case, the
 * pfn is simply shifted by PAGE_SHIFT to obtain the
 * corresponding physical address.
 *
 * remap_pfn_range, however, does not seem to exist in all
 * kernel 2.6 distributions.  remap_page_range was officially
 * removed in 2.6.11 but declared deprecated in 2.6.10. To keep
 * things simple, this driver will default to remap_page_range
 * unless the kernel version is 2.6.10 or greater.
 *
 * For 2.4 kernels, remap_pfn_range obviously does not exist.
 * Although remap_page_range() may be used instead, there
 * was a parameter added in kernel version 2.5.3. The new
 * parameter is a pointer to the VMA structure.  To make
 * matters even more complicated, the kernel source in
 * RedHat 9.0 (v2.4.20-8), however, also uses the new
 * parameter.  As a result, another #define is added if
 * RedHat 9.0 kernel source is used.
 *
 * The #defines below should result in the following usage table:
 *
 *  kernel                        function
 * ====================================================
 *  2.4.0  -> 2.4.19              remap_page_range (no VMA param)
 *  2.4.20 -> 2.5.2  (non-RedHat) remap_page_range (no VMA param)
 *  2.4.20 -> 2.5.2  (RedHat)     remap_page_range (with VMA param)
 *  2.5.3  -> 2.6.9               remap_page_range (with VMA param)
 *  2.6.10 & up                   remap_pfn_range
 *
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
    #if ( (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,3)) || \
         ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)) && defined(RED_HAT_LINUX_KERNEL)) )

        // Revert to remap_page_range
        #define Plx_remap_pfn_range(vma, virt_addr, pfn, size, prot) \
                (                           \
                    remap_page_range(       \
                        (vma),              \
                        (virt_addr),        \
                        (pfn) << PAGE_SHIFT,\
                        (size),             \
                        (prot)              \
                        )                   \
                )
    #else
        // VMA parameter must be removed
        #define Plx_remap_pfn_range(vma, virt_addr, pfn, size, prot) \
                (                           \
                    remap_page_range(       \
                        (virt_addr),        \
                        (pfn) << PAGE_SHIFT,\
                        (size),             \
                        (prot)              \
                        )                   \
                )
    #endif
#else
    // Function already defined
    #define Plx_remap_pfn_range         remap_pfn_range
#endif




/***********************************************************
 * io_remap_pfn_range & io_remap_page_range
 *
 * The io_remap_page_range() function is used to map I/O space
 * into user mode.  Generally, it defaults to remap_page_range,
 * but on some architectures it performs platform-specific code.
 *
 * In kernel 2.6.12, io_remap_page_range was deprecated and
 * replaced with io_remap_pfn_range.
 *
 * Since io_remap_xxx_range usually reverts to remap_xxx_range,
 * the same issues regarding kernel version apply.  Refer to
 * the explanation above regarding remap_page/pfn_range.
 *
 * The #defines below should result in the following usage table:
 *
 *  kernel                        function
 * ====================================================
 *  2.4.0  -> 2.4.19              io_remap_page_range (no VMA param)
 *  2.4.20 -> 2.5.2  (non-RedHat) io_remap_page_range (no VMA param)
 *  2.4.20 -> 2.5.2  (RedHat)     io_remap_page_range (with VMA param)
 *  2.5.3  -> 2.6.11              io_remap_page_range (with VMA param)
 *  2.6.12 & up                   io_remap_pfn_range
 *
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12))
    #if ( (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,3)) || \
         ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)) && defined(RED_HAT_LINUX_KERNEL)) )

        // Revert to io_remap_page_range (with VMA)
        #define Plx_io_remap_pfn_range(vma, virt_addr, pfn, size, prot) \
                (                           \
                    io_remap_page_range(    \
                        (vma),              \
                        (virt_addr),        \
                        (pfn) << PAGE_SHIFT,\
                        (size),             \
                        (prot)              \
                        )                   \
                )
    #else
        // Revert to io_remap_page_range (without VMA)
        #define Plx_io_remap_pfn_range(vma, virt_addr, pfn, size, prot) \
                (                           \
                    io_remap_page_range(    \
                        (virt_addr),        \
                        (pfn) << PAGE_SHIFT,\
                        (size),             \
                        (prot)              \
                        )                   \
                )
    #endif
#else
    // Function already defined
    #define Plx_io_remap_pfn_range      io_remap_pfn_range
#endif



#endif  // _PLX_SYSDEP_H_
