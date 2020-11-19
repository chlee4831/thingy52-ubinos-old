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

#include "thingy_main.h"

static msgq_pt main_msgq;

static int test_count = 0;

void thingy_main_event_send(uint8_t evt, uint8_t state, uint8_t *msg)
{

    mainEvt_msgt main_msg;

    main_msg.event = evt;
    main_msg.state = state;
    main_msg.msg = msg;

    msgq_send(main_msgq, (unsigned char*) &main_msg);
}

void button_init(nrf_drv_gpiote_evt_handler_t handler)
{
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false)
    ;
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    nrf_drv_gpiote_in_init(BUTTON, &in_config, (nrf_drv_gpiote_evt_handler_t) handler);

    nrf_drv_gpiote_in_event_enable(BUTTON, true);
}

void button_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{

    bsp_busywaitms(10);
    if ( nrf_drv_gpiote_in_is_set(pin))
        return;

    thingy_main_event_send(THINGY_MAIN_EVT_BUTTON, 0, NULL);
}

void thingy_main_task(void *arg)
{
    int r;
    mainEvt_msgt read_msg;

    nrf_drv_gpiote_init();

    button_init(button_handler);

    ble_stack_init_wait();

    task_sleepms(1000);

    while (1)
    {
        r = msgq_receive(main_msgq, (unsigned char*) &read_msg);
        if (0 != r)
        {
            logme("fail at msgq_receive\r\n");
        }
        else
        {
            switch (read_msg.event)
            {

            case THINGY_MAIN_EVT_BUTTON:
            {
                paar_packet_t tmp_packet;

                tmp_packet.service_ID = ENV_SENSOR_TAG_SERVICE_ID;
                tmp_packet.seq = 0x11;
                tmp_packet.data_len = 2;
                tmp_packet.data[0] = 0x04;
                tmp_packet.data[1] = test_count;

                test_count++;

                send_packet_peripheral(&tmp_packet);

//                LAP_start_ble_scan(NULL);
                break;
            }

            }
            if (read_msg.msg != NULL)
            {
                free(read_msg.msg);
            }
        }
    }
}

/***************************************************************************************************************/
void thingy_main_task_init(void)
{
    int r;

    r = msgq_create(&main_msgq, sizeof(mainEvt_msgt), 20);
    if (0 != r)
    {
        printf("fail at msgq create\r\n");
    }

    r = task_create(NULL, thingy_main_task, NULL, task_gethighestpriority() - 2, 512, NULL);
    if (r != 0)
    {
        printf("== main_task failed\n\r");
    }
    else
    {
        printf("== main_task created\n\r");
    }
}
