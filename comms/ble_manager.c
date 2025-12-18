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
#include "ble_uuid.h"
#include "svc_custom.h"
#include "control_task.h"

/* ---------- BLE Configuration ---------- */

#define DEVICE_NAME       "MAX32655"
#define DEVICE_NAME_LEN   (sizeof(DEVICE_NAME) - 1)

#define TRIM_TIMER_EVT        0x99
#define TRIM_TIMER_PERIOD_MS 60000

enum
{
    DATS_GATT_SC_CCC_IDX,
    DATS_CUSTOM_TX_CCC_IDX,
    DATS_NUM_CCC_IDX
};

/* ---------- Configuration Structures ---------- */

/*! Advertising configuration */
static const appAdvCfg_t advCfg =
{
    {0, 0, 0},           /*! Advertising durations in ms (0 = infinite) */
    {160, 800, 0}        /*! Advertising intervals in 0.625ms units (100ms, 500ms) */
};

/*! Slave configuration */
static const appSlaveCfg_t slaveCfg =
{
    1,                   /*! Maximum connections */
};

/*! Security configuration */
static const appSecCfg_t secCfg =
{
    0,                   /*! Authentication and bonding flags */
    0,                   /*! Initiator key distribution flags */
    0,                   /*! Responder key distribution flags */
    FALSE,               /*! Out-of-band pairing data present */
    FALSE                /*! Initiate security upon connection */
};

/*! SMP security configuration */
static const smpCfg_t smpCfg =
{
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
static const appUpdateCfg_t updateCfg =
{
    3000,                /*! Connection idle period before update (ms) */
    24,                  /*! Minimum connection interval (30ms in 1.25ms units) */
    40,                  /*! Maximum connection interval (50ms in 1.25ms units) */
    0,                   /*! Connection latency */
    600,                 /*! Supervision timeout (6s in 10ms units) */
    5                    /*! Number of update attempts */
};

/*! ATT configuration */
static const attCfg_t attCfg =
{
    15,                  /*! ATT server service discovery idle timeout (seconds) */
    241,                 /*! Desired ATT MTU */
    ATT_MAX_TRANS_TIMEOUT, /*! Transaction timeout (seconds) */
    4                    /*! Number of queued prepare writes */
};

/*! Local IRK */
static uint8_t localIrk[] =
{
    0x95, 0xC8, 0xEE, 0x6F, 0xC5, 0x0D, 0xEF, 0x93,
    0x35, 0x4E, 0x7C, 0x57, 0x08, 0xE2, 0xA3, 0x85
};

/* ---------- Advertising Data ---------- */

static const uint8_t advData[] =
{
    2, DM_ADV_TYPE_FLAGS, DM_FLAG_LE_GENERAL_DISC | DM_FLAG_LE_BREDR_NOT_SUP,
    DEVICE_NAME_LEN + 1, DM_ADV_TYPE_LOCAL_NAME,
    'M','A','X','3','2','6','5','5'
};

static uint8_t scanData[] =
{
    17, DM_ADV_TYPE_128_UUID,
    CUSTOM_SVC_UUID
};

/* ---------- CCC ---------- */

static const attsCccSet_t cccSet[DATS_NUM_CCC_IDX] =
{
    { GATT_SC_CH_CCC_HDL,     ATT_CLIENT_CFG_INDICATE, DM_SEC_LEVEL_NONE },
    { CUSTOM_TX_CH_CCC_HDL,  ATT_CLIENT_CFG_NOTIFY,  DM_SEC_LEVEL_NONE }
};

/* ---------- App Control Block ---------- */

static struct
{
    wsfHandlerId_t handlerId;
    dmConnId_t     connId;
    bool_t         connected;
} bleCb;

static wsfTimer_t trimTimer;

extern void setAdvTxPower(void);

/* ---------- Public API ---------- */

bool_t BLE_IsConnected(void)
{
    return bleCb.connected;
}

bool_t DataSend(const uint8_t *pData, uint16_t len)
{
    if (!bleCb.connected || bleCb.connId == DM_CONN_ID_NONE)
    {
        APP_TRACE_INFO0("DataSend: Not connected");
        return FALSE;
    }

    if (!AttsCccEnabled(bleCb.connId, DATS_CUSTOM_TX_CCC_IDX))
    {
        APP_TRACE_INFO0("DataSend: Notifications not enabled");
        return FALSE;
    }

    if (len > CUSTOM_MAX_DATA_LEN)
        len = CUSTOM_MAX_DATA_LEN;

    AttsHandleValueNtf(bleCb.connId, CUSTOM_TX_HDL, len, (uint8_t *)pData);
    APP_TRACE_INFO1("DataSend: %d bytes", len);
    return TRUE;
}

bool_t DataSendString(const char *pStr)
{
    return DataSend((const uint8_t *)pStr, strlen(pStr));
}

/* ---------- RX Callback (ESP32 -> MAX) ---------- */

static uint8_t customWriteCback(dmConnId_t connId,
                                uint16_t handle,
                                uint8_t  operation,
                                uint16_t offset,
                                uint16_t len,
                                uint8_t *pValue,
                                attsAttr_t *pAttr)
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
            APP_TRACE_INFO1("ESP32: %s", tempBuf);

            /* Notify control task when ESP signals HR completion */
            if (strstr(tempBuf, "\"cmd\":\"hr_done\"") != NULL)
            {
                ControlTask_SendBleEvent(BLE_CTRL_EVT_HR_DONE);
            }
        }
    }

    return ATT_SUCCESS;
}

