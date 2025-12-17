/*************************************************************************************************/
/*!
 *  \file   buttons.h
 *
 *  \brief  Button input handling interface.
 *
 *  Provides button event types and a test interface via serial console.
 *  In production, hardware button interrupts will generate these events.
 *  For testing, keyboard input via UART can simulate button presses.
 */
/*************************************************************************************************/

#ifndef INPUT_BUTTONS_H
#define INPUT_BUTTONS_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Type Definitions
**************************************************************************************************/

/*! Button event types */
typedef enum
{
    BTN_NONE = 0,      /*!< No event */
    BTN_START,         /*!< Start/Resume workout */
    BTN_LAP,           /*!< Record a lap */
    BTN_STOP,          /*!< Stop/Pause workout */
    BTN_MODE_NEXT,     /*!< Cycle to next workout mode (when idle) */
    BTN_STATUS         /*!< Print current status (debug) */
} ButtonEventType_t;

/*! Button event structure - sent via queue to ControlTask */
typedef struct
{
    ButtonEventType_t type;     /*!< Which button event */
    uint32_t timestamp_ms;      /*!< When button was pressed */
} ButtonEvent_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Button event queue - ControlTask receives from this */
extern QueueHandle_t g_buttonQueue;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize button handling.
 *
 *  Creates the button event queue and optionally starts the test input task.
 *
 *  \return true if successful, false otherwise.
 */
/*************************************************************************************************/
bool Button_Init(void);

/*************************************************************************************************/
/*!
 *  \brief  Send a button event to the control task.
 *
 *  \param  type    Button event type.
 *
 *  \return true if event was queued, false if queue full.
 */
/*************************************************************************************************/
bool Button_SendEvent(ButtonEventType_t type);

/*************************************************************************************************/
/*!
 *  \brief  Start the serial test input task.
 *
 *  This task reads characters from UART and generates button events:
 *  - 's' or 'S' = BTN_START
 *  - 'l' or 'L' = BTN_LAP  
 *  - 'x' or 'X' = BTN_STOP
 *  - 'm' or 'M' = BTN_MODE_NEXT
 *  - '?' = BTN_STATUS (print current state)
 *
 *  \return true if task created, false otherwise.
 */
/*************************************************************************************************/
bool Button_StartTestTask(void);

/*************************************************************************************************/
/*!
 *  \brief  Print the test command help menu.
 */
/*************************************************************************************************/
void Button_PrintHelp(void);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_BUTTONS_H */
