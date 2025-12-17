/*************************************************************************************************/
/*!
 *  \file   max7325.h
 *
 *  \brief  MAX7325 I2C I/O Expander driver for button input.
 *
 *  The MAX7325 provides 8 I/O ports (P0-P7) which are connected to
 *  switches SW1-SW8 on the expansion board.
 *
 *  Button mapping:
 *  - SW1 (P0) = START
 *  - SW2 (P1) = LAP
 *  - SW3 (P2) = STOP
 */
/*************************************************************************************************/

#ifndef INPUT_MAX7325_H
#define INPUT_MAX7325_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! MAX7325 I2C addresses (7-bit)
 *  From working class example with AD2=AD0=GND:
 *  - Input address (P0-P7 open-drain): 0x68
 *  - Output address (O8-O15 push-pull): 0x58
 */
#define MAX7325_INPUT_ADDR          0x68    /* For reading button inputs P0-P7 */
#define MAX7325_OUTPUT_ADDR         0x58    /* For driving outputs O8-O15 */

/*! Which I2C instance to use
 *  From working class example: MXC_I2C2
 *  Pins: SCL = P0.10, SDA = P0.11 (with JP21/JP22 pull-ups)
 */
#define MAX7325_I2C_INSTANCE        2       /* MXC_I2C2 per working example */

/*! Button bit masks (directly correspond to P0-P7 pins) */
#define MAX7325_SW1_MASK            (1 << 0)    /* P0 = SW1 = START */
#define MAX7325_SW2_MASK            (1 << 1)    /* P1 = SW2 = LAP */
#define MAX7325_SW3_MASK            (1 << 2)    /* P2 = SW3 = STOP */
#define MAX7325_SW4_MASK            (1 << 3)    /* P3 = SW4 */
#define MAX7325_SW5_MASK            (1 << 4)    /* P4 = SW5 */
#define MAX7325_SW6_MASK            (1 << 5)    /* P5 = SW6 */
#define MAX7325_SW7_MASK            (1 << 6)    /* P6 = SW7 */
#define MAX7325_SW8_MASK            (1 << 7)    /* P7 = SW8 */

/**************************************************************************************************
  Type Definitions
**************************************************************************************************/

/*! Button state structure */
typedef struct
{
    bool sw1_start;     /*!< SW1 state (START button) */
    bool sw2_lap;       /*!< SW2 state (LAP button) */
    bool sw3_stop;      /*!< SW3 state (STOP button) */
    uint8_t raw;        /*!< Raw register value */
} Max7325ButtonState_t;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Initialize the MAX7325 I/O expander.
 *
 *  Sets up I2C communication and configures pins as inputs.
 *
 *  \return true if initialization successful, false otherwise.
 */
/*************************************************************************************************/
bool Max7325_Init(void);

/*************************************************************************************************/
/*!
 *  \brief  Read the current button states from MAX7325.
 *
 *  \param  state   Pointer to receive button states.
 *
 *  \return true if read successful, false on I2C error.
 */
/*************************************************************************************************/
bool Max7325_ReadButtons(Max7325ButtonState_t *state);

/*************************************************************************************************/
/*!
 *  \brief  Read raw port value from MAX7325.
 *
 *  \return Raw 8-bit port value, or 0xFF on error.
 */
/*************************************************************************************************/
uint8_t Max7325_ReadRaw(void);

/*************************************************************************************************/
/*!
 *  \brief  Check if a specific button is pressed.
 *
 *  \param  mask    Button mask (MAX7325_SW1_MASK, etc.)
 *
 *  \return true if button is pressed (active low).
 */
/*************************************************************************************************/
bool Max7325_IsButtonPressed(uint8_t mask);

/*************************************************************************************************/
/*!
 *  \brief  Start the button polling task.
 *
 *  Creates a FreeRTOS task that polls the MAX7325 and sends
 *  button events to the control task queue.
 *
 *  \return true if task created successfully.
 */
/*************************************************************************************************/
bool Max7325_StartPollingTask(void);

/*************************************************************************************************/
/*!
 *  \brief  Scan I2C bus for devices.
 *
 *  Scans addresses 0x08-0x77 and prints any responding devices.
 *  Useful for finding the correct MAX7325 address.
 */
/*************************************************************************************************/
void Max7325_ScanI2C(void);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_MAX7325_H */

