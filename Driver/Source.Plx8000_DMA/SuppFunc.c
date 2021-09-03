/*******************************************************************************
 * Copyright (c) PLX Technology, Inc.
 *
 * PLX Technology Inc. licenses this source file under the GNU Lesser General Public
 * License (LGPL) version 2.  This source file may be modified or redistributed
 * under the terms of the LGPL and without express permission from PLX Technology.
 *
 * PLX Technology, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
 * EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  PLX makes no guarantee
 * or representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL PLX BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.
 *
 ******************************************************************************/

/******************************************************************************
 *
 * File Name:
 *
 *      SuppFunc.c
 *
 * Description:
 *
 *      Additional support functions
 *
 * Revision History:
 *
 *      10-01-12 : PLX SDK v7.00
 *
 ******************************************************************************/


#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/pagemap.h>
#include <linux/sched.h>
#include "ApiFunc.h"
#include "PciFunc.h"
#include "PlxInterrupt.h"
#include "SuppFunc.h"




/*******************************************************************************
 *
 * Function   :  Plx_sleep
 *
 * Description:  Function as a normal sleep. Parameter is in millisecond
 *
 ******************************************************************************/
VOID
Plx_sleep(
    U32 delay
    )
{
    mdelay(
        delay
        );
}




/*******************************************************************************
 *
 * Function   :  Plx_pow_int
 *
 * Description:  A PLX support routine to compute x^y (integers)
 *
 ******************************************************************************/
U32
Plx_pow_int(
    U32 x,
    U32 y
    )
{
    U32 value;


    // Check for '0' exponent
    if (y == 0)
        return 1;

    // Check for base of '0'
    if (x == 0)
        return 0;

    // Set start value
    value = 1;

    // Perform calculation
    while (y--)
    {
        value = value * x;
    }

    return value;
}




/*******************************************************************************
 *
 * Function   :  PlxSynchronizedRegisterModify
 *
 * Description:  Synchronized function with ISR to modify a PLX register
 *
 ******************************************************************************/
BOOLEAN
PlxSynchronizedRegisterModify(
    PLX_REG_DATA *pRegData
    )
{
    unsigned long flags;
    U32           RegValue;


    /*************************************************
     * This routine sychronizes modification of a
     * register with the ISR.  To do this, it uses
     * a special spinlock routine provided by the
     * kernel, which will temporarily disable interrupts.
     * This code should also work on SMP systems.
     ************************************************/

    // Disable interrupts and acquire lock
    spin_lock_irqsave(
        &(pRegData->pdx->Lock_Isr),
        flags
        );

    RegValue =
        PLX_DMA_REG_READ(
            pRegData->pdx,
            pRegData->offset
            );

    RegValue |= pRegData->BitsToSet;
    RegValue &= ~(pRegData->BitsToClear);

    PLX_DMA_REG_WRITE(
        pRegData->pdx,
        pRegData->offset,
        RegValue
        );

    // Re-enable interrupts and release lock
    spin_unlock_irqrestore(
        &(pRegData->pdx->Lock_Isr),
        flags
        );

    return TRUE;
}




/*******************************************************************************
 *
 * Function   :  PlxSynchronizedGetInterruptSource
 *
 * Description:  Synchronized function with ISR to get interrupt source
 *
 ******************************************************************************/
BOOLEAN
PlxSynchronizedGetInterruptSource(
    PLX_INTERRUPT_DATA *pIntData
    )
{
    unsigned long flags;


    /*************************************************
     * This routine sychronizes access to the DEVICE
     * EXTENSION with the ISR.  To do this, it uses
     * a special spinlock routine provided by the
     * kernel, which will temporarily disable interrupts.
     * This code should also work on SMP systems.
     ************************************************/

    // Disable interrupts and acquire lock
    spin_lock_irqsave(
        &(pIntData->pdx->Lock_Isr),
        flags
        );

    // Get active interrupt flags
    pIntData->Source_Ints = pIntData->pdx->Source_Ints;

    // Clear interrupt flags
    pIntData->pdx->Source_Ints = INTR_TYPE_NONE;

    // Re-enable interrupts and release lock
    spin_unlock_irqrestore(
        &(pIntData->pdx->Lock_Isr),
        flags
        );

    return TRUE;
}




/*******************************************************************************
 *
 * Function   :  PlxSignalNotifications
 *
 * Description:  Called by the DPC to signal any notification events
 *
 * Note       :  This is expected to be called at DPC level
 *
 ******************************************************************************/
