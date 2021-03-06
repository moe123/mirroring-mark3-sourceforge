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

    \file   message.h    

    \brief  Inter-thread communication via message-passing
    
    Embedded systems guru Jack Ganssle once said that without a robust form of
    interprocess communications (IPC), an RTOS is just a toy.  Mark3 implements
    a form of IPC to provide safe and flexible messaging between threads.

    using kernel-managed IPC offers significant benefits over other forms of
    data sharing (i.e. Global variables) in that it avoids synchronization
    issues and race conditions common to the practice.  using IPC also enforces
    a more disciplined coding style that keeps threads decoupled from one
    another and minimizes global data, preventing careless and hard-to-debug
    errors.

    \section MBCreate using Messages, Queues, and the Global Message Pool 

    \code
    
        // Declare a message queue shared between two threads
        MessageQueue my_queue;
        
        int main()
        {
            ...
            // Initialize the message queue
            my_queue.init();
            ...
        }
        
        void Thread1()
        {
            // Example TX thread - sends a message every 10ms
            while(1)
            {
                // Grab a message from the global message pool
                Message *tx_message = GlobalMessagePool::Pop();
                
                // Set the message data/parameters
                tx_message->SetCode( 1234 );
                tx_message->SetData( NULL );
                
                // Send the message on the queue.
                my_queue.Send( tx_message );
                Thread::Sleep(10);
            }
        }
        
        void Thread2()
        {
            while()
            {
                // Blocking receive - wait until we have messages to process
                Message *rx_message = my_queue.Recv();
                
                // Do something with the message data...
                
                // Return back into the pool when done
                GlobalMessagePool::Push(rx_message);                
            }
        }
    \endcode
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "kerneltypes.h"
#include "mark3cfg.h"

#include "ll.h"
#include "ksemaphore.h"

#if KERNEL_USE_MESSAGE

#if KERNEL_USE_TIMEOUTS
    #include "timerlist.h"
#endif

//---------------------------------------------------------------------------
/*!
 *  Class to provide message-based IPC services in the kernel.
 */
class Message : public LinkListNode
{
public:

    void* operator new (size_t sz, void* pv) { return (Message*)pv; };

    /*!
     *  \brief Init
     *
     *  Initialize the data and code in the message.
     */
    void Init() { ClearNode(); m_pvData = NULL; m_u16Code = 0; }
    
    /*!
     *  \brief SetData
     *
     *  Set the data pointer for the message before transmission.
     *  
     *  \param pvData_ Pointer to the data object to send in the message
     */
    void SetData( void *pvData_ ) { m_pvData = pvData_; }
    
    /*!
     *  \brief GetData
     *
     *  Get the data pointer stored in the message upon receipt
     *  
     *  \return Pointer to the data set in the message object
     */
    void *GetData() { return m_pvData; }
    
    /*!
     *  \brief SetCode
     *
     *  Set the code in the message before transmission
     *  
     *  \param u16Code_ Data code to set in the object
     */
    void SetCode( uint16_t u16Code_ ) { m_u16Code = u16Code_; }
    
    /*!
     *  \brief GetCode
     *
     *  Return the code set in the message upon receipt
     *  
     *  \return user code set in the object
     */
    uint16_t GetCode() { return m_u16Code; }
private:

    //! Pointer to the message data
    void *m_pvData;
    
    //! Message code, providing context for the message
    uint16_t m_u16Code;
};


//---------------------------------------------------------------------------
/*!
 *  Implements a list of message objects
 */
class MessagePool
{
public:
    /*!
     *  \brief Init
     *
     *  Initialize the message queue prior to use
     */
    void Init();

    /*!
     *  \brief Push
     *
     *  Return a previously-claimed message object back to the queue.
     *  used once the message has been processed by a receiver.
     *
     *  \param pclMessage_ Pointer to the Message object to return back to
     *         the queue
     */
    void Push( Message *pclMessage_ );

    /*!
     *  \brief Pop
     *
     *  Pop a message from the queue, returning it to the user to be
     *  popu32ated before sending by a transmitter.
     *
     *  \return Pointer to a Message object
     */
    Message *Pop();

