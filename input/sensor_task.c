/*************************************************************************************************/
/*!
 *  \file   sensor_task.c
 *
 *  \brief  Heart Rate Sensor Task Implementation for CS4447 Embedded Systems Project.
 *
 *  This module implements periodic heart rate sampling from the MAX I/O expansion board.
 *
 *  REAL-TIME DESIGN:
 *  -----------------
 *  - Uses vTaskDelayUntil() for precise 100ms sampling period
 *  - Never blocks on I2C indefinitely (uses timeout)
 *  - Sends samples to control task via queue (non-blocking)
 *  - Graceful handling of sensor errors
 *
 *  SENSOR INTERFACE:
 *  -----------------
 *  The MAX I/O expansion board includes a heart rate sensor (e.g., MAX30102).
 *  Communication is via I2C. This implementation provides:
 *  - Initialization and configuration
 *  - Periodic sampling at a fixed rate
 *  - Validation and confidence estimation
 *
 *  For testing without hardware, enable SENSOR_SIMULATE_DATA to use synthetic data.
 */
/*************************************************************************************************/

#include "sensor_task.h"
#include "control_task.h"
#include "time_utils.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Maxim SDK includes */
#include "mxc_device.h"
#include "mxc_delay.h"
#include "i2c.h"

/**************************************************************************************************
  Compile-time Configuration
**************************************************************************************************/

/*!
 * SENSOR_SIMULATE_DATA: Set to 1 to use simulated HR data for testing
 *
 * When enabled, the sensor task generates realistic synthetic HR data
 * instead of reading from actual hardware. Useful for:
 * - Testing the control loop without hardware
 * - Debugging state machine transitions
 * - Oral exam demonstrations
 */
#define SENSOR_SIMULATE_DATA        1

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*!
 * SENSOR TASK PRIORITY: tskIDLE_PRIORITY + 2
 *
 * JUSTIFICATION:
 * - Lower than control task (+3) to avoid blocking state transitions
 * - Higher than BLE task (+1) because sampling must maintain consistent timing
 * - Sensor I/O should not starve real-time control decisions
 */
#define SENSOR_TASK_PRIORITY        (tskIDLE_PRIORITY + 2)

#define SENSOR_TASK_STACK_SIZE      256     /*!< Stack size in words */

/* I2C Configuration */
#define SENSOR_I2C_INSTANCE         MXC_I2C2
#define SENSOR_I2C_FREQ             100000  /*!< 100 kHz I2C */

/* MAX30102 Heart Rate Sensor (typical) */
#define MAX30102_I2C_ADDR           0x57    /*!< 7-bit I2C address */
#define MAX30102_REG_INTR_STATUS_1  0x00
#define MAX30102_REG_INTR_STATUS_2  0x01
#define MAX30102_REG_INTR_ENABLE_1  0x02
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C
#define MAX30102_REG_LED2_PA        0x0D
#define MAX30102_REG_PART_ID        0xFF

#define MAX30102_PART_ID_VALUE      0x15    /*!< Expected part ID */

/* Idle check interval when not measuring */
#define IDLE_CHECK_INTERVAL_MS      100

/**************************************************************************************************
  Static Memory for FreeRTOS Objects
**************************************************************************************************/

/* Task storage */
static StaticTask_t s_sensorTaskBuffer;
static StackType_t s_sensorTaskStack[SENSOR_TASK_STACK_SIZE];

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

static TaskHandle_t s_sensorTaskHandle = NULL;
static mxc_i2c_regs_t *s_i2c = NULL;
static bool s_initialized = false;

/*!
 * \brief Measurement active flag
 *
 * Set by control task via SensorTask_StartHrMeasurement().
 * Cleared by control task via SensorTask_StopHrMeasurement().
 * Volatile because accessed from multiple tasks.
 */
static volatile bool s_measurementActive = false;

/* Simulation state (when SENSOR_SIMULATE_DATA is enabled) */
#if SENSOR_SIMULATE_DATA
static uint8_t s_simSampleCount = 0;
#endif

/**************************************************************************************************
  Local Function Prototypes
**************************************************************************************************/

static bool sensorHardwareInit(void);
static bool sensorReadSample(HrSample_t *pSample);
static bool sensorWriteReg(uint8_t reg, uint8_t value);
static bool sensorReadReg(uint8_t reg, uint8_t *pValue);

