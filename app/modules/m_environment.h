/*
 * m_environment.h
 *
 *  Created on: 2020. 9. 3.
 *      Author: chlee
 */

#ifndef APP_MODULES_M_ENVIRONMENT_H_
#define APP_MODULES_M_ENVIRONMENT_H_

#include <stdint.h>
#include <stdbool.h>
#include "nrf_drv_twi.h"

#ifdef __GNUC__
    #ifdef PACKED
        #undef PACKED
    #endif

    #define PACKED(TYPE) TYPE __attribute__ ((packed))
#endif

typedef PACKED( struct
{
    int8_t  integer;
    uint8_t decimal;
}) env_temperature_t;

typedef PACKED( struct
{
    int32_t  integer;
    uint8_t  decimal;
}) env_pressure_t;

typedef uint8_t env_humidity_t;

typedef PACKED( struct
{
    uint16_t eco2_ppm; ///< The equivalent CO2 (eCO2) value in parts per million.
    uint16_t tvoc_ppb; ///< The Total Volatile Organic Compound (TVOC) value in parts per billion.
}) env_gas_t;

typedef PACKED( struct
{
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t clear;
}) env_color_t;

typedef enum
{
    GAS_MODE_250MS,
    GAS_MODE_1S,
    GAS_MODE_10S,
    GAS_MODE_60S,
} env_gas_mode_t;

typedef PACKED( struct
{
    uint8_t  led_red;
    uint8_t  led_green;
    uint8_t  led_blue;
}) env_color_config_t;

typedef PACKED( struct
{
    uint16_t                temperature_interval_ms;
    uint16_t                pressure_interval_ms;
    uint16_t                humidity_interval_ms;
    uint16_t                color_interval_ms;
    uint8_t                 gas_interval_mode;
    env_color_config_t  color_config;
}) env_config_t;

#define ENV_CONFIG_TEMPERATURE_INT_MIN      100
#define ENV_CONFIG_TEMPERATURE_INT_MAX    60000
#define ENV_CONFIG_PRESSURE_INT_MIN          50
#define ENV_CONFIG_PRESSURE_INT_MAX       60000
#define ENV_CONFIG_HUMIDITY_INT_MIN         100
#define ENV_CONFIG_HUMIDITY_INT_MAX       60000
#define ENV_CONFIG_COLOR_INT_MIN            200
#define ENV_CONFIG_COLOR_INT_MAX          60000
#define ENV_CONFIG_GAS_MODE_MIN               1
#define ENV_CONFIG_GAS_MODE_MAX               3

typedef PACKED( struct
{
    uint16_t mode_250ms;
    uint16_t mode_1s;
    uint16_t mode_10s;
    uint16_t mode_60s;
}) m_gas_baseline_t;

/**@brief Initialization parameters. */
typedef struct
{
    const nrf_drv_twi_t * p_twi_instance;
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

/**@brief Function for starting the environment module.
 *
 * @details This function should be called after m_environment_init to start the environment module.
 *
 * @retval NRF_SUCCESS If initialization was successful.
 */
uint32_t m_environment_start(void);

/**@brief Function for stopping the environment module.
 *
 * @details This function should be called after m_environment_start to stop the environment module.
 *
 * @retval NRF_SUCCESS If initialization was successful.
 */
uint32_t m_environment_stop(void);

/**@brief Function for initializing the environment module.
 *
 * @param[in] p_handle  Pointer to the location to store the service handle.
 * @param[in] p_params  Pointer to the init parameters.
 *
 * @retval NRF_SUCCESS  If initialization was successful.
 */
uint32_t m_environment_init(m_environment_init_t * p_params);

#endif /* APP_MODULES_M_ENVIRONMENT_H_ */
