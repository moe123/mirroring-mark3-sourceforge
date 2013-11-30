/*===========================================================================
     _____        _____        _____        _____
 ___|    _|__  __|_    |__  __|__   |__  __| __  |__  ______
|    \  /  | ||    \      ||     |     ||  |/ /     ||___   |
|     \/   | ||     \     ||     \     ||     \     ||___   |
|__/\__/|__|_||__|\__\  __||__|\__\  __||__|\__\  __||______|
    |_____|      |_____|      |_____|      |_____|

--[Mark3 Realtime Platform]--------------------------------------------------

Copyright (c) 2012 Funkenstein Software Consulting, all rights reserved.
See license.txt for more information
===========================================================================*/
/*!

    \file   atomic.h

    \brief  Basic Atomic Operations
*/

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include "kerneltypes.h"
#include "mark3cfg.h"
#include "threadport.h"

class Atomic
{
public:
    /*!
     * \brief Set Set a variable to a given value in an uninterruptable operation
     * \param pucSource_ Pointer to a variable to set the value of
     * \param ucVal_ New value to set in the variable
     * \return Previously-set value
     */
    static K_UCHAR Set( K_UCHAR *pucSource_, K_UCHAR ucVal_ );

    /*!
     * \brief Add Add a value to a variable in an uninterruptable operation
     * \param pucSource_ Pointer to a variable
     * \param ucVal_ Value to add to the variable
     * \return Previously-held value in pucSource_
     */
    static K_UCHAR Add( K_UCHAR *pucSource_, K_UCHAR ucVal_ );

    /*!
     * \brief Sub Subtract a value from a variable in an uninterruptable operation
     * \param pucSource_ Pointer to a variable
     * \param ucVal_ Value to subtract from the variable
     * \return Previously-held value in pucSource_
     */
    static K_UCHAR Sub( K_UCHAR *pucSource_, K_UCHAR ucVal_ );

    /*!
     * \brief TestAndSet Test to see if a variable is set, and set it if
     *        is not already set.  This is an uninterruptable operation.
     *
     *        If the value is false, set the variable to true, and return
     *        the previously-held value.
     *
     *        If the value is already true, return true.
     *
     * \param pbLock Pointer to a value to test against.  This will always
     *        be set to "true" at the end of a call to TestAndSet.
     *
     * \return true - Lock value was "true" on entry, false - Lock was set
     */
    static K_BOOL TestAndSet( K_BOOL *pbLock );
};

#endif //__ATOMIC_H__
