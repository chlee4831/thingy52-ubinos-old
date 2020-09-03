/*
 * ble_stack.h
 *
 *  Created on: 2020. 06. 11.
 *      Author: YJPark
 */

#ifndef BLE_INIT_H_
#define BLE_INIT_H_

#include <ubinos.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_gattc.h"
#include "ble_gatts.h"
#include "ble_hci.h"
#include "ble_types.h"
#include "nrf_sdm.h"

#include "ble_conn_params.h"
#include "ble_conn_state.h"

#include "app_timer.h"
#include "app_util.h"

#include "ble_profile.h"

#define CENTRAL_LINK_COUNT				7               /**<number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT			1               /**<number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

/**
 * scan parameter
 */
#define SCAN_INTERVAL					0x0C80	        /**< (1000ms) Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW            			0x0140          /**< (100ms) Determines scan window in units of 0.625 millisecond. */
#define SCAN_ACTIVE             		0               /**< If 1, performe active scanning (scan requests). */
#define SCAN_SELECTIVE         			0               /**< If 1, ignore unknown devices (non whitelisted). */
#define SCAN_TIMEOUT            		0x0140          /**< Scan timeout between 0x0001 and 0xFFFF in seconds, 0x0000 disables timeout. */

/**
 * connection parameter
 */
#define MIN_CONNECTION_INTERVAL 		MSEC_TO_UNITS(50, UNIT_1_25_MS) 	/**< Determines minimum connection interval in millisecond. */
#define MAX_CONNECTION_INTERVAL 		MSEC_TO_UNITS(100, UNIT_1_25_MS)		/**< Determines maximum connection interval in millisecond. */
#define SLAVE_LATENCY           		0                               	/**< Determines slave latency in counts of connection events. */
#define SUPERVISION_TIMEOUT     		MSEC_TO_UNITS(1000, UNIT_10_MS) 	/**< Determines supervision time-out in units of 10 millisecond. */

/**
 * Connection Parameters Module init paprameter
 */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    10
#define PAAR_BLE_CONN_HANDLE_INVALID	0xFFFF//BLE_CONN_HANDLE_INVALID

typedef struct
{
	uint8_t event;
	uint8_t status;
	uint16_t conn_handle;
	uint8_t* msg;
}BLEEvt_msgt;

enum
{
	BLE_STACK_CENTRAL_EVT = 0,
	BLE_STACK_PERIPHERAL_EVT,
	BLE_STACK_PERIPHERAL_ADV_EVT,
};

#define LAP_ADV_DATA_LEN		25

typedef struct{
	ble_gap_addr_t peer_address;
	ble_gap_addr_t direct_address;
	int8_t rssi;
	uint8_t data[LAP_ADV_DATA_LEN];
	uint16_t data_len;
}LAP_ble_adv_report;

void BLE_stack_task_init(void);

int32_t PAAR_send_ble_msg_central(uint16_t send_conn_handle, uint16_t handle, uint8_t * msg, uint16_t length);
uint32_t PAAR_send_ble_test_msg_central(uint16_t send_conn_handle, uint16_t handle, uint8_t * msg, uint16_t length);
int32_t PAAR_send_ble_msg_peripheral(uint8_t * msg, uint16_t length);

void PAAR_scan_start(void);
void PAAR_scan_stop(void);

void PAAR_adv_start();
void PAAR_adv_restart();
void PAAR_adv_stop();
void PAAR_set_adv_data(uint8_t* adv_data, uint16_t adv_data_len, uint8_t* scan_rsp_data, uint16_t scan_rsp_data_len);

void PAAR_ble_gap_connect(ble_gap_addr_t * peer_addr);
uint32_t PAAR_ble_gap_disconnect(uint16_t conn_handle);

ble_paar_t * get_peripheral_info(void);

bool get_ble_stack_init_flag();

void ble_stack_init_wait();

void set_ble_write_rsp_flag_set();
uint8_t get_ble_write_rsp_flag();

uint8_t* get_hub_sosp_id();
/**********************************************************************************************************************/

#endif /*BLE_H_*/
