/*************************************************************************************************/
/*!
 *  \file   ble_manager.c
 *
 *  \brief  BLE Data Transfer Application for MAX32655 <-> ESP32 Firebeetle communication.
 *
 *  This application sets up a BLE peripheral with a custom service for bidirectional
 *  data transfer with an ESP32 Firebeetle (or any BLE central).
 */
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "util/bstream.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "wsf_buf.h"
#include "wsf_nvm.h"
#include "wsf_timer.h"
#include "hci_api.h"
#include "sec_api.h"
#include "dm_api.h"
#include "smp_api.h"
#include "att_api.h"
#include "app_api.h"
#include "app_main.h"
#include "app_db.h"
#include "app_ui.h"
#include "svc_ch.h"
#include "svc_core.h"
#include "util/calc128.h"
#include "gatt/gatt_api.h"
#include "ble_manager.h"
#include "wut.h"
#include "trimsir_regs.h"
#include "pal_btn.h"
#include "tmr.h"

/* Custom service includes */
#include "ble_uuid.h"
#include "svc_custom.h"

/* Workout control includes */
#include "buttons.h"
#include "workout_state.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

#define TRIM_TIMER_EVT 0x99
#define TRIM_TIMER_PERIOD_MS 60000

/*! Button press handling constants */
#define BTN_SHORT_MS 200
#define BTN_MED_MS 500
#define BTN_LONG_MS 1000

#define BTN_1_TMR MXC_TMR2
#define BTN_2_TMR MXC_TMR3

/*! Enumeration of client characteristic configuration descriptors */
enum
{
    DATS_GATT_SC_CCC_IDX,   /*! GATT service, service changed characteristic */
    DATS_CUSTOM_TX_CCC_IDX, /*! Custom service, TX characteristic (notify) */
    DATS_NUM_CCC_IDX
};

/**************************************************************************************************
  Configurable Parameters
**************************************************************************************************/

/*! Advertising configuration - adjust intervals as needed */
static const appAdvCfg_t datsAdvCfg = {
    {0, 0, 0},    /*! Advertising durations in ms (0 = infinite) */
    {160, 800, 0} /*! Advertising intervals in 0.625ms units (100ms, 500ms) */
};

/*! Slave configuration */
static const appSlaveCfg_t datsSlaveCfg = {
    1, /*! Maximum connections */
};

/*! Security configuration - simplified for development */
static const appSecCfg_t datsSecCfg = {
    0,     /*! Authentication and bonding flags (disabled for easy pairing) */
    0,     /*! Initiator key distribution flags */
    0,     /*! Responder key distribution flags */
    FALSE, /*! Out-of-band pairing data present */
    FALSE  /*! Initiate security upon connection */
};

/*! SMP security configuration */
static const smpCfg_t datsSmpCfg = {
    500,                 /*! 'Repeated attempts' timeout in msec */
    SMP_IO_NO_IN_NO_OUT, /*! I/O Capability */
    7,                   /*! Minimum encryption key length */
    16,                  /*! Maximum encryption key length */
    1,                   /*! Attempts to trigger 'repeated attempts' timeout */
    0,                   /*! Device authentication requirements */
    64000,               /*! Maximum repeated attempts timeout in msec */
    64000,               /*! Time msec before attemptExp decreases */
    2                    /*! Repeated attempts multiplier exponent */
};

/*! Connection parameter update configuration */
static const appUpdateCfg_t datsUpdateCfg = {
    3000, /*! Connection idle period before update (ms) */
    24,   /*! Minimum connection interval (30ms in 1.25ms units) */
    40,   /*! Maximum connection interval (50ms in 1.25ms units) */
    0,    /*! Connection latency */
    600,  /*! Supervision timeout (6s in 10ms units) */
    5     /*! Number of update attempts */
};

/*! ATT configuration - increased MTU for larger data transfers */
static const attCfg_t datsAttCfg = {
    15,                    /*! ATT server service discovery idle timeout (seconds) */
    241,                   /*! Desired ATT MTU */
    ATT_MAX_TRANS_TIMEOUT, /*! Transaction timeout (seconds) */
    4                      /*! Number of queued prepare writes */
};

