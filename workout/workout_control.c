/*************************************************************************************************/
/*!
 *  \file   workout_control.c
 *
 *  \brief  Workout control task implementation - FULLY STATIC ALLOCATION.
 */
/*************************************************************************************************/

#include "workout_control.h"
#include "workout_state.h"
#include "buttons.h"
#include "time_utils.h"
#include "ble_tx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define CONTROL_TASK_STACK_SIZE 256
#define CONTROL_TASK_PRIORITY (tskIDLE_PRIORITY + 1)

/**************************************************************************************************
  Static Memory for FreeRTOS Objects
**************************************************************************************************/

/* Static task storage */
static StaticTask_t s_controlTaskBuffer;
static StackType_t s_controlTaskStack[CONTROL_TASK_STACK_SIZE];

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

static TaskHandle_t s_controlTaskHandle = NULL;

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Send a workout event to the BLE TX task.
 */
/*************************************************************************************************/
static void sendWorkoutEvent(EventType_t type, const LapRecord_t *lapData)
{
    WorkoutEvent_t event;
    const WorkoutSession_t *session = Workout_GetSession();

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.timestamp_ms = Time_GetMs();
    event.current_lap = session->current_lap;

    if (lapData != NULL)
    {
        event.lap_data = *lapData;
    }

    BleTx_SendEvent(&event);
}

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
void WorkoutControl_Init(void)
{
    /* Initialize workout state */
    Workout_Init();

    printf("[CTRL] Workout control initialized\n");
}

/*************************************************************************************************/
bool WorkoutControl_StartTask(void)
{
    /* Create task using STATIC allocation */
    s_controlTaskHandle = xTaskCreateStatic(
        ControlTask,
        "Control",
        CONTROL_TASK_STACK_SIZE,
        NULL,
        CONTROL_TASK_PRIORITY,
        s_controlTaskStack,
        &s_controlTaskBuffer);

    if (s_controlTaskHandle == NULL)
    {
        printf("[CTRL] ERROR: Failed to create control task\n");
        return false;
    }

    printf("[CTRL] Control task started\n");
    return true;
}

/*************************************************************************************************/
void ControlTask(void *pvParameters)
{
    (void)pvParameters;
    ButtonEvent_t btn;
    LapRecord_t lapData;
    WorkoutState_t currentState;

    printf("[CTRL] Control task running...\n");

    /* Print initial status */
    Workout_PrintStatus();

    while (1)
    {
        /* Wait for button event (blocks until event received) */
        if (xQueueReceive(g_buttonQueue, &btn, portMAX_DELAY) == pdTRUE)
        {
            currentState = Workout_GetState();

            switch (btn.type)
            {
            case BTN_START:
                /*
                 * Start/Resume logic:
                 * - IDLE -> RUNNING (start new workout)
                 * - PAUSED -> RUNNING (resume)
                 * - RUNNING -> no effect
                 * - COMPLETED -> suggest reset
                 */
                if (currentState == STATE_COMPLETED)
                {
                    /* Auto-reset and start new workout */
                    Workout_Reset();
                }
                if (Workout_Start())
                {
                    /* Send workout start event */
                    sendWorkoutEvent(EVENT_WORKOUT_START, NULL);
                }
                break;

            case BTN_LAP:
                /*
                 * Lap logic:
                 * - RUNNING -> record lap, advance or complete
                 * - Other states -> ignored
                 */
                if (currentState == STATE_RUNNING)
                {
                    if (Workout_RecordLap(&lapData))
                    {
                        /* Check if workout is now complete */
                        if (Workout_GetState() == STATE_COMPLETED)
                        {
                            sendWorkoutEvent(EVENT_WORKOUT_DONE, &lapData);
                        }
                        else
                        {
                            sendWorkoutEvent(EVENT_LAP_COMPLETE, &lapData);
                        }
                    }
                }
                else
                {
                    printf("[CTRL] LAP ignored - not running\n");
                }
                break;

            case BTN_STOP:
                /*
                 * Stop/Pause logic:
                 * - RUNNING -> PAUSED
                 * - PAUSED -> COMPLETED (stop entirely)
                 * - Other states -> ignored
                 */
                if (currentState == STATE_RUNNING)
                {
                    Workout_Pause();
                    /* Don't send event for pause - only for stop */
                }
                else if (currentState == STATE_PAUSED)
                {
                    if (Workout_Stop())
                    {
                        sendWorkoutEvent(EVENT_WORKOUT_STOP, NULL);
                    }
                }
                else
                {
                    printf("[CTRL] STOP ignored - not running or paused\n");
                }
                break;

            case BTN_MODE_NEXT:
                /*
                 * Mode cycle logic:
                 * - IDLE -> cycle to next mode
                 * - Other states -> ignored (can't change during workout)
                 */
                if (currentState == STATE_IDLE || currentState == STATE_COMPLETED)
                {
                    if (currentState == STATE_COMPLETED)
                    {
                        Workout_Reset();
                    }
                    Workout_CycleMode();
                }
                else
                {
                    printf("[CTRL] MODE ignored - workout in progress\n");
                }
                break;

            case BTN_STATUS:
                /* Print current status and send status event */
                Workout_PrintStatus();
                sendWorkoutEvent(EVENT_STATUS_UPDATE, NULL);
                break;

            default:
                break;
            }
        }
    }
}
