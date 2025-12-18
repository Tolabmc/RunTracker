#include "control_task.h"
#include "buttons.h"
#include "time_utils.h"
#include "ble_tx.h"
#include "ble_manager.h"
#include "workout_state.h"
#include "workout_types.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdio.h>
#include <string.h>

/*
 * Control task is the highest-priority application task.
 * It owns the workout state machine and timer control.
 */
#define CONTROL_TASK_PRIORITY       (tskIDLE_PRIORITY + 3)
#define CONTROL_TASK_STACK_SIZE     384

/*
 * BLE control events are asynchronous and low-rate.
 * Small queue is sufficient and keeps memory usage predictable.
 */
#define BLE_CTRL_QUEUE_LENGTH       8

/*
 * Short poll timeout keeps the control loop responsive
 * without burning CPU when idle.
 */
#define QUEUE_POLL_TIMEOUT_MS       10

/*
 * If the ESP32 fails to respond, the MAX must not deadlock.
 * This timeout guarantees forward progress.
 */
#define HR_CONFIRM_TIMEOUT_MS        5000

/* ---------- Static RTOS Objects ---------- */

static StaticTask_t s_controlTaskBuffer;
static StackType_t  s_controlTaskStack[CONTROL_TASK_STACK_SIZE];

static StaticQueue_t s_bleCtrlQueueBuffer;
static uint8_t s_bleCtrlQueueStorage[
    BLE_CTRL_QUEUE_LENGTH * sizeof(BleCtrlEvent_t)
];

/* ---------- Globals ---------- */

QueueHandle_t g_bleCtrlQueue = NULL;

/* ---------- Local State ---------- */

static TaskHandle_t s_controlTaskHandle = NULL;

/*
 * Control state is only modified by this task,
 * avoiding the need for locks.
 */
static volatile CtrlState_t s_currentState = CTRL_STATE_IDLE;

/*
 * HR wait session tracks bounded waiting for ESP confirmation.
 */
static struct
{
    uint32_t startMs;
    bool     active;
} s_hrWait;

/* ---------- Forward Declarations ---------- */

static void handleButtonEvent(const ButtonEvent_t *evt);
static void handleBleCtrlEvent(const BleCtrlEvent_t *evt);

static void enterHrWaitState(void);
static void exitHrWaitState(bool confirmed);
static void sendHrRequest(void);

/* ---------- Public API ---------- */

bool ControlTask_Init(void)
{
    g_bleCtrlQueue = xQueueCreateStatic(
        BLE_CTRL_QUEUE_LENGTH,
        sizeof(BleCtrlEvent_t),
        s_bleCtrlQueueStorage,
        &s_bleCtrlQueueBuffer
    );

    if (!g_bleCtrlQueue)
    {
        printf("[CTRL] Failed to create BLE control queue\n");
        return false;
    }

    Workout_Init();
    memset(&s_hrWait, 0, sizeof(s_hrWait));

    return true;
}

bool ControlTask_Start(void)
{
    s_controlTaskHandle = xTaskCreateStatic(
        ControlTask_Run,
        "Control",
        CONTROL_TASK_STACK_SIZE,
        NULL,
        CONTROL_TASK_PRIORITY,
        s_controlTaskStack,
        &s_controlTaskBuffer
    );

    return (s_controlTaskHandle != NULL);
}

CtrlState_t ControlTask_GetState(void)
{
    return s_currentState;
}

bool ControlTask_IsHrMeasurementActive(void)
{
    return (s_currentState == CTRL_STATE_HR_MEASUREMENT);
}

bool ControlTask_SendBleEvent(BleCtrlEventType_t type)
{
    if (!g_bleCtrlQueue)
    {
        /* Gracefully drop events when control task isn't running to avoid hard faults */
        printf("[CTRL] BLE ctrl queue not ready, event %d ignored\n", type);
        return false;
    }

    BleCtrlEvent_t evt = {
        .type = type,
        .timestamp_ms = Time_GetMs()
    };

    return (xQueueSend(g_bleCtrlQueue, &evt, 0) == pdTRUE);
}

/* ---------- Main Control Loop ---------- */

void ControlTask_Run(void *pvParameters)
{
    (void)pvParameters;

    ButtonEvent_t btn;
    BleCtrlEvent_t ble;

    while (1)
    {
        if (xQueueReceive(
                g_buttonQueue,
                &btn,
                pdMS_TO_TICKS(QUEUE_POLL_TIMEOUT_MS)) == pdTRUE)
        {
            handleButtonEvent(&btn);
        }

        if (xQueueReceive(g_bleCtrlQueue, &ble, 0) == pdTRUE)
        {
            handleBleCtrlEvent(&ble);
        }

        /*
         * Waiting for ESP confirmation must always be bounded.
         * Timeout guarantees timer control never stalls indefinitely.
         */
        if (s_currentState == CTRL_STATE_HR_MEASUREMENT && s_hrWait.active)
        {
            if (Time_GetMs() - s_hrWait.startMs > HR_CONFIRM_TIMEOUT_MS)
            {
                printf("[CTRL] HR confirmation timeout\n");
                exitHrWaitState(false);
            }
        }

        taskYIELD();
    }
}

/* ---------- Event Handlers ---------- */

static void handleButtonEvent(const ButtonEvent_t *evt)
{
    switch (s_currentState)
    {
    case CTRL_STATE_IDLE:
        if (evt->type == BTN_START && Workout_Start())
        {
            s_currentState = CTRL_STATE_RUNNING;
        }
        break;

    case CTRL_STATE_RUNNING:
        if (evt->type == BTN_LAP)
        {
            enterHrWaitState();
        }
        else if (evt->type == BTN_STOP)
        {
            Workout_Pause();
        }
        break;

    case CTRL_STATE_HR_MEASUREMENT:
        if (evt->type == BTN_STOP)
        {
            exitHrWaitState(false);
        }
        break;

    default:
        break;
    }
}

static void handleBleCtrlEvent(const BleCtrlEvent_t *evt)
{
    if (evt->type == BLE_CTRL_EVT_HR_DONE &&
        s_currentState == CTRL_STATE_HR_MEASUREMENT &&
        s_hrWait.active)
    {
        exitHrWaitState(true);
    }
}

/* ---------- State Transitions ---------- */

static void enterHrWaitState(void)
{
    Workout_Pause();

    s_hrWait.startMs = Time_GetMs();
    s_hrWait.active  = true;

    s_currentState = CTRL_STATE_HR_MEASUREMENT;

    sendHrRequest();
}

static void exitHrWaitState(bool confirmed)
{
    LapRecord_t lap;

    s_hrWait.active = false;

    if (Workout_RecordLap(&lap))
    {
        WorkoutEvent_t evt = {0};

        evt.type = (Workout_GetState() == STATE_COMPLETED)
                   ? EVENT_WORKOUT_DONE
                   : EVENT_LAP_COMPLETE;

        evt.timestamp_ms = Time_GetMs();
        evt.lap_data = lap;

        BleTx_SendEvent(&evt);
    }

    if (Workout_GetState() != STATE_COMPLETED)
    {
        Workout_Start();
        s_currentState = CTRL_STATE_RUNNING;
    }
    else
    {
        s_currentState = CTRL_STATE_IDLE;
    }
}

/* ---------- BLE Interaction ---------- */

static void sendHrRequest(void)
{
    static const char msg[] = "{\"cmd\":\"hr_req\"}";

    if (BLE_IsConnected())
    {
        DataSend((const uint8_t *)msg, sizeof(msg) - 1);
    }
}