/* ---------- BLE Stack Callbacks ---------- */

static void dmCback(dmEvt_t *pEvt)
{
    dmEvt_t *pMsg;
    uint16_t len;

    if (pEvt->hdr.event == DM_SEC_ECC_KEY_IND)
    {
        DmSecSetEccKey(&pEvt->eccMsg.data.key);
        return;
    }

    len = DmSizeOfEvt(pEvt);
    if ((pMsg = WsfMsgAlloc(len)) != NULL)
    {
        memcpy(pMsg, pEvt, len);
        WsfMsgSend(bleCb.handlerId, pMsg);
    }
}

static void attCback(attEvt_t *pEvt)
{
    attEvt_t *pMsg;

    if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL)
    {
        memcpy(pMsg, pEvt, sizeof(attEvt_t));
        pMsg->pValue = (uint8_t *)(pMsg + 1);
        memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
        WsfMsgSend(bleCb.handlerId, pMsg);
    }
}

static void cccCback(attsCccEvt_t *pEvt)
{
    if (pEvt->idx == DATS_CUSTOM_TX_CCC_IDX)
    {
        APP_TRACE_INFO0(
            pEvt->value == ATT_CLIENT_CFG_NOTIFY ?
            "ESP32 notifications enabled - ready to send" :
            "ESP32 notifications disabled"
        );
    }
}

/* ---------- Trim (optional) ---------- */

static void trimStart(void)
{
    // Optional: Implement crystal trim if needed
}

/* ---------- Advertising Setup ---------- */

static void setupAdvertising(void)
{
    AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(advData), (uint8_t *)advData);
    AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(scanData), scanData);

    AppAdvSetData(APP_ADV_DATA_CONNECTABLE, sizeof(advData), (uint8_t *)advData);
    AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, sizeof(scanData), scanData);

    AppAdvStart(APP_MODE_AUTO_INIT);
}

/* ---------- Message Processing ---------- */

