/*************************************************************************************************/
/*!
 *  \file   time_utils.h
 *
 *  \brief  Time utility functions interface.
 *
 *  Provides millisecond timing using FreeRTOS ticks.
 *  DO NOT use BLE timing - this is the authoritative time source.
 */
/*************************************************************************************************/

#ifndef UTILS_TIME_UTILS_H
#define UTILS_TIME_UTILS_H

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
 *  \brief  Get current time in milliseconds.
 *
 *  \return Current time in milliseconds since system start.
 *
 *  \note   Uses FreeRTOS tick count - safe to call from any task.
 */
/*************************************************************************************************/
uint32_t Time_GetMs(void);

/*************************************************************************************************/
/*!
 *  \brief  Calculate elapsed time since a start time.
 *
 *  \param  start_ms    Start time in milliseconds.
 *
 *  \return Elapsed milliseconds since start_ms.
 */
/*************************************************************************************************/
uint32_t Time_ElapsedMs(uint32_t start_ms);

/*************************************************************************************************/
/*!
 *  \brief  Format milliseconds to MM:SS.mmm string.
 *
 *  \param  ms          Time in milliseconds.
 *  \param  buffer      Output buffer (minimum 12 chars).
 *  \param  buf_len     Buffer length.
 *
 *  \return Pointer to buffer.
 */
/*************************************************************************************************/
char* Time_FormatMmSsMsss(uint32_t ms, char *buffer, uint8_t buf_len);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_TIME_UTILS_H */
