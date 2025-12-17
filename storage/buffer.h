/*************************************************************************************************/
/*!
 *  \file   buffer.h
 *
 *  \brief  Circular buffer for offline workout event storage.
 *
 *  This module provides a RAM-based circular buffer for storing workout events
 *  when BLE is disconnected. Events are flushed when connection is restored.
 */
/*************************************************************************************************/

#ifndef STORAGE_BUFFER_H
#define STORAGE_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "workout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Constants
**************************************************************************************************/

#define BUFFER_MAX_EVENTS   16  /* Maximum events to buffer offline */

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize the event buffer.
 */
/*************************************************************************************************/
void Buffer_Init(void);

/*************************************************************************************************/
/*!
 *  \brief  Push an event to the buffer.
 *
 *  \param  pEvent  Pointer to event to store.
 *
 *  \return true if stored, false if buffer full (oldest event overwritten).
 */
/*************************************************************************************************/
bool Buffer_Push(const WorkoutEvent_t *pEvent);

/*************************************************************************************************/
/*!
 *  \brief  Pop an event from the buffer (FIFO).
 *
 *  \param  pEvent  Pointer to receive event data.
 *
 *  \return true if event retrieved, false if buffer empty.
 */
/*************************************************************************************************/
bool Buffer_Pop(WorkoutEvent_t *pEvent);

/*************************************************************************************************/
/*!
 *  \brief  Get the number of events currently buffered.
 *
 *  \return Number of buffered events.
 */
/*************************************************************************************************/
uint8_t Buffer_GetCount(void);

/*************************************************************************************************/
/*!
 *  \brief  Check if buffer is empty.
 *
 *  \return true if empty, false otherwise.
 */
/*************************************************************************************************/
bool Buffer_IsEmpty(void);

/*************************************************************************************************/
/*!
 *  \brief  Clear all buffered events.
 */
/*************************************************************************************************/
void Buffer_Clear(void);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_BUFFER_H */
