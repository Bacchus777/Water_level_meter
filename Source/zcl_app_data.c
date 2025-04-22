#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ms.h"
#include "zcl_ha.h"

#include "zcl_app.h"

#include "battery.h"
#include "version.h"
/*********************************************************************
 * CONSTANTS
 */

#define APP_DEVICE_VERSION 2
#define APP_FLAGS 0

#define APP_HWVERSION 1
#define APP_ZCLVERSION 1

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Global attributes
const uint16 zclApp_clusterRevision_all = 0x0002;


float    zclApp_PresentValue  = 0; 
float    zclApp_Percentage    = 0; 

// Basic Cluster
const uint8 zclApp_HWRevision = APP_HWVERSION;
const uint8 zclApp_ZCLVersion = APP_ZCLVERSION;
const uint8 zclApp_ApplicationVersion = 3;
const uint8 zclApp_StackVersion = 4;

//{lenght, 'd', 'a', 't', 'a'}
const uint8 zclApp_ManufacturerName[] = {7, 'B', 'a', 'c', 'c', 'h', 'u', 's'};
const uint8 zclApp_ModelId[] = {17, 'W', 'a', 't', 'e', 'r', ' ', 'l', 'e', 'v', 'e', 'l', ' ', 'm', 'e', 't', 'e', 'r'};
const uint8 zclApp_PowerSource = POWER_SOURCE_BATTERY;


#define DEFAULT_MinThreshold        0
#define DEFAULT_MaxThreshold        100
#define DEFAULT_TankHeight          200
#define DEFAULT_MeasurementPeriod   10


application_config_t zclApp_Config = {
    .MinThreshold =       DEFAULT_MinThreshold,
    .MaxThreshold =       DEFAULT_MaxThreshold,
    .TankHeight =         DEFAULT_TankHeight,
    .MeasurementPeriod =  DEFAULT_MeasurementPeriod,
};

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */


CONST zclAttrRec_t zclApp_AttrsFirstEP[] = {
    {BASIC, {ATTRID_BASIC_ZCL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ZCLVersion}},
    {BASIC, {ATTRID_BASIC_APPL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ApplicationVersion}},
    {BASIC, {ATTRID_BASIC_STACK_VERSION, ZCL_UINT8, R, (void *)&zclApp_StackVersion}},
    {BASIC, {ATTRID_BASIC_HW_VERSION, ZCL_UINT8, R, (void *)&zclApp_HWRevision}},
    {BASIC, {ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ManufacturerName}},
    {BASIC, {ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ModelId}},
    {BASIC, {ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, R, (void *)&zclApp_PowerSource}},
    {BASIC, {ATTRID_BASIC_SW_BUILD_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclApp_clusterRevision_all}},

    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE, ZCL_UINT8, RR, (void *)&zclBattery_Voltage}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_UINT8, RR, (void *)&zclBattery_PercentageRemainig}},
    {POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE_RAW_ADC, ZCL_UINT16, RR, (void *)&zclBattery_RawAdc}},

    {ANALOG_INPUT, {ATTRID_PRESENT_VALUE, ZCL_SINGLE, RR, (void *)&zclApp_PresentValue}},
    {ANALOG_INPUT, {ATTRID_MIN_THRESHOLD, ZCL_SINGLE, RW, (void *)&zclApp_Config.MinThreshold}},
    {ANALOG_INPUT, {ATTRID_MAX_THRESHOLD, ZCL_SINGLE, RW, (void *)&zclApp_Config.MaxThreshold}},
    {ANALOG_INPUT, {ATTRID_MAX_VALUE,     ZCL_SINGLE, RW, (void *)&zclApp_Config.TankHeight}},
    {ANALOG_INPUT, {ATTRID_PERC_VALUE,    ZCL_SINGLE, RR, (void *)&zclApp_Percentage}},
    {ANALOG_INPUT, {ATTRID_PERIOD,        ZCL_UINT16, RW, (void *)&zclApp_Config.MeasurementPeriod}},
};


uint8 CONST zclApp_AttrsFirstEPCount = (sizeof(zclApp_AttrsFirstEP) / sizeof(zclApp_AttrsFirstEP[0]));
 
const cId_t zclApp_InClusterListFirstEP[] = {
  BASIC,
  POWER_CFG,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ANALOG_INPUT, 
};

#define APP_MAX_IN_CLUSTERS_FIRST_EP (sizeof(zclApp_InClusterListFirstEP) / sizeof(zclApp_InClusterListFirstEP[0]))

const cId_t zclApp_OutClusterListFirstEP[] = {
  BASIC,
  GEN_ON_OFF,
};

#define APP_MAX_OUT_CLUSTERS_FIRST_EP (sizeof(zclApp_OutClusterListFirstEP) / sizeof(zclApp_OutClusterListFirstEP[0]))

SimpleDescriptionFormat_t zclApp_FirstEP = {
    FIRST_ENDPOINT,                             //  int Endpoint;
    ZCL_HA_PROFILE_ID,                          //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,              //  uint16 AppDeviceId[2];
    APP_DEVICE_VERSION,                         //  int   AppDevVer:4;
    APP_FLAGS,                                  //  int   AppFlags:4;
    APP_MAX_IN_CLUSTERS_FIRST_EP,               //  byte  AppNumInClusters;
    (cId_t *)zclApp_InClusterListFirstEP,       //  byte *pAppInClusterList;
    APP_MAX_OUT_CLUSTERS_FIRST_EP,              //  byte  AppNumInClusters;
    (cId_t *)zclApp_OutClusterListFirstEP       //  byte *pAppInClusterList;
};


void zclApp_ResetAttributesToDefaultValues(void) {
    zclApp_Config.MinThreshold =      DEFAULT_MinThreshold;
    zclApp_Config.MaxThreshold =      DEFAULT_MaxThreshold;
    zclApp_Config.TankHeight =        DEFAULT_TankHeight;
    zclApp_Config.MeasurementPeriod = DEFAULT_MeasurementPeriod;
}

