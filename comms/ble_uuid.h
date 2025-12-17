/*************************************************************************************************/
/*!
 *  \file   ble_uuid.h
 *
 *  \brief  Custom BLE UUIDs for MAX32655 <-> ESP32 Firebeetle communication.
 *
 *  These UUIDs must match the UUIDs used in the ESP32 Firebeetle code.
 *  
 *  Service: MAX32655 Custom Data Service
 *  - TX Characteristic: MAX32655 sends data (notify) -> ESP32 receives
 *  - RX Characteristic: ESP32 sends data (write) -> MAX32655 receives
 */
/*************************************************************************************************/

#ifndef COMMS_BLE_UUID_H
#define COMMS_BLE_UUID_H

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Custom Service UUID (128-bit) 
 *  UUID: 12345678-1234-5678-1234-56789ABCDEF0
 *  
 *  Note: BLE UUIDs are stored in little-endian format in the arrays below.
 */
#define CUSTOM_SVC_UUID             0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12, \
                                    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12

/*! \brief TX Characteristic UUID (128-bit) - MAX32655 -> ESP32 (Notify)
 *  UUID: 12345678-1234-5678-1234-56789ABCDEF1
 */
#define CUSTOM_TX_CHAR_UUID         0xF1, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12, \
                                    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12

/*! \brief RX Characteristic UUID (128-bit) - ESP32 -> MAX32655 (Write)
 *  UUID: 12345678-1234-5678-1234-56789ABCDEF2
 */
#define CUSTOM_RX_CHAR_UUID         0xF2, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12, \
                                    0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12

/*! \brief Maximum data length for characteristics */
#define CUSTOM_MAX_DATA_LEN         128

/*! \brief Human-readable UUID strings for ESP32 code reference:
 *  
 *  Service UUID:        "12345678-1234-5678-1234-56789ABCDEF0"
 *  TX Characteristic:   "12345678-1234-5678-1234-56789ABCDEF1"
 *  RX Characteristic:   "12345678-1234-5678-1234-56789ABCDEF2"
 *  
 *  ESP32 Arduino Code Example:
 *  ---------------------------
 *  #include <BLEDevice.h>
 *  
 *  #define SERVICE_UUID        "12345678-1234-5678-1234-56789ABCDEF0"
 *  #define TX_CHAR_UUID        "12345678-1234-5678-1234-56789ABCDEF1"  // Receive notifications
 *  #define RX_CHAR_UUID        "12345678-1234-5678-1234-56789ABCDEF2"  // Send writes
 */

#ifdef __cplusplus
}
#endif

#endif /* COMMS_BLE_UUID_H */

