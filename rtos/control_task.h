#ifndef RTOS_CONTROL_TASK_H
#define RTOS_CONTROL_TASK_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Control State ---------- */

typedef enum
{
    CTRL_STATE_IDLE,
    CTRL_STATE_RUNNING,
    CTRL_STATE_HR_MEASUREMENT   /* Waiting for HR confirmation from ESP32 */
} CtrlState_t;

/* ---------- BLE Control Events ---------- */

typedef enum
{
    BLE_CTRL_EVT_NONE = 0,
    BLE_CTRL_EVT_HR_DONE,
    BLE_CTRL_EVT_CONNECTED,
    BLE_CTRL_EVT_DISCONNECTED
} BleCtrlEventType_t;

typedef struct
{
    BleCtrlEventType_t type;
    uint32_t timestamp_ms;
} BleCtrlEvent_t;

/* ---------- Queues ---------- */

/* BLE task -> control task */
extern QueueHandle_t g_bleCtrlQueue;

/* ---------- Control Task API ---------- */

bool ControlTask_Init(void);
bool ControlTask_Start(void);

CtrlState_t ControlTask_GetState(void);
bool ControlTask_IsHrMeasurementActive(void);

/* Called by BLE layer to signal protocol events */
bool ControlTask_SendBleEvent(BleCtrlEventType_t type);

/* FreeRTOS task entry point */
void ControlTask_Run(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_CONTROL_TASK_H */
