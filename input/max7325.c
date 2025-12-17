/*************************************************************************************************/
/*!
 *  \file   max7325.c
 *
 *  \brief  MAX7325 I2C I/O Expander driver implementation.
 */
/*************************************************************************************************/

#include "max7325.h"
#include "buttons.h"
#include "time_utils.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* Maxim SDK includes */
#include "mxc_device.h"
#include "mxc_delay.h"
#include "i2c.h"
#include "gpio.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define POLLING_TASK_STACK_SIZE 224
#define POLLING_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define POLLING_INTERVAL_MS 50 /* Poll every 50ms */
#define DEBOUNCE_COUNT 2       /* Require 2 consecutive reads for valid press */

/* I2C configuration */
#define I2C_FREQ 100000 /* 100 kHz I2C */

/**************************************************************************************************
  Static Memory for FreeRTOS Objects
**************************************************************************************************/

/* Static task storage */
static StaticTask_t s_pollingTaskBuffer;
static StackType_t s_pollingTaskStack[POLLING_TASK_STACK_SIZE];

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

static mxc_i2c_regs_t *s_i2c = NULL;
static TaskHandle_t s_pollingTaskHandle = NULL;
static bool s_initialized = false;

/* Previous button states for edge detection */
static uint8_t s_prevState = 0xFF; /* All high = not pressed (active low) */

/**************************************************************************************************
  Local Function Prototypes
**************************************************************************************************/

static void pollingTask(void *pvParameters);
static void processButtonChange(uint8_t current, uint8_t previous);

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
bool Max7325_Init(void)
{
    int result;

    /* Use MXC_I2C2 with pins P0.10 (SCL) / P0.11 (SDA) */
    s_i2c = MXC_I2C2;

    /* Shutdown first in case it was already initialized */
    MXC_I2C_Shutdown(s_i2c);

    /* Initialize I2C as master */
    result = MXC_I2C_Init(s_i2c, 1, 0); /* master = 1, slave_addr = 0 */
    if (result != E_NO_ERROR)
    {
        printf("[MAX7325] ERROR: I2C init failed (%d)\n", result);
        return false;
    }

    /* Set I2C frequency */
    MXC_I2C_SetFrequency(s_i2c, I2C_FREQ);

    printf("[MAX7325] I2C2 initialized (SCL=P0.10, SDA=P0.11)\n");

    /* Configure P0-P7 as inputs by writing 0xFF (per datasheet Table 2) */
    uint8_t config_mask = 0xFF;
    mxc_i2c_req_t req = {0};
    req.i2c = s_i2c;
    req.addr = MAX7325_INPUT_ADDR;
    req.tx_buf = &config_mask;
    req.tx_len = 1;
    req.rx_buf = NULL;
    req.rx_len = 0;
    req.restart = 1;
    req.callback = NULL;

    result = MXC_I2C_MasterTransaction(&req);
    if (result != E_NO_ERROR)
    {
        printf("[MAX7325] ERROR: Failed to configure inputs (%d)\n", result);
        printf("[MAX7325] Check I2C wiring: SCL=P0.10, SDA=P0.11\n");
        return false;
    }

    printf("[MAX7325] Configured P0-P7 as inputs\n");

    s_initialized = true;
    s_prevState = 0xFF; /* Assume all released initially */

    printf("[MAX7325] Ready! SW1=START, SW2=LAP, SW3=STOP\n");

    return true;
}

/*************************************************************************************************/
bool Max7325_ReadButtons(Max7325ButtonState_t *state)
{
    if (!s_initialized || state == NULL)
    {
        return false;
    }

    uint8_t raw = Max7325_ReadRaw();

    state->raw = raw;
    /* Buttons are active LOW (pressed = 0) */
    state->sw1_start = !(raw & MAX7325_SW1_MASK);
    state->sw2_lap = !(raw & MAX7325_SW2_MASK);
    state->sw3_stop = !(raw & MAX7325_SW3_MASK);

    return true;
}

/*************************************************************************************************/
uint8_t Max7325_ReadRaw(void)
{
    if (!s_initialized || s_i2c == NULL)
    {
        return 0xFF;
    }

    int result;
    uint8_t raw[2] = {0xFF, 0x00}; /* [0]=port levels, [1]=transition flags */

    mxc_i2c_req_t req = {0};
    req.i2c = s_i2c;
    req.addr = MAX7325_INPUT_ADDR;
    req.tx_buf = NULL;
    req.tx_len = 0;
    req.rx_buf = raw;
    req.rx_len = 2;  /* Read 2 bytes per datasheet */
    req.restart = 1; /* Use restart condition */
    req.callback = NULL;

    result = MXC_I2C_MasterTransaction(&req);

    if (result != E_NO_ERROR)
    {
        return 0xFF;
    }

    return raw[0]; /* Return port levels (P7-P0) */
}

