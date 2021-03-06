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

    \file   kerneltimer.cpp

    \brief  Kernel Timer Implementation for ARM Cortex-M4
*/

#include "kerneltypes.h"
#include "kerneltimer.h"
#include "threadport.h"

#include "m3_core_cm4.h"
//---------------------------------------------------------------------------
void KernelTimer::Config(void)
{
       
}

//---------------------------------------------------------------------------
void KernelTimer::Start(void)
{    
    // Barely higher priority than the SVC and PendSV interrupts.
    uint8_t u8Priority = (uint8_t)((1 << __NVIC_PRIO_BITS) - 2);

    M3_SysTick_Config(TIMER_FREQ); // 1KHz fixed clock...
    M3_NVIC_SetPriority(M3_SYSTICK_IRQn, u8Priority);
    M3_NVIC_EnableIRQ(M3_SYSTICK_IRQn);
}

//---------------------------------------------------------------------------
void KernelTimer::Stop(void)
{
    SysTick->CTRL = ~SysTick_CTRL_ENABLE_Msk;
}

//---------------------------------------------------------------------------
uint16_t KernelTimer::Read(void)
{
    // Not implemented in this port
    return 0;
}

//---------------------------------------------------------------------------
uint32_t KernelTimer::SubtractExpiry(uint32_t u32Interval_)
{
    return 0;
}

//---------------------------------------------------------------------------
uint32_t KernelTimer::TimeToExpiry(void)
{
    return 0;
}

//---------------------------------------------------------------------------
uint32_t KernelTimer::GetOvertime(void)
{
    return 0;
}

//---------------------------------------------------------------------------
uint32_t KernelTimer::SetExpiry(uint32_t u32Interval_)
{    
    return 0;
}

//---------------------------------------------------------------------------
void KernelTimer::ClearExpiry(void)
{
}

//-------------------------------------------------------------------------
uint8_t KernelTimer::DI(void)
{
    return 0;
}

//---------------------------------------------------------------------------
void KernelTimer::EI(void)
{    
    KernelTimer::RI(0);
}

//---------------------------------------------------------------------------
void KernelTimer::RI(bool bEnable_)
{
}

//---------------------------------------------------------------------------