/*! Local IRK (Identity Resolving Key) */
static uint8_t localIrk[] = {
    0x95, 0xC8, 0xEE, 0x6F, 0xC5, 0x0D, 0xEF, 0x93,
    0x35, 0x4E, 0x7C, 0x57, 0x08, 0xE2, 0xA3, 0x85};

/**************************************************************************************************
  Advertising Data
**************************************************************************************************/

/*! Device name - change this to identify your device */
#define DEVICE_NAME "MAX32655"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/*! Advertising data */
static const uint8_t datsAdvDataDisc[] = {
    /* Flags */
    2,                 /*! Length */
    DM_ADV_TYPE_FLAGS, /*! AD type */
    DM_FLAG_LE_GENERAL_DISC | DM_FLAG_LE_BREDR_NOT_SUP,

    /* Complete local name */
    DEVICE_NAME_LEN + 1,    /*! Length */
    DM_ADV_TYPE_LOCAL_NAME, /*! AD type */
    'M', 'A', 'X', '3', '2', '6', '5', '5'};

/*! Scan response data - includes service UUID for filtering */
static uint8_t datsScanDataDisc[] = {
    /* Complete 128-bit service UUID */
    17,                   /*! Length */
    DM_ADV_TYPE_128_UUID, /*! AD type */
    CUSTOM_SVC_UUID       /*! Custom service UUID */
};

/**************************************************************************************************
  Client Characteristic Configuration Descriptors
**************************************************************************************************/

/*! CCC descriptors */
static const attsCccSet_t datsCccSet[DATS_NUM_CCC_IDX] = {
    /* Handle                   Value Range                 Security Level */
    {GATT_SC_CH_CCC_HDL, ATT_CLIENT_CFG_INDICATE, DM_SEC_LEVEL_NONE},
    {CUSTOM_TX_CH_CCC_HDL, ATT_CLIENT_CFG_NOTIFY, DM_SEC_LEVEL_NONE}};

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/*! Application control block */
static struct
{
    wsfHandlerId_t handlerId; /*! WSF handler ID */
    dmConnId_t connId;        /*! Current connection ID */
    bool_t connected;         /*! Connection state */
} datsCb;

/*! Timer for 32kHz crystal trim */
static wsfTimer_t trimTimer;

extern void setAdvTxPower(void);

/**************************************************************************************************
  Data Transfer Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Send data to the connected ESP32 via notification.
 *
 *  \param  pData   Pointer to data to send.
 *  \param  len     Length of data.
 *
 *  \return TRUE if sent, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t DataSend(const uint8_t *pData, uint16_t len)
{
    if (!datsCb.connected || datsCb.connId == DM_CONN_ID_NONE)
    {
        APP_TRACE_INFO0("DataSend: Not connected");
        return FALSE;
    }

    if (!AttsCccEnabled(datsCb.connId, DATS_CUSTOM_TX_CCC_IDX))
    {
        APP_TRACE_INFO0("DataSend: Notifications not enabled");
        return FALSE;
    }

    if (len > CUSTOM_MAX_DATA_LEN)
    {
        len = CUSTOM_MAX_DATA_LEN;
    }

    /* Send notification */
    AttsHandleValueNtf(datsCb.connId, CUSTOM_TX_HDL, len, (uint8_t *)pData);

    APP_TRACE_INFO1("DataSend: %d bytes", len);

    return TRUE;
}

/*************************************************************************************************/
/*!
 *  \brief  Send a string to the connected ESP32.
 *
 *  \param  pStr    Null-terminated string to send.
 *
 *  \return TRUE if sent, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t DataSendString(const char *pStr)
{
    return DataSend((const uint8_t *)pStr, strlen(pStr));
}

/*************************************************************************************************/
/*!
 *  \brief  Check if BLE is connected.
 *
 *  \return TRUE if connected, FALSE otherwise.
 */
/*************************************************************************************************/
bool_t BLE_IsConnected(void)
{
    return datsCb.connected;
}

