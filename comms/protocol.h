/*************************************************************************************************/
/*!
 *  \file   protocol.h
 *
 *  \brief  Communication protocol for workout data serialization.
 *
 *  This module handles JSON serialization of workout events for BLE transmission.
 */
/*************************************************************************************************/

#ifndef COMMS_PROTOCOL_H
#define COMMS_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "workout_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**************************************************************************************************
    Constants
  **************************************************************************************************/

#define PROTOCOL_MAX_MSG_LEN 128 /* Maximum serialized message length */

  /**************************************************************************************************
    Function Declarations
  **************************************************************************************************/

  /*************************************************************************************************/
  /*!
   *  \brief  Serialize a workout event to JSON format.
   *
   *  \param  pEvent      Pointer to workout event.
   *  \param  pBuffer     Output buffer for JSON string.
   *  \param  bufLen      Size of output buffer.
   *
   *  \return Number of bytes written, or 0 on error.
   */
  /*************************************************************************************************/
  uint16_t Protocol_SerializeEvent(const WorkoutEvent_t *pEvent, char *pBuffer, uint16_t bufLen);

  /*************************************************************************************************/
  /*!
   *  \brief  Get event type as string.
   *
   *  \param  type    Event type enum.
   *
   *  \return String representation of event type.
   */
  /*************************************************************************************************/
  const char *Protocol_EventTypeToString(EventType_t type);

#ifdef __cplusplus
}
#endif

#endif /* COMMS_PROTOCOL_H */
