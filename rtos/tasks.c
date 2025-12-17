/*************************************************************************************************/
/*!
 *  \file   tasks.c
 *
 *  \brief  FreeRTOS task management implementation.
 */
/*************************************************************************************************/

#include "tasks.h"
#include "buttons.h"
#include "workout_control.h"
#include "ble_tx.h"
#include "buffer.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
bool Tasks_Init(bool enableTestInput)
{
    printf("[TASKS] Initializing application tasks...\n");

    /* Initialize offline event buffer */
    Buffer_Init();

    /* Initialize button system (creates button queue) */
    if (!Button_Init())
    {
        printf("[TASKS] ERROR: Button init failed\n");
        return false;
    }

    /* Initialize BLE TX (creates event queue) */
    if (!BleTx_Init())
    {
        printf("[TASKS] ERROR: BLE TX init failed\n");
        return false;
    }

    /* Initialize workout control */
    WorkoutControl_Init();

    /* Start the control task */
    if (!WorkoutControl_StartTask())
    {
        printf("[TASKS] ERROR: Control task creation failed\n");
        return false;
    }

    /* Start BLE TX task */
    if (!BleTx_StartTask())
    {
        printf("[TASKS] ERROR: BLE TX task creation failed\n");
        return false;
    }

    /* Start test input task if requested */
    if (enableTestInput)
    {
        if (!Button_StartTestTask())
        {
            printf("[TASKS] WARNING: Test input task creation failed\n");
            /* Non-fatal - continue without test input */
        }
    }

    printf("[TASKS] All tasks initialized successfully\n");
    return true;
}

/*************************************************************************************************/
void Tasks_PrintStatus(void)
{
#if (configUSE_TRACE_FACILITY == 1)
    char buffer[512];

    printf("\n======== TASK STATUS ========\n");
    vTaskList(buffer);
    printf("%s", buffer);
    printf("=============================\n\n");
#else
    printf("[TASKS] Task tracing not enabled in FreeRTOSConfig.h\n");
#endif
}