/*************************************************************************************************/
/*!
 *  \brief  Get the current connection ID.
 *
 *  \return Connection ID or DM_CONN_ID_NONE if not connected.
 */
/*************************************************************************************************/
uint8_t BLE_GetConnId(void)
{
    return datsCb.connId;
}

/**************************************************************************************************
  Callback Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Custom service write callback - called when ESP32 sends data.
 *
 *  \param  connId      Connection ID.
 *  \param  handle      Attribute handle.
 *  \param  operation   Write operation type.
 *  \param  offset      Write offset.
 *  \param  len         Data length.
 *  \param  pValue      Pointer to data.
 *  \param  pAttr       Pointer to attribute.
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
static uint8_t customWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                                uint16_t offset, uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{
    if (handle == CUSTOM_RX_HDL)
    {
        /* Data received from ESP32 - process commands here */
        if (len > 0 && len < CUSTOM_MAX_DATA_LEN)
        {
            char tempBuf[CUSTOM_MAX_DATA_LEN + 1];
            memcpy(tempBuf, pValue, len);
            tempBuf[len] = '\0';

            /* Parse commands from ESP32 if needed */
            /* For now, just acknowledge receipt */
            APP_TRACE_INFO1("ESP32: %s", tempBuf);
        }
    }

    return ATT_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \brief  Application DM callback.
 */
