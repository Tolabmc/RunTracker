/*************************************************************************************************/
/*!
 *  \file   protocol.c
 *
 *  \brief  Communication protocol implementation for workout data serialization.
 */
/*************************************************************************************************/

#include "protocol.h"
#include "workout_state.h"
#include <stdio.h>
#include <string.h>

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
const char* Protocol_EventTypeToString(EventType_t type)
{
    switch (type)
    {
        case EVENT_WORKOUT_START:   return "start";
        case EVENT_WORKOUT_STOP:    return "stop";
        case EVENT_LAP_COMPLETE:    return "lap";
        case EVENT_WORKOUT_DONE:    return "done";
        case EVENT_STATUS_UPDATE:   return "status";
        default:                    return "unknown";
    }
}

/*************************************************************************************************/
uint16_t Protocol_SerializeEvent(const WorkoutEvent_t *pEvent, char *pBuffer, uint16_t bufLen)
{
    int len = 0;
    const WorkoutSession_t *session = Workout_GetSession();
    
    if (pEvent == NULL || pBuffer == NULL || bufLen < 50)
    {
        return 0;
    }

    switch (pEvent->type)
    {
        case EVENT_WORKOUT_START:
            len = snprintf(pBuffer, bufLen,
                "{\"event\":\"start\",\"mode\":\"%s\",\"laps\":%d,\"ts\":%lu}",
                Workout_ModeToString(session->config.mode),
                session->config.total_laps,
                (unsigned long)pEvent->timestamp_ms);
            break;

        case EVENT_LAP_COMPLETE:
            len = snprintf(pBuffer, bufLen,
                "{\"event\":\"lap\",\"lap\":%d,\"lap_ms\":%lu,\"split_ms\":%lu,\"ts\":%lu}",
                pEvent->lap_data.lap_number,
                (unsigned long)pEvent->lap_data.lap_time_ms,
                (unsigned long)pEvent->lap_data.split_time_ms,
                (unsigned long)pEvent->timestamp_ms);
            break;

        case EVENT_WORKOUT_STOP:
            len = snprintf(pBuffer, bufLen,
                "{\"event\":\"stop\",\"laps\":%d,\"total_ms\":%lu,\"ts\":%lu}",
                pEvent->current_lap,
                (unsigned long)pEvent->timestamp_ms - session->workout_start_ms,
                (unsigned long)pEvent->timestamp_ms);
            break;

        case EVENT_WORKOUT_DONE:
            len = snprintf(pBuffer, bufLen,
                "{\"event\":\"done\",\"laps\":%d,\"total_ms\":%lu,\"ts\":%lu}",
                session->config.total_laps,
                (unsigned long)pEvent->lap_data.split_time_ms,
                (unsigned long)pEvent->timestamp_ms);
            break;

        case EVENT_STATUS_UPDATE:
            len = snprintf(pBuffer, bufLen,
                "{\"event\":\"status\",\"state\":\"%s\",\"lap\":%d,\"elapsed_ms\":%lu}",
                Workout_StateToString(session->state),
                pEvent->current_lap,
                (unsigned long)Workout_GetElapsedMs());
            break;

        default:
            len = snprintf(pBuffer, bufLen,
                "{\"event\":\"unknown\",\"type\":%d}",
                pEvent->type);
            break;
    }

    if (len < 0 || len >= (int)bufLen)
    {
        return 0;
    }

    return (uint16_t)len;
}
