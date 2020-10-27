/*
 * cell_management.c
 *
 *  Created on: 2019. 10. 24.
 *      Author: YuJin Park
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "malloc.h"

#include "ble_gap.h"
#include "LAP_api.h"
#include "LAP_main.h"
#include "thingy_main.h"
#include "sw_config.h"

#include "cell_management.h"

#define CELL_MANAGEMENT_MAX_CONNECTION					6
#define CELL_MANAGEMENT_DATA_MAX_COUNT					7
#define CELL_MANAGEMENT_CONN_DELAY_TIME					5
#define CELL_MANAGEMENT_DEFAULT_CONNECTION_RETRY_COUNT	3
#define CELL_MANAGEMENT_LIFE_TIME						10
#define CELL_MANAGEMENT_LIFE_TIME_CONNECTING			20
#define CELL_MANAGEMENT_HB_COUNT						5
#define CELL_MANAGEMENT_HB_PACKET_SIZE					19

// FE Management
static uint8_t cell_management_data_count = 0;
static uint8_t cell_management_connection_count = 0;
static CellManagement_data_t cell_management_data[CELL_MANAGEMENT_DATA_MAX_COUNT];
static uint8_t current_connecting_index = 0xFF;

mutex_pt cell_mutex;

#define ENABLE_MUTEX		1

uint8_t get_cell_management_data_count() {
	return cell_management_data_count;
}

void cell_management_data_init() {
	cell_management_data_count = 0;
	cell_management_connection_count = 0;
	memset(cell_management_data, 0, sizeof(CellManagement_data_t) * CELL_MANAGEMENT_DATA_MAX_COUNT);

	uint8_t i;
	for (i = 0; i < CELL_MANAGEMENT_DATA_MAX_COUNT; i++) {
		cell_management_data[i].is_empty = true;
	}

	int r;
	r = mutex_create(&cell_mutex);
	if (r != 0) {
		printf("fail at mutex_create : cell_mutex");
	}
}

uint8_t cell_management_search_data_index(uint8_t *PAAR_id) {
	uint8_t i;
	for (i = 0; i < CELL_MANAGEMENT_DATA_MAX_COUNT; i++) {
		if (cell_management_data[i].is_empty == true)
			continue;

		if (memcmp(&cell_management_data[i].paarID[0], PAAR_id, PAAR_ID_SIZE) == 0)
			return i;
	}
	return 0xFF;
}

uint8_t cell_management_search_data_index_by_connhandle(uint16_t connhandle) {
	uint8_t i;
	for (i = 0; i < CELL_MANAGEMENT_DATA_MAX_COUNT; i++) {
		if (cell_management_data[i].conn_handle == connhandle)
			return i;
	}
	return 0xFF;
}

uint8_t* cell_management_search_paar_id_by_connhandle(uint16_t connhandle) {
	uint8_t i;
	for (i = 0; i < CELL_MANAGEMENT_DATA_MAX_COUNT; i++) {
		if (cell_management_data[i].conn_handle == connhandle) {
			if (cell_management_data[i].is_empty == false) {
				return &(cell_management_data[i].paarID);
			}
		}
	}

	return NULL;

}

uint8_t* cell_management_get_PAAR_ID_by_index(uint8_t index) {
	if (cell_management_data[index].is_empty == false) {
		return &(cell_management_data[index].paarID);
	}
	return NULL;
}

static uint8_t cell_management_search_empty_index() {
	uint8_t i;
	uint8_t check_paar_id[PAAR_ID_SIZE] = { 0, };
	for (i = 0; i < CELL_MANAGEMENT_DATA_MAX_COUNT; i++) {
		if (cell_management_data[i].is_empty == true)
			return i;
	}
	return 0xFF;
}

int cell_management_data_add(LAP_ble_adv_report *pPkt) {
#if(ENABLE_MUTEX == 1)
	mutex_lock(cell_mutex);
#endif

	if (cell_management_data_count > CELL_MANAGEMENT_DATA_MAX_COUNT) {
#if(ENABLE_MUTEX == 1)
		mutex_unlock(cell_mutex);
#endif
		return -1;
	}

	uint8_t temp_paar_id[4];

	memcpy(temp_paar_id, &(pPkt->data[LAP_ADV_IDX_PAAR_DEVICE_ID0]), 4);

//#if(SW_MODE_SETUP == SW_MODE_ADL_NEW_DEVICE_CENTRAL)
//	if(pPkt->data[LAP_ADV_IDX_PAAR_DEVICE_ID3] != EDGE_TAG_DEVICE_ID)
//		return -1;
//#endif

	if (cell_management_search_data_index(&(pPkt->data[LAP_ADV_IDX_PAAR_DEVICE_ID0])) == 0xFF) {
		uint8_t index = cell_management_search_empty_index();

		if (index == 0xFF) {
#if(ENABLE_MUTEX == 1)
			mutex_unlock(cell_mutex);
#endif
			return -2;
		}

		//save FE cell Management data
		cell_management_data[index].is_empty = false;
		cell_management_data[index].connecting_state = CONNECTING_STATE_DISCONNECTED;
		cell_management_data[index].is_authourized = false;
		cell_management_data[index].life_time = CELL_MANAGEMENT_LIFE_TIME;

//#if(SW_MODE_SETUP == SW_MODE_ADL_NEW_DEVICE_CENTRAL)
		cell_management_data[index].is_authourized = true;
//#endif

		cell_management_data[index].conn_handle = PAAR_BLE_CONN_HANDLE_INVALID;
		cell_management_data[index].connection_retry_cnt = CELL_MANAGEMENT_DEFAULT_CONNECTION_RETRY_COUNT;

		memcpy(&cell_management_data[index].paarID[0], &(pPkt->data[LAP_ADV_IDX_PAAR_DEVICE_ID0]), PAAR_ID_SIZE);
		memcpy(&cell_management_data[index].gap_addr, &pPkt->peer_address, sizeof(ble_gap_addr_t));

		//save attribute handle value
		cell_management_data[index].profile_data.tx_handle = (pPkt->data[LAP_ADV_IDX_NOTIFICATION_HANDLE + 1] << 8)
				| (pPkt->data[LAP_ADV_IDX_NOTIFICATION_HANDLE]);

		cell_management_data[index].profile_data.rx_handle = (pPkt->data[LAP_ADV_IDX_WRITE_REQUEST_HANDLE + 1] << 8)
				| (pPkt->data[LAP_ADV_IDX_WRITE_REQUEST_HANDLE]);

		cell_management_data[index].profile_data.cccd_handle = (pPkt->data[LAP_ADV_IDX_NOTI_EN_HANDLE + 1] << 8) | (pPkt->data[LAP_ADV_IDX_NOTI_EN_HANDLE]);

		cell_management_data[index].conn_delay = CELL_MANAGEMENT_CONN_DELAY_TIME;
		cell_management_data_count++;

#if(ENABLE_MUTEX == 1)
		mutex_unlock(cell_mutex);
#endif
		return 0;
	} else {
		uint8_t index = cell_management_search_data_index(&(pPkt->data[LAP_ADV_IDX_PAAR_DEVICE_ID0]));
		if (cell_management_data[index].connecting_state != CONNECTING_STATE_DISCONNECTED)
			return -1;

		cell_management_data[index].life_time = CELL_MANAGEMENT_LIFE_TIME;
	}

#if(ENABLE_MUTEX == 1)
	mutex_unlock(cell_mutex);
#endif
	return -1;
}

void cell_management_set_data_authourize(uint8_t index) {
#if(ENABLE_MUTEX == 1)
	mutex_lock(cell_mutex);
#endif
	cell_management_data[index].is_authourized = true;

#if(ENABLE_MUTEX == 1)
	mutex_unlock(cell_mutex);
#endif
}

int cell_management_data_delete(uint8_t index) {
#if(ENABLE_MUTEX == 1)
	mutex_lock(cell_mutex);
#endif
	if (cell_management_data_count <= 0)
		return -1;

	memset(&cell_management_data[index], 0, sizeof(CellManagement_data_t));

	cell_management_data[index].is_empty = true;
	cell_management_data[index].is_authourized = false;
	cell_management_data[index].connection_retry_cnt = 3;
	cell_management_data[index].life_time = 0;
	cell_management_data[index].conn_handle = PAAR_BLE_CONN_HANDLE_INVALID;
	cell_management_data[index].connecting_state = CONNECTING_STATE_DISCONNECTED;

	cell_management_data_count--;
#if(ENABLE_MUTEX == 1)
	mutex_unlock(cell_mutex);
#endif
	return 0;
}

uint8_t cell_management_check_connection(uint16_t conn_handle) {
#if(ENABLE_MUTEX == 1)
	mutex_lock(cell_mutex);
#endif
	if (current_connecting_index != 0xFF) {
		cell_management_data[current_connecting_index].connecting_state = CONNECTING_STATE_CONNECTED;
		cell_management_data[current_connecting_index].conn_handle = conn_handle;
		cell_management_data[current_connecting_index].heart_beat_cnt = CELL_MANAGEMENT_HB_COUNT;

		current_connecting_index = 0xFF;
#if(ENABLE_MUTEX == 1)
		mutex_unlock(cell_mutex);
#endif
		return 0;
	} else {
		task_sleep(500);

		PAAR_ble_gap_disconnect(conn_handle);
	}
#if(ENABLE_MUTEX == 1)
	mutex_unlock(cell_mutex);
#endif
	return 0xFF;
}

int get_cell_management_profile_data_by_index(uint8_t index, uuidhandle *handle) {

	if (index >= CELL_MANAGEMENT_DATA_MAX_COUNT)
		return -1;

	if (cell_management_data[index].is_empty == false) {
		memcpy(handle, &(cell_management_data[index].profile_data), sizeof(uuidhandle));
		return 0;
	} else {
		return -1;
	}

}

int cell_management_current_cccd_handle(uuidhandle *handle) {
	if (current_connecting_index != 0xFF) {
		memcpy(handle, &(cell_management_data[current_connecting_index].profile_data), sizeof(uuidhandle));
		return 0;
	}

	return -1;
}

uint8_t test_connecting_index = 0;

static int processing_cell_index(uint8_t index) {
	if (cell_management_data[index].is_empty == true)
		return 0;

	//check Location authorization
	if (cell_management_data[index].is_authourized == false) {
		if (cell_management_data[index].conn_delay > 0) {
			cell_management_data[index].conn_delay--;
			return 0;
		}

//		FE_send_msg_location_request((uint8_t*) &(cell_management_data[index].paarID) );

		cell_management_data[index].life_time--;

		if (cell_management_data[index].life_time <= 0)
			cell_management_data_delete(index);

		return 0;
	}

	if (cell_management_data[index].connecting_state == CONNECTING_STATE_CONNECTED) {
//		send_FE_adv_report_dummy((uint8_t*) &(cell_management_data[index].paarID));

		uint32_t r;

		cell_management_data[index].heart_beat_cnt--;

		if (cell_management_data[index].heart_beat_cnt <= 0) {
			uint8_t FE_HB_packet[CELL_MANAGEMENT_HB_PACKET_SIZE];

			memset(FE_HB_packet, 0, CELL_MANAGEMENT_HB_PACKET_SIZE);

#if(FE_SW_CONFIG_HB_ENABLE == 1)
			cell_management_data[index].heart_beat_cnt = CELL_MANAGEMENT_HB_COUNT;
			r = PAAR_send_ble_test_msg_central(cell_management_data[index].conn_handle,
					cell_management_data[index].profile_data.rx_handle, FE_HB_packet, CELL_MANAGEMENT_HB_PACKET_SIZE);
			task_sleep(50);
			if(r != NRF_SUCCESS && r != NRF_ERROR_BUSY)
			{
				PAAR_ble_gap_disconnect(cell_management_data[index].conn_handle);
				cell_management_data_delete(index);
			}
#endif
		}

		return 0;
	}

	if (cell_management_data[index].connecting_state == CONNECTING_STATE_DISCONNECTED) {
		if (current_connecting_index != 0xFF)
			return 0;

		ble_gap_addr_t *target_address = (ble_gap_addr_t*) malloc(sizeof(ble_gap_addr_t));

		memcpy(target_address, &(cell_management_data[index].gap_addr), sizeof(ble_gap_addr_t));

		cell_management_data[index].connecting_state = CONNECTING_STATE_CONNECTING;

		cell_management_data[index].life_time = CELL_MANAGEMENT_LIFE_TIME_CONNECTING;

		current_connecting_index = index;

		BLE_process_event_send(BLE_CENTRAL_EVT, BLE_CENTRAL_CMD_CONNECT,
		PAAR_BLE_CONN_HANDLE_INVALID, 0, sizeof(target_address), (uint8_t*) target_address);

		task_sleep(1500);

		return 0xFF;
	}

	if (cell_management_data[index].connecting_state == CONNECTING_STATE_CONNECTING) {
		cell_management_data[index].life_time--;

		if (cell_management_data[index].life_time <= 0) {
			cell_management_data_delete(index);
			current_connecting_index = 0xFF;
		}

		return 0;
	}

	return 0;

}

void processing_cell_management() {
	if (cell_management_data_count == 0)
		return;

	uint8_t i, result;
	for (i = 0; i < CELL_MANAGEMENT_DATA_MAX_COUNT; i++) {
		result = processing_cell_index(i);
		if (result == 0xFF)
			break;
	}
}

void cell_management_connect_timeout() {
//	if(current_connecting_index < CELL_MANAGEMENT_DATA_MAX_COUNT)
//	{
//		cell_management_data_delete(current_connecting_index);
//		current_connecting_index = 0xFF;
//	}

//cell_management_data[current_connecting_index].connecting_state = FE_CONNECTING_STATE_DISCONNECTED;
}

