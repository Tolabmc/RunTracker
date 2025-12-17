/*************************************************************************************************/
/*!
 *  \file   workout_state.h
 *
 *  \brief  Workout state management interface.
 *
 *  Manages the workout session state machine and provides functions
 *  for starting, stopping, pausing, and recording laps.
 */
/*************************************************************************************************/

#ifndef WORKOUT_WORKOUT_STATE_H
#define WORKOUT_WORKOUT_STATE_H

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
 *  \brief  Initialize the workout state module.
 *
 *  Sets the session to IDLE with default configuration.
 */
/*************************************************************************************************/
void Workout_Init(void);

/*************************************************************************************************/
/*!
 *  \brief  Get pointer to the current workout session.
 *
 *  \return Pointer to the global workout session (read-only recommended).
 */
/*************************************************************************************************/
const WorkoutSession_t* Workout_GetSession(void);

/*************************************************************************************************/
/*!
 *  \brief  Get the current workout state.
 *
 *  \return Current state (IDLE, RUNNING, PAUSED, etc.).
 */
/*************************************************************************************************/
WorkoutState_t Workout_GetState(void);

/*************************************************************************************************/
/*!
 *  \brief  Set the workout mode/configuration.
 *
 *  Can only be changed when in IDLE state.
 *
 *  \param  mode    The workout mode to set.
 *
 *  \return true if mode was set, false if not allowed (not in IDLE).
 */
/*************************************************************************************************/
bool Workout_SetMode(WorkoutMode_t mode);

/*************************************************************************************************/
/*!
 *  \brief  Cycle to the next workout mode.
 *
 *  Can only be changed when in IDLE state.
 *
 *  \return true if mode was changed, false if not allowed.
 */
/*************************************************************************************************/
bool Workout_CycleMode(void);

/*************************************************************************************************/
/*!
 *  \brief  Start or resume the workout.
 *
 *  - From IDLE: Starts a new workout
 *  - From PAUSED: Resumes the workout
 *  - From RUNNING: No effect
 *
 *  \return true if state changed, false otherwise.
 */
/*************************************************************************************************/
bool Workout_Start(void);

/*************************************************************************************************/
/*!
 *  \brief  Record a lap.
 *
 *  Only valid when RUNNING. Records the current lap time and
 *  advances to the next lap (or completes workout if last lap).
 *
 *  \param  lap_out     Optional pointer to receive the recorded lap data.
 *
 *  \return true if lap was recorded, false if not in RUNNING state.
 */
/*************************************************************************************************/
bool Workout_RecordLap(LapRecord_t *lap_out);

/*************************************************************************************************/
/*!
 *  \brief  Pause the workout.
 *
 *  Only valid when RUNNING. Workout can be resumed with Workout_Start().
 *
 *  \return true if paused, false if not in RUNNING state.
 */
/*************************************************************************************************/
bool Workout_Pause(void);

/*************************************************************************************************/
/*!
 *  \brief  Stop the workout completely.
 *
 *  Valid from RUNNING or PAUSED. Ends the workout (cannot be resumed).
 *
 *  \return true if stopped, false if already IDLE or COMPLETED.
 */
/*************************************************************************************************/
bool Workout_Stop(void);

/*************************************************************************************************/
/*!
 *  \brief  Reset the workout to IDLE state.
 *
 *  Clears all lap data and returns to initial state.
 */
/*************************************************************************************************/
void Workout_Reset(void);

/*************************************************************************************************/
/*!
 *  \brief  Get elapsed time for current workout.
 *
 *  \return Elapsed milliseconds since workout started (0 if IDLE).
 */
/*************************************************************************************************/
uint32_t Workout_GetElapsedMs(void);

/*************************************************************************************************/
/*!
 *  \brief  Get elapsed time for current lap.
 *
 *  \return Elapsed milliseconds since current lap started (0 if not RUNNING).
 */
/*************************************************************************************************/
uint32_t Workout_GetCurrentLapMs(void);

/*************************************************************************************************/
/*!
 *  \brief  Print current workout status to console.
 */
/*************************************************************************************************/
void Workout_PrintStatus(void);

/*************************************************************************************************/
/*!
 *  \brief  Get string representation of workout state.
 *
 *  \param  state   The state to convert.
 *
 *  \return String name of the state.
 */
/*************************************************************************************************/
const char* Workout_StateToString(WorkoutState_t state);

/*************************************************************************************************/
/*!
 *  \brief  Get string representation of workout mode.
 *
 *  \param  mode    The mode to convert.
 *
 *  \return String name of the mode.
 */
/*************************************************************************************************/
const char* Workout_ModeToString(WorkoutMode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* WORKOUT_WORKOUT_STATE_H */
