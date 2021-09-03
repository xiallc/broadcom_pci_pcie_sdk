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
 *      SuppFunc.c
 *
 * Description:
 *
 *      Additional support functions
 *
 * Revision History:
 *
 *      03-01-19 : PCI/PCIe SDK v8.00
 *
 ******************************************************************************/


#include <linux/uaccess.h>   // For user page access
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include "ApiFunc.h"
#include "PciFunc.h"
#include "PciRegs.h"
#include "PlxChipApi.h"
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
    mdelay( delay );
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
        PLX_9000_REG_READ(
            pRegData->pdx,
            pRegData->offset
            );

    RegValue |= pRegData->BitsToSet;
    RegValue &= ~(pRegData->BitsToClear);

    PLX_9000_REG_WRITE(
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
    U32               SourceDB;
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
        SourceDB  = pWaitObject->Notify_Doorbell & pIntData->Source_Doorbell;

        // Check if waiting for active interrupt
        if (SourceInt || SourceDB)
        {
            DebugPrintf((
                "DPC signal wait object (%p)\n",
                pWaitObject
                ));

            // Set state to triggered
            pWaitObject->state = PLX_STATE_TRIGGERED;

            // Save new interrupt sources in case later requested
            pWaitObject->Source_Ints     |= SourceInt;
            pWaitObject->Source_Doorbell |= SourceDB;

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
 * Function   :  PlxPciFindCapability
 *
 * Description:  Scans a device's capability list to find the base offset of the
 *               specified PCI or PCIe extended capability. If the capability ID
 *               is PCI or PCIe vendor-specific (VSEC), an optional instance
 *               number can be provided since a device could have multiple VSEC
 *               capabilities. A value of 0 returns the 1st matching instance but
 *               the parameter is ignored for non-VSEC capability search.
 *
 ******************************************************************************/
U16
PlxPciFindCapability(
    DEVICE_EXTENSION *pdx,
    U16               CapID,
    U8                bPCIeCap,
    U8                InstanceNum
    )
{
    U8  matchCount;
    U16 offset;
    U16 currID;
    U16 probeLimit;
    U32 regVal;


    // Get PCI command register (04h)
    PLX_PCI_REG_READ( pdx, PCI_REG_CMD_STAT, &regVal );

    // Verify device responds to PCI accesses (in case link down)
    if (regVal == (U32)-1)
    {
        return 0;
    }

    // Verify device supports extended capabilities (04h[20])
    if ((regVal & ((U32)1 << 20)) == 0)
    {
        return 0;
    }

    // Set capability pointer offset
    if (bPCIeCap)
    {
        // PCIe capabilities must start at 100h
        offset = PCIE_REG_CAP_PTR;

        // Ignore instance number for non-VSEC capabilities
        if (CapID != PCIE_CAP_ID_VENDOR_SPECIFIC)
        {
            InstanceNum = 0;
        }
    }
    else
    {
        // Get offset of first capability from capability pointer (34h[7:0])
        PLX_PCI_REG_READ( pdx, PCI_REG_CAP_PTR, &regVal );

        // Set first capability offset
        offset = (U8)regVal;

        // Ignore instance number for non-VSEC capabilities
        if (CapID != PCI_CAP_ID_VENDOR_SPECIFIC)
        {
            InstanceNum = 0;
        }
    }

    // Start with 1st match
    matchCount = 0;

    // Set max probe limit
    probeLimit = 100;

    // Traverse capability list searching for desired ID
    while ((offset != 0) && (regVal != (U32)-1))
    {
        // Get next capability
        PLX_PCI_REG_READ( pdx, offset, &regVal );

        // Verify capability is valid
        if ((regVal == 0) || (regVal == (U32)-1))
        {
            return 0;
        }

        // Some chipset pass 100h+ to PCI devices which decode
        // back to 0-FFh, so check if 100h matches Dev/Ven ID(0)
        if ((offset == PCIE_REG_CAP_PTR) &&
            (regVal == (((U32)pdx->Key.DeviceId << 16) |
                              pdx->Key.VendorId)))
        {
            return 0;
        }

        // Extract the capability ID
        if (bPCIeCap)
        {
            // PCIe ID in [15:0]
            currID = (U16)((regVal >> 0) & 0xFFFF);
        }
        else
        {
            // PCI ID in [7:0]
            currID = (U16)((regVal >> 0) & 0xFF);
        }

        // Compare with desired capability
        if (currID == CapID)
        {
            // Verify correct instance
            if (InstanceNum == matchCount)
            {
                // Capability found, return base offset
                return offset;
            }

            // Increment count of matches
            matchCount++;
        }

        // Enforce an upper bound on probe in case device has issues
        probeLimit--;
        if (probeLimit == 0)
        {
            return 0;
        }

        // Jump to next capability
        if (bPCIeCap)
        {
            // PCIe next cap offset in [31:20]
            offset = (U16)((regVal >> 20) & 0xFFF);
        }
        else
        {
            // PCI next cap offset in [15:8]
            offset = (U8)((regVal >> 8) & 0xFF);
        }
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
                // Unmap from kernel space
                if (pdx->PciBar[i].pVa != NULL)
                {
                    DebugPrintf((
                        "Unmap BAR %d (VA=%p)\n",
                        i, pdx->PciBar[i].pVa
                        ));
                    iounmap( pdx->PciBar[i].pVa );
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


    // Verify size
    if (pMemObject->Size == 0)
    {
        ErrorPrintf(("ERROR: Unable to allocate buffer, requested size = 0\n"));
        RtlZeroMemory( pMemObject, sizeof(PLX_PHYS_MEM_OBJECT) );
        return NULL;
    }

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
        dma_alloc_coherent(
            &(pdx->pPciDevice->dev),
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
    RtlZeroMemory( pMemObject->pKernelVa, pMemObject->Size );

    DebugPrintf(("Allocated physical memory...\n"));

    DebugPrintf((
        "    CPU Phys Addr: %08llx\n",
        pMemObject->CpuPhysical
        ));

    DebugPrintf((
        "    Bus Phys Addr: %08llx\n",
        pMemObject->BusPhysical
        ));

    DebugPrintf((
        "    Kernel VA    : %p\n",
        pMemObject->pKernelVa
        ));

    DebugPrintf((
        "    Size         : %Xh (%d%s)\n",
        pMemObject->Size,
        (pMemObject->Size > (1 << 30)) ? (pMemObject->Size >> 30) :
          (pMemObject->Size > (1 << 20)) ? (pMemObject->Size >> 20) :
          (pMemObject->Size > (1 << 10)) ? (pMemObject->Size >> 10) :
          pMemObject->Size,
        (pMemObject->Size > (1 << 30)) ? "GB" :
          (pMemObject->Size > (1 << 20)) ? "MB" :
          (pMemObject->Size > (1 << 10)) ? "KB" : "B"
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
    dma_free_coherent(
        &(pdx->pPciDevice->dev),
        pMemObject->Size,
        pMemObject->pKernelVa,
        (dma_addr_t)pMemObject->BusPhysical
        );

    DebugPrintf((
        "Released physical memory at %08llxh (%d%s)\n",
        pMemObject->CpuPhysical,
        (pMemObject->Size > (1 << 30)) ? (pMemObject->Size >> 30) :
          (pMemObject->Size > (1 << 20)) ? (pMemObject->Size >> 20) :
          (pMemObject->Size > (1 << 10)) ? (pMemObject->Size >> 10) :
          pMemObject->Size,
        (pMemObject->Size > (1 << 30)) ? "GB" :
          (pMemObject->Size > (1 << 20)) ? "MB" :
          (pMemObject->Size > (1 << 10)) ? "KB" : "B"
        ));

    // Clear memory object properties
    RtlZeroMemory( pMemObject, sizeof(PLX_PHYS_MEM_OBJECT) );
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
#if defined(PLX_DMA_SUPPORT)
    U8 i;


    // Close any DMA channels opened by the owner
    for (i = 0; i < NUM_DMA_CHANNELS; i++)
    {
        if (pdx->DmaInfo[i].bOpen)
        {
            PlxChip_DmaChannelClose(
                pdx,
                i,
                FALSE,
                pOwner
                );
        }
    }
#endif  // PLX_DMA_SUPPORT
}




#if defined(PLX_DMA_SUPPORT)
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
    U32          i;
    U32          BusAddr;
    U32          BlockSize;
    PLX_UINT_PTR VaSgl;


    if (pdx->DmaInfo[channel].bSglPending == FALSE)
    {
        DebugPrintf(("No pending SGL DMA to complete\n"));
        return;
    }

    DebugPrintf(("Unlock user-mode buffer used for SGL DMA transfer...\n"));

    // Get pointer to SGL list
    VaSgl = (PLX_UINT_PTR)pdx->DmaInfo[channel].SglBuffer.pKernelVa;

    // Jump to next 16-byte aligned boundary
    VaSgl = (VaSgl + 0xF) & ~(0xF);

    // Unmap and unlock user buffer pages
    for (i = 0; i < pdx->DmaInfo[channel].NumPages; i++)
    {
        // Get PCI bus address from descriptor
        BusAddr = PLX_LE_DATA_32(*(((U32*)VaSgl) + SGL_DESC_IDX_PCI_LOW));

        // Get byte count from descriptor
        BlockSize = PLX_LE_DATA_32(*(((U32*)VaSgl) + SGL_DESC_IDX_COUNT));

        // Adjust virtual address to next descriptor
        VaSgl += (4 * sizeof(U32));

        // Unmap the page
        dma_unmap_page(
            &(pdx->pPciDevice->dev),
            BusAddr,
            BlockSize,
            pdx->DmaInfo[channel].direction
            );

        // Mark page as dirty if Loc->PCI DMA (user app read)
        if (pdx->DmaInfo[channel].direction == DMA_FROM_DEVICE)
        {
            // Mark page as dirty if necessary
            if (!PageReserved(pdx->DmaInfo[channel].PageList[i]))
            {
                SetPageDirty( pdx->DmaInfo[channel].PageList[i] );
            }
        }

        // Unlock the page
        put_page( pdx->DmaInfo[channel].PageList[i] );
    }

    // Release page-list memory
    kfree( pdx->DmaInfo[channel].PageList );

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
    U32              *pSglAddress,
    BOOLEAN          *pbBits64
    )
{
    int          rc;
    U8           SizeDescr;
    U32          i;
    U32          offset;
    U32          BusSgl;
    U32          BusSglOriginal;
    U32          SglSize;
    U32          BlockSize;
    U32          LocalAddr;
    U32          TotalDescr;
    U32          BytesRemaining;
    U64          BusAddr;
    BOOLEAN      bDirLocalToPci;
    PLX_UINT_PTR VaSgl;
    PLX_UINT_PTR UserVa;


    DebugPrintf(("Build SGL descriptors for buffer...\n"));
    DebugPrintf(("   User VA   : %08lX\n", (PLX_UINT_PTR)pDma->UserVa));
    DebugPrintf(("   Local Addr: %08X\n", pDma->LocalAddr));
    DebugPrintf(("   Size      : %dB\n", pDma->ByteCount));
    DebugPrintf(("   Direction : %s\n",
        (pDma->Direction == PLX_DMA_LOC_TO_PCI) ? "Local --> PCI" : "PCI --> Local"
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
        "Allocate %d bytes for user buffer page list (%d pages)...\n",
        (U32)(TotalDescr * sizeof(struct page *)), TotalDescr
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
        return PLX_STATUS_PAGE_GET_ERROR;
    }

    // Store number of pages
    pdx->DmaInfo[channel].NumPages = TotalDescr;

    // Determine & store DMA transfer direction
    if (pDma->Direction == PLX_DMA_LOC_TO_PCI)
    {
        bDirLocalToPci                  = TRUE;
        pdx->DmaInfo[channel].direction = DMA_FROM_DEVICE;
    }
    else
    {
        bDirLocalToPci                  = FALSE;
        pdx->DmaInfo[channel].direction = DMA_TO_DEVICE;
    }

    // Obtain the mmap reader/writer semaphore
    down_read( &current->mm->mmap_sem );

    // Attempt to lock the user buffer into memory
    rc =
        Plx_get_user_pages(
            UserVa & PAGE_MASK,                // Page-aligned user buffer start address
            TotalDescr,                        // Length of the buffer in pages
            (bDirLocalToPci ? FOLL_WRITE : 0), // Flags
            pdx->DmaInfo[channel].PageList,    // List of page pointers describing buffer
            NULL                               // List of associated VMAs
            );

    // Release mmap semaphore
    up_read( &current->mm->mmap_sem );

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

            // Unlock user buffer pages that were mapped
            for (i = 0; i < rc; i++)
            {
                put_page( pdx->DmaInfo[channel].PageList[i] );
            }
        }
        kfree( pdx->DmaInfo[channel].PageList );
        return PLX_STATUS_PAGE_LOCK_ERROR;
    }

    DebugPrintf(("Page-locked %d user buffer pages...\n", TotalDescr));

    // Default to 32-bit transfer
    *pbBits64 = FALSE;

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
     * 16 or 32 bytes are added to support rounding up to the next
     * 16 or 32-byte boundary, which is a requirement of the hardware.
     ************************************************************/
    // Calculate descriptor size
    if (*pbBits64)
    {
        SizeDescr = 8 * sizeof(U32);
    }
    else
    {
        SizeDescr = 4 * sizeof(U32);
    }

    // Calculate SGL size
    SglSize = (TotalDescr * SizeDescr) + SizeDescr;

    // Check if a previously allocated buffer can be re-used
    if (pdx->DmaInfo[channel].SglBuffer.pKernelVa != NULL)
    {
        if (pdx->DmaInfo[channel].SglBuffer.Size >= SglSize)
        {
            // Buffer can be re-used, do nothing
            DebugPrintf(("Re-use previously allocated SGL descriptor buffer\n"));
        }
        else
        {
            DebugPrintf(("Release previously allocated SGL descriptor buffer\n"));

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
        DebugPrintf(("Allocate PCI memory for SGL descriptor buffer...\n"));

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
                pdx->DmaInfo[channel].SglBuffer.Size, TotalDescr
                ));
            // Unlock user buffer pages
            for (i = 0; i < TotalDescr; i++)
            {
                put_page( pdx->DmaInfo[channel].PageList[i] );
            }
            kfree( pdx->DmaInfo[channel].PageList );
            return PLX_STATUS_INSUFFICIENT_RES;
        }
    }
    else
    {
        VaSgl = (PLX_UINT_PTR)pdx->DmaInfo[channel].SglBuffer.pKernelVa;
    }

    // Prepare for build of SGL
    LocalAddr = pDma->LocalAddr;

    // Get bus physical address of SGL descriptors
    BusSgl = (U32)pdx->DmaInfo[channel].SglBuffer.BusPhysical;

    // Make sure addresses are aligned on next descriptor boundary
    VaSgl  = (VaSgl + (SizeDescr - 1)) & ~((PLX_UINT_PTR)SizeDescr - 1);
    BusSgl = (BusSgl + (SizeDescr - 1)) & ~((PLX_UINT_PTR)SizeDescr - 1);

    // Store the starting address of the SGL for later return
    BusSglOriginal = BusSgl;

    DebugPrintf((
        "Build SGL at %08xh (%d descriptors)\n",
        BusSglOriginal, TotalDescr
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
            dma_map_page(
                &(pdx->pPciDevice->dev),
                pdx->DmaInfo[channel].PageList[i],
                offset,
                BlockSize,
                pdx->DmaInfo[channel].direction
                );

        // Enable the following to display the parameters of each SGL descriptor
        if (PLX_DEBUG_DISPLAY_SGL_DESCR)
        {
            DebugPrintf((
                "SGL Desc %02d: PCI=%08llX  Loc=%08X  Size=%X (%dB)\n",
                i, BusAddr, LocalAddr, BlockSize, BlockSize
                ));
        }

        // Write PCI address in descriptor
        *(((U32*)VaSgl) + SGL_DESC_IDX_PCI_LOW) = PLX_LE_DATA_32( (U32)BusAddr );

        // Write upper 32-bit of 64-bit PCI address in descriptor
        if (*pbBits64)
        {
            *(((U32*)VaSgl) + SGL_DESC_IDX_PCI_HIGH) =
                           PLX_LE_DATA_32( (U32)(BusAddr >> 32) );
        }

        // Write Local address in descriptor
        *(((U32*)VaSgl) + SGL_DESC_IDX_LOC_ADDR) = PLX_LE_DATA_32( LocalAddr );

        // Write transfer count in descriptor
        *(((U32*)VaSgl) + SGL_DESC_IDX_COUNT) = PLX_LE_DATA_32( BlockSize );

        // Adjust byte count
        BytesRemaining -= BlockSize;

        if (BytesRemaining == 0)
        {
            // Write the last descriptor
            *(((U32*)VaSgl) + SGL_DESC_IDX_NEXT_DESC) =
                PLX_LE_DATA_32(
                    (bDirLocalToPci << 3) | (1 << 1) | (1 << 0)
                    );
        }
        else
        {
            // Calculate address of next descriptor
            BusSgl += SizeDescr;

            // Write next descriptor address
            *(((U32*)VaSgl) + SGL_DESC_IDX_NEXT_DESC) =
                PLX_LE_DATA_32(
                    BusSgl | (bDirLocalToPci << 3) | (1 << 0)
                    );

            // Adjust Local address
            if (pdx->DmaInfo[channel].bConstAddrLocal == FALSE)
            {
                LocalAddr += BlockSize;
            }

            // Adjust virtual address to next descriptor
            VaSgl += SizeDescr;

            // Clear offset
            offset = 0;
        }
    }

    // Return the physical address of the SGL
    *pSglAddress = BusSglOriginal;

    return PLX_STATUS_OK;
}

#endif  // PLX_DMA_SUPPORT




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
        value = PHYS_MEM_READ_8( VaDev );

        // Copy value to user-buffer
        __put_user( value, VaUser );

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
        value = PHYS_MEM_READ_16( VaDev );

        // Copy value to user-buffer
        __put_user( value, VaUser );

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
        value = PHYS_MEM_READ_32( VaDev );

        // Copy value to user-buffer
        __put_user( value, VaUser );

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
        __get_user( value, VaUser );

        // Write value to device
        PHYS_MEM_WRITE_8( VaDev, value );

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
        __get_user( value, VaUser );

        // Write value to device
        PHYS_MEM_WRITE_16( VaDev, value );

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
        __get_user( value, VaUser );

        // Write value to device
        PHYS_MEM_WRITE_32( VaDev, value );

        // Increment pointers
        VaDev++;
        VaUser++;

        // Decrement count
        count -= sizeof(U32);
    }
}