VOID
PlxSignalNotifications(
    DEVICE_EXTENSION   *pdx,
    PLX_INTERRUPT_DATA *pIntData
    )
{
    U32               SourceInt;
    struct list_head *pEntry;
    PLX_WAIT_OBJECT  *pWaitObject;

    
    spin_lock(
        &(pdx->Lock_WaitObjectsList)
        );

    // Get the interrupt wait list
    pEntry = pdx->List_WaitObjects.next;

    // Traverse wait objects and wake-up processes
    while (pEntry != &(pdx->List_WaitObjects))
    {
        // Get the wait object
        pWaitObject =
            list_entry(
                pEntry,
                PLX_WAIT_OBJECT,
                ListEntry
                );

        // Set active notifications
        SourceInt = pWaitObject->Notify_Flags & pIntData->Source_Ints;

        // Check if waiting for active interrupt
        if (SourceInt)
        {
            DebugPrintf((
                "DPC signaling wait object (%p)\n",
                pWaitObject
                ));

            // Set state to triggered
            pWaitObject->state = PLX_STATE_TRIGGERED;

            // Save new interrupt sources in case later requested
            pWaitObject->Source_Ints |= SourceInt;

            // Signal wait object
            wake_up_interruptible(
                &(pWaitObject->WaitQueue)
                );
        }

        // Jump to next item in the list
        pEntry = pEntry->next;
    }

    spin_unlock(
        &(pdx->Lock_WaitObjectsList)
        );
}




/*******************************************************************************
 *
 * Function   :  PlxGetExtendedCapabilityOffset
 *
 * Description:  Scans the capability list to search for a specific capability
 *
 ******************************************************************************/
U16
PlxGetExtendedCapabilityOffset(
    DEVICE_EXTENSION *pdx,
    U16               CapabilityId
    )
{
    U16 Offset_Cap;
    U32 RegValue;


    // Get offset of first capability
    PLX_PCI_REG_READ(
        pdx,
        0x34,           // PCI capabilities pointer
        &RegValue
        );

    // If link is down, PCI reg accesses will fail
    if (RegValue == (U32)-1)
        return 0;

    // Set first capability
    Offset_Cap = (U16)RegValue;

    // Traverse capability list searching for desired ID
    while ((Offset_Cap != 0) && (RegValue != (U32)-1))
    {
        // Get next capability
        PLX_PCI_REG_READ(
            pdx,
            Offset_Cap,
            &RegValue
            );

        if ((U8)RegValue == (U8)CapabilityId)
        {
            // Capability found, return base offset
            return Offset_Cap;
        }

        // Jump to next capability
        Offset_Cap = (U16)((RegValue >> 8) & 0xFF);
    }

    // Capability not found
    return 0;
}




/*******************************************************************************
 *
 * Function   :  PlxPciBarResourceMap
 *
 * Description:  Maps a PCI BAR resource into kernel space
 *
 ******************************************************************************/
int
PlxPciBarResourceMap(
    DEVICE_EXTENSION *pdx,
    U8                BarIndex
    )
{
    // Default resource to not claimed
    pdx->PciBar[BarIndex].bResourceClaimed = FALSE;

    // Default to NULL kernel VA
    pdx->PciBar[BarIndex].pVa = NULL;

    // Request and Map space
    if (pdx->PciBar[BarIndex].Properties.Flags & PLX_BAR_FLAG_IO)
    {
        // Request I/O port region
        if (request_region(
                pdx->PciBar[BarIndex].Properties.Physical,
                pdx->PciBar[BarIndex].Properties.Size,
                PLX_DRIVER_NAME
                ) != NULL)
        {
            // Note that resource was claimed
            pdx->PciBar[BarIndex].bResourceClaimed = TRUE;
        }
    }
    else
    {
        // Request memory region
        if (request_mem_region(
                pdx->PciBar[BarIndex].Properties.Physical,
                pdx->PciBar[BarIndex].Properties.Size,
                PLX_DRIVER_NAME
                ) == NULL)
        {
            return (-ENOMEM);
        }
        else
        {
            // Note that resource was claimed
            pdx->PciBar[BarIndex].bResourceClaimed = TRUE;

            // Get a kernel-mapped virtual address
            pdx->PciBar[BarIndex].pVa =
                ioremap(
                    pdx->PciBar[BarIndex].Properties.Physical,
                    pdx->PciBar[BarIndex].Properties.Size
                    );

            if (pdx->PciBar[BarIndex].pVa == NULL)
            {
                return (-ENOMEM);
            }
        }
    }

    return 0;
}




