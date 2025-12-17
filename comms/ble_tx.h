/*************************************************************************************************/
/*!
 *  \file   ble_tx.h
 *
 *  \brief  BLE Transmit Task for sending workout events.
 *
 *  This task receives workout events from the Control task and sends them
 *  to the connected ESP32 via BLE. If BLE is disconnected, events are
 *  forwarded to the storage queue for offline buffering.
 */
/*************************************************************************************************/

#ifndef COMMS_BLE_TX_H
#define COMMS_BLE_TX_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "workout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Event queue from Control task to BLE TX task */
extern QueueHandle_t g_eventQueue;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize the BLE TX module and create the event queue.
 *
 *  \return true if successful, false otherwise.
 */
/*************************************************************************************************/
bool BleTx_Init(void);

/*************************************************************************************************/
/*!
 *  \brief  Start the BLE TX task.
 *
 *  \return true if task started successfully, false otherwise.
 */
/*************************************************************************************************/
bool BleTx_StartTask(void);

/*************************************************************************************************/
/*!
 *  \brief  Send a workout event (called by Control task).
 *
 *  \param  pEvent  Pointer to event to send.
 *
 *  \return true if event queued successfully, false otherwise.
 */
/*************************************************************************************************/
bool BleTx_SendEvent(const WorkoutEvent_t *pEvent);

/*************************************************************************************************/
/*!
 *  \brief  Flush any buffered events when BLE reconnects.
 *
 *  \return Number of events flushed.
 */
/*************************************************************************************************/
uint8_t BleTx_FlushBuffer(void);

/*************************************************************************************************/
/*!
 *  \brief  BLE TX Task function.
 *
 *  \param  pvParameters  Task parameters (unused).
 */
/*************************************************************************************************/
void BleTxTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* COMMS_BLE_TX_H */

