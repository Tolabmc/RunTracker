/*************************************************************************************************/
/*!
 *  \file   app_init.c
 *
 *  \brief  Application initialization implementation.
 */
/*************************************************************************************************/

#include "app_init.h"
#include "tasks.h"
#include "max7325.h"
#include <stdio.h>

/**************************************************************************************************
  Macros
**************************************************************************************************/

/* Set to 1 to use MAX7325 I/O expander buttons, 0 for serial test input */
#define USE_MAX7325_BUTTONS     1

/* Set to 1 to enable serial keyboard test input (conflicts with BLE terminal) */
#define ENABLE_SERIAL_TEST      0

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
bool App_Init(void)
{
    printf("\n");
    printf("========================================\n");
    printf("   MAX32655 WORKOUT TRACKER\n");
    printf("========================================\n");
    printf("\n");
    
    /* Initialize all tasks (workout control, etc.) */
    if (!Tasks_Init(ENABLE_SERIAL_TEST))
    {
        printf("[APP] ERROR: Task initialization failed\n");
        return false;
    }
    
#if USE_MAX7325_BUTTONS
    /* Initialize MAX7325 I/O expander for hardware buttons */
    printf("[APP] Initializing MAX7325 I/O expander...\n");
    if (!Max7325_Init())
    {
        printf("[APP] WARNING: MAX7325 init failed - buttons won't work\n");
        printf("[APP] Check I2C wiring and address configuration\n");
        /* Non-fatal - continue anyway */
    }
    else
    {
        /* Start button polling task */
        if (!Max7325_StartPollingTask())
        {
            printf("[APP] WARNING: Button polling task failed to start\n");
        }
    }
#endif
    
    printf("\n");
    printf("[APP] Application initialized successfully\n");
    printf("[APP] FreeRTOS scheduler will start...\n");
    printf("\n");
    
    return true;
}
