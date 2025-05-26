
#include "AF.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_PwrMgr.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "ZDNwkMgr.h"
#include "ZDObject.h"
#include "math.h"

#include "nwk_util.h"
#include "zcl.h"
#include "zcl_app.h"
#include "zcl_diagnostic.h"
#include "zcl_general.h"
#include "zcl_lighting.h"
#include "zcl_ms.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "gp_interface.h"
#include "bdb_touchlink.h"
#include "bdb_touchlink_target.h"

#include "Debug.h"

#include "OnBoard.h"

/* HAL */
#include "ds18b20.h"
#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_i2c.h"
#include "hal_key.h"
#include "hal_led.h"

#include "battery.h"
#include "commissioning.h"
#include "factory_reset.h"
#include "utils.h"
#include "version.h"

/*********************************************************************
 * MACROS
 */
#define HAL_KEY_CODE_RELEASE_KEY HAL_KEY_CODE_NOKEY

// use led4 as output pin, osal will shitch it low when go to PM
#define POWER_ON_SENSORS()                                                                                                                 \
    do {                                                                                                                                   \
        HAL_TURN_ON_LED4();                                                                                                                \
        IO_PUD_PORT(DS18B20_PORT, IO_PUP);                                                                                                 \
    } while (0)
#define POWER_OFF_SENSORS()                                                                                                                \
    do {                                                                                                                                   \
        HAL_TURN_OFF_LED4();                                                                                                               \
        IO_PUD_PORT(DS18B20_PORT, IO_PDN);                                                                                                 \
    } while (0)

/*********************************************************************
 * CONSTANTS
 */

#define RESPONSE_LENGHT     8

      
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

uint8 request[1]      = {0x00};
      
extern bool requestNewTrustCenterLinkKey;
byte zclApp_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};

static uint8 currentSensorsReadingPhase = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclApp_HandleKeys(byte shift, byte keys);
static void zclApp_Report(void);
static void zclApp_BasicResetCB(void);
static void zclApp_InitA02Uart(void);
static void SerialApp_CallBack(uint8 port, uint8 event);   // Receive data will trigger
static void RequestMeasure(void);

static void zclApp_RestoreAttributesFromNV(void);
static void zclApp_SaveAttributesToNV(void);
static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper);

static void zclApp_ReadSensors(void);
static void zclApp_ReadDS18B20(void);
static void zclApp_SetOutput(void);

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclApp_CmdCallbacks = {
    zclApp_BasicResetCB, // Basic Cluster Reset command
    NULL, // Identify Trigger Effect command
    NULL, // On/Off cluster commands
    NULL, // On/Off cluster enhanced command Off with Effect
    NULL, // On/Off cluster enhanced command On with Recall Global Scene
    NULL, // On/Off cluster enhanced command On with Timed Off
    NULL, // RSSI Location command
    NULL  // RSSI Location Response command
};

void zclApp_Init(byte task_id) {
    IO_PUD_PORT(0, IO_PDN);
    IO_PUD_PORT(1, IO_PDN);
    IO_PUD_PORT(2, IO_PUP);
    POWER_OFF_SENSORS();

    zclApp_InitA02Uart();
    // this is important to allow connects throught routers
    // to make this work, coordinator should be compiled with this flag #define TP2_LEGACY_ZC
    requestNewTrustCenterLinkKey = FALSE;

    zclApp_TaskID = task_id;

    zclGeneral_RegisterCmdCallbacks(1, &zclApp_CmdCallbacks);
    zcl_registerAttrList(zclApp_FirstEP.EndPoint, zclApp_AttrsFirstEPCount, zclApp_AttrsFirstEP);
    bdb_RegisterSimpleDescriptor(&zclApp_FirstEP);

    zcl_registerForMsg(zclApp_TaskID);

    zcl_registerReadWriteCB(1, NULL, zclApp_ReadWriteAuthCB);

    zclApp_RestoreAttributesFromNV();
    // Register for all key events - This app will handle all key events
    RegisterForKeys(zclApp_TaskID);
    LREP("Started build %s \r\n", zclApp_DateCodeNT);
    


    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, ((uint32) zclApp_Config.MeasurementPeriod * 1000));
}

uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;

    (void)task_id; // Intentionally unreferenced parameter
    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclApp_TaskID))) {
            switch (MSGpkt->hdr.event) {
            case KEY_CHANGE:
                zclApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
            case ZCL_INCOMING_MSG:
                if (((zclIncomingMsg_t *)MSGpkt)->attrCmd) {
                    osal_mem_free(((zclIncomingMsg_t *)MSGpkt)->attrCmd);
                }
                break;

            default:
                break;
            }
            // Release the memory
            osal_msg_deallocate((uint8 *)MSGpkt);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & APP_REPORT_EVT) {
        LREPMaster("APP_REPORT_EVT\r\n");
        zclApp_Report();
        return (events ^ APP_REPORT_EVT);
    }

    if (events & APP_READ_SENSORS_EVT) {
        LREPMaster("APP_READ_SENSORS_EVT\r\n");
        zclApp_ReadSensors();
        return (events ^ APP_READ_SENSORS_EVT);
    }
    if (events & APP_SAVE_ATTRS_EVT) {
        LREPMaster("APP_SAVE_ATTRS_EVT\r\n");
        zclApp_SaveAttributesToNV();
        return (events ^ APP_SAVE_ATTRS_EVT);
    }

    // Discard unknown events
    return 0;
}

static void zclApp_BasicResetCB(void) {
    LREPMaster("BasicResetCB\r\n");
    zclApp_ResetAttributesToDefaultValues();
    zclApp_SaveAttributesToNV();
}

static void zclApp_HandleKeys(byte portAndAction, byte keyCode) {
    LREP("zclApp_HandleKeys portAndAction=0x%X keyCode=0x%X\r\n", portAndAction, keyCode);
    zclFactoryResetter_HandleKeys(portAndAction, keyCode);
    zclCommissioning_HandleKeys(portAndAction, keyCode);
    if (portAndAction & HAL_KEY_PRESS) {
        LREPMaster("Key press\r\n");
        osal_start_timerEx(zclApp_TaskID, APP_REPORT_EVT, 200);
    }
}

static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper) {
    LREPMaster("AUTH CB called\r\n");
    osal_start_timerEx(zclApp_TaskID, APP_SAVE_ATTRS_EVT, 2000);
    return ZSuccess;
}

static void zclApp_SaveAttributesToNV(void) {
    uint8 writeStatus = osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    LREP("Saving attributes to NV write=%d\r\n", writeStatus);
    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, ((uint32) zclApp_Config.MeasurementPeriod * 1000));
}

static void zclApp_RestoreAttributesFromNV(void) {
    uint8 status = osal_nv_item_init(NW_APP_CONFIG, sizeof(application_config_t), NULL);
    LREP("Restoring attributes from NV  status=%d \r\n", status);
    if (status == NV_ITEM_UNINIT) {
        uint8 writeStatus = osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
        LREP("NV was empty, writing %d\r\n", writeStatus);
    }
    if (status == ZSUCCESS) {
        LREPMaster("Reading from NV\r\n");
        osal_nv_read(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    }
}

static void zclApp_ReadSensors(void) {
  LREP("currentSensorsReadingPhase %d\r\n", currentSensorsReadingPhase);

  HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
  switch (currentSensorsReadingPhase++) {
  case 0:
      POWER_ON_SENSORS();
      break;
  case 1:
      zclBattery_Report();
      break;
  case 2:
      zclApp_ReadDS18B20();
      break;
  case 3:
      osal_pwrmgr_device(PWRMGR_ALWAYS_ON);
      RequestMeasure();
      osal_start_reload_timer(zclApp_TaskID, APP_READ_SENSORS_EVT, 600); 
      break;
  default:
    currentSensorsReadingPhase = 0;
    osal_stop_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT);
    osal_clear_event(zclApp_TaskID, APP_READ_SENSORS_EVT);
    POWER_OFF_SENSORS();
    osal_pwrmgr_device(PWRMGR_BATTERY);
    break;
  }
  LREP("currentSensorsReadingPhase %d\r\n", currentSensorsReadingPhase);
}