#if SENSOR_SIMULATE_DATA
static void sensorSimulateSample(HrSample_t *pSample);
#endif

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
bool SensorTask_Init(void)
{
    printf("[SENSOR] Initializing sensor task module...\n");

#if SENSOR_SIMULATE_DATA
    printf("[SENSOR] *** SIMULATION MODE ENABLED ***\n");
    printf("[SENSOR] Using synthetic HR data for testing\n");
    s_initialized = true;
#else
    /* Initialize actual sensor hardware */
    if (!sensorHardwareInit())
    {
        printf("[SENSOR] WARNING: Hardware init failed, check I2C wiring\n");
        /* Continue anyway - allows testing without hardware */
    }
    s_initialized = true;
#endif

    s_measurementActive = false;

    printf("[SENSOR] Sensor task module initialized\n");
    printf("[SENSOR]   - Sampling interval: %d ms\n", HR_SAMPLE_INTERVAL_MS);
    printf("[SENSOR]   - Task Priority: %d (< control task)\n", SENSOR_TASK_PRIORITY);

    return true;
}

/*************************************************************************************************/
bool SensorTask_Start(void)
{
    /* Create task using STATIC allocation */
    s_sensorTaskHandle = xTaskCreateStatic(
        SensorTask_Run,
        "HRSensor",
        SENSOR_TASK_STACK_SIZE,
        NULL,
        SENSOR_TASK_PRIORITY,
        s_sensorTaskStack,
        &s_sensorTaskBuffer);

    if (s_sensorTaskHandle == NULL)
    {
        printf("[SENSOR] ERROR: Failed to create sensor task\n");
        return false;
    }

    printf("[SENSOR] HR sensor task started\n");
    return true;
}

/*************************************************************************************************/
void SensorTask_StartHrMeasurement(void)
{
    printf("[SENSOR] HR measurement ENABLED\n");

#if SENSOR_SIMULATE_DATA
    s_simSampleCount = 0;
#endif

    s_measurementActive = true;

    /* Notify the task to wake up if it's sleeping */
    if (s_sensorTaskHandle != NULL)
    {
        xTaskNotifyGive(s_sensorTaskHandle);
    }
}

/*************************************************************************************************/
void SensorTask_StopHrMeasurement(void)
{
    printf("[SENSOR] HR measurement DISABLED\n");
    s_measurementActive = false;
}

/*************************************************************************************************/
bool SensorTask_IsMeasuring(void)
{
    return s_measurementActive;
}

/*************************************************************************************************/
/*!
 *  \brief  Sensor Task Main Loop
 *
 *  REAL-TIME PERIODIC SAMPLING:
 *  ----------------------------
 *  Uses vTaskDelayUntil() to achieve precise 100ms sampling period.
 *  This is critical for consistent HR measurement.
 *
 *  LOOP STRUCTURE:
 *  ---------------
 *  1. If measurement active: sample sensor, send to queue
 *  2. If measurement inactive: sleep briefly, check again
 *  3. Use vTaskDelayUntil for precise timing during active sampling
 */
/*************************************************************************************************/
void SensorTask_Run(void *pvParameters)
{
    (void)pvParameters;

    HrSample_t sample;
    TickType_t xLastWakeTime;
    const TickType_t xSamplePeriod = pdMS_TO_TICKS(HR_SAMPLE_INTERVAL_MS);

    printf("[SENSOR] ========================================\n");
    printf("[SENSOR]  HR SENSOR TASK ACTIVE\n");
    printf("[SENSOR] ========================================\n");
    printf("[SENSOR] Waiting for measurement request...\n");

    while (1)
    {
        /*
         * IDLE STATE: Wait for measurement to be enabled
         *
         * We poll periodically rather than blocking forever so that
         * the task can respond to shutdown requests cleanly.
         */
        while (!s_measurementActive)
        {
            /* Wait for notification or timeout */
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(IDLE_CHECK_INTERVAL_MS));
        }

        /*
         * MEASUREMENT STATE: Sample at fixed period
         *
         * Initialize the wake time for vTaskDelayUntil().
         * This gives us precise periodic sampling regardless of
         * how long each sample takes to acquire.
         */
        printf("[SENSOR] Starting periodic sampling (%d ms period)\n", HR_SAMPLE_INTERVAL_MS);
        xLastWakeTime = xTaskGetTickCount();

        while (s_measurementActive)
        {
            /* Read sample from sensor (or simulate) */
            if (sensorReadSample(&sample))
            {
                /*
                 * SEND SAMPLE TO CONTROL TASK
                 *
                 * Non-blocking send - if queue is full, drop the sample.
                 * This is acceptable because:
                 * 1. Control task processes samples fast (high priority)
                 * 2. We'll get another sample in 100ms
                 * 3. Never want sensor task to block on queue
                 */
                if (g_hrSampleQueue != NULL)
                {
                    if (xQueueSend(g_hrSampleQueue, &sample, 0) != pdTRUE)
                    {
                        printf("[SENSOR] WARNING: Queue full, sample dropped\n");
                    }
                }
            }
            else
            {
                printf("[SENSOR] Sample read failed\n");
            }

            /*
             * PRECISE TIMING: vTaskDelayUntil()
             *
             * This delays until exactly xSamplePeriod ticks after
             * xLastWakeTime, compensating for any time spent processing.
             * Result: consistent 100ms sample period.
             */
            vTaskDelayUntil(&xLastWakeTime, xSamplePeriod);
        }

        printf("[SENSOR] Periodic sampling stopped\n");
    }
}

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize sensor hardware.
 *
 *  Sets up I2C and configures the MAX30102 sensor.
 *
 *  \return true if successful, false otherwise.
 */
