/*************************************************************************************************/
/*!
 *  \file   svc_custom.c
 *
 *  \brief  Custom BLE service implementation for MAX32655 <-> ESP32 communication.
 */
/*************************************************************************************************/

#include "wsf_types.h"
#include "att_api.h"
#include "att_uuid.h"
#include "wsf_trace.h"
#include "util/bstream.h"
#include "svc_ch.h"
#include "svc_cfg.h"
#include "svc_custom.h"
#include "ble_uuid.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Characteristic properties */
#define CUSTOM_TX_CH_PROPS    (ATT_PROP_READ | ATT_PROP_NOTIFY)
#define CUSTOM_RX_CH_PROPS    (ATT_PROP_WRITE | ATT_PROP_WRITE_NO_RSP)

/**************************************************************************************************
  Service Variables
**************************************************************************************************/

/*! Custom service UUID */
static const uint8_t customSvcUuid[] = { CUSTOM_SVC_UUID };
static const uint16_t customSvcUuidLen = sizeof(customSvcUuid);

/*! Custom TX characteristic UUID */
static const uint8_t customTxChUuid[] = { CUSTOM_TX_CHAR_UUID };

/*! Custom RX characteristic UUID */
static const uint8_t customRxChUuid[] = { CUSTOM_RX_CHAR_UUID };

/**************************************************************************************************
  Characteristic Declaration Values (Properties + Handle + UUID)
**************************************************************************************************/

/* TX Characteristic declaration value */
static const uint8_t customTxChDecl[] = {
    CUSTOM_TX_CH_PROPS,                 /* Properties */
    UINT16_TO_BYTES(CUSTOM_TX_HDL),     /* Handle */
    CUSTOM_TX_CHAR_UUID                 /* UUID */
};
static const uint16_t customTxChDeclLen = sizeof(customTxChDecl);

/* RX Characteristic declaration value */
static const uint8_t customRxChDecl[] = {
    CUSTOM_RX_CH_PROPS,                 /* Properties */
    UINT16_TO_BYTES(CUSTOM_RX_HDL),     /* Handle */
    CUSTOM_RX_CHAR_UUID                 /* UUID */
};
static const uint16_t customRxChDeclLen = sizeof(customRxChDecl);

/**************************************************************************************************
  Characteristic Values
**************************************************************************************************/

/*! TX characteristic value */
static uint8_t customTxVal[CUSTOM_MAX_DATA_LEN];
static uint16_t customTxValLen = 1;

/*! RX characteristic value */
static uint8_t customRxVal[CUSTOM_MAX_DATA_LEN];
static uint16_t customRxValLen = 1;

/*! TX client characteristic configuration */
static uint8_t customTxChCcc[] = { UINT16_TO_BYTES(0x0000) };
static uint16_t customTxChCccLen = sizeof(customTxChCcc);

/**************************************************************************************************
  Attribute Definitions
**************************************************************************************************/

/*! Custom service group - attribute list */
static const attsAttr_t customSvcAttrList[] = {
    /* Custom Service Declaration */
    {
        attPrimSvcUuid,                     /* pUuid */
        (uint8_t *)customSvcUuid,           /* pValue */
        (uint16_t *)&customSvcUuidLen,      /* pLen */
        sizeof(customSvcUuid),              /* maxLen */
        0,                                  /* settings */
        ATTS_PERMIT_READ                    /* permissions */
    },
    
    /* TX Characteristic Declaration */
    {
        attChUuid,                          /* pUuid */
        (uint8_t *)customTxChDecl,          /* pValue */
        (uint16_t *)&customTxChDeclLen,     /* pLen */
        sizeof(customTxChDecl),             /* maxLen */
        0,                                  /* settings */
        ATTS_PERMIT_READ                    /* permissions */
    },
    /* TX Characteristic Value */
    {
        customTxChUuid,                     /* pUuid */
        customTxVal,                        /* pValue */
        &customTxValLen,                    /* pLen */
        sizeof(customTxVal),                /* maxLen */
        ATTS_SET_VARIABLE_LEN,              /* settings */
        ATTS_PERMIT_READ                    /* permissions */
    },
    /* TX Client Characteristic Configuration Descriptor */
    {
        attCliChCfgUuid,                    /* pUuid */
        customTxChCcc,                      /* pValue */
        &customTxChCccLen,                  /* pLen */
        sizeof(customTxChCcc),              /* maxLen */
        ATTS_SET_CCC,                       /* settings */
        ATTS_PERMIT_READ | ATTS_PERMIT_WRITE  /* permissions */
    },
    
    /* RX Characteristic Declaration */
    {
        attChUuid,                          /* pUuid */
        (uint8_t *)customRxChDecl,          /* pValue */
        (uint16_t *)&customRxChDeclLen,     /* pLen */
        sizeof(customRxChDecl),             /* maxLen */
        0,                                  /* settings */
        ATTS_PERMIT_READ                    /* permissions */
    },
    /* RX Characteristic Value */
    {
        customRxChUuid,                     /* pUuid */
        customRxVal,                        /* pValue */
        &customRxValLen,                    /* pLen */
        sizeof(customRxVal),                /* maxLen */
        ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK,  /* settings */
        ATTS_PERMIT_WRITE                   /* permissions */
    }
};

/*! Custom service group structure */
static attsGroup_t customSvcGroup = {
    NULL,                               /* pNext */
    (attsAttr_t *)customSvcAttrList,    /* pAttr */
    NULL,                               /* readCback */
    NULL,                               /* writeCback */
    CUSTOM_SVC_HDL,                     /* startHdl */
    CUSTOM_SVC_HDL_END - 1              /* endHdl */
};

/**************************************************************************************************
  Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Add the custom service to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCustomAddGroup(void)
{
    /* Register the service group */
    AttsAddGroup(&customSvcGroup);
}

/*************************************************************************************************/
/*!
 *  \brief  Remove the custom service from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCustomRemoveGroup(void)
{
    AttsRemoveGroup(CUSTOM_SVC_HDL);
}

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
void SvcCustomCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
    customSvcGroup.readCback = readCback;
    customSvcGroup.writeCback = writeCback;
}