/*******************************************************************************
 *
 * Function   :  PlxPciBarResourcesUnmap
 *
 * Description:  Unmap all mapped PCI BAR memory for a device
 *
 ******************************************************************************/
VOID
PlxPciBarResourcesUnmap(
    DEVICE_EXTENSION *pdx
    )
{
    U8 i;


    // Go through all the BARS
    for (i = 0; i < PCI_NUM_BARS_TYPE_00; i++)
    {
        // Unmap the space from Kernel space if previously mapped
        if (pdx->PciBar[i].Properties.Physical != 0)
        {
            if (pdx->PciBar[i].Properties.Flags & PLX_BAR_FLAG_IO)
            {
                // Release I/O port region if it was claimed
                if (pdx->PciBar[i].bResourceClaimed)
                {
                    release_region(
                        pdx->PciBar[i].Properties.Physical,
                        pdx->PciBar[i].Properties.Size
                        );
                }
            }
            else
            {
                DebugPrintf((
                    "Unmap BAR %d from kernel space (VA=%p)\n",
                    i, pdx->PciBar[i].pVa
                    ));

                // Unmap from kernel space
                if (pdx->PciBar[i].pVa != NULL)
                {
                    iounmap(
                        pdx->PciBar[i].pVa
                        );

                    pdx->PciBar[i].pVa = NULL;
                }

                // Release memory region if it was claimed
                if (pdx->PciBar[i].bResourceClaimed)
                {
                    release_mem_region(
                        pdx->PciBar[i].Properties.Physical,
                        pdx->PciBar[i].Properties.Size
                        );
                }
            }
        }
    }
}




/*******************************************************************************
 *
 * Function   :  PlxPciPhysicalMemoryFreeAll_ByOwner
 *
 * Description:  Unmap & release all physical memory assigned to the specified owner
 *
 ******************************************************************************/
VOID
PlxPciPhysicalMemoryFreeAll_ByOwner(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    )
{
    PLX_PHYSICAL_MEM     PciMem;
    struct list_head    *pEntry;
    PLX_PHYS_MEM_OBJECT *pMemObject;


    spin_lock(
        &(pdx->Lock_PhysicalMemList)
        );

    pEntry = pdx->List_PhysicalMem.next;

    // Traverse list to find the desired list objects
    while (pEntry != &(pdx->List_PhysicalMem))
    {
        // Get the object
        pMemObject =
            list_entry(
                pEntry,
                PLX_PHYS_MEM_OBJECT,
                ListEntry
                );

        // Check if owner matches
        if (pMemObject->pOwner == pOwner)
        {
            // Copy memory information
            PciMem.PhysicalAddr = pMemObject->BusPhysical;
            PciMem.Size         = pMemObject->Size;

            // Release list lock
            spin_unlock(
                &(pdx->Lock_PhysicalMemList)
                );

            // Release the memory & remove from list
            PlxPciPhysicalMemoryFree(
                pdx,
                &PciMem
                ); 

            spin_lock(
                &(pdx->Lock_PhysicalMemList)
                );

            // Restart parsing the list from the beginning
            pEntry = pdx->List_PhysicalMem.next;
        }
        else
        {
            // Jump to next item
            pEntry = pEntry->next;
        }
    }

    spin_unlock(
        &(pdx->Lock_PhysicalMemList)
        );
}




/*******************************************************************************
 *
 * Function   :  Plx_dma_buffer_alloc
 *
 * Description:  Allocates physically contiguous non-paged memory
 *
 * Note       :  The function allocates a contiguous block of system memory and
 *               marks it as reserved. Marking the memory as reserved is required
 *               in case the memory is later mapped to user virtual space.
 *
 ******************************************************************************/
