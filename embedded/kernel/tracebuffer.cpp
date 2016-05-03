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
	\file tracebuffer.cpp
	\brief Kernel trace buffer class definition	
*/

#include "kerneltypes.h"
#include "tracebuffer.h"
#include "mark3cfg.h"

#define _CAN_HAS_DEBUG
//--[Autogenerated - Do Not Modify]------------------------------------------
#include "dbg_file_list.h"
#include "buffalogger.h"
#if defined(DBG_FILE)
# error "Debug logging file token already defined!  Bailing."
#else
# define DBG_FILE _DBG___KERNEL_TRACEBUFFER_CPP
#endif

#include "kerneldebug.h"

//--[End Autogenerated content]----------------------------------------------

#if KERNEL_USE_DEBUG && !KERNEL_AWARE_SIMULATION
//---------------------------------------------------------------------------
TraceBufferCallback_t TraceBuffer::m_pfCallback;
uint16_t TraceBuffer::m_u16Index;
uint16_t TraceBuffer::m_u16SyncNumber;
uint16_t TraceBuffer::m_au16Buffer[ (TRACE_BUFFER_SIZE/sizeof(uint16_t)) ];

//---------------------------------------------------------------------------
void TraceBuffer::Init()
{
}

//---------------------------------------------------------------------------
void TraceBuffer::Write( uint16_t *pu16Data_, uint16_t u16Size_ )
{
	// Pipe the data directly to the circular buffer
    uint16_t u16Start;

    // Update the circular buffer index in a critical section. The
    // rest of the operations can take place in any context.
    CS_ENTER();
    uint16_t u16NextIndex;
    u16Start = m_u16Index;
    u16NextIndex = m_u16Index + u16Size_;
    if (u16NextIndex >= (sizeof(m_au16Buffer) / sizeof(uint16_t)) )
    {
        u16NextIndex -= (sizeof(m_au16Buffer) / sizeof(uint16_t));
    }
    m_u16Index = u16NextIndex;
    CS_EXIT();

    // Write the data into the circular buffer.
    uint16_t i;
    bool bCallback = false;
    bool bPingPong = false;
    for (i = 0; i < u16Size_; i++)
    {
        m_au16Buffer[u16Start++] = pu16Data_[i];
        if (u16Start >= (sizeof(m_au16Buffer) / sizeof(uint16_t)) )
        {
            u16Start = 0;
            bCallback = true;
        }
        else if (u16Start == ((sizeof(m_au16Buffer) / sizeof(uint16_t)) / 2))
        {
            bPingPong = true;
            bCallback = true;
        }
    }

    // Done writing - see if there's a 50% or rollover callback
    if (bCallback && m_pfCallback) {
        uint16_t u16Size = (sizeof(m_au16Buffer) / sizeof(uint16_t)) / 2;
        if (bPingPong) {
            m_pfCallback(m_au16Buffer, u16Size, bPingPong);
        } else {
            m_pfCallback(m_au16Buffer + u16Size, u16Size, bPingPong);
        }
    }
}

#endif

