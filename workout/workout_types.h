/*************************************************************************************************/
/*!
 *  \file   workout_types.h
 *
 *  \brief  Core data structures for workout tracking.
 *
 *  This file defines the fundamental data types and structures used
 *  throughout the workout tracking system.
 */
/*************************************************************************************************/

#ifndef WORKOUT_WORKOUT_TYPES_H
#define WORKOUT_WORKOUT_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_LAPS 8 /* Maximum laps for any workout mode */

    /*! Workout mode - defines the interval structure */
    typedef enum
    {
        MODE_4x500M,
        MODE_5x1000M,
        MODE_2x2000M,
        MODE_1x4000M
    } WorkoutMode_t;

    typedef enum
    {
        STATE_IDLE,
        STATE_RUNNING,
        STATE_REST,
        STATE_PAUSED,
        STATE_COMPLETED
    } WorkoutState_t;

    /*! Workout configuration */
    typedef struct
    {
        WorkoutMode_t mode;
        uint8_t total_laps;
        uint16_t lap_distance_m;
        uint16_t rest_time_sec;
    } WorkoutConfig_t;

    /*! Single lap/interval record */
    typedef struct
    {
        uint8_t lap_number;
        uint32_t lap_time_ms;
        uint32_t split_time_ms;
    } LapRecord_t;

    /*! Complete workout session */
    typedef struct
    {
        WorkoutConfig_t config;
        WorkoutState_t state;
        uint8_t current_lap;
        uint32_t workout_start_ms;
        uint32_t lap_start_ms;
        LapRecord_t laps[MAX_LAPS];
    } WorkoutSession_t;

    /*! Event types for BLE communication */
    typedef enum
    {
        EVENT_WORKOUT_START, /* Workout started */
        EVENT_WORKOUT_STOP,  /* Workout stopped/cancelled */
        EVENT_LAP_COMPLETE,  /* Lap finished */
        EVENT_WORKOUT_DONE,  /* All laps completed */
        EVENT_STATUS_UPDATE  /* Periodic status update */
    } EventType_t;

    /*! Event structure for sending to app via BLE */
    typedef struct
    {
        EventType_t type;      /* Event type */
        uint32_t timestamp_ms; /* When event occurred */
        uint8_t current_lap;   /* Current lap number */
        LapRecord_t lap_data;  /* Lap data (valid for LAP_COMPLETE) */
    } WorkoutEvent_t;
#ifdef __cplusplus
}
#endif

#endif /* WORKOUT_WORKOUT_TYPES_H */
