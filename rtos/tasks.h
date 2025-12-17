/*************************************************************************************************/
/*!
 *  \file   tasks.h
 *
 *  \brief  FreeRTOS task management interface.
 *
 *  Creates and manages all application tasks.
 */
/*************************************************************************************************/

#ifndef RTOS_TASKS_H
#define RTOS_TASKS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize all application tasks.
 *
 *  Creates:
 *  - ControlTask: Handles workout state machine
 *  - TestInputTask: Reads keyboard for testing (optional)
 *
 *  \param  enableTestInput     If true, start the keyboard test input task.
 *
 *  \return true if all tasks created successfully.
 */
/*************************************************************************************************/
bool Tasks_Init(bool enableTestInput);

/*************************************************************************************************/
/*!
 *  \brief  Print task status information.
 *
 *  Displays stack usage and state for debugging.
 */
/*************************************************************************************************/
void Tasks_PrintStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* RTOS_TASKS_H */
