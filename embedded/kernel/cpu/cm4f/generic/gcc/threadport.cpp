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

    \file   threadport.cpp   

    \brief  ARM Cortex-M4 Multithreading; FPU Enabled.

*/

#include "kerneltypes.h"
#include "mark3cfg.h"
#include "thread.h"
#include "threadport.h"
#include "kernelswi.h"
#include "kerneltimer.h"
#include "timerlist.h"
#include "quantum.h"

#include "m3_core_cm4.h"

//---------------------------------------------------------------------------
#if KERNEL_USE_IDLE_FUNC
# error "KERNEL_USE_IDLE_FUNC not supported in this port"
#endif

//---------------------------------------------------------------------------
static void ThreadPort_StartFirstThread( void ) __attribute__ (( naked ));
extern "C" {
    void SVC_Handler( void ) __attribute__ (( naked ));
    void PendSV_Handler( void ) __attribute__ (( naked ));
    void SysTick_Handler( void );
}

//---------------------------------------------------------------------------
volatile uint32_t g_ulCriticalCount;

//---------------------------------------------------------------------------
/*
    1) Setting up the thread stacks

    Stack consists of 2 separate frames mashed together.
    a) Exception Stack Frame

    Contains the 8 registers the CPU pushes/pops to/from the stack on execution
    of an exception:

    [ XPSR ]
    [ PC   ]
    [ LR   ]
    [ R12  ]
    [ R3   ]
    [ R2   ]
    [ R1   ]
    [ R0   ]

    XPSR - Needs to be set to 0x01000000; the "T" bit (thumb) must be set for
           any thread executing on an ARMv6-m processor
    PC - Should be set with the initial entry point for the thread
    LR - The base link register.  We can leave this as 0, and set to 0xD on
         first context switch to tell the CPU to resume execution using the
         stack pointer held in the PSP as the regular stack.

    This is done by the CPU automagically- this format is part of the
    architecture, and there's nothing we can do to change or modify it.

    b) "Other" Register Context

    [ R11   ]
    ...
    [ R4    ]

    These are the other GP registers that need to be backed up/restored on a
    context switch, but aren't by default on a CM0 exception.  If there were
    any additional hardware registers to back up, then we'd also have to
    include them in this part of the context.

    These all need to be manually pushed to the stack on stack creation, and
    puhsed/pop as part of a normal context switch.
*/
void ThreadPort::InitStack(Thread *pclThread_)
{
    uint32_t *pu32Stack;
    uint32_t *pu32Temp;
    uint32_t u32Addr;
    uint16_t i;

    // Get the entrypoint for the thread
    u32Addr = (uint32_t)(pclThread_->m_pfEntryPoint);

    // Get the top-of-stack pointer for the thread
    pu32Stack = (uint32_t*)pclThread_->m_pwStackTop;

    // Initialize the stack to all FF's to aid in stack depth checking
    pu32Temp = (uint32_t*)pclThread_->m_pwStack;
    for (i = 0; i < pclThread_->m_u16StackSize / sizeof(uint32_t); i++)
    {
        pu32Temp[i] = 0xFFFFFFFF;
    }

    PUSH_TO_STACK(pu32Stack, 0);                // We need one word of padding, apparently...
    
    //-- Simulated Exception Stack Frame --
    PUSH_TO_STACK(pu32Stack, 0x01000000);    // XSPR
    PUSH_TO_STACK(pu32Stack, u32Addr);       // PC
    PUSH_TO_STACK(pu32Stack, 0);
    PUSH_TO_STACK(pu32Stack, 0x12);
    PUSH_TO_STACK(pu32Stack, 0x3);
    PUSH_TO_STACK(pu32Stack, 0x2);
    PUSH_TO_STACK(pu32Stack, 0x1);
    PUSH_TO_STACK(pu32Stack, (uint32_t)pclThread_->m_pvArg);    // R0 = argument

    //-- Simulated Manually-Stacked Registers --
    PUSH_TO_STACK(pu32Stack, 0xFFFFFFFD); // Default "EXC_RETURN" value -- Thread mode, floating point.
    PUSH_TO_STACK(pu32Stack, 0x11);
    PUSH_TO_STACK(pu32Stack, 0x10);
    PUSH_TO_STACK(pu32Stack, 0x09);
    PUSH_TO_STACK(pu32Stack, 0x08);
    PUSH_TO_STACK(pu32Stack, 0x07);
    PUSH_TO_STACK(pu32Stack, 0x06);
    PUSH_TO_STACK(pu32Stack, 0x05);
    PUSH_TO_STACK(pu32Stack, 0x04);
    pu32Stack++;

    pclThread_->m_pwStackTop = pu32Stack;
}

