#define TC_LINKKEY_JOIN
#define NV_INIT
#define NV_RESTORE


#define TP2_LEGACY_ZC
//patch sdk
// #define ZDSECMGR_TC_ATTEMPT_DEFAULT_KEY TRUE

#define NWK_AUTO_POLL
#define MULTICAST_ENABLED FALSE

#define ZCL_READ
#define ZCL_WRITE
#define ZCL_BASIC
#define ZCL_ON_OFF
#define ZCL_IDENTIFY
#define ZCL_REPORTING_DEVICE

#define ZSTACK_DEVICE_BUILD (DEVICE_BUILD_ENDDEVICE)

#define DISABLE_GREENPOWER_BASIC_PROXY
#define BDB_FINDING_BINDING_CAPABILITY_ENABLED 1
#define BDB_REPORTING TRUE


#define ISR_KEYINTERRUPT
#define HAL_BUZZER FALSE

#define HAL_LED TRUE
#define HAL_I2C TRUE
#define BLINK_LEDS TRUE


//one of this boards
// #define HAL_BOARD_FLOWER
// #define HAL_BOARD_CHDTECH_DEV

#if !defined(HAL_BOARD_METER) && !defined(HAL_BOARD_CHDTECH_DEV)
#error "Board type must be defined"
#endif

#define BDB_MAX_CLUSTERENDPOINTS_REPORTING 10

#define BME280_32BIT_ENABLE
//TODO: refactor ds18b20 driver
#define DS18B20_PORT 1
#define DS18B20_PIN 3

#define TSENS_SBIT P1_3
#define TSENS_BV BV(3)
#define TSENS_DIR P1DIR

#if defined(HAL_BOARD_METER)
    #define HAL_KEY_P2_INPUT_PINS BV(0)
    #define HAL_KEY_P2_INPUT_PINS_EDGE HAL_KEY_FALLING_EDGE
    #define HLK_PORT  HAL_UART_PORT_0
    #define HAL_UART_DMA 1
    #define HAL_UART_ISR 0
    #define INT_HEAP_LEN 2060//(2256 - 0xE)
    #define POWER_SAVING
#elif defined(HAL_BOARD_CHDTECH_DEV)
    #define HAL_KEY_P2_INPUT_PINS BV(0)
    #define HAL_KEY_P2_INPUT_PINS_EDGE HAL_KEY_FALLING_EDGE
    #define HLK_PORT  HAL_UART_PORT_1
    #define HAL_UART_DMA 1
    #define HAL_UART_ISR 2
//    #define POWER_SAVING
    #define DO_DEBUG_UART
#endif

#ifdef DO_DEBUG_UART
#define HAL_UART TRUE
#define HAL_UART_DMA 1
#define INT_HEAP_LEN (2685 - 0x4B - 0xBB)
#endif

#define HAL_UART TRUE

#include "hal_board_cfg.h"

#include "stdint.h"
#include "stddef.h"
