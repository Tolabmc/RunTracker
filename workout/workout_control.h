/*************************************************************************************************/
/*!
 *  \file   workout_control.h
 *
 *  \brief  Workout control task interface.
 *
 *  The ControlTask receives button events from the queue and drives
 *  the workout state machine accordingly.
 */
/*************************************************************************************************/

#ifndef WORKOUT_WORKOUT_CONTROL_H
#define WORKOUT_WORKOUT_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "workout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize the workout control module.
 *
 *  Initializes the workout state and prepares for control events.
 */
/*************************************************************************************************/
void WorkoutControl_Init(void);

/*************************************************************************************************/
/*!
 *  \brief  Create and start the Control Task.
 *
 *  The Control Task waits for button events and handles:
 *  - BTN_START: Start or resume workout
 *  - BTN_LAP: Record a lap (if running)
 *  - BTN_STOP: Pause or stop workout
 *  - BTN_MODE_NEXT: Cycle workout mode (if idle)
 *  - BTN_STATUS: Print current status
 *
 *  \return true if task created successfully.
 */
/*************************************************************************************************/
bool WorkoutControl_StartTask(void);

/*************************************************************************************************/
/*!
 *  \brief  The Control Task function.
 *
 *  \param  pvParameters    FreeRTOS task parameter (unused).
 *
 *  \note   This is the main control loop - do not call directly.
 */
/*************************************************************************************************/
void ControlTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* WORKOUT_WORKOUT_CONTROL_H */