VOID*
Plx_dma_buffer_alloc(
    DEVICE_EXTENSION    *pdx,
    PLX_PHYS_MEM_OBJECT *pMemObject
    )
{
    dma_addr_t   BusAddress;
    PLX_UINT_PTR virt_addr;


    /*********************************************************
     * Attempt to allocate contiguous memory
     *
     * Additional flags are specified as follows:
     *
     * __GFP_NOWARN : Prevents the kernel from dumping warnings
     *                about a failed allocation attempt.  The
     *                PLX driver already handles failures.
     *
     * __GFP_REPEAT : Not enabled by default, but may be added
     *                manually.  It asks the kernel to "try a
     *                little harder" in the allocation effort.
     ********************************************************/
    pMemObject->pKernelVa =
        Plx_dma_alloc_coherent(
            pdx,
            pMemObject->Size,
            &BusAddress,
            GFP_KERNEL | __GFP_NOWARN
            );

    if (pMemObject->pKernelVa == NULL)
    {
        return NULL;
    }

    // Store the bus address
    pMemObject->BusPhysical = (U64)BusAddress;

    // Tag all pages as reserved
    for (virt_addr = (PLX_UINT_PTR)pMemObject->pKernelVa;
         virt_addr < ((PLX_UINT_PTR)pMemObject->pKernelVa + pMemObject->Size);
         virt_addr += PAGE_SIZE)
    {
        SetPageReserved(
            virt_to_page( PLX_INT_TO_PTR(virt_addr) )
            );
    }

    // Get CPU physical address of buffer
    pMemObject->CpuPhysical =
        virt_to_phys(
            pMemObject->pKernelVa
            );

    // Clear the buffer
    RtlZeroMemory(
        pMemObject->pKernelVa,
        pMemObject->Size
        );

    DebugPrintf(("Allocated physical memory...\n"));

    DebugPrintf((
        "    CPU Phys Addr: %08lx\n",
        (PLX_UINT_PTR)pMemObject->CpuPhysical
        ));

    DebugPrintf((
        "    Bus Phys Addr: %08lx\n",
        (PLX_UINT_PTR)pMemObject->BusPhysical
        ));

    DebugPrintf((
        "    Kernel VA    : %p\n",
        pMemObject->pKernelVa
        ));

    DebugPrintf((
        "    Size         : %8X (%d %s)\n",
        pMemObject->Size,
        (pMemObject->Size < (10 << 10)) ? pMemObject->Size : (pMemObject->Size >> 10),
        (pMemObject->Size < (10 << 10)) ? "bytes" : "KB"
        ));

    // Return the kernel virtual address of the allocated block
    return pMemObject->pKernelVa;
}




/*******************************************************************************
 *
 * Function   :  Plx_dma_buffer_free
 *
 * Description:  Frees previously allocated physical memory
 *
 ******************************************************************************/
VOID
Plx_dma_buffer_free(
    DEVICE_EXTENSION    *pdx,
    PLX_PHYS_MEM_OBJECT *pMemObject
    )
{
    PLX_UINT_PTR virt_addr;


    // Remove reservation tag for all pages
    for (virt_addr = (PLX_UINT_PTR)pMemObject->pKernelVa;
         virt_addr < ((PLX_UINT_PTR)pMemObject->pKernelVa + PAGE_ALIGN(pMemObject->Size));
         virt_addr += PAGE_SIZE)
    {
        ClearPageReserved(
            virt_to_page( PLX_INT_TO_PTR(virt_addr) )
            );
    }

    // Release the buffer
    Plx_dma_free_coherent(
        pdx,
        pMemObject->Size,
        pMemObject->pKernelVa,
        (dma_addr_t)pMemObject->BusPhysical
        );

    DebugPrintf((
        "Released physical memory at %08llx (%d %s)\n",
        pMemObject->CpuPhysical,
        (pMemObject->Size < (10 << 10)) ? pMemObject->Size : (pMemObject->Size >> 10),
        (pMemObject->Size < (10 << 10)) ? "bytes" : "KB"
        ));

    // Clear memory object properties
    RtlZeroMemory(
        pMemObject,
        sizeof(PLX_PHYS_MEM_OBJECT)
        );
}




/*******************************************************************************
 *
 * Function   :  PlxDmaChannelCleanup
 *
 * Description:  Called by the Cleanup routine to close any open channels
 *
 ******************************************************************************/
VOID
PlxDmaChannelCleanup(
    DEVICE_EXTENSION *pdx,
    VOID             *pOwner
    )
{
    U8 i;


    // Close any DMA channels opened by the owner
    for (i = 0; i < pdx->NumDmaChannels; i++)
    {
        if (pdx->DmaInfo[i].bOpen)
        {
            PlxDmaChannelClose(
                pdx,
                i,
                FALSE,
                pOwner
                );
        }
    }
}




/*******************************************************************************
 *
 * Function   :  PlxSglDmaTransferComplete
 *
 * Description:  Perform any necessary cleanup after an SGL DMA transfer
 *
 ******************************************************************************/