    /*!
     * \brief GetHead
     *
     * Return a pointer to the first element in the message list
     *
     * \return
     */
    Message *GetHead();

private:

    //! Linked list used to manage the Message objects
    DoubleLinkList m_clList;
};

//---------------------------------------------------------------------------
/*!
 *  Implements a list of message objects shared between all threads.
 */
class GlobalMessagePool
{
public:
    /*!
     *  \brief Init
     *
     *  Initialize the message queue prior to use
     */
    static void Init(); 
    
    /*!
     *  \brief Push
     *
     *  Return a previously-claimed message object back to the global queue.
     *  used once the message has been processed by a receiver.
     *  
     *  \param pclMessage_ Pointer to the Message object to return back to 
     *         the global queue
     */
    static void Push( Message *pclMessage_ );
    
    /*!
     *  \brief Pop
     *
     *  Pop a message from the global queue, returning it to the user to be 
     *  popu32ated before sending by a transmitter.
     *  
     *  \return Pointer to a Message object
     */
    static Message *Pop();

    /*!
     * \brief GetHead
     *
     * Return a pointer to the first element in the message list
     *
     * \return Pointer to head message element, or NULL if empty
     */
    static Message *GetHead();

    /*!
     * \brief GetPool
     *
     * Get the pointer to the underlying message pool object
     *
     * \return Pointer to message pool.
     */
    static MessagePool *GetPool();

private:
    //! Array of message objects that make up the message pool
    static Message m_aclMessagePool[GLOBAL_MESSAGE_POOL_SIZE];

    static MessagePool m_clPool;
};

//---------------------------------------------------------------------------
/*!
 *  List of messages, used as the channel for sending and receiving messages
 *  between threads.
 */
class MessageQueue
{
public:

    void* operator new (size_t sz, void* pv) { return (MessageQueue*)pv; };

    /*!
     *  \brief Init
     *
     *  Initialize the message queue prior to use.
     */
    void Init();        
    
    /*!        
     *  \brief Receive
     *
     *  Receive a message from the message queue.  If the message queue
     *  is empty, the thread will block until a message is available.
     *  
     *  \return Pointer to a message object at the head of the queue
     */
    Message *Receive();
    
#if KERNEL_USE_TIMEOUTS
    /*!        
     *  \brief Receive
     *
     *  Receive a message from the message queue.  If the message queue
     *  is empty, the thread will block until a message is available for
     *  the duration specified.  If no message arrives within that 
     *  duration, the call will return with NULL.
     *  
     *  \param u32TimeWaitMS_ The amount of time in ms to wait for a
     *          message before timing out and unblocking the waiting thread.
     *  
     *  \return Pointer to a message object at the head of the queue or
     *          NULL on timeout.
     */
    Message *Receive( uint32_t u32TimeWaitMS_ );
#endif  
    
    /*!
     *  \brief Send
     *
     *  Send a message object into this message queue.  Will un-block the 
     *  first waiting thread blocked on this queue if that occurs.
     *  
     *  \param pclSrc_ Pointer to the message object to add to the queue
     */
    void Send( Message *pclSrc_ );
    
    /*!
     *  \brief GetCount
     *
     *  Return the number of messages pending in the "receive" queue.
     *  
     *  \return Count of pending messages in the queue.
     */
    uint16_t GetCount();
private:

#if KERNEL_USE_TIMEOUTS
    /*!
     * \brief Receive_i
     *
     * Internal function used to abstract timed and un-timed Receive calls.
     *
     * \param u32TimeWaitMS_ Time (in ms) to block, 0 for un-timed call.
     *
     * \return Pointer to a message, or 0 on timeout.
     */
    Message *Receive_i( uint32_t u32TimeWaitMS_ );
#else
    /*!
     * \brief Receive_i
     *
     * Internal function used to abstract Receive calls.
     *
     * \return Pointer to a message.
     */
    Message *Receive_i( void );
#endif

    //! Counting semaphore used to manage thread blocking
    Semaphore m_clSemaphore;
    
    //! List object used to store messages
    DoubleLinkList m_clLinkList;
};

#endif //KERNEL_USE_MESSAGE

#endif
