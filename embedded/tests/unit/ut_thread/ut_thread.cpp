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

//---------------------------------------------------------------------------

#include "kerneltypes.h"
#include "kernel.h"
#include "../ut_platform.h"
#include "thread.h"
#include "ksemaphore.h"
#include "kernelprofile.h"
#include "profile.h"
#include "kerneltimer.h"
#include "driver.h"
#include "memutil.h"
//===========================================================================
// Local Defines
//===========================================================================
#define TEST_STACK_SIZE     (224)
static K_WORD aucStack1[TEST_STACK_SIZE];
static K_WORD aucStack2[TEST_STACK_SIZE];
static K_WORD aucStack3[TEST_STACK_SIZE];

static Thread clThread1;
static Thread clThread2;
static Thread clThread3;

static Semaphore clSem1;
static Semaphore clSem2;

static volatile uint32_t u32RR1;
static volatile uint32_t u32RR2;
static volatile uint32_t u32RR3;

//===========================================================================
static void ThreadEntryPoint1(void *unused_)
{
    while(1)
    {
        clSem2.Pend(); // block until the test thread kicks u16
        clSem1.Post();
    }

    unused_ = unused_;
}

//===========================================================================
// Define Test Cases Here
//===========================================================================
TEST(ut_threadcreate)
{
    // Test point - Create a thread, verify that the thread actually starts.
    clSem1.Init(0, 1);
    clSem2.Init(0,1);

    // Initialize our thread
    clThread1.Init(aucStack1, TEST_STACK_SIZE, 7, ThreadEntryPoint1, NULL);

    // Start the thread (threads are created in the stopped state)
    clThread1.Start();

    // Poke the thread using a semaphore, verify it's working
    clSem2.Post();
    clSem1.Pend(10);

    // Ensure that the semaphore was posted before we got to the 10ms timeout
    EXPECT_FALSE(g_pclCurrent->GetExpired());
}
TEST_END

//===========================================================================
TEST(ut_threadstop)
{
    // Test point - stop and restart a thread
    clThread1.Stop();
    Thread::Sleep(10);
    clThread1.Start();

    // Poke the thread using a semaphore, verify it's still responding
    clSem2.Post();
    clSem1.Pend(10);

    EXPECT_FALSE(g_pclCurrent->GetExpired());
}
TEST_END

//===========================================================================
TEST(ut_threadexit)
{
    // Test point - force a thread exit; ensure it doesn't respond once
    // it's un-scheduled.
    clThread1.Exit();
    clSem2.Post();
    clSem1.Pend(10);

    EXPECT_TRUE(g_pclCurrent->GetExpired());
}
TEST_END

//===========================================================================
static ProfileTimer clProfiler1;
static void ThreadSleepEntryPoint(void *unused_)
{
    unused_ = unused_;

    // Thread will sleep for various intervals, synchronized
    // to semaphore-based IPC.
    clSem1.Pend();
    Thread::Sleep(5);
    clSem2.Post();

    clSem1.Pend();
    Thread::Sleep(50);
    clSem2.Post();

    clSem1.Pend();
    Thread::Sleep(500);
    clSem2.Post();

    // Exit this thread.
    Scheduler::GetCurrentThread()->Exit();
}

//===========================================================================
TEST(ut_threadsleep)
{
    Profiler::Init();
    Profiler::Start();

    // Start another thread, which sleeps for a various length of time
    clSem1.Init(0, 1);
    clSem2.Init(0, 1);

    // Initialize our thread
    clThread1.Init(aucStack1, TEST_STACK_SIZE, 7, ThreadSleepEntryPoint, NULL);

    // Start the thread (threads are created in the stopped state)
    clThread1.Start();

    clProfiler1.Init();

    clProfiler1.Start();
    clSem1.Post();
    clSem2.Pend();
    clProfiler1.Stop();


    EXPECT_GTE( (clProfiler1.GetCurrent() * CLOCK_DIVIDE), (SYSTEM_FREQ / 200));
    EXPECT_LTE( (clProfiler1.GetCurrent() * CLOCK_DIVIDE), (SYSTEM_FREQ / 200) + (SYSTEM_FREQ / 200) );

    clSem1.Post();
    clProfiler1.Start();
    clSem2.Pend();
    clProfiler1.Stop();

    EXPECT_GTE( (clProfiler1.GetCurrent() * CLOCK_DIVIDE), SYSTEM_FREQ / 20 );
    EXPECT_LTE( (clProfiler1.GetCurrent() * CLOCK_DIVIDE), (SYSTEM_FREQ / 20) + (SYSTEM_FREQ / 200));


    clSem1.Post();
    clProfiler1.Start();
    clSem2.Pend();
    clProfiler1.Stop();

    EXPECT_GTE( (clProfiler1.GetCurrent() * CLOCK_DIVIDE), SYSTEM_FREQ / 2 );
    EXPECT_LTE( (clProfiler1.GetCurrent() * CLOCK_DIVIDE), (SYSTEM_FREQ / 2) + (SYSTEM_FREQ / 200) );

    Profiler::Stop();
}
TEST_END

//===========================================================================
void RR_EntryPoint(void *value_)
{
    volatile uint32_t *pu32Value = (uint32_t*)value_;
    while(1)
    {
        (*pu32Value)++;
    }
}

