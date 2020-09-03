/*
 * ADL_main.c
 *
 *  Created on: 2020. 8. 13.
 *      Author: YJPark
 */

#include <ubinos.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ubinos.h>

#include "malloc.h"

// nrf library
#include "app_timer.h"
#include "app_uart.h"
#include "nrf_sdm.h"
#include "nrf_drv_gpiote.h"

//application header
#include "sw_config.h"
#include "pca20020.h"

#include "ble_stack.h"
#include "LAP_api.h"
#include "LAP_main.h"

#include "twi_manager.h"
#include "thingy_main.h"

#include "m_environment.h"
#include "m_motion.h"
//#include "m_sound.h"
#include "m_ui.h"


//APP_TIMER_DEF(test_timer);
static const nrf_drv_twi_t m_twi_sensors = NRF_DRV_TWI_INSTANCE(TWI_SENSOR_INSTANCE);

static msgq_pt main_msgq;

//============================================================================================

static void thingy_init(void)
{
    uint32_t                 err_code;
    m_ui_init_t              ui_params;
    m_environment_init_t     env_params;
    m_motion_init_t          motion_params;

    /**@brief Initialize the TWI manager. */
    err_code = twi_manager_init(APP_IRQ_PRIORITY_THREAD);
    APP_ERROR_CHECK(err_code);

    /**@brief Initialize LED and button UI module. */
    ui_params.p_twi_instance = &m_twi_sensors;
    err_code = m_ui_init(&ui_params);
    APP_ERROR_CHECK(err_code);

    /**@brief Initialize environment module. */
    env_params.p_twi_instance = &m_twi_sensors;
    err_code = m_environment_init(&env_params);
    APP_ERROR_CHECK(err_code);

    /**@brief Initialize motion module. */
    motion_params.p_twi_instance = &m_twi_sensors;

    err_code = m_motion_init(&motion_params);
    APP_ERROR_CHECK(err_code);

//    err_code = m_sound_init();
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

//============================================================================================

void thingy_main_event_send(uint8_t evt, uint8_t state, uint8_t *msg) {

	mainEvt_msgt main_msg;

	main_msg.event = evt;
	main_msg.state = state;
	main_msg.msg = msg;

	msgq_send(main_msgq, (unsigned char*) &main_msg);
}

void thingy_main_task(void *arg) {
	int r;
	mainEvt_msgt read_msg;

	nrf_drv_gpiote_init();

//	LED_init();

	ble_stack_init_wait();

	task_sleepms(1000);

	board_init();
	thingy_init();

	while (1) {
		r = msgq_receive(main_msgq, (unsigned char*) &read_msg);
		if (0 != r) {
			logme("fail at msgq_receive\r\n");
		} else {
			switch (read_msg.event) {

			case THINGY_EVT_TEST_TIMEOUT:

				break;

			case THINGY_EVT_BUTTON:

				break;
			}

			if (read_msg.msg != NULL) {
				free(read_msg.msg);
			}
		}
	}
}

/***************************************************************************************************************/
void thingy_main_task_init(void) {
	int r;

	r = msgq_create(&main_msgq, sizeof(mainEvt_msgt), 20);
	if (0 != r) {
		printf("fail at msgq create\r\n");
	}

	r = task_create(NULL, thingy_main_task, NULL, task_gethighestpriority() - 2, 1024, NULL);
	if (r != 0) {
		printf("== main_task failed\n\r");
	} else {
		printf("== main_task created\n\r");
	}
}
