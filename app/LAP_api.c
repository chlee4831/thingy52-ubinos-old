/*
 * LAP_api.c
 *
 *  Created on: 2020. 06. 11.
 *      Author: YJPark
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>

#include "nrf.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "ble_conn_params.h"

#include "ble_gap.h"
#include "ble_profile.h"

#include "ble_profile.h"
#include "ble_process.h"
#include "ble_stack.h"

#include "LAP_api.h"
#include "LAP_main.h"
#include "sw_config.h"

static scanDevRev_t scanDevList[DEFAULT_MAX_SCAN_RES];
static uint8_t		scanDevCnt = 0;

static conn_info_t 	connInfo;
static conn_info_t	g_connInfoList[CENTRAL_LINK_COUNT];


static uint8_t LAP_adv_data[] = {
		// Extend Advertising message Handle
		0x02, //Length
		0x01, //type
		0x06, //Data Field

		0x15,// EAS field length	(Fixed)
		0xFF,// (Fixed)
		0x59, 0x00,

		0x00,  // status byte ( smartphone_conn_bit | request_connection_bit | service_bit )
		PAAR_ID_0, PAAR_ID_1, PAAR_ID_2, PAAR_ID_3,  // SOSp device ID
		0x00,							  	// LF RSSI value
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // anchor node MAC address

		0x00, 0x00,							// SEND_MSG_UUID_HANDLE(PNIP, APP -> Watch)
		0x00, 0x00,							// INDI_MSG_UUID_HANDLE(Watch -> APP, PNIP)
		0x00, 0x00,							// Notification Enable Handle(CCCD)
};

static uint8_t LAP_scan_rsp_data[] = {
		0x07,							  	// EAH field length
		0x03,
		0x50, 0x26,							// profile UUID
		0x02, 0x7F,							// Rx service UUID
		0x03, 0x7F,							// Tx service UUID

		0x09,								//length of this data
		0x09,
		// ascii code
		'T',								//
		'H',								//
		'I',								//
		'N',								//
		'G',								//
		'Y',								//
		'5',								//
		'2',								//
};

//========================================================================================

//========================================================================================

uint8_t get_scanDevCnt(void)
{
	return scanDevCnt;
}

void set_scanDevCnt(uint8_t val)
{
	scanDevCnt = val;
}

scanDevRev_t* get_scanDevList(void)
{
	return scanDevList;
}

void clear_scanDevList(void)
{
	memset(scanDevList, 0, sizeof(scanDevList));
}

conn_info_t * get_connInfo(void)
{
	return &connInfo;
}

void clear_connInfo(void)
{
	memset(&connInfo, 0, sizeof(connInfo));
	connInfo.connHandle = PAAR_BLE_CONN_HANDLE_INVALID;
}

void add_connInfo()
{

}


static uint8_t search_scandevdata(scanDevRev_t * scan_list, uint32_t scan_paarID){

	uint8_t i;

	for(i=0; i<scanDevCnt; i++){
		if( scan_list[i].paarID == scan_paarID ){
			return i;
		}
	}

	return 0xFF;
}

void add_List_LAP_advdata(ble_gap_evt_adv_report_t* pPkt)
{

	uint8_t			idx;
	scanDevRev_t	scan_data;

	memset(&scan_data, 0, sizeof(scanDevRev_t));

	scan_data.gap_addr		= pPkt->peer_addr;
	scan_data.status_byte	= pPkt->data.p_data[LAP_ADV_IDX_STATUS];
	scan_data.LF_dist 		= pPkt->data.p_data[LAP_ADV_IDX_LF_RSSI];
	scan_data.RF_rssi 		= pPkt->rssi;

	memcpy(&scan_data.paarID, &(pPkt->data.p_data[LAP_ADV_IDX_PAAR_DEVICE_ID0]), sizeof(PAAR_ID_SIZE));

	scan_data.profile_data.tx_handle = (pPkt->data.p_data[LAP_ADV_IDX_NOTIFICATION_HANDLE + 1] << 8) |
										(pPkt->data.p_data[LAP_ADV_IDX_NOTIFICATION_HANDLE]);

	scan_data.profile_data.rx_handle = (pPkt->data.p_data[LAP_ADV_IDX_WRITE_REQUEST_HANDLE + 1] << 8) |
										(pPkt->data.p_data[LAP_ADV_IDX_WRITE_REQUEST_HANDLE]);

	if( pPkt->data.p_data[LAP_ADV_IDX_EAH1_LENGTH] > 19) {
		scan_data.profile_data.cccd_handle = (pPkt->data.p_data[LAP_ADV_IDX_NOTI_EN_HANDLE + 1] << 8) |
											(pPkt->data.p_data[LAP_ADV_IDX_NOTI_EN_HANDLE]);
	}

	idx = search_scandevdata(scanDevList, scan_data.paarID);
	if( idx != 0xFF ){
		if( scanDevList[idx].LF_dist > scan_data.LF_dist ){
			scanDevList[idx].LF_dist = scan_data.LF_dist;
		}
	}else{
		if(scanDevCnt < DEFAULT_MAX_SCAN_RES)
		{
			scanDevList[scanDevCnt] = scan_data;
			scanDevCnt++;
		}
	}
}

bool is_LAP_adv_packet(LAP_ble_adv_report* pPkt)
{

	//Check LAP Advertising Data Length
	if(pPkt->data_len < LAP_ADV_DATA_LEN)
		return false;
	if(pPkt->data[LAP_ADV_IDX_EAH0_LENGTH] != 0x02)
		return false;
	if(pPkt->data[LAP_ADV_IDX_EAH1_LENGTH] != 0x15)
		return false;

	//check PAAR ID : Device Type

	return true;
}

int process_LAP_location_request_packet(LAP_ble_adv_report* pPkt)
{
	return 0;
}

bool add_LAP_location_request()
{
	return false;
}

conn_info_t* get_LAP_connInfoList(void)
{
	return g_connInfoList;
}

void LAP_start_ble_adv_LIDx()
{
	//Stop BLE ADV
	//BLE_process_event_send(BLE_PERIPHERAL_EVT, BLE_PERIPHERAL_CMD_ADV_STOP, PAAR_BLE_CONN_HANDLE_INVALID, 0, 0, NULL);

	ble_paar_t * LAP_ble_peripheral_info;

	//Get BLE Peripheral Information : rx/tx/cccd handle
	LAP_ble_peripheral_info = get_peripheral_info();

	//set LAP ADV Data : LIDx Status byte
	//LAP_adv_data[LAP_ADV_IDX_STATUS] = LAP_ADV_STATUS_BYTE_LIDX;
	LAP_adv_data[LAP_ADV_IDX_STATUS] = LAP_ADV_STATUS_BYTE_LOCATION_REQ;

	//set LAP ADV Data : PAAR ID
	LAP_adv_data[LAP_ADV_IDX_PAAR_DEVICE_ID0] = PAAR_ID_0;
	LAP_adv_data[LAP_ADV_IDX_PAAR_DEVICE_ID1] = PAAR_ID_1;
	LAP_adv_data[LAP_ADV_IDX_PAAR_DEVICE_ID2] = PAAR_ID_2;
	LAP_adv_data[LAP_ADV_IDX_PAAR_DEVICE_ID3] = PAAR_ID_3;

	//set LAP ADV DAta : LF RSSI Value
	LAP_adv_data[LAP_ADV_IDX_LF_RSSI] = DEFAULT_LAP_ADV_LF_RSSI_VALUE;

	//set LAP ADV Data : rx characteristic value handle
	memcpy(&LAP_adv_data[LAP_ADV_IDX_NOTIFICATION_HANDLE], &(LAP_ble_peripheral_info->tx_handles.value_handle), sizeof(uint16_t));

	//set LAP ADV Data : tx characteristic value handle
	memcpy(&LAP_adv_data[LAP_ADV_IDX_WRITE_REQUEST_HANDLE], &(LAP_ble_peripheral_info->rx_handles.value_handle), sizeof(uint16_t));

	//set LAP ADV Data : cccd characteristic value handle
	memcpy(&LAP_adv_data[LAP_ADV_IDX_NOTI_EN_HANDLE], &(LAP_ble_peripheral_info->tx_handles.cccd_handle), sizeof(uint16_t));

	//BLE adv data setup
	PAAR_set_adv_data(LAP_adv_data, LAP_ADV_DATA_LEN, LAP_scan_rsp_data, LAP_SCAN_RSP_DATA_LEN);

	//Start BLE ADV
	BLE_process_event_send(BLE_PERIPHERAL_EVT, BLE_PERIPHERAL_CMD_ADV_START, PAAR_BLE_CONN_HANDLE_INVALID, 0, 0, NULL);
}

void LAP_stop_ble_adv_LIDx()
{
	//Stop BLE ADV
	BLE_process_event_send(BLE_PERIPHERAL_EVT, BLE_PERIPHERAL_CMD_ADV_STOP, PAAR_BLE_CONN_HANDLE_INVALID, 0, 0, NULL);
}

void LAP_start_ble_scan(uint8_t* target_paar_id)
{
	BLE_process_event_send(BLE_CENTRAL_EVT, BLE_CENTRAL_CMD_SCAN_START, PAAR_BLE_CONN_HANDLE_INVALID, 0, sizeof(target_paar_id), target_paar_id);
}

void LAP_start_ble_connect(ble_gap_evt_adv_report_t* adv_data)
{
	ble_gap_addr_t* target_address;
	target_address = (ble_gap_addr_t*) malloc(sizeof(ble_gap_addr_t));

	memcpy(target_address, &(adv_data->peer_addr), sizeof(ble_gap_addr_t));

	BLE_process_event_send(BLE_CENTRAL_EVT, BLE_CENTRAL_CMD_CONNECT,
			PAAR_BLE_CONN_HANDLE_INVALID, 0, sizeof(target_address), (uint8_t*) target_address);
}

void LAP_start_ble_disconnect(uint16_t conn_handle)
{
	BLE_process_event_send(BLE_CENTRAL_EVT, BLE_CENTRAL_CMD_DISCONNECT,	conn_handle, 0, 0, NULL);
}

void LAP_save_uuid_handle(ble_gap_evt_adv_report_t* pPkt, paar_uuidhandle* uuid_handle)
{
	memcpy(&(uuid_handle->tx_handle), &(pPkt->data.p_data[LAP_ADV_IDX_NOTIFICATION_HANDLE]), 2);
	memcpy(&(uuid_handle->rx_handle), &(pPkt->data.p_data[LAP_ADV_IDX_WRITE_REQUEST_HANDLE]), 2);
	memcpy(&(uuid_handle->cccd_handle), &(pPkt->data.p_data[LAP_ADV_IDX_NOTI_EN_HANDLE]), 2);
}

void LAP_send_ble_msg_central(uint16_t conn_handle, uint16_t handle, uint8_t* msg, uint8_t msg_len)
{
	BLE_process_event_send(BLE_CENTRAL_EVT, BLE_CENTRAL_CMD_SEND_MSG,conn_handle, handle, msg_len, msg);
}

void LAP_send_ble_msg_peripheral(uint8_t* msg, uint8_t msg_len)
{
	BLE_process_event_send(BLE_PERIPHERAL_EVT, BLE_PERIPHERAL_CMD_SEND_MSG,
			PAAR_BLE_CONN_HANDLE_INVALID, 0, msg_len, msg);
}


