/*
 * sampling_main.c
 *
 *  Created on: 2020. 9. 15.
 *      Author: chlee
 */

#include <ubinos.h>

#include <stdint.h>
#include <float.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
//#include "softdevice_handler.h"
#include "app_scheduler.h"
#include "app_button.h"
#include "app_util_platform.h"
#include "m_ble.h"
#include "m_environment.h"
#include "m_sound.h"
#include "m_motion.h"
#include "m_ui.h"
#include "drv_ext_light.h"
#include "drv_ext_gpio.h"
#include "nrf_delay.h"
#include "twi_manager.h"
#include "support_func.h"
#include "pca20020.h"
#include "app_error.h"

#define  NRF_LOG_MODULE_NAME main
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "sampling.h"
#include "nrf_drv_gpiote.h"

#define DEAD_BEEF   0xDEADBEEF          /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#define SCHED_MAX_EVENT_DATA_SIZE   APP_TIMER_SCHED_EVENT_DATA_SIZE /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE            60  /**< Maximum number of events in the scheduler queue. */

msgq_pt sampling_msgq;

static const nrf_drv_twi_t m_twi_sensors = NRF_DRV_TWI_INSTANCE(TWI_SENSOR_INSTANCE)
;
static m_ble_service_handle_t m_ble_service_handles[THINGY_SERVICES_MAX];

/**@brief Function for initializing the Thingy.
 */
static void thingy_init(void)
{
    uint32_t err_code;
    m_ui_init_t ui_params;
    m_environment_init_t env_params;
    m_motion_init_t motion_params;
    m_ble_init_t ble_params;

    /**@brief Initialize the TWI manager. */
    err_code = twi_manager_init(APP_IRQ_PRIORITY_THREAD);
    APP_ERROR_CHECK(err_code);

    /**@brief Initialize LED and button UI module. */
    ui_params.p_twi_instance = &m_twi_sensors;
    err_code = m_ui_init(&m_ble_service_handles[THINGY_SERVICE_UI], &ui_params);
    APP_ERROR_CHECK(err_code);

    /**@brief Initialize environment module. */
    env_params.p_twi_instance = &m_twi_sensors;
    err_code = m_environment_init(&m_ble_service_handles[THINGY_SERVICE_ENVIRONMENT], &env_params);
    APP_ERROR_CHECK(err_code);

    /**@brief Initialize motion module. */
    motion_params.p_twi_instance = &m_twi_sensors;

    err_code = m_motion_init(&m_ble_service_handles[THINGY_SERVICE_MOTION], &motion_params);
    APP_ERROR_CHECK(err_code);

    err_code = m_sound_init(&m_ble_service_handles[THINGY_SERVICE_SOUND]);
    APP_ERROR_CHECK(err_code);

    err_code = m_ui_led_set_event(M_UI_BLE_DISCONNECTED);
    APP_ERROR_CHECK(err_code);
}

static void board_init(void)
{
    uint32_t err_code;
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
            .scl = TWI_SCL,
            .sda = TWI_SDA,
            .frequency = NRF_TWI_FREQ_400K,
            .interrupt_priority = APP_IRQ_PRIORITY_LOW };

    static const drv_sx1509_cfg_t sx1509_cfg =
    {
            .twi_addr = SX1509_ADDR,
            .p_twi_instance = &m_twi_sensors,
            .p_twi_cfg = &twi_config };

    ext_gpio_init.p_cfg = &sx1509_cfg;

    err_code = support_func_configure_io_startup(&ext_gpio_init);
    APP_ERROR_CHECK(err_code);

    task_sleepms(100);
}

/***************************************************************************************************************/
void sampling_event_send(uint8_t evt, uint8_t state, uint8_t *msg)
{
    smpEvt_msgt sampling_msg;

    sampling_msg.event = evt;
    sampling_msg.state = state;
    sampling_msg.msg = msg;

    msgq_send(sampling_msgq, (unsigned char*) &sampling_msg);
}

static void sampling_task(void *arg)
{
    int r;
    uint32_t err_code;
    smpEvt_msgt read_msg;

    nrf_drv_gpiote_init();

    ble_stack_init_wait();

    task_sleepms(1000);

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    printf("===== Thingy started! =====\r\n");

    // Initialize.
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    board_init();
    thingy_init();

    m_environment_start();

    for (;;)
    {
        app_sched_execute();
        task_sleepms(10);
    }
}

void sampling_task_init(void)
{
    int r;

    r = msgq_create(&sampling_msgq, sizeof(smpEvt_msgt), 20);
    if (0 != r)
    {
        printf("fail at msgq create\r\n");
    }

    r = task_create(NULL, sampling_task, NULL, task_gethighestpriority() - 2, 512, NULL);
    if (r != 0)
    {
        printf("== sampling_task failed\n\r");
    }
    else
    {
        printf("== sampling_task created\n\r");
    }
}
/***************************************************************************************************************/
