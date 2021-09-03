#ifndef _PLX_SYSDEP_H_
#define _PLX_SYSDEP_H_

/*******************************************************************************
 * Copyright 2013-2019 Broadcom Inc
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
 *      04-01-19 : PCI/PCIe SDK v8.00
 *
 *****************************************************************************/


#ifndef LINUX_VERSION_CODE
    #include <linux/version.h>
#endif


// Only allow 2.6.18 and higher kernels
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
    #error "ERROR: Linux kernel versions prior to v2.6.18 not supported"
#endif




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
 * This function is supported after 2.6.27 only one some
 * architectures, like x86 & PowerPC. Other architectures
 * added support for it in later kernels. For platforms
 * that support it, HAVE_IOREMAP_PROT is expected defined.
 * SDK drivers use the function for probing ACPI tables. In
 * newer kernels, calls to ioremap() for ACPI locations may
 * report errors if default flags conflict with kernel mappings.
 **********************************************************/
#if !defined(CONFIG_HAVE_IOREMAP_PROT)
    // Revert to ioremap() for unsupported architectures
    #define ioremap_prot(addr,size,flags)     ioremap((addr), (size))
#endif




/***********************************************************
 * pgprot_writecombine
 *
 * This function is not supported on all platforms. There is
 * a standard definition for it starting with 2.6.29.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29))
    #define pgprot_writecombine               pgprot_noncached
#endif




/***********************************************************
 * access_ok
 *
 * access_ok() removed type param in 5.0
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0))
    #define Plx_access_ok                     access_ok
#else
    #define Plx_access_ok(type,addr,size)     access_ok( (addr),(size) )
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
 * defined below.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
    #define PLX_DPC_PARAM      VOID
#else
    #define PLX_DPC_PARAM      struct work_struct
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
 * pci_enable_msi/pci_enable_msix deprecated
 *
 * The pci_*_msi/msix MSI functions are deprecated as of
 * kernel 4.8. A new set of PCI subsystem functions have
 * replaced them.
 **********************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,8,0))

    #define Plx_pci_enable_msi          pci_enable_msi
    #define Plx_pci_disable_msi         pci_disable_msi
    #define Plx_pci_enable_msix         pci_enable_msix
    #define Plx_pci_disable_msix        pci_disable_msix

#else

    #define Plx_pci_enable_msi( pdev )  pci_alloc_irq_vectors( (pdev), 1, 1, PCI_IRQ_MSI )
    #define Plx_pci_disable_msi         pci_free_irq_vectors
    #define Plx_pci_disable_msix        pci_free_irq_vectors

    #define Plx_pci_enable_msix(pdev,entries,nvec)                       \
        ({                                                               \
            int _rc;                                                     \
            int _idx;                                                    \
                                                                         \
            /* Attempt to allocate MSI-X vectors */                      \
            _rc = pci_alloc_irq_vectors(                                 \
                      (pdev), (nvec), (nvec), PCI_IRQ_MSIX );            \
            if (_rc == (nvec))                                           \
            {                                                            \
                /* Set successful return value */                        \
                _rc = 0;                                                 \
                                                                         \
                /* Fill in the vector table */                           \
                for (_idx = 0; _idx < (nvec); _idx++)                    \
                {                                                        \
                    (entries)[_idx].vector =                             \
                        pci_irq_vector( (pdev), (entries)[_idx].entry ); \
                }                                                        \
            }                                                            \
                                                                         \
            _rc;                                                         \
        })

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
 * get_user_pages
 *
 * Parameters to this function changed as follows:
 *   4.6: Removed first two params (curr task & task's mem-manage struct)
 *   4.9: Replaced write & force params with a single gup_flags param
 **********************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
    #define Plx_get_user_pages(start, nr_pages, gup_flags, pages, vmas) \
            (                                           \
                get_user_pages(                         \
                    current,                            \
                    current->mm,                        \
                    (start),                            \
                    (nr_pages),                         \
                    ((gup_flags) & FOLL_WRITE) ? 1 : 0, \
                    0,                                  \
                    (pages),                            \
                    (vmas)                              \
                    )                                   \
            )
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0)
    #define Plx_get_user_pages(start, nr_pages, gup_flags, pages, vmas) \
            (                                           \
                get_user_pages(                         \
                    (start),                            \
                    (nr_pages),                         \
                    ((gup_flags) & FOLL_WRITE) ? 1 : 0, \
                    0,                                  \
                    (pages),                            \
                    (vmas)                              \
                    )                                   \
            )
#else
    #define Plx_get_user_pages          get_user_pages
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



#endif  // _PLX_SYSDEP_H_
