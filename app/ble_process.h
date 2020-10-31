/*
 * ble_process.h
 *
 *  Created on: 2020. 06. 11.
 *      Author: YuJin Park
 */

#ifndef APPLICATION_PAAR_FRONTEND_EXE_PAAR_FRONTEND_SRC_BLE_PROCESS_H_
#define APPLICATION_PAAR_FRONTEND_EXE_PAAR_FRONTEND_SRC_BLE_PROCESS_H_

#include <stdio.h>
#include <stdint.h>

#include "ble_gap.h"
#include "ble_stack.h"

typedef struct
{
	uint8_t event;
	uint8_t status;
	uint16_t conn_handle;
	uint16_t handle;
	uint32_t msg_len;
	uint8_t* msg;
}BLEPEvt_msgt;

typedef struct
{
	uint8_t PAAR_id[4];
	uint16_t conn_handle;
	uint16_t tx_handler;
	uint16_t rx_handler;
	uint16_t cccd_handler;
}BLE_Ctr_Connection_info;

typedef struct {
	uint16_t tx_handle;
	uint16_t rx_handle;
	uint16_t cccd_handle;
} paar_uuidhandle;

#define MAXIMUM_NUM_CENTRAL_CONNECTION	5

enum
{
	BLE_CENTRAL_EVT = 0,
	BLE_PERIPHERAL_EVT,
};

enum{
	BLE_CENTRAL_ST_SCAN_TIMEOUT = 0,
	BLE_CENTRAL_ST_CONNECTED,
	BLE_CENTRAL_ST_CONNECTION_TIMEOUT,
	BLE_CENTRAL_ST_DISCONNECTED,
	BLE_CENTRAL_ST_DATA_RECEIVED,

	BLE_CENTRAL_CMD_SCAN_START,
	BLE_CENTRAL_CMD_CONNECT,
	BLE_CENTRAL_CMD_DISCONNECT,
	BLE_CENTRAL_CMD_SEND_MSG,
};

enum
{
	BLE_PERIPHERAL_ST_CONNECTED = 0,
	BLE_PERIPHERAL_ST_DISCONNECTED,
	BLE_PERIPHERAL_ST_DATA_RECEIVED,
	BLE_PERIPHERAL_ST_CCCD_ENABLED,

	BLE_PERIPHERAL_CMD_ADV_TIMEOUT,
	BLE_PERIPHERAL_CMD_ADV_START,
	BLE_PERIPHERAL_CMD_ADV_STOP,
	BLE_PERIPHERAL_CMD_SEND_MSG,
};

enum
{
	BLE_DICONNECT_ROLE_CENTRAL = 0,
	BLE_DICONNECT_ROLE_PERIPHERAL,
	BLE_DICONNECT_ROLE_ERROR
};

void BLE_process_task_init(void);

void BLE_process_event_send(uint8_t event, uint8_t status, uint16_t conn_handle,
									uint16_t handle, uint32_t msg_len, uint8_t* msg);

void process_ADV_Report(LAP_ble_adv_report* pPkt);

uint32_t BLE_check_disconnect_role();

LAP_ble_adv_report* PAAR_get_ble_scan_target_result();

#endif /* APPLICATION_PAAR_FRONTEND_EXE_PAAR_FRONTEND_SRC_BLE_PROCESS_H_ */
