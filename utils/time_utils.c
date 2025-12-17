/*************************************************************************************************/
/*!
 *  \file   time_utils.c
 *
 *  \brief  Time utility functions implementation.
 */
/*************************************************************************************************/

#include "time_utils.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/*************************************************************************************************/
/*!
 *  \brief  Get current time in milliseconds.
 */
/*************************************************************************************************/
uint32_t Time_GetMs(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/*************************************************************************************************/
/*!
 *  \brief  Calculate elapsed time since a start time.
 */
/*************************************************************************************************/
uint32_t Time_ElapsedMs(uint32_t start_ms)
{
    uint32_t now = Time_GetMs();
    
    /* Handle wraparound (occurs every ~49 days at 1ms tick) */
    if (now >= start_ms)
    {
        return now - start_ms;
    }
    else
    {
        /* Wraparound occurred */
        return (0xFFFFFFFF - start_ms) + now + 1;
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Format milliseconds to MM:SS.mmm string.
 */
/*************************************************************************************************/
char* Time_FormatMmSsMsss(uint32_t ms, char *buffer, uint8_t buf_len)
{
    uint32_t total_seconds = ms / 1000;
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;
    uint32_t millis = ms % 1000;
    
    snprintf(buffer, buf_len, "%02lu:%02lu.%03lu", 
             (unsigned long)minutes, 
             (unsigned long)seconds, 
             (unsigned long)millis);
    
    return buffer;
}
