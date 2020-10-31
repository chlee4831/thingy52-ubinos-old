/*
 * sampling_main.c
 *
 *  Created on: 2020. 9. 15.
 *      Author: chlee
 */

#include <ubinos.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ubinos.h>

#include "malloc.h"

// nrf library
#include "app_timer.h"
#include "app_util_platform.h"
//#include "nrf_sdm.h"
#include "nrf_drv_gpiote.h"

#define  NRF_LOG_MODULE_NAME m_env
#include "nrf_log.h"

//application header
#include "sw_config.h"
#include "pca20020.h"

#include "ble_stack.h"
#include "LAP_api.h"
#include "LAP_main.h"

#include "twi_manager.h"
#include "support_func.h"
#include "macros_common.h"

#include "drv_humidity.h"
#include "drv_pressure.h"
#include "drv_gas_sensor.h"
#include "drv_color.h"

#include "sampling.h"
#include "m_environment_flash.h"

static msgq_pt sampling_msgq;

static const nrf_drv_twi_t m_twi_sensors = NRF_DRV_TWI_INSTANCE(TWI_SENSOR_INSTANCE);

typedef enum
{
    GAS_STATE_IDLE,
    GAS_STATE_WARMUP,
    GAS_STATE_ACTIVE
}gas_state_t;

#define M_GAS_CALIB_INTERVAL_MS (1000 * 60 * 60) ///< Humidity and temperature calibration interval for the gas sensor [ms].
#define M_GAS_BASELINE_WRITE_MS (1000 * 60 * 30) ///< Stored baseline calibration delay for the gas sensor [ms].

static void temperature_timeout_handler(void * p_context); ///< Temperature handler, forward declaration.
static void humidity_timeout_handler(void * p_context);    ///< Humidity handler, forward declaration.

static tes_config_t     * m_p_config;       ///< Configuraion pointer.
static tes_config_t     m_config;       	///< Configuraion.
static tes_config_t     m_config_store;     ///< Configuraion store.
static const tes_config_t m_default_config = ENVIRONMENT_CONFIG_DEFAULT; ///< Default configuraion.
static m_gas_baseline_t     * m_p_baseline;     ///< Baseline pointer.
static m_gas_baseline_t     m_baseline;     	///< Baseline.
static m_gas_baseline_t     m_baseline_store;   ///< Baseline store.
static const m_gas_baseline_t m_default_baseline = ENVIRONMENT_BASELINE_DEFAULT; ///< Default baseline.


static bool        m_get_humidity                   = false;
static bool        m_get_temperature                = false;
static bool        m_calib_gas_sensor               = false;
static gas_state_t m_gas_state                      = GAS_STATE_IDLE;
static bool        m_temp_humid_for_gas_calibration = false;    ///< Set when the gas sensor requires temperature and humidity for calibration.
static bool        m_temp_humid_for_ble_transfer    = false;    ///< Set when humidity or temperature is requested over BLE.

static uint32_t calibrate_gas_sensor(uint16_t humid, float temp);
static uint32_t gas_load_baseline_flash(uint16_t * p_gas_baseline);

APP_TIMER_DEF(temperature_timer_id);
APP_TIMER_DEF(pressure_timer_id);
APP_TIMER_DEF(humidity_timer_id);
APP_TIMER_DEF(color_timer_id);
APP_TIMER_DEF(gas_calib_timer_id);

/**@brief Function for converting the temperature sample.
 */
static void temperature_conv_data(float in_temp, tes_temperature_t * p_out_temp)
{
    float f_decimal;

    p_out_temp->integer = (int8_t)in_temp;
    f_decimal = in_temp - p_out_temp->integer;
    p_out_temp->decimal = (uint8_t)(f_decimal * 100.0f);
    NRF_LOG_DEBUG("temperature_conv_data: Temperature: ,%d.%d,C\r\n", p_out_temp->integer, p_out_temp->decimal);
}


