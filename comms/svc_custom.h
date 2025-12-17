/*************************************************************************************************/
/*!
 *  \file   svc_custom.h
 *
 *  \brief  Custom BLE service for MAX32655 <-> ESP32 Firebeetle communication.
 *
 *  This service provides:
 *  - TX Characteristic: Send data from MAX32655 to ESP32 (Notify)
 *  - RX Characteristic: Receive data from ESP32 to MAX32655 (Write)
 */
/*************************************************************************************************/

#ifndef COMMS_SVC_CUSTOM_H
#define COMMS_SVC_CUSTOM_H

#include "att_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Handle Definitions
**************************************************************************************************/

/*! \brief Custom service handle definitions */
enum {
    CUSTOM_SVC_HDL = 0x0100,           /*!< Service declaration */
    CUSTOM_TX_CH_HDL,                   /*!< TX characteristic declaration */
    CUSTOM_TX_HDL,                      /*!< TX characteristic value */
    CUSTOM_TX_CH_CCC_HDL,               /*!< TX characteristic CCCD */
    CUSTOM_RX_CH_HDL,                   /*!< RX characteristic declaration */
    CUSTOM_RX_HDL,                      /*!< RX characteristic value */
    CUSTOM_SVC_HDL_END                  /*!< End of handles marker */
};

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Add the custom service to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCustomAddGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Remove the custom service from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCustomRemoveGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Register read and write callbacks for the custom service.
 *
 *  \param  readCback   Read callback function.
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCustomCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);

#ifdef __cplusplus
}
#endif

#endif /* COMMS_SVC_CUSTOM_H */