static void zclApp_SetOutput(void) {
  
  LREP("zclApp_SoilHumiditySensor_MeasuredValue=%d\r\n", zclApp_Percentage);
  LREP("zclApp_Config.Threshold=%d\r\n", zclApp_Config.MinThreshold);
  LREP("zclApp_Config.Threshold=%d\r\n", zclApp_Config.MaxThreshold);
  zclApp_LevelOutput = ((zclApp_Percentage < (zclApp_Config.MaxThreshold * 100)) & (zclApp_Percentage > (zclApp_Config.MinThreshold * 100)) ^ zclApp_Config.InvertOutput) ;
  if (zclApp_LevelOutput) 
    zclGeneral_SendOnOff_CmdOn(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
  else
    zclGeneral_SendOnOff_CmdOff(zclApp_FirstEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());

}

static void zclApp_ReadDS18B20(void) {
    int16 temp = readTemperature();
    if (temp != 1) {
        zclApp_DS18B20_MeasuredValue = temp;
        LREP("ReadDS18B20 t=%d\r\n", zclApp_DS18B20_MeasuredValue);
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
    } else {
        LREPMaster("ReadDS18B20 error\r\n");
    }

}

static void _delay_us(uint16 microSecs) {
    while (microSecs--) {
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
    }
}

void user_delay_ms(uint32_t period) { _delay_us(1000 * period); }

static void zclApp_Report(void) 
{ 
  osal_start_reload_timer(zclApp_TaskID, APP_READ_SENSORS_EVT, 20); 
}

void SerialApp_CallBack(uint8 port, uint8 event)   // Receive data will trigger
{

  uint8 response[RESPONSE_LENGHT] = {0x00};

  HalUARTRead(A02_PORT, (uint8 *)&response, sizeof(response) / sizeof(response[0]));

    LREPMaster("CALLBACK UART \r\n");
    for (int i = 0; i <= (RESPONSE_LENGHT - 1); i++) 
    {
      LREP("0x%X ", response[i]);
    }
    LREP("\r\n");
 
    if (((response[0] + response[1] + response[2]) & 0x00FF) == response[3]){
      uint16 distance = (response[1] * 256 + response[2]);
      if (distance > 0) {
        zclApp_PresentValue = zclApp_Config.TankHeight - distance;
        zclApp_Percentage = (uint8)(zclApp_PresentValue / zclApp_Config.TankHeight * 100);
        LREP("PresentValue = %d mm\r\n", (uint16)zclApp_PresentValue);
        LREP("TankHeight = %d mm\r\n", (uint16)zclApp_Config.TankHeight);
        LREP("Percentage = %d\r\n", (uint16)(zclApp_Percentage));
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, ANALOG_INPUT, ATTRID_PRESENT_VALUE);
        zclApp_SetOutput();
    }
  }
  else
    LREPMaster("ERROR \r\n");

  osal_pwrmgr_device(PWRMGR_BATTERY);

}

static void zclApp_InitA02Uart(void) {
  halUARTCfg_t halUARTConfig;
  halUARTConfig.configured = TRUE;
  halUARTConfig.baudRate = HAL_UART_BR_9600;
  halUARTConfig.flowControl = FALSE;
  halUARTConfig.flowControlThreshold = 64; // this parameter indicates number of bytes left before Rx Buffer
                                           // reaches maxRxBufSize
  halUARTConfig.idleTimeout = 10;          // this parameter indicates rx timeout period in millisecond
  halUARTConfig.rx.maxBufSize = 128;
  halUARTConfig.tx.maxBufSize = 128;
  halUARTConfig.intEnable = TRUE;
  halUARTConfig.callBackFunc = SerialApp_CallBack;
  HalUARTInit();
  if (HalUARTOpen(A02_PORT, &halUARTConfig) == HAL_UART_SUCCESS) {
    LREPMaster("Initialized HLK UART \r\n");
  }
}

static void RequestMeasure(void)
{

#if defined(HAL_BOARD_METER)
  U0CSR &= ~0x80; // Disable UART1
  P0SEL &= ~0x20; // Clear P1.5 alternate function
  P0DIR |= 0x20;  // Set P1.5 as output
  P0_3 = 1; // Set P1.5 high

  for (volatile uint8_t i = 0; i < 32; i++) {} // ????????? ????????
  P0_3 = 0; // Set P1.5 low

  P0SEL |= 0x20;  // Set P1.5 alternate function back to UART
  U0CSR |= 0x80;  // Enable UART1}
#elif defined(HAL_BOARD_CHDTECH_DEV)
  U1CSR &= ~0x80; // Disable UART1
  P1SEL &= ~0x20; // Clear P1.5 alternate function
  P1DIR |= 0x20;  // Set P1.5 as output
  P1_6 = 1; // Set P1.5 high

  for (volatile uint8_t i = 0; i < 32; i++) {} // ????????? ????????
  P1_6 = 0; // Set P1.5 low

  P1SEL |= 0x20;  // Set P1.5 alternate function back to UART
  U1CSR |= 0x80;  // Enable UART1}
#endif

}

/****************************************************************************
****************************************************************************/