//===========================================================================
TEST(ut_roundrobin)
{
    uint32_t u32Avg;
    uint32_t u32Max;
    uint32_t u32Min;
    uint32_t u32Range;

    // Create three threads that only increment counters, and keep them at
    // the same priority in order to test the roundrobin functionality of
    // the scheduler
    clThread1.Init( aucStack1, TEST_STACK_SIZE, 1, RR_EntryPoint, (void*)&u32RR1);
    clThread2.Init( aucStack2, TEST_STACK_SIZE, 1, RR_EntryPoint, (void*)&u32RR2);
    clThread3.Init( aucStack3, TEST_STACK_SIZE, 1, RR_EntryPoint, (void*)&u32RR3);

    u32RR1 = 0;
    u32RR2 = 0;
    u32RR3 = 0;

    // Adjust thread priority before starting test threads to ensure
    // they all start at the same time (when we hit the 1 second sleep)
    Scheduler::GetCurrentThread()->SetPriority(2);
    clThread1.Start();
    clThread2.Start();
    clThread3.Start();

    Thread::Sleep(5000);

    // When the sleep ends, this will preempt the thread in progress,
    // allowing u16 to stop them, and drop priority.
    clThread1.Stop();
    clThread2.Stop();
    clThread3.Stop();
    Scheduler::GetCurrentThread()->SetPriority(1);

    // Compare the three counters - they should be nearly identical
    if (u32RR1 > u32RR2)
    {
        u32Max = u32RR1;
    }
    else
    {
        u32Max = u32RR2;
    }
    if (u32Max < u32RR3)
    {
        u32Max = u32RR3;
    }

    if (u32RR1 < u32RR2)
    {
        u32Min = u32RR1;
    }
    else
    {
        u32Min = u32RR2;
    }
    if (u32Min > u32RR3)
    {
        u32Min = u32RR3;
    }
    u32Range = u32Max - u32Min;
    u32Avg = (u32RR1 + u32RR2 + u32RR3) / 3;

    // Max-Min delta should not exceed 1% of average for this simple test
    EXPECT_LT( u32Range, u32Avg / 100);

    // Make sure none of the component values are 0
    EXPECT_FAIL_EQUALS( u32RR1, 0 );
    EXPECT_FAIL_EQUALS( u32RR2, 0 );
    EXPECT_FAIL_EQUALS( u32RR3, 0 );

}
TEST_END

//===========================================================================
TEST(ut_quanta)
{
    uint32_t u32Avg;
    uint32_t u32Max;
    uint32_t u32Min;
    uint32_t u32Range;

    // Create three threads that only increment counters - similar to the
    // previous test.  However, modify the thread quanta such that each thread
    // will get a different proportion of the CPU cycles.
    clThread1.Init( aucStack1, TEST_STACK_SIZE, 1, RR_EntryPoint, (void*)&u32RR1);
    clThread2.Init( aucStack2, TEST_STACK_SIZE, 1, RR_EntryPoint, (void*)&u32RR2);
    clThread3.Init( aucStack3, TEST_STACK_SIZE, 1, RR_EntryPoint, (void*)&u32RR3);

    u32RR1 = 0;
    u32RR2 = 0;
    u32RR3 = 0;

    // Adjust thread priority before starting test threads to ensure
    // they all start at the same time (when we hit the 1 second sleep)
    Scheduler::GetCurrentThread()->SetPriority(2);

    // Set a different execution quanta for each thread
    clThread1.SetQuantum(3);
    clThread2.SetQuantum(6);
    clThread3.SetQuantum(9);

    clThread1.Start();
    clThread2.Start();
    clThread3.Start();

    Thread::Sleep(1800);

    // When the sleep ends, this will preempt the thread in progress,
    // allowing u16 to stop them, and drop priority.
    clThread1.Stop();
    clThread2.Stop();
    clThread3.Stop();
    Scheduler::GetCurrentThread()->SetPriority(1);

    // Test point - make sure that Q3 > Q2 > Q1
    EXPECT_GT( u32RR2, u32RR1 );
    EXPECT_GT( u32RR3, u32RR2 );

    // scale the counters relative to the largest value, and compare.
    u32RR1 *= 3;
    u32RR2 *= 3;
    u32RR2 = (u32RR2 + 1) / 2;

    // After scaling, they should be nearly identical (well, close at least)
    if (u32RR1 > u32RR2)
    {
        u32Max = u32RR1;
    }
    else
    {
        u32Max = u32RR2;
    }
    if (u32Max < u32RR3)
    {
        u32Max = u32RR3;
    }

    if (u32RR1 < u32RR2)
    {
        u32Min = u32RR1;
    }
    else
    {
        u32Min = u32RR2;
    }
    if (u32Min > u32RR3)
    {
        u32Min = u32RR3;
    }
    u32Range = u32Max - u32Min;
    u32Avg = (u32RR1 + u32RR2 + u32RR3) / 3;

#if KERNEL_TIMERS_TICKLESS
    // Max-Min delta should not exceed 5% of average for this test
    EXPECT_LT( u32Range, u32Avg / 20);
#else
    // Max-Min delta should not exceed 20% of average for this test -- tick-based timers
    // are coarse, and prone to thread preference due to phase.
    EXPECT_LT( u32Range, u32Avg / 5);
#endif


    // Make sure none of the component values are 0
    EXPECT_FAIL_EQUALS( u32RR1, 0 );
    EXPECT_FAIL_EQUALS( u32RR2, 0 );
    EXPECT_FAIL_EQUALS( u32RR3, 0 );
}
TEST_END

//===========================================================================
// Test Whitelist Goes Here
//===========================================================================
TEST_CASE_START
  TEST_CASE(ut_threadcreate),
  TEST_CASE(ut_threadstop),
  TEST_CASE(ut_threadexit),
  TEST_CASE(ut_threadsleep),
  TEST_CASE(ut_roundrobin),
  TEST_CASE(ut_quanta),
TEST_CASE_END
