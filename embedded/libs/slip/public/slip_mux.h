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
    \file slip_mux.h
    \brief FunkenSlip Channel Multiplexer
    
    Demultiplexes FunkenSlip packets transmitted over a single serial channel
*/

#include "kerneltypes.h"
#include "driver.h"
#include "ksemaphore.h"
#include "message.h"
#include "slip.h"

#ifndef __SLIP_MUX_H__
#define __SLIP_MUX_H__

//---------------------------------------------------------------------------
#define SLIP_BUFFER_SIZE    (32)

#define SLIP_RX_MESSAGE_ID    (0xD00D)

//---------------------------------------------------------------------------
typedef void (*Slip_Channel)( Driver *pclDriver_, uint8_t u8Channel_, uint8_t *pu8Data_, uint16_t u16Len_ );

//---------------------------------------------------------------------------
/*!
    Static-class which implements a multiplexed stream of SLIP data over a 
    single interface.
*/
class SlipMux
{
public:
    /*!        
     *  \brief Init

        Attach a driver to the Slip-stream multiplexer and initialize the 
        internal data associated with the module.  
                
        Must be called before any of the other functions in this module 
        are called.
        
        \param pcDriverPath_ Filesystem path to the driver to attach to
        \param u16RxSize_ Size of the RX Buffer to attach to the driver
        \param aucRx_ Pointer to the RX Buffer to attach to the driver
        \param u16TxSize_ Size of the TX Buffer to attach to the driver
        \param aucTx_ Pointer to the TX Buffer to attach to the driver
    */
    static void Init(const char *pcDriverPath_, uint16_t u16RxSize_, uint8_t *aucRx_, uint16_t u16TxSize_, uint8_t *aucTx_);
    
    /*!
        \brief InstallHandler

         Install a slip handler function for the given communication channel.
        
        \param u8Channel_ Channel to attach the handler to
        \param pfHandler_ Pointer to the handler function to attach
    */    
    static void InstallHandler( uint8_t u8Channel_, Slip_Channel pfHandler_ );

    /*!        
        \brief MessageReceive

        Wait for a valid packet to arrive, and call the appropriate handler function
        for the channel the message was attached to.  This is essentially the entry
        point for a thread whose purpose is to service slip Rx data.    
    */    
    static void MessageReceive();

    /*!
        Return the pointer of the current driver used by the SlipMux module
        
        \return Pointer to the current handle owned by SlipMux
    */    
    static Driver *GetDriver(){ return m_pclDriver; }
    
    /*!
        Return the pointer to the message queue attached to the
        slip mux channel.
        
        \return Pointer to the message Queue
    */
    static MessageQueue *GetQueue(){ return m_pclMessageQueue; }

    /*!
        Set the message queue that will receive the notification when the
        slip mux channel has received data.
        
        \param pclMessageQueue_ Pointer to the message queue to use for 
               notification.
    */
    static void SetQueue( MessageQueue *pclMessageQueue_ )
        { m_pclMessageQueue = pclMessageQueue_; }

    
    /*!
        Return the pointer to the SlipMux' Slip object 
        
        \return Pointer to the Slip object
    */
    static Slip *GetSlip(){ return &m_clSlip; }
        
private:
    static MessageQueue *m_pclMessageQueue;
    static Driver *m_pclDriver;
    static Slip_Channel m_apfChannelHandlers[SLIP_CHANNEL_COUNT];    
    static uint8_t m_aucData[SLIP_BUFFER_SIZE];    
    static Semaphore m_clSlipSem;
    static Slip m_clSlip;
};

#endif