VOID
PlxSglDmaTransferComplete(
    DEVICE_EXTENSION *pdx,
    U8                channel
    )
{
    U8           Index_PciAddr;
    U32          i;
    U32          BusAddr;
    U32          BlockSize;
    PLX_UINT_PTR VaSgl;


    if (pdx->DmaInfo[channel].bSglPending == FALSE)
    {
        DebugPrintf(("No pending SGL DMA to complete\n"));
        return;
    }

    DebugPrintf(("Unlocking user-mode buffer used for SGL DMA transfer...\n"));

    // Get pointer to SGL list
    VaSgl = (PLX_UINT_PTR)pdx->DmaInfo[channel].SglBuffer.pKernelVa;

    // Jump to next 16-byte aligned boundary
    VaSgl = (VaSgl + 0xF) & ~(0xF);

    // Detemine which descriptor field contains mapped PCI address
    if (pdx->DmaInfo[channel].direction == DMA_FROM_DEVICE)
        Index_PciAddr = 0x4;
    else
        Index_PciAddr = 0x8;

    // Unmap and unlock user buffer pages
    for (i = 0; i < pdx->DmaInfo[channel].NumPages; i++)
    {
        // Get PCI bus address from descriptor
        BusAddr = PLX_LE_DATA_32( *(U32*)(VaSgl + Index_PciAddr) );

        // Get byte count from descriptor
        BlockSize = PLX_LE_DATA_32( *(U32*)(VaSgl + 0) );

        // Adjust virtual address to next descriptor
        VaSgl += (4 * sizeof(U32));

        // Unmap the page
        Plx_dma_unmap_page(
            pdx,
            BusAddr,
            BlockSize,
            pdx->DmaInfo[channel].direction
            );

        // Mark page as dirty if PCI->User buffer DMA (user app read)
        if (pdx->DmaInfo[channel].direction == DMA_FROM_DEVICE)
        {
            // Mark page as dirty if necessary
            if (!PageReserved(pdx->DmaInfo[channel].PageList[i]))
            {
                SetPageDirty(
                    pdx->DmaInfo[channel].PageList[i]
                    );
            }
        }

        // Unlock the page
        page_cache_release(
            pdx->DmaInfo[channel].PageList[i]
            );
    }

    // Release page-list memory
    kfree(
        pdx->DmaInfo[channel].PageList
        );

    // Clear the DMA pending flag
    pdx->DmaInfo[channel].bSglPending = FALSE;
}




/*******************************************************************************
 *
 * Function   :  PlxLockBufferAndBuildSgl
 *
 * Description:  Lock a user buffer and build an SGL for it
 *
 ******************************************************************************/