/**@brief Function for converting the humidity sample.
 */
static void humidity_conv_data(uint8_t humid, tes_humidity_t * p_out_humid)
{
   *p_out_humid = (uint8_t)humid;
   NRF_LOG_DEBUG("humidity_conv_data: Relative Humidty: ,%d,%%\r\n", humid);
}


/**@brief Function for converting the pressure sample.
 */
static void pressure_conv_data(float in_press, tes_pressure_t * p_out_press)
{
    float f_decimal;

    p_out_press->integer = (int32_t)in_press;
    f_decimal = in_press - p_out_press->integer;
    p_out_press->decimal = (uint8_t)(f_decimal * 100.0f);
    NRF_LOG_DEBUG("pressure_conv_data: Pressure/Altitude: %d.%d Pa/m\r\n", p_out_press->integer, p_out_press->decimal);
}


/**@brief Function for handling temperature timer timeout event.
 *
 * @details This function will read the temperature at the configured rate.
 */
static void temperature_timeout_handler(void * p_context)
{
    uint32_t err_code;
    m_get_temperature = true;

    // Read temperature from humidity sensor.
    err_code = drv_humidity_sample();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting temperature sampling.
 */
static uint32_t temperature_start(void)
{
    uint32_t err_code;

    m_get_temperature = true;
    m_temp_humid_for_ble_transfer = true;

    err_code = drv_humidity_enable();
    RETURN_IF_ERROR(err_code);

    err_code = drv_humidity_sample();
    RETURN_IF_ERROR(err_code);

    err_code = app_timer_start(temperature_timer_id,
                               APP_TIMER_TICKS(m_config.temperature_interval_ms),
                               NULL);
    RETURN_IF_ERROR(err_code);

    return NRF_SUCCESS;
}


/**@brief Function for stopping temperature sampling.
 */
static uint32_t temperature_stop(bool disable_drv)
{
    uint32_t err_code;
    m_get_temperature = false;

    err_code = app_timer_stop(temperature_timer_id);
    RETURN_IF_ERROR(err_code);

    if (disable_drv)
    {
        m_temp_humid_for_ble_transfer = false;

        if (!m_temp_humid_for_gas_calibration) // If used by the gas sensor, do not turn off.
        {
            return drv_humidity_disable();
        }
        else
        {
            return NRF_SUCCESS;
        }
    }
    else
    {
        return NRF_SUCCESS;
    }
}


/**@brief Function for handling pressure timer timout event.
 *
 * @details This function will read the pressure at the configured rate.
 */
static void pressure_timeout_handler(void * p_context)
{
//    uint32_t err_code;
//
//    err_code = drv_pressure_sample();
//    APP_ERROR_CHECK(err_code);

    sampling_event_send(SAMP_EVT_ENV, ENV_STATE_PRESSURE, NULL);
}


/**@brief Function for starting pressure sampling.
 */
static uint32_t pressure_start(void)
{
    uint32_t err_code;

    err_code = drv_pressure_enable();
    APP_ERROR_CHECK(err_code);

    err_code = drv_pressure_sample();
    APP_ERROR_CHECK(err_code);


    return app_timer_start(pressure_timer_id,
                           APP_TIMER_TICKS(m_config.pressure_interval_ms),
                           NULL);
}


/**@brief Function for stopping pressure sampling.
 */
static uint32_t pressure_stop(void)
{
    uint32_t err_code;

    err_code = app_timer_stop(pressure_timer_id);
    RETURN_IF_ERROR(err_code);

    return drv_pressure_disable();
}

/**@brief Function for handling gas sensor calibration.
 *
 * @details This function will read the humidity and temperature at the configured rate.
 */
static void gas_calib_timeout_handler(void * p_context)
{
    uint32_t err_code;
    uint16_t gas_baseline = 0;

    if (m_gas_state == GAS_STATE_WARMUP)
    {
        err_code = gas_load_baseline_flash(&gas_baseline);
        APP_ERROR_CHECK(err_code);

        if (gas_baseline == 0)
        {
            NRF_LOG_WARNING("No valid baseline stored in flash. Baseline not written to gas sensor.\r\n");
        }
        else
        {
            err_code = drv_gas_sensor_baseline_set(gas_baseline);
            APP_ERROR_CHECK(err_code);
        }

        m_gas_state = GAS_STATE_ACTIVE;

        (void)app_timer_stop(gas_calib_timer_id);

    }
    else if (m_gas_state == GAS_STATE_ACTIVE)
    {
        // For later implementation of gas sensor humidity and temperature calibration.
    }
    else
    {
        // Should never happen.
    }
}


/**@brief Sends the sampled humidity and temperature to the gas sensor for calibration.
 *
 * @note Not currently used.
 */
static uint32_t calibrate_gas_sensor(uint16_t humid, float temp)
{
    uint32_t err_code;

    if (m_temp_humid_for_gas_calibration) // Check that the gas sensor is still enabled.
    {
        uint16_t rh_ppt    = humid * 10;
        int32_t temp_mdeg = (int32_t)(temp * 1000.0f);

        NRF_LOG_DEBUG("Calibrating gas sensor: humid out %d [ppt], temp out: %d [mdeg C]\r\n", rh_ppt, temp_mdeg);

        err_code = drv_gas_sensor_calibrate_humid_temp(rh_ppt, temp_mdeg);
        RETURN_IF_ERROR(err_code);

        return NRF_SUCCESS;
    }
    else
    {
        return NRF_SUCCESS; // Do nothing.
    }
}


/**@brief Stops the humidity and temperature sensor, used to calibrate the gas sensor.
 *
 * @note Not currently used.
 */
static uint32_t humidity_temp_stop_for_gas_calibration(void)
{
    uint32_t err_code;

    m_temp_humid_for_gas_calibration = false;

    if (m_temp_humid_for_ble_transfer)
    {
        // The temprature or humidity is being transferred over BLE. Do nothing.
    }
    else
    {
        err_code = drv_humidity_disable();
        RETURN_IF_ERROR(err_code);
    }

    return app_timer_stop(gas_calib_timer_id);
}


/**@brief Function for handling humidity timer timout event.
 *
 * @details This function will read the humidity at the configured rate.
 */
static void humidity_timeout_handler(void * p_context)
{
    uint32_t err_code;
    m_get_humidity = true;

    // Sample humidity sensor.
    err_code = drv_humidity_sample();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting humidity sampling.
 */
static uint32_t humidity_start(void)
{
    uint32_t err_code;

    m_get_humidity = true;
    m_temp_humid_for_ble_transfer = true;

    err_code = drv_humidity_enable();
    RETURN_IF_ERROR(err_code);

    err_code = drv_humidity_sample();
    RETURN_IF_ERROR(err_code);

    return app_timer_start(humidity_timer_id,
                           APP_TIMER_TICKS(m_config.humidity_interval_ms),
                           NULL);
}


/**@brief Function for stopping humidity sampling.
 */
static uint32_t humidity_stop(bool disable_drv)
{
    uint32_t err_code;

    m_get_humidity = false;

    err_code = app_timer_stop(humidity_timer_id);
    RETURN_IF_ERROR(err_code);

    if (disable_drv)
    {
        m_temp_humid_for_ble_transfer = false;

        if (!m_temp_humid_for_gas_calibration) // If used by the gas sensor, do not turn off.
        {
            return drv_humidity_disable();
        }
        else
        {
            return NRF_SUCCESS;
        }
    }
    else
    {
        return NRF_SUCCESS;
    }
}

/**@brief Loads the gas sensor baseline values from flash storage.
 */
static uint32_t gas_load_baseline_flash(uint16_t * p_gas_baseline)
{
    // No explicit flash load performed here, since this is done by the m_environment module at init and stored in m_p_config.
    switch(m_config.gas_interval_mode)
    {
        case GAS_MODE_250MS:
            *p_gas_baseline = m_p_baseline->mode_250ms;
            NRF_LOG_DEBUG("Gas sensor baseline loaded from flash, value 0x%04x, mode: GAS_MODE_250MS \r\n", *p_gas_baseline);
        break;

        case GAS_MODE_1S:
            *p_gas_baseline = m_p_baseline->mode_1s;
            NRF_LOG_DEBUG("Gas sensor baseline loaded from flash, value 0x%04x, mode: GAS_MODE_1S \r\n", *p_gas_baseline);
        break;

        case GAS_MODE_10S:
            *p_gas_baseline = m_p_baseline->mode_10s;
            NRF_LOG_DEBUG("Gas sensor baseline loaded from flash, value 0x%04x, mode: GAS_MODE_10S \r\n", *p_gas_baseline);
        break;

        case GAS_MODE_60S:
            *p_gas_baseline = m_p_baseline->mode_60s;
            NRF_LOG_DEBUG("Gas sensor baseline loaded from flash, value 0x%04x, mode: GAS_MODE_60S \r\n", *p_gas_baseline);
        break;

        default:
            return NRF_ERROR_INVALID_STATE;
    }

    return NRF_SUCCESS;
}


/**@brief Stores the gas sensor baseline values to flash storage.
 */
static uint32_t gas_store_baseline_flash(uint16_t baseline)
{
    uint32_t err_code;

    switch(m_config.gas_interval_mode)
    {
        case GAS_MODE_250MS:
            m_p_baseline->mode_250ms = baseline;
            NRF_LOG_DEBUG("Gas sensor baseline stored to flash, value 0x%04x, mode: GAS_MODE_250MS\r\n", baseline);
        break;

        case GAS_MODE_1S:
            m_p_baseline->mode_1s = baseline;
            NRF_LOG_DEBUG("Gas sensor baseline stored to flash, value 0x%04x, mode: GAS_MODE_1S\r\n", baseline);
        break;

        case GAS_MODE_10S:
            m_p_baseline->mode_10s = baseline;
            NRF_LOG_DEBUG("Gas sensor baseline stored to flash, value 0x%04x, mode: GAS_MODE_10S\r\n", baseline);
        break;

        case GAS_MODE_60S:
            m_p_baseline->mode_60s = baseline;
            NRF_LOG_DEBUG("Gas sensor baseline stored to flash, value 0x%04x, mode: GAS_MODE_60S\r\n", baseline);
        break;

        default:
            return NRF_ERROR_INVALID_STATE;
    }

    err_code = m_env_flash_baseline_store(m_p_baseline); // Store new baseline values to flash.
    RETURN_IF_ERROR(err_code);

    return NRF_SUCCESS;
}


static uint32_t gas_start(void)
{
    NRF_LOG_DEBUG("Gas start: mode: 0x%x \r\n", m_config.gas_interval_mode);

    uint32_t err_code;
    drv_gas_sensor_mode_t mode;

    switch (m_config.gas_interval_mode)
    {
        case GAS_MODE_250MS:
            mode = DRV_GAS_SENSOR_MODE_250MS;
            break;
        case GAS_MODE_1S:
            mode = DRV_GAS_SENSOR_MODE_1S;
            break;
        case GAS_MODE_10S:
            mode = DRV_GAS_SENSOR_MODE_10S;
            break;
        case GAS_MODE_60S:
            mode = DRV_GAS_SENSOR_MODE_60S;
            break;
        default:
            mode = DRV_GAS_SENSOR_MODE_10S;
            break;
    }

    err_code = drv_gas_sensor_start(mode);
    RETURN_IF_ERROR(err_code);

    m_gas_state = GAS_STATE_WARMUP;

    return app_timer_start(gas_calib_timer_id,
                           APP_TIMER_TICKS(M_GAS_BASELINE_WRITE_MS),
                           NULL);
}


static uint32_t gas_stop(void)
{
    uint32_t err_code;
    uint16_t baseline;

    if (m_gas_state == GAS_STATE_ACTIVE)
    {
        err_code = humidity_temp_stop_for_gas_calibration();
        RETURN_IF_ERROR(err_code);

        err_code = drv_gas_sensor_baseline_get(&baseline);
        RETURN_IF_ERROR(err_code);

        err_code = gas_store_baseline_flash(baseline);
        RETURN_IF_ERROR(err_code);
    }

    m_gas_state = GAS_STATE_IDLE;

    return drv_gas_sensor_stop();
}


/**@brief Function for handling color timer timeout event.
 *
 * @details This function will read the color at the configured rate.
 */
static void color_timeout_handler(void * p_context)
{
    uint32_t                    err_code;
    drv_ext_light_rgb_intensity_t color;

    color.r = m_config.color_config.led_red;
    color.g = m_config.color_config.led_green;
    color.b = m_config.color_config.led_blue;
    (void)drv_ext_light_rgb_intensity_set(DRV_EXT_RGB_LED_SENSE, &color);

    err_code = drv_color_sample();
    APP_ERROR_CHECK(err_code);
}


static uint32_t color_start(void)
{
    uint32_t                    err_code;
    drv_ext_light_rgb_intensity_t color;

    color.r = m_config.color_config.led_red;
    color.g = m_config.color_config.led_green;
    color.b = m_config.color_config.led_blue;

    (void)drv_ext_light_rgb_intensity_set(DRV_EXT_RGB_LED_SENSE, &color);

    err_code = drv_color_start();
    APP_ERROR_CHECK(err_code);

    err_code = drv_color_sample();
    APP_ERROR_CHECK(err_code);

    return app_timer_start(color_timer_id,
                           APP_TIMER_TICKS(m_config.color_interval_ms),
                           NULL);
}


static uint32_t color_stop(void)
{
    uint32_t err_code;

    err_code = app_timer_stop(color_timer_id);
    APP_ERROR_CHECK(err_code);

    (void)drv_ext_light_off(DRV_EXT_RGB_LED_SENSE);

    err_code = drv_color_stop();
    APP_ERROR_CHECK(err_code);

    return NRF_SUCCESS;
}


static uint32_t config_verify(tes_config_t * p_config)
{
    uint32_t err_code;

    if ( (p_config->temperature_interval_ms < TES_CONFIG_TEMPERATURE_INT_MIN)    ||
         (p_config->temperature_interval_ms > TES_CONFIG_TEMPERATURE_INT_MAX)    ||
         (p_config->pressure_interval_ms < TES_CONFIG_PRESSURE_INT_MIN)          ||
         (p_config->pressure_interval_ms > TES_CONFIG_PRESSURE_INT_MAX)          ||
         (p_config->humidity_interval_ms < TES_CONFIG_HUMIDITY_INT_MIN)          ||
         (p_config->humidity_interval_ms > TES_CONFIG_HUMIDITY_INT_MAX)          ||
         (p_config->color_interval_ms < TES_CONFIG_COLOR_INT_MIN)                ||
         (p_config->color_interval_ms > TES_CONFIG_COLOR_INT_MAX)                ||
         (p_config->gas_interval_mode < TES_CONFIG_GAS_MODE_MIN)                 ||
         ((int)p_config->gas_interval_mode > (int)TES_CONFIG_GAS_MODE_MAX))
    {
//        err_code = m_env_flash_config_store((tes_config_t *)&m_default_config);
//        APP_ERROR_CHECK(err_code);
    	memcpy(p_config, &m_default_config, sizeof(tes_config_t));
    }

    return NRF_SUCCESS;
}


/**@brief Function for applying the configuration.
 *
 */
static uint32_t config_apply(tes_config_t * p_config)
{
    uint32_t err_code;

    NULL_PARAM_CHECK(p_config);

    (void)temperature_stop(false);
    (void)pressure_stop();
    (void)humidity_stop(true);
    (void)color_stop();

    if ((p_config->temperature_interval_ms > 0))
    {
        err_code = temperature_start();
        APP_ERROR_CHECK(err_code);
    }

    if ((p_config->pressure_interval_ms > 0))
    {
        err_code = pressure_start();
        APP_ERROR_CHECK(err_code);
    }

    if ((p_config->humidity_interval_ms > 0))
    {
        err_code = humidity_start();
        APP_ERROR_CHECK(err_code);
    }

    if ((p_config->color_interval_ms > 0))
    {
        err_code = color_start();
        APP_ERROR_CHECK(err_code);
    }

    return NRF_SUCCESS;
}


/**@brief Function for initializing the humidity/temperature sensor
 */
static uint32_t humidity_sensor_init(const nrf_drv_twi_t * p_twi_instance)
{
    ret_code_t               err_code = NRF_SUCCESS;

    static const nrf_drv_twi_config_t twi_config =
    {
        .scl                = TWI_SCL,
        .sda                = TWI_SDA,
        .frequency          = NRF_TWI_FREQ_400K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };

    drv_humidity_init_t    init_params =
    {
        .twi_addr            = HTS221_ADDR,
        .pin_int             = HTS_INT,
        .p_twi_instance      = p_twi_instance,
        .p_twi_cfg           = &twi_config,
//        .evt_handler         = drv_humidity_evt_handler
    };

    err_code = drv_humidity_init(&init_params);

    return err_code;
}


static uint32_t pressure_sensor_init(const nrf_drv_twi_t * p_twi_instance)
{
    drv_pressure_init_t init_params;

    static const nrf_drv_twi_config_t twi_config =
    {
        .scl                = TWI_SCL,
        .sda                = TWI_SDA,
        .frequency          = NRF_TWI_FREQ_400K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };

    init_params.twi_addr                = LPS22HB_ADDR;
    init_params.pin_int                 = LPS_INT;
    init_params.p_twi_instance          = p_twi_instance;
    init_params.p_twi_cfg               = &twi_config;
//    init_params.evt_handler             = drv_pressure_evt_handler;
    init_params.mode                    = DRV_PRESSURE_MODE_BAROMETER;

    return drv_pressure_init(&init_params);
}


static uint32_t gas_sensor_init(const nrf_drv_twi_t * p_twi_instance)
{
    uint32_t       err_code;
    drv_gas_init_t init_params;

    static const nrf_drv_twi_config_t twi_config =
    {
        .scl                = TWI_SCL,
        .sda                = TWI_SDA,
        .frequency          = NRF_TWI_FREQ_400K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };

    init_params.p_twi_instance = p_twi_instance;
    init_params.p_twi_cfg      = &twi_config;
    init_params.twi_addr       = CCS811_ADDR;
//    init_params.data_handler   = drv_gas_data_handler;

    err_code = drv_gas_sensor_init(&init_params);
    RETURN_IF_ERROR(err_code);

    return NRF_SUCCESS;
}


static uint32_t color_sensor_init(const nrf_drv_twi_t * p_twi_instance)
{
    uint32_t err_code;
    drv_color_init_t init_params;

    static const nrf_drv_twi_config_t twi_config =
    {
        .scl                = TWI_SCL,
        .sda                = TWI_SDA,
        .frequency          = NRF_TWI_FREQ_400K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };

    init_params.p_twi_instance = p_twi_instance;
    init_params.p_twi_cfg      = &twi_config;
    init_params.twi_addr       = BH1745_ADDR;
//    init_params.data_handler   = drv_color_data_handler;

    err_code = drv_color_init(&init_params);
    RETURN_IF_ERROR(err_code);

    return NRF_SUCCESS;
}


uint32_t m_environment_start(void)
{
	uint32_t err_code;

//	err_code = temperature_start();
//	APP_ERROR_CHECK(err_code);

	err_code = pressure_start();
	APP_ERROR_CHECK(err_code);

//	err_code = humidity_start();
//	APP_ERROR_CHECK(err_code);
//
//	err_code = color_start();
//	APP_ERROR_CHECK(err_code);
//
//	err_code = gas_start();
//	APP_ERROR_CHECK(err_code);

	return NRF_SUCCESS;
}


uint32_t m_environment_stop(void)
{
    uint32_t err_code;

    err_code = temperature_stop(false);
    APP_ERROR_CHECK(err_code);

    err_code = pressure_stop();
    APP_ERROR_CHECK(err_code);

    err_code = humidity_stop(true);
    APP_ERROR_CHECK(err_code);

    err_code = color_stop();
    APP_ERROR_CHECK(err_code);

    err_code = gas_stop();
    APP_ERROR_CHECK(err_code);

    return NRF_SUCCESS;
}


uint32_t m_environment_init(m_environment_init_t * p_params)
{
    uint32_t err_code;

    NULL_PARAM_CHECK(p_params);

    NRF_LOG_INFO("Init: \r\n");


    /**@brief Init drivers */
    err_code = pressure_sensor_init(p_params->p_twi_instance);
    APP_ERROR_CHECK(err_code);

//    err_code = humidity_sensor_init(p_params->p_twi_instance);
//    APP_ERROR_CHECK(err_code);
//
//    err_code = gas_sensor_init(p_params->p_twi_instance);
//    APP_ERROR_CHECK(err_code);
//
//    err_code = color_sensor_init(p_params->p_twi_instance);
//    APP_ERROR_CHECK(err_code);

    /**@brief Init application timers */
//    err_code = app_timer_create(&temperature_timer_id, APP_TIMER_MODE_REPEATED, temperature_timeout_handler);
//    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&pressure_timer_id, APP_TIMER_MODE_REPEATED, pressure_timeout_handler);
    APP_ERROR_CHECK(err_code);

//    err_code = app_timer_create(&humidity_timer_id, APP_TIMER_MODE_REPEATED, humidity_timeout_handler);
//    RETURN_IF_ERROR(err_code);
//
//    err_code = app_timer_create(&color_timer_id, APP_TIMER_MODE_REPEATED, color_timeout_handler);
//    RETURN_IF_ERROR(err_code);
//
//    err_code = app_timer_create(&gas_calib_timer_id, APP_TIMER_MODE_REPEATED, gas_calib_timeout_handler);
//    RETURN_IF_ERROR(err_code);

    return NRF_SUCCESS;
}


static void thingy_init(void)
{
    uint32_t                 err_code;
//    m_ui_init_t              ui_params;
    m_environment_init_t     env_params;
//    m_motion_init_t          motion_params;
//    m_ble_init_t             ble_params;
//    batt_meas_init_t         batt_meas_init = BATT_MEAS_PARAM_CFG;

    /**@brief Initialize the TWI manager. */
    err_code = twi_manager_init(APP_IRQ_PRIORITY_THREAD);
    APP_ERROR_CHECK(err_code);

//    /**@brief Initialize LED and button UI module. */
//    ui_params.p_twi_instance = &m_twi_sensors;
//    err_code = m_ui_init(&m_ble_service_handles[THINGY_SERVICE_UI],
//                         &ui_params);
//    APP_ERROR_CHECK(err_code);

    /**@brief Initialize environment module. */
	env_params.p_twi_instance = &m_twi_sensors;
	err_code = m_environment_init(&env_params);
	APP_ERROR_CHECK(err_code);

//    /**@brief Initialize motion module. */
//    motion_params.p_twi_instance = &m_twi_sensors;
//
//    err_code = m_motion_init(&m_ble_service_handles[THINGY_SERVICE_MOTION],
//                             &motion_params);
//    APP_ERROR_CHECK(err_code);
//
//    err_code = m_sound_init(&m_ble_service_handles[THINGY_SERVICE_SOUND]);
//    APP_ERROR_CHECK(err_code);

//    err_code = m_ui_led_set_event(M_UI_BLE_DISCONNECTED);
//    APP_ERROR_CHECK(err_code);
}


static void board_init(void)
{
    uint32_t            err_code;
    drv_ext_gpio_init_t ext_gpio_init;

    #if defined(THINGY_HW_v0_7_0)
        #error   "HW version v0.7.0 not supported."
    #elif defined(THINGY_HW_v0_8_0)
        NRF_LOG_WARNING("FW compiled for depricated Thingy HW v0.8.0 \r\n");
    #elif defined(THINGY_HW_v0_9_0)
        NRF_LOG_WARNING("FW compiled for depricated Thingy HW v0.9.0 \r\n");
    #endif

    static const nrf_drv_twi_config_t twi_config =
    {
        .scl                = TWI_SCL,
        .sda                = TWI_SDA,
        .frequency          = NRF_TWI_FREQ_400K,
        .interrupt_priority = APP_IRQ_PRIORITY_LOW
    };

    static const drv_sx1509_cfg_t sx1509_cfg =
    {
        .twi_addr       = SX1509_ADDR,
        .p_twi_instance = &m_twi_sensors,
        .p_twi_cfg      = &twi_config
    };

    ext_gpio_init.p_cfg = &sx1509_cfg;

    err_code = support_func_configure_io_startup(&ext_gpio_init);
    APP_ERROR_CHECK(err_code);

    task_sleepms(100);
}


/***************************************************************************************************************/
void sampling_event_send(uint8_t evt, uint8_t state, uint8_t *msg) {

	smpEvt_msgt sampling_msg;

	sampling_msg.event = evt;
	sampling_msg.state = state;
	sampling_msg.msg = msg;

	msgq_send(sampling_msgq, (unsigned char*) &sampling_msg);
}

static void sampling_task(void *arg) {
	int r;
    uint32_t err_code;
	smpEvt_msgt read_msg;

	tes_pressure_t *pressure;

	nrf_drv_gpiote_init();

	ble_stack_init_wait();

	task_sleepms(1000);

	board_init();
	thingy_init();

//	/**@brief Load configuration from flash. */
//	err_code = m_env_flash_init(&m_default_config, &m_p_config, &m_default_baseline, &m_p_baseline);
//	APP_ERROR_CHECK(err_code);

	config_verify(&m_config);

	m_environment_start();

	while (1) {
		r = msgq_receive(sampling_msgq, (unsigned char*) &read_msg);
		if (0 != r) {
			logme("fail at msgq_receive\r\n");
		} else {
			switch (read_msg.event) {

			case SAMP_EVT_ENV:
				switch (read_msg.state) {
				case ENV_STATE_TEMP_HUMID:

					break;

				case ENV_STATE_PRESSURE:

				    err_code = drv_pressure_sample();
				    APP_ERROR_CHECK(err_code);

					pressure = (tes_pressure_t*) malloc(sizeof(tes_pressure_t));
					pressure_conv_data(drv_pressure_get(), pressure);

					free(pressure);
					break;

				case ENV_STATE_GAS:

					break;

				case ENV_STATE_COLOR:

					break;
				}
				break; //SAMP_EVT_ENV
			}

			if (read_msg.msg != NULL) {
				free(read_msg.msg);
			}
		}
	}
}

void sampling_task_init(void) {
	int r;

	r = msgq_create(&sampling_msgq, sizeof(smpEvt_msgt), 20);
	if (0 != r) {
		printf("fail at msgq create\r\n");
	}

	r = task_create(NULL, sampling_task, NULL, task_gethighestpriority() - 2, 512, NULL);
	if (r != 0) {
		printf("== sampling_task failed\n\r");
	} else {
		printf("== sampling_task created\n\r");
	}
}
/***************************************************************************************************************/
