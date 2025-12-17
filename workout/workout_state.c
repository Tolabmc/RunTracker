/*************************************************************************************************/
/*!
 *  \file   workout_state.c
 *
 *  \brief  Workout state management implementation.
 */
/*************************************************************************************************/

#include "workout_state.h"
#include "time_utils.h"
#include <stdio.h>
#include <string.h>

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! The global workout session */
static WorkoutSession_t s_session;

/*! Paused time tracking (for accurate elapsed time when paused) */
static uint32_t s_pauseStartMs = 0;
static uint32_t s_totalPausedMs = 0;

/**************************************************************************************************
  Local Function Prototypes
**************************************************************************************************/

static void setModeConfig(WorkoutMode_t mode);

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
void Workout_Init(void)
{
    memset(&s_session, 0, sizeof(s_session));
    s_session.state = STATE_IDLE;

    /* Set default mode */
    setModeConfig(MODE_4x500M);

    s_pauseStartMs = 0;
    s_totalPausedMs = 0;

    printf("[WORKOUT] Initialized - Mode: %s\n", Workout_ModeToString(s_session.config.mode));
}

/*************************************************************************************************/
const WorkoutSession_t *Workout_GetSession(void)
{
    return &s_session;
}

/*************************************************************************************************/
WorkoutState_t Workout_GetState(void)
{
    return s_session.state;
}

/*************************************************************************************************/
bool Workout_SetMode(WorkoutMode_t mode)
{
    if (s_session.state != STATE_IDLE)
    {
        printf("[WORKOUT] Cannot change mode - not in IDLE state\n");
        return false;
    }

    setModeConfig(mode);
    printf("[WORKOUT] Mode set to: %s\n", Workout_ModeToString(mode));
    return true;
}

/*************************************************************************************************/
bool Workout_CycleMode(void)
{
    if (s_session.state != STATE_IDLE)
    {
        printf("[WORKOUT] Cannot change mode - not in IDLE state\n");
        return false;
    }

    /* Cycle through modes */
    WorkoutMode_t newMode = (s_session.config.mode + 1) % 4;
    setModeConfig(newMode);

    printf("[WORKOUT] Mode changed to: %s\n", Workout_ModeToString(newMode));
    return true;
}

/*************************************************************************************************/
bool Workout_Start(void)
{
    uint32_t now = Time_GetMs();

    switch (s_session.state)
    {
    case STATE_IDLE:
        /* Start new workout */
        s_session.state = STATE_RUNNING;
        s_session.current_lap = 1;
        s_session.workout_start_ms = now;
        s_session.lap_start_ms = now;
        s_totalPausedMs = 0;

        printf("[WORKOUT] ===== WORKOUT STARTED =====\n");
        printf("[WORKOUT] Mode: %s (%d laps)\n",
               Workout_ModeToString(s_session.config.mode),
               s_session.config.total_laps);
        printf("[WORKOUT] Lap 1 started...\n");
        return true;

    case STATE_PAUSED:
        /* Resume workout */
        s_session.state = STATE_RUNNING;

        /* Account for paused time */
        s_totalPausedMs += (now - s_pauseStartMs);

        printf("[WORKOUT] ===== WORKOUT RESUMED =====\n");
        printf("[WORKOUT] Continuing lap %d...\n", s_session.current_lap);
        return true;

    case STATE_RUNNING:
        printf("[WORKOUT] Already running\n");
        return false;

    case STATE_COMPLETED:
        printf("[WORKOUT] Workout completed - reset to start new\n");
        return false;

    default:
        return false;
    }
}

/*************************************************************************************************/
bool Workout_RecordLap(LapRecord_t *lap_out)
{
    if (s_session.state != STATE_RUNNING)
    {
        printf("[WORKOUT] Cannot record lap - not running\n");
        return false;
    }

    uint32_t now = Time_GetMs();
    uint8_t lapIndex = s_session.current_lap - 1;

    /* Record lap data */
    LapRecord_t *lap = &s_session.laps[lapIndex];
    lap->lap_number = s_session.current_lap;
    lap->lap_time_ms = now - s_session.lap_start_ms;
    lap->split_time_ms = now - s_session.workout_start_ms - s_totalPausedMs;

    /* Format times for display */
    char lapTimeStr[16], splitTimeStr[16];
    Time_FormatMmSsMsss(lap->lap_time_ms, lapTimeStr, sizeof(lapTimeStr));
    Time_FormatMmSsMsss(lap->split_time_ms, splitTimeStr, sizeof(splitTimeStr));

    printf("[WORKOUT] *** LAP %d COMPLETE ***\n", lap->lap_number);
    printf("[WORKOUT]     Lap Time:   %s\n", lapTimeStr);
    printf("[WORKOUT]     Split Time: %s\n", splitTimeStr);

    /* Return lap data if requested */
    if (lap_out != NULL)
    {
        *lap_out = *lap;
    }

    /* Check if workout complete */
    if (s_session.current_lap >= s_session.config.total_laps)
    {
        s_session.state = STATE_COMPLETED;
        printf("[WORKOUT] ===== WORKOUT COMPLETE =====\n");
        printf("[WORKOUT] Total time: %s\n", splitTimeStr);

        /* Print all laps summary */
        printf("[WORKOUT] --- LAP SUMMARY ---\n");
        for (uint8_t i = 0; i < s_session.config.total_laps; i++)
        {
            Time_FormatMmSsMsss(s_session.laps[i].lap_time_ms, lapTimeStr, sizeof(lapTimeStr));
            printf("[WORKOUT]   Lap %d: %s\n", i + 1, lapTimeStr);
        }
    }
    else
    {
        /* Start next lap */
        s_session.current_lap++;
        s_session.lap_start_ms = now;
        printf("[WORKOUT] Lap %d started...\n", s_session.current_lap);
    }

    return true;
}

