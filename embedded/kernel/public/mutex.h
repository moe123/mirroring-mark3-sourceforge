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
=========================================================================== */
/*!

    \file   mutex.h    

    \brief  Mutual exclusion class declaration
    
    Resource locks are implemented using mutual exclusion semaphores (Mutex_t).
    Protected blocks can be placed around any resource that may only be accessed
    by one thread at a time.  If additional threads attempt to access the
    protected resource, they will be placed in a wait queue until the resource
    becomes available.  When the resource becomes available, the thread with
    the highest original priority claims the resource and is activated.
    Priority inheritance is included in the implementation to prevent priority
    inversion.  Always ensure that you claim and release your mutex objects
    consistently, otherwise you may end up with a deadlock scenario that's
    hard to debug.

    \section MInit Initializing

    Initializing a mutex object by calling:

    \code
    clMutex.Init();
    \endcode

    \section MUsage Resource protection example

    \code
    clMutex.Claim();
    ...
    <resource protected block>
    ...
    clMutex.Release();
    \endcode

 */
#ifndef __MUTEX_H_
#define __MUTEX_H_

#include "kerneltypes.h"
#include "mark3cfg.h"

#include "blocking.h"

#if KERNEL_USE_MUTEX

#if KERNEL_USE_TIMEOUTS
#include "timerlist.h"
#endif

//---------------------------------------------------------------------------
/*!
 *  Mutual-exclusion locks, based on BlockingObject.
 */
class Mutex : public BlockingObject
{
public:

    void* operator new (size_t sz, void* pv) { return (Mutex*)pv; };

    ~Mutex();

    /*!
     *  \brief Init
     *
     *  Initialize a mutex object for use - must call this function before using
     *  the object.
     */
    void Init();

    /*!
     *  \brief Claim
     *
     *  Claim the mutex.  When the mutex is claimed, no other thread can claim a
     *  region protected by the object.  If another Thread currently holds the
     *  Mutex when the Claim method is called, that Thread will block until the
     *  current owner of the mutex releases the Mutex.
     *
     *  If the calling Thread's priority is lower than that of a Thread that
     *  currently owns the Mutex object, then the priority of that Thread will
     *  be elevated to that of the highest-priority calling object until the
     *  Mutex is released.  This property is known as "Priority Inheritence"
     *
     *  Note:  A single thread can recursively claim a mutex up to a count of
     *  255.  Attempting to claim a mutex beyond that will cause a kernel panic.
     *
     */
    void Claim();

#if KERNEL_USE_TIMEOUTS

    /*!
     *  \brief Claim
     *
     *  Claim a mutex, with timeout.
     *
     *  \param u32WaitTimeMS_
     *  
     *  \return true - mutex was claimed within the time period specified
     *          false - mutex operation timed-out before the claim operation.
     */
    bool Claim(uint32_t u32WaitTimeMS_);
    
    /*!
     *  \brief WakeMe
     *
     *  Wake a thread blocked on the mutex.  This is an
     *  internal function used for implementing timed mutexes
     *  relying on timer callbacks.  Since these do not have
     *  access to the private data of the mutex and its base
     *  classes, we have to wrap this as a public method - do not
     *  use this for any other purposes.
     *
     *  \param pclOwner_ Thread to unblock from this object.        
     */
    void WakeMe( Thread *pclOwner_ );

#endif

    /*!
     *  \brief Release
     *  
     *  Release the mutex.  When the mutex is released, another object can enter
     *  the mutex-protected region.
     *
     *  If there are Threads waiting for the Mutex to become available, then the
     *  highest priority Thread will be unblocked at this time and will claim
     *  the Mutex lock immediately - this may result in an immediate context
     *  switch, depending on relative priorities.
     *
     *  If the calling Thread's priority was boosted as a result of priority
     *  inheritence, the Thread's previous priority will also be restored at this
     *  time.
     *
     *  Note that if a Mutex is held recursively, it must be Release'd the same
     *  number of times that it was Claim'd before it will be availabel for use
     *  by another Thread.
     *
     */
    void Release();
    
private:

    /*!
     *  \brief WakeNext
     *
     *  Wake the next thread waiting on the Mutex.
     */
    uint8_t WakeNext();
    

#if KERNEL_USE_TIMEOUTS
    /*!
     * \brief Claim_i
     *
     * Abstracts out timed/non-timed mutex claim operations.
     *
     * \param u32WaitTimeMS_ Time in MS to wait, 0 for infinite
     * \return true on successful claim, false otherwise
      */
    bool Claim_i( uint32_t u32WaitTimeMS_ );
#else
    /*!
     * \brief Claim_i
     *
     * Abstraction for mutex claim operations.
     *
     */
    void Claim_i(void);
#endif

    uint8_t m_u8Recurse;    //!< The recursive lock-count when a mutex is claimed multiple times by the same owner
    bool m_bReady;          //!< State of the mutex - true = ready, false = claimed
    uint8_t m_u8MaxPri;     //!< Maximum priority of thread in queue, used for priority inheritence
    Thread *m_pclOwner;     //!< Pointer to the thread that owns the mutex (when claimed)
    
};

#endif //KERNEL_USE_MUTEX

#endif //__MUTEX_H_

