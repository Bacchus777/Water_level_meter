#ifndef ZCL_APP_H
#define ZCL_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "version.h"
#include "zcl.h"


/*********************************************************************
 * CONSTANTS
 */

// Application Events
#define APP_REPORT_EVT                  0x0001
#define APP_READ_SENSORS_EVT            0x0002
#define APP_SAVE_ATTRS_EVT              0x0004

#define NW_APP_CONFIG                   0x0402

#define FIRST_ENDPOINT          1

#define APP_REPORT_DELAY ((uint32) 1800000) //30 minutes


#define R           ACCESS_CONTROL_READ
#define RR          (R | ACCESS_REPORTABLE)
#define RW          (R | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_WRITE)
#define RWR         (RW | ACCESS_REPORTABLE)

#define BASIC           ZCL_CLUSTER_ID_GEN_BASIC
#define POWER_CFG       ZCL_CLUSTER_ID_GEN_POWER_CFG
#define ANALOG_INPUT    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC
#define GEN_ON_OFF      ZCL_CLUSTER_ID_GEN_ON_OFF
#define TEMP            ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT

#define ZCL_UINT8       ZCL_DATATYPE_UINT8
#define ZCL_UINT16      ZCL_DATATYPE_UINT16
#define ZCL_UINT32      ZCL_DATATYPE_UINT32
#define ZCL_INT16       ZCL_DATATYPE_INT16
#define ZCL_INT8        ZCL_DATATYPE_INT8
#define ZCL_BOOLEAN     ZCL_DATATYPE_BOOLEAN
#define ZCL_SINGLE      ZCL_DATATYPE_SINGLE_PREC


#define ATTRID_PRESENT_VALUE    ATTRID_IOV_BASIC_PRESENT_VALUE
#define ATTRID_MIN_THRESHOLD    ATTRID_IOV_BASIC_MIN_PRESENT_VALUE
#define ATTRID_MAX_THRESHOLD    ATTRID_IOV_BASIC_MAX_PRESENT_VALUE
#define ATTRID_MAX_VALUE        0xF005
#define ATTRID_PERC_VALUE       0xF006
#define ATTRID_PERIOD           0xF007
#define ATTRID_INVERT           0xF008

/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    float     MinThreshold;
    float     MaxThreshold;
    float     TankHeight;
    uint16    MeasurementPeriod;
    bool      InvertOutput;
} application_config_t;


/*********************************************************************
 * VARIABLES
 */

extern SimpleDescriptionFormat_t zclApp_FirstEP;

extern void zclApp_ResetAttributesToDefaultValues(void);

extern uint8  zclApp_BatteryVoltage;
extern uint8  zclApp_BatteryPercentageRemainig;
extern uint16 zclApp_BatteryVoltageRawAdc;


// attribute list
extern CONST zclAttrRec_t zclApp_AttrsFirstEP[];
extern CONST uint8 zclApp_AttrsFirstEPCount;


extern const uint8 zclApp_ManufacturerName[];
extern const uint8 zclApp_ModelId[];
extern const uint8 zclApp_PowerSource;

extern float zclApp_PresentValue; 
extern float zclApp_Percentage; 

extern int16  zclApp_DS18B20_MeasuredValue;
extern bool   zclApp_LevelOutput;

extern application_config_t zclApp_Config;

// APP_TODO: Declare application specific attributes here

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialization for the task
 */
extern void zclApp_Init(byte task_id);

extern void zclApp_ResetAttributesToDefaultValues(void);

/*
 *  Event Process for the task
 */
extern UINT16 zclApp_event_loop(byte task_id, UINT16 events);

void user_delay_ms(uint32_t period);

#ifdef __cplusplus
}
#endif

#endif /* ZCL_APP_H */