//---------------------------------------------------------------------------
void Thread_Switch(void)
{
    g_pclCurrent = (Thread*)g_pclNext;
}

//---------------------------------------------------------------------------
void ThreadPort::StartThreads()
{
    KernelSWI::Config();             // configure the task switch SWI
    KernelTimer::Config();           // configure the kernel timer

    Scheduler::SetScheduler(1);      // enable the scheduler
    Scheduler::Schedule();           // run the scheduler - determine the first thread to run

    Thread_Switch();                 // Set the next scheduled thread to the current thread

    KernelTimer::Start();            // enable the kernel timer
    KernelSWI::Start();              // enable the task switch SWI

#if KERNEL_USE_QUANTUM
    Quantum::RemoveThread();
    Quantum::AddThread(g_pclCurrent);
#endif

    SCB->CPACR |= 0x00F00000;        // Enable floating-point

    FPU->FPCCR |= (FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk); // Enable lazy-stacking

    ThreadPort_StartFirstThread();     // Jump to the first thread (does not return)
}

//---------------------------------------------------------------------------
/*
    The same general process applies to starting the kernel as per usual

    We can either:
        1) Simulate a return from an exception manually to start the first
           thread, or..
        2) use a software exception to trigger the first "Context Restore
            /Return from Interrupt" that we have otherwised used to this point.

    For 1), we basically have to restore the whole stack manually, not relying
    on the CPU to do any of this for u16.  That's certainly doable, but not all
    Cortex parts support this (due to other members of the family supporting
    priveleged modes).  So, we will opt for the second option.

    So, to implement a software interrupt to restore our first thread, we will
    use the SVC instruction to generate that exception.

    At the end of thread initialization, we have to do 2 things:

    -Enable exceptions/interrupts
    -Call SVC

    Optionally, we can reset the MSP stack pointer to the top-of-stack;
    load the top-of-stack value from the NVIC's stack offset register.

    (While Mark3 avoids assembler code as much as possible, there are some
    places where it cannot be avoided.  However, we can at least inline it
    in most circumstances.)
*/
void ThreadPort_StartFirstThread( void )
{
    ASM (
        " ldr r0, =0xE000ED08 \n"
        " mov r0, [r0] \n"
        " ldr r1, [r0] \n"
        " msr msp, r1 \n"
        " cpsie i \n"
        " svc 0 \n"
    );
}

//---------------------------------------------------------------------------
/*
    The SVC Call

    This handler has the job of taking the first thread object's stack, and
    restoring the default state data in a way that ensures that the thread
    starts executing when returning from the call.

    We also keep in mind that there's an 8-byte offset from the beginning of
    the thread object to the location of the thread stack pointer.  This 
    offset is a result of the thread object inheriting from the linked-list
    node class, which has 8-bytes of data.  This is stored first in the 
    object, before the first element of the class, which is the "stack top"
    pointer.

    get_thread_stack:
        ; Get the stack pointer for the current thread
        ldr r0, g_pclCurrent
        ldr r1, [r0]
        add r1, #8
        ldr r2, [r1]         ; r2 contains the current stack-top

    load_manually_placed_context_r11_r4:
        ; Handle the bottom 36-bytes of the stack frame
        ; On cortex m3 and up, we can do this in one ldmia instruction.
        ldmia r2!, {r4-r11, r14}

    set_psp:
        ; Since r2 is coincidentally back to where the stack pointer should be,
        ; Set the program stack pointer such that returning from the exception handler
        msr psp, r2

    ** Note - Since we don't care about these registers on init, we could take a shortcut if we wanted to **
    shortcut_init:
        add r2, #32
        msr psp, r2

    set_lr:
        ; Set up the link register such that on return, the code operates in thread mode using the PSP
        ; To do this, we or 0x0D to the value stored in the lr by the exception hardware EXC_RETURN.
        ; Alternately, we could just force lr to be 0xFFFFFFFD (we know that's what we want from the hardware, anyway)
        mov  r0, #0x0D
        mov  r1, lr
        orr r0, r1

    exit_exception:
        ; Return from the exception handler.  The CPU will automagically unstack R0-R3, R12, PC, LR, and xPSR
        ; for u16.  If all goes well, our thread will start execution at the entrypoint, with the user-specified
        ; argument.
        bx r0

        This code is identical to what we need to restore the context, so
        we'll just make it a macro and be done with it.
*/
void SVC_Handler(void)
{
    ASM(
    // Get the pointer to the first thread's stack
    " mov r3, %[CURRENT_THREAD]\n "
    " add r3, #8 \n "
    " ldr r2, [r3] \n "
    // Stack pointer is in r2, start loading registers from the "manually-stacked" set
    " ldmia r2!, {r4-r11, r14} \n "
    // After subtracting R2 by #32 due to stack popping, our PSP is where it
    // needs to be when we return from the exception handler
    " msr psp, r2 \n "
    " isb \n "
    // Return into thread mode, using PSP as the thread's stack pointer
    " bx lr \n "
    : : [CURRENT_THREAD] "r" (g_pclCurrent)
    );
}

