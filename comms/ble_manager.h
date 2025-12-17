/*************************************************************************************************/
/*!
 *  \file   ble_manager.h
 *
 *  \brief  BLE Manager API for MAX32655 firmware.
 *
 *  This module provides the high-level BLE management interface including:
 *  - BLE stack initialization and startup
 *  - Connection management
 *  - Data transmission to connected devices
 */
/*************************************************************************************************/

#ifndef COMMS_BLE_MANAGER_H
#define COMMS_BLE_MANAGER_H

#include "wsf_os.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**************************************************************************************************
      Function Declarations
    **************************************************************************************************/

    /*************************************************************************************************/
    /*!
     *  \brief  Start the BLE application.
     *
     *  \return None.
     */
    /*************************************************************************************************/
    void DatsStart(void);

    /*************************************************************************************************/
    /*!
     *  \brief  Application handler init function called during system initialization.
     *
     *  \param  handlerID  WSF handler ID for App.
     *
     *  \return None.
     */
    /*************************************************************************************************/
    void DatsHandlerInit(wsfHandlerId_t handlerId);

    /*************************************************************************************************/
    /*!
     *  \brief  WSF event handler for the application.
     *
     *  \param  event   WSF event mask.
     *  \param  pMsg    WSF message.
     *
     *  \return None.
     */
    /*************************************************************************************************/
    void DatsHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

    /*************************************************************************************************/
    /*!
     *  \brief  Send data to the connected device via BLE notification.
     *
     *  \param  pData   Pointer to data to send.
     *  \param  len     Length of data in bytes.
     *
     *  \return TRUE if sent successfully, FALSE otherwise.
     */
    /*************************************************************************************************/
    bool_t DataSend(const uint8_t *pData, uint16_t len);

    /*************************************************************************************************/
    /*!
     *  \brief  Send a null-terminated string to the connected device.
     *
     *  \param  pStr    Pointer to null-terminated string.
     *
     *  \return TRUE if sent successfully, FALSE otherwise.
     */
    /*************************************************************************************************/
    bool_t DataSendString(const char *pStr);

    /*************************************************************************************************/
    /*!
     *  \brief  Initialize the BLE stack.
     *
     *  \return None.
     */
    /*************************************************************************************************/
    void StackInitDats(void);

    /*************************************************************************************************/
    /*!
     *  \brief  Start BLE subsystem (called from main).
     *
     *  \return None.
     */
    /*************************************************************************************************/
    void bleStartup(void);

    /*************************************************************************************************/
    /*!
     *  \brief  Check if BLE is connected.
     *
     *  \return TRUE if connected, FALSE otherwise.
     */
    /*************************************************************************************************/
    bool_t BLE_IsConnected(void);

    /*************************************************************************************************/
    /*!
     *  \brief  Get the current connection ID.
     *
     *  \return Connection ID or DM_CONN_ID_NONE if not connected.
     */
    /*************************************************************************************************/
    uint8_t BLE_GetConnId(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMS_BLE_MANAGER_H */
