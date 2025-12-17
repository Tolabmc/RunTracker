/*************************************************************************************************/
/*!
 *  \file   app_init.h
 *
 *  \brief  Application initialization interface.
 *
 *  Initializes all application modules in the correct order.
 */
/*************************************************************************************************/

#ifndef APP_APP_INIT_H
#define APP_APP_INIT_H

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
 *  \brief  Initialize all application modules.
 *
 *  Called before the FreeRTOS scheduler starts.
 *  Initializes (in order):
 *  1. Time utilities
 *  2. Workout system
 *  3. Button/input system
 *  4. Application tasks
 *
 *  \return true if all modules initialized successfully.
 */
/*************************************************************************************************/
bool App_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_APP_INIT_H */