static void processMsg(dmEvt_t *pMsg)
{
    switch (pMsg->hdr.event)
    {
    case DM_RESET_CMPL_IND:
        AttsCalculateDbHash();
        DmSecGenerateEccKeyReq();
        setupAdvertising();
        setAdvTxPower();
        APP_TRACE_INFO0("=== MAX32655 BLE Ready ===");
        APP_TRACE_INFO0("Advertising as 'MAX32655'");
        APP_TRACE_INFO0("Waiting for ESP32 connection...");
        break;

    case DM_ADV_START_IND:
        WsfTimerStartMs(&trimTimer, TRIM_TIMER_PERIOD_MS);
        APP_TRACE_INFO0("Advertising started");
        break;

    case DM_ADV_STOP_IND:
        WsfTimerStop(&trimTimer);
        APP_TRACE_INFO0("Advertising stopped");
        break;

    case DM_CONN_OPEN_IND:
        bleCb.connected = TRUE;
        bleCb.connId = (dmConnId_t)pMsg->hdr.param;
        ControlTask_SendBleEvent(BLE_CTRL_EVT_CONNECTED);
        APP_TRACE_INFO0("=== ESP32 Connected! ===");
        APP_TRACE_INFO1("Connection ID: %d", bleCb.connId);
        break;

    case DM_CONN_CLOSE_IND:
        bleCb.connected = FALSE;
        bleCb.connId = DM_CONN_ID_NONE;
        WsfTimerStop(&trimTimer);
        ControlTask_SendBleEvent(BLE_CTRL_EVT_DISCONNECTED);
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
}

/* ---------- Application Hooks ---------- */

void DatsHandlerInit(wsfHandlerId_t handlerId)
{
    uint8_t addr[6] = {0};
    
    APP_TRACE_INFO0("=== MAX32655 BLE Initializing ===");
    
    AppGetBdAddr(addr);
    APP_TRACE_INFO6("MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                   addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

    /* Initialize control block */
    bleCb.handlerId = handlerId;
    bleCb.connId = DM_CONN_ID_NONE;
    bleCb.connected = FALSE;

    /* ✅ CRITICAL: Set configuration pointers */
    pAppSlaveCfg = (appSlaveCfg_t *)&slaveCfg;
    pAppAdvCfg = (appAdvCfg_t *)&advCfg;
    pAppSecCfg = (appSecCfg_t *)&secCfg;
    pAppUpdateCfg = (appUpdateCfg_t *)&updateCfg;
    pSmpCfg = (smpCfg_t *)&smpCfg;
    pAttCfg = (attCfg_t *)&attCfg;

    /* ✅ CRITICAL: Initialize application framework */
    AppSlaveInit();
    AppServerInit();

    /* Set local IRK */
    DmSecSetLocalIrk(localIrk);

    /* Setup trim timer */
    trimTimer.handlerId = handlerId;
    trimTimer.msg.event = TRIM_TIMER_EVT;
}

void DatsHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    if (!pMsg)
        return;

    if (pMsg->event >= ATT_CBACK_START && pMsg->event <= ATT_CBACK_END)
    {
        // ATT events are processed by the stack automatically
        // MTU exchange is handled automatically based on attCfg.desiredMtu (241)
        AppServerProcAttMsg(pMsg);
    }
    else if (pMsg->event >= DM_CBACK_START && pMsg->event <= DM_CBACK_END)
    {
        AppSlaveProcDmMsg((dmEvt_t *)pMsg);
        AppSlaveSecProcDmMsg((dmEvt_t *)pMsg);
    }

    processMsg((dmEvt_t *)pMsg);
}

void DatsStart(void)
{
    APP_TRACE_INFO0("Starting BLE services...");

    /* Register callbacks */
    DmRegister(dmCback);
    DmConnRegister(DM_CLIENT_ID_APP, dmCback);
    AttRegister(attCback);
    AttConnRegister(AppServerConnCback);
    AttsCccRegister(DATS_NUM_CCC_IDX, (attsCccSet_t *)cccSet, cccCback);

    /* Add GATT service */
    SvcCoreGattCbackRegister(GattReadCback, GattWriteCback);
    SvcCoreAddGroup();

    /* Add custom service for ESP32 communication */
    SvcCustomCbackRegister(NULL, customWriteCback);
    SvcCustomAddGroup();

    /* Set Service Changed CCCD index */
    GattSetSvcChangedIdx(DATS_GATT_SC_CCC_IDX);

    /* Initialize NVM if available */
    WsfNvmInit();

    /* Reset the device to start */
    APP_TRACE_INFO0("Resetting BLE stack...");
    DmDevReset();
}
