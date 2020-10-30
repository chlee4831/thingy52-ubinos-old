/*
 * tes.h
 *
 *  Created on: 2020. 9. 15.
 *      Author: chlee
 */

#ifndef APP_TES_H_
#define APP_TES_H_

#include <stdint.h>
#include "nrf_drv_twi.h"

/**@brief Initialization parameters. */
typedef struct
{
    const nrf_drv_twi_t *p_twi_instance;
} m_environment_init_t;

/**@brief Weather station default configuration. */
#define ENVIRONMENT_CONFIG_DEFAULT {        \
    .temperature_interval_ms = 2000,        \
    .pressure_interval_ms    = 2000,        \
    .humidity_interval_ms    = 2000,        \
    .color_interval_ms       = 1500,        \
    .color_config =                         \
    {                                       \
        .led_red             = 103,         \
        .led_green           = 78,          \
        .led_blue            = 29           \
    },                                      \
    .gas_interval_mode       = GAS_MODE_10S \
}

#define ENVIRONMENT_BASELINE_DEFAULT {      \
        .mode_250ms = 0,                    \
        .mode_1s    = 0,                    \
        .mode_10s   = 0,                    \
        .mode_60s   = 0,                    \
}

#ifdef __GNUC__
#ifdef PACKED
        #undef PACKED
    #endif

#define PACKED(TYPE) TYPE __attribute__ ((packed))
#endif

typedef PACKED( struct
        {
            int8_t integer;
            uint8_t decimal;
        })
tes_temperature_t;

typedef PACKED( struct
        {
            int32_t integer;
            uint8_t decimal;
        })
tes_pressure_t;

typedef uint8_t tes_humidity_t;

typedef PACKED( struct
        {
            uint16_t eco2_ppm; ///< The equivalent CO2 (eCO2) value in parts per million.
            uint16_t tvoc_ppb;///< The Total Volatile Organic Compound (TVOC) value in parts per billion.
        })
tes_gas_t;

typedef PACKED( struct
        {
            uint16_t red;
            uint16_t green;
            uint16_t blue;
            uint16_t clear;
        })
tes_color_t;

typedef enum
{
    GAS_MODE_250MS,
    GAS_MODE_1S,
    GAS_MODE_10S,
    GAS_MODE_60S,
} tes_gas_mode_t;

typedef PACKED( struct
        {
            uint8_t led_red;
            uint8_t led_green;
            uint8_t led_blue;
        })
tes_color_config_t;

typedef PACKED( struct
        {
            uint16_t temperature_interval_ms;
            uint16_t pressure_interval_ms;
            uint16_t humidity_interval_ms;
            uint16_t color_interval_ms;
            uint8_t gas_interval_mode;
            tes_color_config_t color_config;
        })
tes_config_t;

#define TES_CONFIG_TEMPERATURE_INT_MIN      100
#define TES_CONFIG_TEMPERATURE_INT_MAX    60000
#define TES_CONFIG_PRESSURE_INT_MIN          50
#define TES_CONFIG_PRESSURE_INT_MAX       60000
#define TES_CONFIG_HUMIDITY_INT_MIN         100
#define TES_CONFIG_HUMIDITY_INT_MAX       60000
#define TES_CONFIG_COLOR_INT_MIN            200
#define TES_CONFIG_COLOR_INT_MAX          60000
#define TES_CONFIG_GAS_MODE_MIN               1
#define TES_CONFIG_GAS_MODE_MAX               3

#endif /* APP_TES_H_ */