/*************************************************************************************************/
bool Max7325_IsButtonPressed(uint8_t mask)
{
    uint8_t raw = Max7325_ReadRaw();
    /* Active low: pressed = bit is 0 */
    return !(raw & mask);
}

/*************************************************************************************************/
bool Max7325_StartPollingTask(void)
{
    if (!s_initialized)
    {
        printf("[MAX7325] ERROR: Not initialized, cannot start polling\n");
        return false;
    }

    /* Create task using STATIC allocation */
    s_pollingTaskHandle = xTaskCreateStatic(
        pollingTask,
        "BtnPoll",
        POLLING_TASK_STACK_SIZE,
        NULL,
        POLLING_TASK_PRIORITY,
        s_pollingTaskStack,
        &s_pollingTaskBuffer);

    if (s_pollingTaskHandle == NULL)
    {
        printf("[MAX7325] ERROR: Failed to create polling task\n");
        return false;
    }

    printf("[MAX7325] Button polling task started\n");
    return true;
}

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Button polling task.
 */
/*************************************************************************************************/
static void pollingTask(void *pvParameters)
{
    (void)pvParameters;
    uint8_t currentState;
    uint8_t stableState = 0xFF;
    uint8_t readCount = 0;
    uint8_t lastRead = 0xFF;

    printf("[MAX7325] Button polling active\n");

    while (1)
    {
        /* Read current button state */
        currentState = Max7325_ReadRaw();

        /* Simple debouncing: require same reading twice */
        if (currentState == lastRead)
        {
            readCount++;
            if (readCount >= DEBOUNCE_COUNT)
            {
                /* State is stable */
                if (currentState != stableState)
                {
                    /* State changed - process it */
                    processButtonChange(currentState, stableState);
                    stableState = currentState;
                }
                readCount = DEBOUNCE_COUNT; /* Cap at max */
            }
        }
        else
        {
            readCount = 0;
        }
        lastRead = currentState;

        /* Wait before next poll */
        vTaskDelay(pdMS_TO_TICKS(POLLING_INTERVAL_MS));
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Process button state change and send events.
 */
/*************************************************************************************************/
static void processButtonChange(uint8_t current, uint8_t previous)
{
    /* Check each button for press (transition from high to low = released to pressed)
     * Active LOW: bit=1 means released, bit=0 means pressed
     * We want to detect when button goes from released (1) to pressed (0)
     */

    /* SW1 - START button */
    if ((previous & MAX7325_SW1_MASK) && !(current & MAX7325_SW1_MASK))
    {
        printf("[MAX7325] SW1 (START) pressed\n");
        Button_SendEvent(BTN_START);
    }

    /* SW2 - LAP button */
    if ((previous & MAX7325_SW2_MASK) && !(current & MAX7325_SW2_MASK))
    {
        printf("[MAX7325] SW2 (LAP) pressed\n");
        Button_SendEvent(BTN_LAP);
    }

    /* SW3 - STOP button */
    if ((previous & MAX7325_SW3_MASK) && !(current & MAX7325_SW3_MASK))
    {
        printf("[MAX7325] SW3 (STOP) pressed\n");
        Button_SendEvent(BTN_STOP);
    }

/* Optional: debug print for any other button changes */
#if 0
    if ((previous & MAX7325_SW4_MASK) && !(current & MAX7325_SW4_MASK))
    {
        printf("[MAX7325] SW4 pressed\n");
    }
    /* ... etc for SW5-SW8 ... */
#endif
}

/*************************************************************************************************/
void Max7325_ScanI2C(void)
{
    if (s_i2c == NULL)
    {
        printf("[MAX7325] I2C not initialized, cannot scan\n");
        return;
    }

    printf("[MAX7325] Scanning I2C%d...\n", MAX7325_I2C_INSTANCE);

    int found = 0;
    uint8_t data[2];

    /* Only scan likely MAX7325 addresses to speed up */
    uint8_t addrs_to_try[] = {0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
                              0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F};

    for (int i = 0; i < sizeof(addrs_to_try); i++)
    {
        uint8_t addr = addrs_to_try[i];

        printf("[MAX7325] Trying 0x%02X...", addr);
        fflush(stdout);

        /* Use transaction-based probe with restart (per working example) */
        mxc_i2c_req_t req = {0};
        req.i2c = s_i2c;
        req.addr = addr;
        req.tx_buf = NULL;
        req.tx_len = 0;
        req.rx_buf = data;
        req.rx_len = 2;
        req.restart = 1;
        req.callback = NULL;

        int result = MXC_I2C_MasterTransaction(&req);

        if (result == E_NO_ERROR)
        {
            printf(" FOUND (0x%02X)!\n", data[0]);
            found++;
        }
        else
        {
            printf(" no\n");
        }
    }

    printf("[MAX7325] Scan done. Found %d device(s)\n", found);

    if (found == 0)
    {
        printf("[MAX7325] Check: SCL=P0.10, SDA=P0.11 wiring\n");
    }
}
