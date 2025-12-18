/*************************************************************************************************/
/*!
 *  \file   sensor_task.h
 *
 *  \brief  Heart Rate Sensor Task Interface for CS4447 Embedded Systems Project.
 *
 *  This module implements a periodic sensor sampling task that:
 *  - Reads heart rate data from the MAX I/O expansion board sensor
 *  - Sends HR samples to the control task via a FreeRTOS queue
 *  - Operates at a fixed sampling interval (100ms by default)
 *
 *  SENSOR TASK DESIGN:
 *  -------------------
 *  The sensor task runs at a fixed period using vTaskDelayUntil() for
 *  precise timing. It only actively samples when HR measurement is enabled
 *  by the control task.
 *
 *  PRIORITY JUSTIFICATION:
 *  -----------------------
 *  Sensor Task Priority: tskIDLE_PRIORITY + 2
 *  - Lower than control task (priority +3) to avoid blocking state transitions
 *  - Higher than BLE task (priority +1) because sampling must be consistent
 *  - Deterministic sampling interval is maintained by vTaskDelayUntil()
 *
 *  INTER-TASK COMMUNICATION:
 *  -------------------------
 *  - Sends HrSample_t structs to control task via g_hrSampleQueue
 *  - Control task calls SensorTask_StartHrMeasurement() to enable sampling
 *  - Control task calls SensorTask_StopHrMeasurement() to disable sampling
 *
 *  HARDWARE INTERFACE:
 *  -------------------
 *  The sensor communicates via I2C on the MAX I/O expansion board.
 *  Specific sensor model: MAX30102 (or similar pulse oximeter/HR sensor)
 *  I2C address and register map defined in implementation file.
 */
/*************************************************************************************************/

#ifndef INPUT_SENSOR_TASK_H
#define INPUT_SENSOR_TASK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Constants
**************************************************************************************************/

/*!
 * HR SAMPLING INTERVAL: 100ms (10 Hz)
 *
 * JUSTIFICATION:
 * - Fast enough to capture pulse variations
 * - Slow enough to not overwhelm the sensor or queues
 * - Matches typical HR sensor output rate
 * - 10 samples = 1 second of measurement time
 */
#define HR_SAMPLE_INTERVAL_MS       100

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize the sensor task module.
 *
 *  Sets up the I2C interface and configures the HR sensor.
 *  Does NOT start the task - call SensorTask_Start() separately.
 *
 *  \return true if initialization successful, false otherwise.
 */
/*************************************************************************************************/
bool SensorTask_Init(void);

/*************************************************************************************************/
/*!
 *  \brief  Start the sensor task.
 *
 *  Creates the FreeRTOS task. Task will idle until HR measurement is requested.
 *
 *  \return true if task started successfully, false otherwise.
 */
/*************************************************************************************************/
bool SensorTask_Start(void);

/*************************************************************************************************/
/*!
 *  \brief  Enable HR measurement.
 *
 *  Called by control task when entering HR_MEASUREMENT state.
 *  Wakes up the sensor and begins periodic sampling.
 *
 *  Thread-safe - can be called from any task.
 */
/*************************************************************************************************/
void SensorTask_StartHrMeasurement(void);

/*************************************************************************************************/
/*!
 *  \brief  Disable HR measurement.
 *
 *  Called by control task when exiting HR_MEASUREMENT state.
 *  Puts the sensor into low-power mode and stops sampling.
 *
 *  Thread-safe - can be called from any task.
 */
/*************************************************************************************************/
void SensorTask_StopHrMeasurement(void);

/*************************************************************************************************/
/*!
 *  \brief  Check if HR measurement is currently active.
 *
 *  \return true if actively sampling, false otherwise.
 */
/*************************************************************************************************/
bool SensorTask_IsMeasuring(void);

/*************************************************************************************************/
/*!
 *  \brief  The sensor task function (do not call directly).
 *
 *  This is the FreeRTOS task entry point. It implements periodic
 *  sampling using vTaskDelayUntil() for precise timing.
 *
 *  TASK BEHAVIOR:
 *  --------------
 *  - When measurement disabled: sleeps, checking periodically
 *  - When measurement enabled: samples at HR_SAMPLE_INTERVAL_MS
 *  - Sends samples to g_hrSampleQueue for control task
 *  - Handles sensor errors gracefully (marks samples invalid)
 *
 *  \param  pvParameters    FreeRTOS task parameter (unused).
 */
/*************************************************************************************************/
void SensorTask_Run(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_SENSOR_TASK_H */