/*************************************************************************************************/
bool Workout_Pause(void)
{
    if (s_session.state != STATE_RUNNING)
    {
        printf("[WORKOUT] Cannot pause - not running\n");
        return false;
    }

    s_session.state = STATE_PAUSED;
    s_pauseStartMs = Time_GetMs();

    char elapsedStr[16];
    Time_FormatMmSsMsss(Workout_GetElapsedMs(), elapsedStr, sizeof(elapsedStr));

    printf("[WORKOUT] ===== WORKOUT PAUSED =====\n");
    printf("[WORKOUT] Elapsed: %s, Lap %d\n", elapsedStr, s_session.current_lap);
    printf("[WORKOUT] Press START to resume, STOP to end\n");

    return true;
}

/*************************************************************************************************/
bool Workout_Stop(void)
{
    if (s_session.state == STATE_IDLE || s_session.state == STATE_COMPLETED)
    {
        printf("[WORKOUT] No active workout to stop\n");
        return false;
    }

    char elapsedStr[16];
    Time_FormatMmSsMsss(Workout_GetElapsedMs(), elapsedStr, sizeof(elapsedStr));

    s_session.state = STATE_COMPLETED;

    printf("[WORKOUT] ===== WORKOUT STOPPED =====\n");
    printf("[WORKOUT] Completed %d of %d laps\n",
           s_session.current_lap - 1, s_session.config.total_laps);
    printf("[WORKOUT] Total time: %s\n", elapsedStr);

    return true;
}

/*************************************************************************************************/
void Workout_Reset(void)
{
    printf("[WORKOUT] Resetting workout...\n");

    WorkoutMode_t currentMode = s_session.config.mode;

    memset(&s_session, 0, sizeof(s_session));
    s_session.state = STATE_IDLE;
    setModeConfig(currentMode);

    s_pauseStartMs = 0;
    s_totalPausedMs = 0;

    printf("[WORKOUT] Ready - Mode: %s\n", Workout_ModeToString(currentMode));
}

/*************************************************************************************************/
uint32_t Workout_GetElapsedMs(void)
{
    if (s_session.state == STATE_IDLE)
    {
        return 0;
    }

    uint32_t elapsed = Time_GetMs() - s_session.workout_start_ms - s_totalPausedMs;

    /* If paused, don't count current pause time */
    if (s_session.state == STATE_PAUSED)
    {
        elapsed -= (Time_GetMs() - s_pauseStartMs);
    }

    return elapsed;
}

/*************************************************************************************************/
uint32_t Workout_GetCurrentLapMs(void)
{
    if (s_session.state != STATE_RUNNING)
    {
        return 0;
    }

    return Time_GetMs() - s_session.lap_start_ms;
}

/*************************************************************************************************/
void Workout_PrintStatus(void)
{
    char timeStr[16];

    printf("\n");
    printf("======== WORKOUT STATUS ========\n");
    printf("  State:  %s\n", Workout_StateToString(s_session.state));
    printf("  Mode:   %s\n", Workout_ModeToString(s_session.config.mode));
    printf("  Laps:   %d / %d\n",
           (s_session.state == STATE_IDLE) ? 0 : s_session.current_lap,
           s_session.config.total_laps);

    if (s_session.state != STATE_IDLE)
    {
        Time_FormatMmSsMsss(Workout_GetElapsedMs(), timeStr, sizeof(timeStr));
        printf("  Total:  %s\n", timeStr);

        if (s_session.state == STATE_RUNNING)
        {
            Time_FormatMmSsMsss(Workout_GetCurrentLapMs(), timeStr, sizeof(timeStr));
            printf("  Lap:    %s\n", timeStr);
        }
    }
    printf("================================\n");
    printf("\n");
}

/*************************************************************************************************/
const char *Workout_StateToString(WorkoutState_t state)
{
    switch (state)
    {
    case STATE_IDLE:
        return "IDLE";
    case STATE_RUNNING:
        return "RUNNING";
    case STATE_REST:
        return "REST";
    case STATE_PAUSED:
        return "PAUSED";
    case STATE_COMPLETED:
        return "COMPLETED";
    default:
        return "UNKNOWN";
    }
}

/*************************************************************************************************/
const char *Workout_ModeToString(WorkoutMode_t mode)
{
    switch (mode)
    {
    case MODE_4x500M:
        return "4x500m";
    case MODE_5x1000M:
        return "5x1000m";
    case MODE_2x2000M:
        return "2x2000m";
    case MODE_1x4000M:
        return "1x4000m";
    default:
        return "UNKNOWN";
    }
}

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Set workout configuration based on mode.
 */
/*************************************************************************************************/
static void setModeConfig(WorkoutMode_t mode)
{
    s_session.config.mode = mode;

    switch (mode)
    {
    case MODE_4x500M:
        s_session.config.total_laps = 4;
        s_session.config.lap_distance_m = 500;
        s_session.config.rest_time_sec = 60;
        break;

    case MODE_5x1000M:
        s_session.config.total_laps = 5;
        s_session.config.lap_distance_m = 1000;
        s_session.config.rest_time_sec = 90;
        break;

    case MODE_2x2000M:
        s_session.config.total_laps = 2;
        s_session.config.lap_distance_m = 2000;
        s_session.config.rest_time_sec = 120;
        break;

    case MODE_1x4000M:
        s_session.config.total_laps = 1;
        s_session.config.lap_distance_m = 4000;
        s_session.config.rest_time_sec = 0;
        break;

    default:
        break;
    }
}