//---------------------------------------------------------------------------
/*
    Context Switching:
    
    On ARM Cortex parts, there's dedicated hardware that's used primarily to 
    support RTOS (or RTOS-like) funcationlity.  This functionality includes
    the SysTick timer, and the PendSV Exception.  SysTick is used for the 
    kernel timer (I need to learn how to use it to see whether or not I can
    do tickless timers), while the PendSV exception is used for triggering
    context switches.  In reality, it's a "special SVC" call that's designed
    to be lower-overhead, in that it isn't mux'd with a bunch of other system
    or application functionality.
    
    Alright, so how do we go about actually implementing a context switch here?
    There are a lot of different parts involved, but it essentially comes down
    to 3 steps:
    
    1) Save Context
    2) Swap "current" and "next" thread pointers
    3) Restore Context
    
1) Saving the context.

    !!ToDo -- add documentation about how this works on cortex m4f, especially
    in the context of the floating-point registers, lazy stacking, etc.

2)  Swap threads

    This is the easy part - we just call a function to swap in the thread "current" thread    
    from the "next" thread.
    
3)    Restore Context

    This is more or less identical to what we did when restoring the first context. 
    
*/    
void PendSV_Handler(void)
{    
    ASM(
    // Thread_SaveContext()
    " ldr r1, CURR_ \n"
    " ldr r1, [r1] \n "
    " mov r3, r1 \n "
    " add r3, #8 \n "

    " mrs r2, psp \n "

    // Check to see if the thread was using floating point -- if so, we need to
    // store the remaining registers not automatically handled automagically on
    // entry to the exception handler.
    " tst r14, #0x10\n "
    " it eq \n "
    " vstmdbeq r2!, {s16-s31} \n "

    // And, while r2 is at the bottom of the stack frame, stack r4-r11, lr
    " stmdb r2!, {r4-r11, r14} \n "

    // Store the new top-of-stack value to the current thread object
    " str r2, [r3] \n "

    // Equivalent of Thread_Swap() -- g_pclNext -> g_pclCurrent
    " cpsid i \n "
    " ldr r1, CURR_ \n"
    " ldr r0, NEXT_ \n"
    " ldr r0, [r0] \n"
    " str r0, [r1] \n"
    " cpsie i \n "

    // Get the pointer to the next thread's stack
    " add r0, #8 \n "
    " ldr r2, [r0] \n "

    // Stack pointer is in r2, start loading registers from the "manually-stacked" set
    " ldmia r2!, {r4-r11, r14} \n "

    // Check to see if the thread was using floating point -- if so, we need to
    // store the remaining registers not automatically handled due to lazy stacking
    " tst r14, #0x10\n "
    " it eq \n "
    " vldmiaeq r2!, {s16-s31} \n "

    // After subbing R2 #32 through ldmia/stack popping, our PSP is where it
    // needs to be when we return from the exception handler
    " msr psp, r2 \n "

    // lr contains the proper EXC_RETURN value, we're done with exceptions.
    " bx lr \n "

    // Must be 4-byte aligned.  Also - GNU assembler, I hate you for making me resort to this.
    " NEXT_: .word g_pclNext \n"
    " CURR_: .word g_pclCurrent \n"
    );
}
//---------------------------------------------------------------------------
void SysTick_Handler(void)
{
#if KERNEL_USE_TIMERS
    TimerScheduler::Process();
#endif
#if KERNEL_USE_QUANTUM
    Quantum::UpdateTimer();
#endif

    // Clear the systick interrupt pending bit.
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
}