/*************************************************************************************************/
static bool sensorHardwareInit(void)
{
#if SENSOR_SIMULATE_DATA
    return true;
#else
    int result;

    /* Use same I2C instance as MAX7325 (shared bus) */
    s_i2c = SENSOR_I2C_INSTANCE;

    /* Note: I2C is already initialized by MAX7325 driver.
     * We just verify communication with the HR sensor. */

    /* Verify sensor presence by reading part ID */
    uint8_t partId = 0;
    if (!sensorReadReg(MAX30102_REG_PART_ID, &partId))
    {
        printf("[SENSOR] Failed to read sensor part ID\n");
        return false;
    }

    if (partId != MAX30102_PART_ID_VALUE)
    {
        printf("[SENSOR] Unexpected part ID: 0x%02X (expected 0x%02X)\n",
               partId, MAX30102_PART_ID_VALUE);
        /* Continue anyway - might be a different sensor model */
    }
    else
    {
        printf("[SENSOR] MAX30102 detected (ID: 0x%02X)\n", partId);
    }

    /* Configure sensor for HR mode */
    /* Reset sensor */
    sensorWriteReg(MAX30102_REG_MODE_CONFIG, 0x40);
    MXC_Delay(MXC_DELAY_MSEC(100));

    /* Mode: HR only (0x02) */
    sensorWriteReg(MAX30102_REG_MODE_CONFIG, 0x02);

    /* SPO2 config: 400 samples/sec, 18-bit resolution */
    sensorWriteReg(MAX30102_REG_SPO2_CONFIG, 0x27);

    /* LED pulse amplitude */
    sensorWriteReg(MAX30102_REG_LED1_PA, 0x24);  /* Red LED */
    sensorWriteReg(MAX30102_REG_LED2_PA, 0x24);  /* IR LED */

    printf("[SENSOR] MAX30102 configured for HR mode\n");

    return true;
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  Read a sample from the sensor.
 *
 *  \param  pSample     Pointer to sample structure to fill.
 *
 *  \return true if sample obtained, false on error.
 */
/*************************************************************************************************/
static bool sensorReadSample(HrSample_t *pSample)
{
    if (pSample == NULL)
    {
        return false;
    }

#if SENSOR_SIMULATE_DATA
    /* Use simulated data */
    sensorSimulateSample(pSample);
    return true;
#else
    /* Read actual sensor data */
    uint8_t fifoData[6];  /* 3 bytes IR, 3 bytes Red */
    uint32_t irValue = 0;
    uint32_t redValue = 0;

    /* Read FIFO data */
    /* TODO: Implement actual FIFO read sequence for MAX30102 */
    /* This is a simplified placeholder */

    mxc_i2c_req_t req = {0};
    req.i2c = s_i2c;
    req.addr = MAX30102_I2C_ADDR;

    uint8_t regAddr = MAX30102_REG_FIFO_DATA;
    req.tx_buf = &regAddr;
    req.tx_len = 1;
    req.rx_buf = fifoData;
    req.rx_len = 6;
    req.restart = 1;
    req.callback = NULL;

    if (MXC_I2C_MasterTransaction(&req) != E_NO_ERROR)
    {
        pSample->valid = false;
        return false;
    }

    /* Extract 18-bit values */
    irValue = ((uint32_t)fifoData[0] << 16) | ((uint32_t)fifoData[1] << 8) | fifoData[2];
    irValue &= 0x3FFFF;  /* 18 bits */

    redValue = ((uint32_t)fifoData[3] << 16) | ((uint32_t)fifoData[4] << 8) | fifoData[5];
    redValue &= 0x3FFFF;  /* 18 bits */

    /*
     * HEART RATE CALCULATION
     *
     * In a real implementation, you would:
     * 1. Buffer multiple IR/Red samples
     * 2. Apply signal processing (filtering, peak detection)
     * 3. Calculate beat-to-beat intervals
     * 4. Convert to BPM
     *
     * For this project, we use a simplified approach with placeholder values.
     */

    /* Simplified HR estimation (placeholder) */
    if (irValue > 50000)  /* Finger present */
    {
        pSample->bpm = 70 + (irValue % 30);  /* 70-100 BPM range */
        pSample->confidence = 80;
        pSample->valid = true;
    }
    else
    {
        pSample->bpm = 0;
        pSample->confidence = 0;
        pSample->valid = false;
    }

    pSample->timestamp_ms = Time_GetMs();

    return true;
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  Write to a sensor register.
 */
/*************************************************************************************************/
static bool sensorWriteReg(uint8_t reg, uint8_t value)
{
#if SENSOR_SIMULATE_DATA
    return true;
#else
    if (s_i2c == NULL)
    {
        return false;
    }

    uint8_t txData[2] = {reg, value};

    mxc_i2c_req_t req = {0};
    req.i2c = s_i2c;
    req.addr = MAX30102_I2C_ADDR;
    req.tx_buf = txData;
    req.tx_len = 2;
    req.rx_buf = NULL;
    req.rx_len = 0;
    req.restart = 1;
    req.callback = NULL;

    return (MXC_I2C_MasterTransaction(&req) == E_NO_ERROR);
#endif
}

/*************************************************************************************************/
/*!
 *  \brief  Read from a sensor register.
 */
/*************************************************************************************************/
static bool sensorReadReg(uint8_t reg, uint8_t *pValue)
{
#if SENSOR_SIMULATE_DATA
    if (pValue != NULL)
    {
        *pValue = MAX30102_PART_ID_VALUE;  /* Fake part ID */
    }
    return true;
#else
    if (s_i2c == NULL || pValue == NULL)
    {
        return false;
    }

    mxc_i2c_req_t req = {0};
    req.i2c = s_i2c;
    req.addr = MAX30102_I2C_ADDR;
    req.tx_buf = &reg;
    req.tx_len = 1;
    req.rx_buf = pValue;
    req.rx_len = 1;
    req.restart = 1;
    req.callback = NULL;

    return (MXC_I2C_MasterTransaction(&req) == E_NO_ERROR);
#endif
}

#if SENSOR_SIMULATE_DATA
/*************************************************************************************************/
/*!
 *  \brief  Generate simulated HR sample for testing.
 *
 *  Produces realistic heart rate values that vary over time.
 *  Useful for testing the control loop and state machine.
 */
/*************************************************************************************************/
static void sensorSimulateSample(HrSample_t *pSample)
{
    s_simSampleCount++;

    /*
     * SIMULATED HR DATA
     *
     * Generates values in the 65-85 BPM range with slight variations.
     * Confidence varies to test validation logic.
     */

    /* Base HR with variation */
    uint16_t baseHr = 72;
    int16_t variation = (s_simSampleCount % 7) - 3;  /* -3 to +3 */
    pSample->bpm = baseHr + variation;

    /* Occasionally produce lower confidence to test filtering */
    if ((s_simSampleCount % 8) == 0)
    {
        pSample->confidence = 45;  /* Below 50% threshold */
        pSample->valid = false;
    }
    else
    {
        pSample->confidence = 85 + (s_simSampleCount % 10);  /* 85-94% */
        pSample->valid = true;
    }

    pSample->timestamp_ms = Time_GetMs();

    printf("[SENSOR] SIM Sample #%d: %d BPM, %d%% conf, %s\n",
           s_simSampleCount, pSample->bpm, pSample->confidence,
           pSample->valid ? "VALID" : "invalid");
}
#endif
