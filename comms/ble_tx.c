/*************************************************************************************************/
/*!
 *  \file   ble_tx.c
 *
 *  \brief  BLE Transmit Task implementation - FULLY STATIC ALLOCATION.
 */
/*************************************************************************************************/

#include "ble_tx.h"
#include "ble_manager.h"
#include "protocol.h"
#include "buffer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define BLE_TX_TASK_STACK_SIZE 320
#define BLE_TX_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define EVENT_QUEUE_LENGTH 4

/**************************************************************************************************
  Static Memory for FreeRTOS Objects
**************************************************************************************************/

/* Static queue storage */
static StaticQueue_t s_eventQueueBuffer;
static uint8_t s_eventQueueStorage[EVENT_QUEUE_LENGTH * sizeof(WorkoutEvent_t)];

/* Static task storage */
static StaticTask_t s_bleTxTaskBuffer;
static StackType_t s_bleTxTaskStack[BLE_TX_TASK_STACK_SIZE];

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

QueueHandle_t g_eventQueue = NULL;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

static TaskHandle_t s_bleTxTaskHandle = NULL;
static bool s_wasConnected = false;
static char s_msgBuf[PROTOCOL_MAX_MSG_LEN]; /* Static buffer for serialization */

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
bool BleTx_Init(void)
{
    /* Create event queue using STATIC allocation */
    g_eventQueue = xQueueCreateStatic(
        EVENT_QUEUE_LENGTH,
        sizeof(WorkoutEvent_t),
        s_eventQueueStorage,
        &s_eventQueueBuffer);

    if (g_eventQueue == NULL)
    {
        printf("[BLE_TX] ERROR: Failed to create event queue\n");
        return false;
    }

    printf("[BLE_TX] Event queue initialized (static)\n");
    return true;
}

/*************************************************************************************************/
bool BleTx_StartTask(void)
{
    /* Create task using STATIC allocation */
    s_bleTxTaskHandle = xTaskCreateStatic(
        BleTxTask,
        "BLE_TX",
        BLE_TX_TASK_STACK_SIZE,
        NULL,
        BLE_TX_TASK_PRIORITY,
        s_bleTxTaskStack,
        &s_bleTxTaskBuffer);

    if (s_bleTxTaskHandle == NULL)
    {
        printf("[BLE_TX] ERROR: Failed to create task\n");
        return false;
    }

    printf("[BLE_TX] Task started (static)\n");
    return true;
}

/*************************************************************************************************/
bool BleTx_SendEvent(const WorkoutEvent_t *pEvent)
{
    if (g_eventQueue == NULL || pEvent == NULL)
    {
        return false;
    }

    /* Try to send without blocking */
    if (xQueueSend(g_eventQueue, pEvent, 0) != pdTRUE)
    {
        printf("[BLE_TX] WARNING: Event queue full\n");
        return false;
    }

    return true;
}

/*************************************************************************************************/
uint8_t BleTx_FlushBuffer(void)
{
    uint8_t count = 0;
    WorkoutEvent_t event;
    uint16_t len;

    /* Flush buffered events to BLE */
    while (Buffer_Pop(&event))
    {
        len = Protocol_SerializeEvent(&event, s_msgBuf, sizeof(s_msgBuf));
        if (len > 0)
        {
            if (DataSend((const uint8_t *)s_msgBuf, len))
            {
                count++;
            }
        }
        /* Small delay between sends */
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    return count;
}

/*************************************************************************************************/
void BleTxTask(void *pvParameters)
{
    (void)pvParameters;
    WorkoutEvent_t event;
    uint16_t len;
    bool connected;

    /* Initialize as "was connected" to avoid false reconnection flush on first connect */
    s_wasConnected = true;

    while (1)
    {
        /* Wait for event from Control task */
        if (xQueueReceive(g_eventQueue, &event, portMAX_DELAY) == pdTRUE)
        {
            /* Check connection status */
            connected = BLE_IsConnected();

            /* Check for reconnection - flush buffer */
            if (connected && !s_wasConnected)
            {
                vTaskDelay(pdMS_TO_TICKS(100)); /* Wait for connection to stabilize */
                BleTx_FlushBuffer();
            }
            s_wasConnected = connected;

            /* Serialize event to JSON */
            len = Protocol_SerializeEvent(&event, s_msgBuf, sizeof(s_msgBuf));

            if (len > 0)
            {
                if (connected)
                {
                    /* Send via BLE */
                    if (DataSend((const uint8_t *)s_msgBuf, len))
                    {
                        /* Brief yield to let BLE stack process */
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                    else
                    {
                        /* Failed to send - buffer it */
                        Buffer_Push(&event);
                    }
                }
                else
                {
                    /* Not connected - buffer event for later */
                    Buffer_Push(&event);
                }
            }
        }
    }
}
