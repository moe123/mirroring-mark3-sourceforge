/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012-2016 Funkenstein Software Consulting, all rights reserved.
See license.txt for more information
===========================================================================*/
/*!
    \file fixed_heap.cpp
    
    Fixed-block-size memory management.  This allows a user to create heaps
    containing multiple lists, each list containing a linked-list of blocks
    that are each the same size.  As a result of the linked-list format, 
    these heaps are very fast - requiring only a linked list pop/push to 
    allocated/free memory.  Array traversal is required to allow for the optimal
    heap to be used.  Blocks are chosen from the first heap with free blocks
    large enough to fulfill the request.
    
    Only simple malloc/free functionlality is supported in this implementation,
    no complex vector-allocate or reallocation functions are supported.
    
    Heaps are protected by critical section, and are thus thread-safe.
    
    When creating a heap, a user supplies an array of heap configuration objects,
    which determines how many objects of what size are available.
        
    The configuration objects are defined from smallest list to largest, 
    the memory to back the heap is supplied as a pointer to a "blob" of memory
    which will be used to create the underlying heap objects that make up the
    heap internal data structures.  This blob must be large enough to contain
    all of the requested heap objects, with all of the additional metadata 
    required to manage the objects.    
    
    Multiple heaps can be created using this library (heaps are not singleton). 
*/

#include "kerneltypes.h"
#include "fixed_heap.h"
#include "threadport.h"

//---------------------------------------------------------------------------
class BlockHeapNode : public LinkListNode
{
friend class BlockHeap;
friend class FixedHeap;
protected:
    BlockHeap *m_clHeap;
};

//---------------------------------------------------------------------------
void *BlockHeap::Create( void *pvHeap_, uint16_t u16Size_, uint16_t u16BlockSize_ )
{
    uint16_t u16NodeCount = u16Size_ / (sizeof(BlockHeapNode) + u16BlockSize_);

    K_ADDR adNode = (K_ADDR)pvHeap_;
    K_ADDR adMaxNode = (K_ADDR)((K_ADDR)pvHeap_ + (K_ADDR)u16Size_);
    m_clList.Init();
    
    // Create a heap (linked-list nodes + byte pool) in the middle of
    // the data blob
    for (uint16_t i = 0; i < u16NodeCount; i++ )
    {
        // Create a pointer back to the source list.  
        BlockHeapNode *pclTemp = (BlockHeapNode*)adNode;
        pclTemp->m_clHeap = this;

        // Add the node to the block list
        m_clList.Add( (LinkListNode*)pclTemp);
        
        // Move the pointer in the pool to point to the next block to allocate
        adNode += (sizeof(BlockHeapNode) + u16BlockSize_);
        
        // Bail if we would be going past the end of the allocated space...
        if (adNode >= adMaxNode)
        {
            break;
        }
    }
    m_u16BlocksFree = u16NodeCount;
    
    // Return pointer to end of heap (usedd for heap-chaining)
    return (void*)adNode;
}

//---------------------------------------------------------------------------
void *BlockHeap::Alloc()
{
    LinkListNode *pclNode = m_clList.GetHead();

    // Return the first node from the head of the list
    if (pclNode)
    {
        m_clList.Remove( pclNode );
        m_u16BlocksFree--;
        
        // Account for block-management metadata
        return (void*)((K_ADDR)pclNode + sizeof(BlockHeapNode));
    }
    
    // Or null, if the heap is empty.
    return 0;
}

//---------------------------------------------------------------------------
void BlockHeap::Free( void* pvData_ )
{
    // Compute the address of the original object (class metadata included)
    LinkListNode *pclNode = (LinkListNode*)((K_ADDR)pvData_ - sizeof(BlockHeapNode));
    
    // Add the object back to the block data pool
    m_clList.Add(pclNode);
    m_u16BlocksFree++;
}

//---------------------------------------------------------------------------
void FixedHeap::Create( void *pvHeap_, HeapConfig *pclHeapConfig_ )
{
    uint16_t i = 0;
    void *pvTemp = pvHeap_;
    while( pclHeapConfig_[i].m_u16BlockSize != 0)
    {
        pvTemp = pclHeapConfig_[i].m_clHeap.Create( pvTemp,
                    ((pclHeapConfig_[i].m_u16BlockSize + sizeof(BlockHeapNode))
                     * pclHeapConfig_[i].m_u16BlockCount),
                     pclHeapConfig_[i].m_u16BlockSize );
        i++;
    }
    m_paclHeaps = pclHeapConfig_;
}

//---------------------------------------------------------------------------
void *FixedHeap::Alloc( uint16_t u16Size_ )
{
    void *pvRet = 0;
    uint16_t i = 0;
    
    // Go through all heaps, trying to find the smallest one that 
    // has a free item to satisfy the allocation
    while (m_paclHeaps[i].m_u16BlockSize != 0)
    {
        CS_ENTER();        
        if ((m_paclHeaps[i].m_u16BlockSize >= u16Size_) && m_paclHeaps[i].m_clHeap.IsFree() )
        {
            // Found a match            
            pvRet = m_paclHeaps[i].m_clHeap.Alloc();        
        }
        CS_EXIT();

        // Return an object if found
        if (pvRet)
        {
            return pvRet;
        }
        i++;
    }
    
    // Or null on no-match
    return pvRet;
}

//---------------------------------------------------------------------------
void FixedHeap::Free( void *pvNode_ )
{
    // Compute the pointer to the block-heap this block belongs to, and
    // return it.
    CS_ENTER();
    BlockHeapNode *pclNode = (BlockHeapNode*)((K_ADDR)pvNode_ - sizeof(BlockHeapNode));
    pclNode->m_clHeap->Free( pvNode_ );
    CS_EXIT();
}