PLX_STATUS
PlxLockBufferAndBuildSgl(
    DEVICE_EXTENSION *pdx,
    U8                channel,
    PLX_DMA_PARAMS   *pDma,
    U64              *pSglAddress,
    U32              *pNumDescr
    )
{
    int          rc;
    U32          i;
    U32          offset;
    U32          SglSize;
    U32          TmpValue;
    U32          BlockSize;
    U32          TotalDescr;
    U32          BytesRemaining;
    U64          BusSgl;
    U64          BusAddr;
    U64          PciAddr;
    U64          AddrSrc;
    U64          AddrDest;
    BOOLEAN      bDirPciToUser;
    PLX_UINT_PTR UserVa;
    PLX_UINT_PTR VaSgl;


    DebugPrintf(("Building SGL descriptors for buffer...\n"));
    DebugPrintf(("   User VA : %08lX\n", (PLX_UINT_PTR)pDma->UserVa));
    DebugPrintf(("   PCI Addr: %08lX\n", (PLX_UINT_PTR)pDma->PciAddr));
    DebugPrintf(("   Size    : %d bytes\n", pDma->ByteCount));
    DebugPrintf((
        "   Dir     : %s\n",
        (pDma->Direction == PLX_DMA_USER_TO_PCI) ? "User --> PCI" : "PCI --> User"
        ));

    // Set default return address
    *pSglAddress = 0;

    // Store buffer page offset
    pdx->DmaInfo[channel].InitialOffset = (U32)(pDma->UserVa & ~PAGE_MASK);

    offset         = pdx->DmaInfo[channel].InitialOffset;
    UserVa         = pDma->UserVa;
    BytesRemaining = pDma->ByteCount;
    TotalDescr     = 0;

    // Count number of user pages
    while (BytesRemaining != 0)
    {
        // Add an I/O buffer
        TotalDescr++;

        if (BytesRemaining <= (PAGE_SIZE - offset))
        {
            BytesRemaining = 0;
        }
        else
        {
            BytesRemaining -= (PAGE_SIZE - offset);
        }

        // Clear offset
        offset = 0;
    }

    DebugPrintf((
        "Allocating %d bytes for user buffer page list (%d pages)...\n",
        (U32)(TotalDescr * sizeof(struct page *)),
        TotalDescr
        ));

    // Allocate memory to store page list
    pdx->DmaInfo[channel].PageList = 
        kmalloc(
            TotalDescr * sizeof(struct page *),
            GFP_KERNEL
            );

    if (pdx->DmaInfo[channel].PageList == NULL)
    {
        DebugPrintf(("ERROR - Unable to allocate memory for list of pages\n"));
        return ApiDmaSglPagesGetError;
    }

    // Store number of pages
    pdx->DmaInfo[channel].NumPages = TotalDescr;

    // Determine & store DMA transfer direction
    if (pDma->Direction == PLX_DMA_PCI_TO_USER)
    {
        bDirPciToUser                   = TRUE;
        pdx->DmaInfo[channel].direction = DMA_FROM_DEVICE;
    }
    else
    {
        bDirPciToUser                   = FALSE;
        pdx->DmaInfo[channel].direction = DMA_TO_DEVICE;
    }

    // Obtain the mmap reader/writer semaphore
    down_read(
        &current->mm->mmap_sem
        );

    // Attempt to lock the user buffer into memory
    rc =
        get_user_pages(
            current,                          // Task performing I/O
            current->mm,                      // The tasks memory-management structure
            UserVa & PAGE_MASK,               // Page-aligned starting address of user buffer
            TotalDescr,                       // Length of the buffer in pages
            bDirPciToUser,                    // Map for write access (i.e. user app performing a read)?
            0,                                // Do not force an override of page protections
            pdx->DmaInfo[channel].PageList,   // Will contain list of page pointers describing buffer
            NULL                              // Will contain list of associated VMAs
            );

    // Release mmap semaphore
    up_read(
        &current->mm->mmap_sem
        );

    if (rc != TotalDescr)
    {
        if (rc <= 0)
        {
            DebugPrintf(("ERROR - Unable to map user buffer (code=%d)\n", rc));
        }
        else
        {
            DebugPrintf((
                "ERROR - Only able to map %d of %d total pages\n",
                rc, TotalDescr
                ));
        }

        kfree( pdx->DmaInfo[channel].PageList );
        return ApiDmaSglPagesLockError;
    }

    DebugPrintf((
        "Page-locked %d user buffer pages...\n",
        TotalDescr
        ));

    /*************************************************************
     * Build SGL descriptors
     *
     * The following code will build the SGL descriptors
     * in PCI memory.  There will be one descriptor for
     * each page of memory since the pages are scattered
     * throughout physical memory.
     ************************************************************/

    /*************************************************************
     * Calculate memory needed for SGL descriptors
     *
     * Mem needed = (#descriptors * descriptor size) + (rounding bytes)
     *
     * 64 bytes are added to support rounding up to the next 64-byte
     * boundary, which is a requirement of the hardware.
     ************************************************************/

    // Calculate SGL size
    SglSize = (TotalDescr * (4 * sizeof(U32))) + 64;

    // Check if a previously allocated buffer can be re-used
    if (pdx->DmaInfo[channel].SglBuffer.pKernelVa != NULL)
    {
        if (pdx->DmaInfo[channel].SglBuffer.Size >= SglSize)
        {
            // Buffer can be re-used, do nothing
            DebugPrintf(("Re-using previously allocated SGL descriptor buffer\n"));
        }
        else
        {
            DebugPrintf(("Releasing previously allocated SGL descriptor buffer\n"));

            // Release memory used for SGL descriptors
            Plx_dma_buffer_free(
                pdx,
                &pdx->DmaInfo[channel].SglBuffer
                );

            pdx->DmaInfo[channel].SglBuffer.pKernelVa = NULL;
        }
    }

    // Allocate memory for SGL descriptors if necessary
    if (pdx->DmaInfo[channel].SglBuffer.pKernelVa == NULL)
    {
        DebugPrintf(("Allocating PCI memory for SGL descriptor buffer...\n"));

        // Setup for transfer
        pdx->DmaInfo[channel].SglBuffer.Size = SglSize;

        VaSgl =
            (PLX_UINT_PTR)Plx_dma_buffer_alloc(
                pdx,
                &pdx->DmaInfo[channel].SglBuffer
                );

        if (VaSgl == 0)
        {
            DebugPrintf((
                "ERROR - Unable to allocate %d bytes for %d SGL descriptors\n",
                pdx->DmaInfo[channel].SglBuffer.Size,
                TotalDescr
                ));

            kfree( pdx->DmaInfo[channel].PageList );
            return ApiInsufficientResources;
        }
    }
    else
    {
        VaSgl = (PLX_UINT_PTR)pdx->DmaInfo[channel].SglBuffer.pKernelVa;
    }

    // Prepare for build of SGL
    PciAddr = pDma->PciAddr;

    // Get bus physical address of SGL descriptors
    BusSgl = (U32)pdx->DmaInfo[channel].SglBuffer.BusPhysical;

    // Make sure addresses are aligned on next descriptor boundary
    VaSgl  = (VaSgl + (64 - 1)) & ~((PLX_UINT_PTR)64 - 1);
    BusSgl = (BusSgl + (64 - 1)) & ~((PLX_UINT_PTR)64 - 1);

    DebugPrintf((
        "Building SGL at %08lx (%d descriptors, 0 extended)\n",
        (PLX_UINT_PTR)BusSgl, TotalDescr
        ));

    // Store total buffer size
    pdx->DmaInfo[channel].BufferSize = pDma->ByteCount;

    // Set offset of first page
    offset = pdx->DmaInfo[channel].InitialOffset;

    // Initialize bytes remaining
    BytesRemaining = pDma->ByteCount;

    // Build the SGL list
    for (i = 0; i < TotalDescr; i++)
    {
        // Calculate transfer size
        if (BytesRemaining > (PAGE_SIZE - offset))
        {
            BlockSize = PAGE_SIZE - offset;
        }
        else
        {
            BlockSize = BytesRemaining;
        }

        // Get bus address of buffer
        BusAddr =
            Plx_dma_map_page(
                pdx,
                pdx->DmaInfo[channel].PageList[i],
                offset,
                BlockSize,
                pdx->DmaInfo[channel].direction
                );

        // Enable the following to display the parameters of each SGL descriptor
        if (PLX_DEBUG_DISPLAY_SGL_DESCR)
        {
            DebugPrintf((
                "SGL Desc %02d: User=%08lX  PCI=%08lX  Size=%X (%d) bytes\n",
                i, (PLX_UINT_PTR)BusAddr, (PLX_UINT_PTR)PciAddr, BlockSize, BlockSize
                ));
        }

        // Set source destination addresses & increment to next PCI address
        if (pDma->Direction == PLX_DMA_USER_TO_PCI)
        {
            AddrSrc  = BusAddr;
            AddrDest = PciAddr;

            // Increment destination PCI address unless should remain constant
            if (pDma->bConstAddrDest == FALSE)
                PciAddr += BlockSize;
        }
        else
        {
            AddrSrc  = PciAddr;
            AddrDest = BusAddr;

            // Increment source PCI address unless should remain constant
            if (pDma->bConstAddrSrc == FALSE)
                PciAddr += BlockSize;
        }

        // Descriptor upper bits of addresses ([47:32])
        TmpValue  = (PLX_64_HIGH_32( AddrSrc ) & 0x0000FFFF) << 16;
        TmpValue |= (PLX_64_HIGH_32( AddrDest ) & 0x0000FFFF) <<  0;

        *(U32*)(VaSgl + 0x4) = PLX_LE_DATA_32( TmpValue );

        // Descriptor lower bits of destination address ([31:0])
        TmpValue = PLX_64_LOW_32( AddrDest );
        *(U32*)(VaSgl + 0x8) = PLX_LE_DATA_32( TmpValue );

        // Descriptor lower bits of source address ([31:0])
        TmpValue = PLX_64_LOW_32( AddrSrc );
        *(U32*)(VaSgl + 0xC) = PLX_LE_DATA_32( TmpValue );

        // Adjust byte count
        BytesRemaining -= BlockSize;

        // Descriptor transfer count
        TmpValue = PLX_LE_U32_BIT( 31 ) |       // Descriptor valid
                   PLX_LE_DATA_32( BlockSize ); // Transfer count

        if (pDma->bConstAddrSrc)
            TmpValue |= PLX_LE_U32_BIT( 29 );    // Keep source address constant

        if (pDma->bConstAddrDest)
            TmpValue |= PLX_LE_U32_BIT( 28 );    // Keep destination address constant

        if (BytesRemaining == 0)
            TmpValue |= PLX_LE_U32_BIT( 30 );    // Interrupt when done

        *(U32*)(VaSgl + 0x0) = TmpValue;

        // Adjust virtual address to next descriptor
        VaSgl += (4 * sizeof(U32));

        // Clear offset
        offset = 0;
    }

    // Return the physical address of the SGL
    *pSglAddress = BusSgl;

    // Return number of descriptors created
    *pNumDescr = TotalDescr;

    return ApiSuccess;
}