/*************************************************************************************************/
static void datsDmCback(dmEvt_t *pDmEvt)
{
    dmEvt_t *pMsg;
    uint16_t len;

    if (pDmEvt->hdr.event == DM_SEC_ECC_KEY_IND)
    {
        DmSecSetEccKey(&pDmEvt->eccMsg.data.key);
    }
    else
    {
        len = DmSizeOfEvt(pDmEvt);
        if ((pMsg = WsfMsgAlloc(len)) != NULL)
        {
            memcpy(pMsg, pDmEvt, len);
            WsfMsgSend(datsCb.handlerId, pMsg);
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Application ATT callback.
 */
/*************************************************************************************************/
static void datsAttCback(attEvt_t *pEvt)
{
    attEvt_t *pMsg;

    if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL)
    {
        memcpy(pMsg, pEvt, sizeof(attEvt_t));
        pMsg->pValue = (uint8_t *)(pMsg + 1);
        memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
        WsfMsgSend(datsCb.handlerId, pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Application CCC callback.
 */
/*************************************************************************************************/
static void datsCccCback(attsCccEvt_t *pEvt)
{
    if (pEvt->idx == DATS_CUSTOM_TX_CCC_IDX)
    {
        if (pEvt->value == ATT_CLIENT_CFG_NOTIFY)
        {
            APP_TRACE_INFO0("ESP32 enabled notifications - ready to send data");
        }
        else
        {
            APP_TRACE_INFO0("ESP32 disabled notifications");
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Start the 32kHz crystal trim procedure.
 */
/*************************************************************************************************/
static void trimStart(void)
{
    extern void wutTrimCb(int err);
    int err = MXC_WUT_TrimCrystalAsync(MXC_WUT0, wutTrimCb);
    if (err != E_NO_ERROR)
    {
        APP_TRACE_INFO1("Error starting 32kHz crystal trim %d", err);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Set up advertising after device reset.
 */
/*************************************************************************************************/
static void datsSetup(dmEvt_t *pMsg)
{
    /* Set advertising data */
    AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(datsAdvDataDisc), (uint8_t *)datsAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(datsScanDataDisc), datsScanDataDisc);

    AppAdvSetData(APP_ADV_DATA_CONNECTABLE, sizeof(datsAdvDataDisc), (uint8_t *)datsAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, sizeof(datsScanDataDisc), datsScanDataDisc);

    /* Start advertising */
    AppAdvStart(APP_MODE_AUTO_INIT);
}

/*************************************************************************************************/
/*!
 *  \brief  Process application messages.
 */
/*************************************************************************************************/
static void datsProcMsg(dmEvt_t *pMsg)
{
    uint8_t uiEvent = APP_UI_NONE;

    switch (pMsg->hdr.event)
    {
    case DM_RESET_CMPL_IND:
        AttsCalculateDbHash();
        DmSecGenerateEccKeyReq();
        datsSetup(pMsg);
        setAdvTxPower();
        uiEvent = APP_UI_RESET_CMPL;
        APP_TRACE_INFO0("=== MAX32655 BLE Ready ===");
        APP_TRACE_INFO0("Advertising as 'MAX32655'");
        APP_TRACE_INFO0("Waiting for ESP32 connection...");
        break;

    case DM_ADV_START_IND:
        WsfTimerStartMs(&trimTimer, TRIM_TIMER_PERIOD_MS);
        uiEvent = APP_UI_ADV_START;
        APP_TRACE_INFO0("Advertising started");
        break;

    case DM_ADV_STOP_IND:
        WsfTimerStop(&trimTimer);
        uiEvent = APP_UI_ADV_STOP;
        APP_TRACE_INFO0("Advertising stopped");
        break;

    case DM_CONN_OPEN_IND:
        datsCb.connected = TRUE;
        datsCb.connId = (dmConnId_t)pMsg->hdr.param;
        uiEvent = APP_UI_CONN_OPEN;
        APP_TRACE_INFO0("=== ESP32 Connected! ===");
        APP_TRACE_INFO1("Connection ID: %d", datsCb.connId);
        break;

    case DM_CONN_CLOSE_IND:
        datsCb.connected = FALSE;
        datsCb.connId = DM_CONN_ID_NONE;
        WsfTimerStop(&trimTimer);
        uiEvent = APP_UI_CONN_CLOSE;
        APP_TRACE_INFO0("=== Connection Closed ===");
        APP_TRACE_INFO1("Reason: 0x%02x", pMsg->connClose.reason);
        break;

    case DM_CONN_UPDATE_IND:
        APP_TRACE_INFO0("Connection parameters updated");
        break;

    case TRIM_TIMER_EVT:
        trimStart();
        WsfTimerStartMs(&trimTimer, TRIM_TIMER_PERIOD_MS);
        break;

    default:
        break;
    }

    if (uiEvent != APP_UI_NONE)
    {
        AppUiAction(uiEvent);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Button callback.
 */
/*************************************************************************************************/
static void datsBtnCback(uint8_t btn)
{
    if (datsCb.connected)
    {
        switch (btn)
        {
        case APP_UI_BTN_1_SHORT:
            /* Send test message when button pressed while connected */
            DataSendString("Hello from MAX32655!");
            break;
        case APP_UI_BTN_2_SHORT:
            /* Disconnect */
            AppConnClose(datsCb.connId);
            break;
        default:
            break;
        }
    }
    else
    {
        switch (btn)
        {
        case APP_UI_BTN_1_SHORT:
            /* Start advertising */
            AppAdvStart(APP_MODE_AUTO_INIT);
            break;
        case APP_UI_BTN_2_SHORT:
            /* Stop advertising */
            AppAdvStop();
            break;
        default:
            break;
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Platform button press handler.
 */
/*************************************************************************************************/
static void btnPressHandler(uint8_t btnId, PalBtnPos_t state)
{
    if (btnId == 1)
    {
        if (state == PAL_BTN_POS_UP)
        {
            unsigned btnUs = MXC_TMR_SW_Stop(BTN_1_TMR);
            if ((btnUs > 0) && (btnUs < BTN_SHORT_MS * 1000))
            {
                AppUiBtnTest(APP_UI_BTN_1_SHORT);
            }
            else if (btnUs < BTN_MED_MS * 1000)
            {
                AppUiBtnTest(APP_UI_BTN_1_MED);
            }
            else if (btnUs < BTN_LONG_MS * 1000)
            {
                AppUiBtnTest(APP_UI_BTN_1_LONG);
            }
            else
            {
                AppUiBtnTest(APP_UI_BTN_1_EX_LONG);
            }
        }
        else
        {
            MXC_TMR_SW_Start(BTN_1_TMR);
        }
    }
    else if (btnId == 2)
    {
        if (state == PAL_BTN_POS_UP)
        {
            unsigned btnUs = MXC_TMR_SW_Stop(BTN_2_TMR);
            if ((btnUs > 0) && (btnUs < BTN_SHORT_MS * 1000))
            {
                AppUiBtnTest(APP_UI_BTN_2_SHORT);
            }
            else if (btnUs < BTN_MED_MS * 1000)
            {
                AppUiBtnTest(APP_UI_BTN_2_MED);
            }
            else if (btnUs < BTN_LONG_MS * 1000)
            {
                AppUiBtnTest(APP_UI_BTN_2_LONG);
            }
            else
            {
                AppUiBtnTest(APP_UI_BTN_2_EX_LONG);
            }
        }
        else
        {
            MXC_TMR_SW_Start(BTN_2_TMR);
        }
    }
}

/**************************************************************************************************
  Public Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Application handler init.
 */
/*************************************************************************************************/
void DatsHandlerInit(wsfHandlerId_t handlerId)
{
    uint8_t addr[6] = {0};

    APP_TRACE_INFO0("=== MAX32655 BLE Application ===");
    AppGetBdAddr(addr);
    APP_TRACE_INFO6("MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                    addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    /* Initialize control block */
    datsCb.handlerId = handlerId;
    datsCb.connId = DM_CONN_ID_NONE;
    datsCb.connected = FALSE;

    /* Set configuration pointers */
    pAppSlaveCfg = (appSlaveCfg_t *)&datsSlaveCfg;
    pAppAdvCfg = (appAdvCfg_t *)&datsAdvCfg;
    pAppSecCfg = (appSecCfg_t *)&datsSecCfg;
    pAppUpdateCfg = (appUpdateCfg_t *)&datsUpdateCfg;
    pSmpCfg = (smpCfg_t *)&datsSmpCfg;
    pAttCfg = (attCfg_t *)&datsAttCfg;

    /* Initialize application framework */
    AppSlaveInit();
    AppServerInit();

    /* Set local IRK */
    DmSecSetLocalIrk(localIrk);

    /* Setup trim timer */
    trimTimer.handlerId = handlerId;
    trimTimer.msg.event = TRIM_TIMER_EVT;
}

/*************************************************************************************************/
/*!
 *  \brief  WSF event handler.
 */
/*************************************************************************************************/
void DatsHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    if (pMsg != NULL)
    {
        /* Process ATT messages */
        if (pMsg->event >= ATT_CBACK_START && pMsg->event <= ATT_CBACK_END)
        {
            AppServerProcAttMsg(pMsg);
        }
        else if (pMsg->event >= DM_CBACK_START && pMsg->event <= DM_CBACK_END)
        {
            /* Process DM messages */
            AppSlaveProcDmMsg((dmEvt_t *)pMsg);
            AppSlaveSecProcDmMsg((dmEvt_t *)pMsg);
        }

        datsProcMsg((dmEvt_t *)pMsg);
    }
}

/*************************************************************************************************/
/*!
 *  \brief  Start the application.
 */
/*************************************************************************************************/
void DatsStart(void)
{
    /* Register callbacks */
    DmRegister(datsDmCback);
    DmConnRegister(DM_CLIENT_ID_APP, datsDmCback);
    AttRegister(datsAttCback);
    AttConnRegister(AppServerConnCback);
    AttsCccRegister(DATS_NUM_CCC_IDX, (attsCccSet_t *)datsCccSet, datsCccCback);

    /* Add GATT service */
    SvcCoreGattCbackRegister(GattReadCback, GattWriteCback);
    SvcCoreAddGroup();

    /* Add custom service for ESP32 communication */
    SvcCustomCbackRegister(NULL, customWriteCback);
    SvcCustomAddGroup();

    /* Set Service Changed CCCD index */
    GattSetSvcChangedIdx(DATS_GATT_SC_CCC_IDX);

    /* Register button callbacks */
    AppUiBtnRegister(datsBtnCback);

    /* Initialize NVM */
    WsfNvmInit();

    /* Initialize buttons */
    PalBtnInit(btnPressHandler);

    /* Reset the device to start */
    DmDevReset();
}
