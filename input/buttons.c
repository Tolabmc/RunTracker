/*************************************************************************************************/
/*!
 *  \file   buttons.c
 *
 *  \brief  Button input handling implementation - FULLY STATIC ALLOCATION.
 *
 *  Includes a serial test interface for testing without hardware buttons.
 */
/*************************************************************************************************/

#include "buttons.h"
#include "time_utils.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

/* Maxim SDK includes for UART */
#include "mxc_device.h"
#include "uart.h"
#include "board.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define BUTTON_QUEUE_LENGTH     8
#define TEST_TASK_STACK_SIZE    256
#define TEST_TASK_PRIORITY      (tskIDLE_PRIORITY + 1)

/* Console UART instance - typically UART0 on most boards */
#define CONSOLE_UART_INST       MXC_UART_GET_UART(CONSOLE_UART)

/**************************************************************************************************
  Static Memory for FreeRTOS Objects
**************************************************************************************************/

/* Static queue storage */
static StaticQueue_t s_buttonQueueBuffer;
static uint8_t s_buttonQueueStorage[BUTTON_QUEUE_LENGTH * sizeof(ButtonEvent_t)];

/* Static task storage (only used if test task is enabled) */
static StaticTask_t s_testTaskBuffer;
static StackType_t s_testTaskStack[TEST_TASK_STACK_SIZE];

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Button event queue */
QueueHandle_t g_buttonQueue = NULL;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

static TaskHandle_t s_testTaskHandle = NULL;

/**************************************************************************************************
  Local Function Prototypes
**************************************************************************************************/

static void testInputTask(void *pvParameters);
static int uartGetCharNonBlocking(void);

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
bool Button_Init(void)
{
    /* Create the button event queue using STATIC allocation */
    g_buttonQueue = xQueueCreateStatic(
        BUTTON_QUEUE_LENGTH,
        sizeof(ButtonEvent_t),
        s_buttonQueueStorage,
        &s_buttonQueueBuffer);
    
    if (g_buttonQueue == NULL)
    {
        printf("[BTN] ERROR: Failed to create button queue\n");
        return false;
    }
    
    printf("[BTN] Button system initialized\n");
    return true;
}

/*************************************************************************************************/
bool Button_SendEvent(ButtonEventType_t type)
{
    if (g_buttonQueue == NULL)
    {
        return false;
    }
    
    ButtonEvent_t event;
    event.type = type;
    event.timestamp_ms = Time_GetMs();
    
    if (xQueueSend(g_buttonQueue, &event, 0) != pdTRUE)
    {
        printf("[BTN] WARNING: Queue full, event dropped\n");
        return false;
    }
    
    return true;
}

/*************************************************************************************************/
bool Button_StartTestTask(void)
{
    /* Create task using STATIC allocation */
    s_testTaskHandle = xTaskCreateStatic(
        testInputTask,
        "TestInput",
        TEST_TASK_STACK_SIZE,
        NULL,
        TEST_TASK_PRIORITY,
        s_testTaskStack,
        &s_testTaskBuffer);
    
    if (s_testTaskHandle == NULL)
    {
        printf("[BTN] ERROR: Failed to create test input task\n");
        return false;
    }
    
    printf("[BTN] Test input task started\n");
    Button_PrintHelp();
    
    return true;
}

/*************************************************************************************************/
void Button_PrintHelp(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  WORKOUT TRACKER - TEST COMMANDS\n");
    printf("========================================\n");
    printf("  s = START / RESUME workout\n");
    printf("  l = LAP (record lap time)\n");
    printf("  x = STOP / PAUSE workout\n");
    printf("  m = MODE (cycle workout mode)\n");
    printf("  ? = STATUS (print current state)\n");
    printf("  h = HELP (show this menu)\n");
    printf("========================================\n");
    printf("\n");
}

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Non-blocking character read from UART.
 *
 *  \return Character read, or -1 if no character available.
 */
/*************************************************************************************************/
static int uartGetCharNonBlocking(void)
{
    int result;
    
    /* Check if there's data available */
    result = MXC_UART_GetRXFIFOAvailable(CONSOLE_UART_INST);
    
    if (result > 0)
    {
        /* Read one character - returns the character directly */
        return MXC_UART_ReadCharacter(CONSOLE_UART_INST);
    }
    
    return -1;
}

/*************************************************************************************************/
/*!
 *  \brief  Test input task - reads UART and generates button events.
 */
/*************************************************************************************************/
static void testInputTask(void *pvParameters)
{
    (void)pvParameters;
    int ch;
    
    printf("[TEST] Ready for keyboard input (s/l/x/m/?/h)...\n");
    
    while (1)
    {
        /* Non-blocking check for input character */
        ch = uartGetCharNonBlocking();
        
        if (ch != -1)
        {
            ButtonEventType_t eventType = BTN_NONE;
            
            switch (ch)
            {
                case 's':
                case 'S':
                    eventType = BTN_START;
                    printf("\n[INPUT] -> START\n");
                    break;
                    
                case 'l':
                case 'L':
                    eventType = BTN_LAP;
                    printf("\n[INPUT] -> LAP\n");
                    break;
                    
                case 'x':
                case 'X':
                    eventType = BTN_STOP;
                    printf("\n[INPUT] -> STOP\n");
                    break;
                    
                case 'm':
                case 'M':
                    eventType = BTN_MODE_NEXT;
                    printf("\n[INPUT] -> MODE\n");
                    break;
                    
                case '?':
                    eventType = BTN_STATUS;
                    break;
                    
                case 'h':
                case 'H':
                    Button_PrintHelp();
                    break;
                    
                case '\n':
                case '\r':
                    /* Ignore newlines */
                    break;
                    
                default:
                    printf("\n[INPUT] Unknown '%c' (0x%02X) - press 'h' for help\n", 
                           (ch >= 32 && ch < 127) ? ch : '?', ch);
                    break;
            }
            
            if (eventType != BTN_NONE)
            {
                Button_SendEvent(eventType);
            }
        }
        
        /* Don't busy-wait - yield to other tasks */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
