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
 *      07-01-12 : PLX SDK v7.00
 *
 ******************************************************************************/


#include <linux/delay.h>
#include <linux/ioport.h>
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

    PLX_PCI_REG_READ(
        pRegData->pdx,
        (U16)pRegData->offset,
        &RegValue
        );

    RegValue |= pRegData->BitsToSet;
    RegValue &= ~(pRegData->BitsToClear);

    PLX_PCI_REG_WRITE(
        pRegData->pdx,
        (U16)pRegData->offset,
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
    pIntData->Source_Ints     = pIntData->pdx->Source_Ints;
    pIntData->Source_Doorbell = pIntData->pdx->Source_Doorbell;

    // Clear interrupt flags
    pIntData->pdx->Source_Ints     = INTR_TYPE_NONE;
    pIntData->pdx->Source_Doorbell = 0;

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
                "DPC signaling wait object (%p)\n",
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