/*******************************************************************************
 *
 * Function   :  Plx_dev_mem_to_user_8
 *
 * Description:  Copy data from device to a user-mode buffer, 8-bits at a time
 *
 ******************************************************************************/
void
Plx_dev_mem_to_user_8(
    U8            *VaUser,
    U8            *VaDev,
    unsigned long  count
    )
{
    U8 value;


    while (count)
    {
        // Get next value from device
        value =
            PHYS_MEM_READ_8(
                VaDev
                );

        // Copy value to user-buffer
        __put_user(
            value,
            VaUser
            );

        // Increment pointers
        VaDev++;
        VaUser++;

        // Decrement count
        count -= sizeof(U8);
    }
}




/*******************************************************************************
 *
 * Function   :  Plx_dev_mem_to_user_16
 *
 * Description:  Copy data from device to a user-mode buffer, 16-bits at a time
 *
 ******************************************************************************/
void
Plx_dev_mem_to_user_16(
    U16           *VaUser,
    U16           *VaDev,
    unsigned long  count
    )
{
    U16 value;


    while (count)
    {
        // Get next value from device
        value =
            PHYS_MEM_READ_16(
                VaDev
                );

        // Copy value to user-buffer
        __put_user(
            value,
            VaUser
            );

        // Increment pointers
        VaDev++;
        VaUser++;

        // Decrement count
        count -= sizeof(U16);
    }
}




