/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012-2015 Funkenstein Software Consulting, all rights reserved.
See license.txt for more information
===========================================================================*/
/*!
	\file writebuf16.cpp
	
	\brief 16 bit circular buffer implementation with callbacks.
*/

#include "kerneltypes.h"
#include "writebuf16.h"
#include "kerneldebug.h"
#include "threadport.h"

#if KERNEL_USE_DEBUG && !KERNEL_AWARE_SIMULATION

//---------------------------------------------------------------------------
void WriteBuffer16::WriteData( uint16_t *pu16Buf_, uint16_t u16Len_ )
{
	uint16_t *apu16Buf[1];
    uint16_t au16Len[1];
	
	apu16Buf[0] = pu16Buf_;
    au16Len[0] = u16Len_;
	
    WriteVector( apu16Buf, au16Len, 1 );
}

//---------------------------------------------------------------------------
void WriteBuffer16::WriteVector( uint16_t **ppu16Buf_, uint16_t *pu16Len_, uint8_t u8Count_ )
{
	uint16_t u16TempHead;	
	uint8_t i;
	uint8_t j;
	uint16_t u16TotalLen = 0;
    bool bCallback = false;
    bool bRollover = false;
    // Update the head pointer synchronously, using a small
	// critical section in order to provide thread safety without
	// compromising on responsiveness by adding lots of extra
	// interrupt latency.
	
	CS_ENTER();
	
	u16TempHead = m_u16Head;
	{		
		for (i = 0; i < u8Count_; i++)
		{
            u16TotalLen += pu16Len_[i];
		}
		m_u16Head = (u16TempHead + u16TotalLen) % m_u16Size;
	}	
	CS_EXIT();
	
	// Call the callback if we cross the 50% mark or rollover 
	if (m_u16Head < u16TempHead)
	{
		if (m_pfCallback)
		{
			bCallback = true;
			bRollover = true;
		}
	}
	else if ((u16TempHead < (m_u16Size >> 1)) && (m_u16Head >= (m_u16Size >> 1)))
	{
		// Only trigger the callback if it's non-null
		if (m_pfCallback)
		{
			bCallback = true;
		}
	}	
	
	// Are we going to roll-over?
	for (j = 0; j < u8Count_; j++)
	{
        uint16_t u16SegmentLength = pu16Len_[j];
		if (u16SegmentLength + u16TempHead >= m_u16Size)
		{	
			// We need to two-part this... First part: before the rollover
			uint16_t u16TempLen;
			uint16_t *pu16Tmp = &m_pu16Data[ u16TempHead ];
			uint16_t *pu16Src = ppu16Buf_[j];
			u16TempLen = m_u16Size - u16TempHead;
			for (i = 0; i < u16TempLen; i++)
			{
				*pu16Tmp++ = *pu16Src++;
			}

			// Second part: after the rollover
			u16TempLen = u16SegmentLength - u16TempLen;
			pu16[A-Z]mp = m_pu16Data;
			for (i = 0; i < u16TempLen; i++)
			{		
				*pu16Tmp++ = *pu16Src++;
			}
		}
		else
		{	
			// No rollover - do the copy all at once.
			uint16_t *pu16Src = ppu16Buf_[j];
			uint16_t *pu16Tmp = &m_pu16Data[ u16TempHead ];
			for (uint16_t i = 0; i < u16SegmentLength; i++)
			{		
				*pu16Tmp++ = *pu16Src++;
			}
		}
	}


	// Call the callback if necessary
	if (bCallback)
	{
		if (bRollover)
		{
			// Rollover - process the back-half of the buffer
			m_pfCallback( &m_pu16Data[ m_u16Size >> 1], m_u16Size >> 1 );
		} 
		else 
		{
			// 50% point - process the front-half of the buffer
			m_pfCallback( m_pu16Data, m_u16Size >> 1);
		}
	}
}

#endif