/*******************************************************************************
 *
 * Function   :  Plx_dev_mem_to_user_32
 *
 * Description:  Copy data from device to a user-mode buffer, 32-bits at a time
 *
 ******************************************************************************/
void
Plx_dev_mem_to_user_32(
    U32           *VaUser,
    U32           *VaDev,
    unsigned long  count
    )
{
    U32 value;


    while (count)
    {
        // Get next value from device
        value =
            PHYS_MEM_READ_32(
                VaDev
                );

        // Copy value to user-buffer
        __put_user(
            value,
            VaUser
            );

        // Increment pointers
        VaDev++;
        VaUser++;

        // Decrement count
        count -= sizeof(U32);
    }
}




/*******************************************************************************
 *
 * Function   :  Plx_user_to_dev_mem_8
 *
 * Description:  Copy data from a user-mode buffer to device, 16-bits at a time
 *
 ******************************************************************************/
void
Plx_user_to_dev_mem_8(
    U8            *VaDev,
    U8            *VaUser,
    unsigned long  count
    )
{
    U8 value;


    while (count)
    {
        // Get next data from user-buffer
        __get_user(
            value,
            VaUser
            );

        // Write value to device
        PHYS_MEM_WRITE_8(
            VaDev,
            value
            );

        // Increment pointers
        VaDev++;
        VaUser++;

        // Decrement count
        count -= sizeof(U8);
    }
}




/*******************************************************************************
 *
 * Function   :  Plx_user_to_dev_mem_16
 *
 * Description:  Copy data from a user-mode buffer to device, 16-bits at a time
 *
 ******************************************************************************/
void
Plx_user_to_dev_mem_16(
    U16           *VaDev,
    U16           *VaUser,
    unsigned long  count
    )
{
    U16 value;


    while (count)
    {
        // Get next data from user-buffer
        __get_user(
            value,
            VaUser
            );

        // Write value to device
        PHYS_MEM_WRITE_16(
            VaDev,
            value
            );

        // Increment pointers
        VaDev++;
        VaUser++;

        // Decrement count
        count -= sizeof(U16);
    }
}




/*******************************************************************************
 *
 * Function   :  Plx_user_to_dev_mem_32
 *
 * Description:  Copy data from a user-mode buffer to device, 32-bits at a time
 *
 ******************************************************************************/
void
Plx_user_to_dev_mem_32(
    U32           *VaDev,
    U32           *VaUser,
    unsigned long  count
    )
{
    U32 value;


    while (count)
    {
        // Get next data from user-buffer
        __get_user(
            value,
            VaUser
            );

        // Write value to device
        PHYS_MEM_WRITE_32(
            VaDev,
            value
            );

        // Increment pointers
        VaDev++;
        VaUser++;

        // Decrement count
        count -= sizeof(U32);
    }
}
