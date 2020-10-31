/*
 * LAP_api.h
 *
 *  Created on: 2020. 06. 11.
 *      Author: YJPark
 */

#ifndef APPLICATION_SPB_EXE_SPB_SRC_LAP_API_H_
#define APPLICATION_SPB_EXE_SPB_SRC_LAP_API_H_

#include <stdint.h>
#include "ble_gap.h"
#include "ble.h"
#include "ble_stack.h"
#include "ble_process.h"
#include "sw_config.h"

#define PAAR_ID_SIZE	4

/* status byte in Advertising packet payloads */
#define LIDx_STATUS								0x00
#define PINP_STATUS								0x01
#define REQ_CONN_SOSP_STATUS					0x02
#define REQ_CONN_SMARTDEVICE_STATUS				0x04
#define REQ_LOCAION_ADV							0x08
#define SLIMHUB_INFO_ADV						0x10
#define REQ_CONN_SOSP_WO_MAC					0x40

/**
 * see @ref LAP_ADV_IDX_STATUS
 */
#define LAP_ADV_STATUS_BYTE_LIDX				0x00
#define LAP_ADV_STATUS_BYTE_PNIP				0x01
#define LAP_ADV_STATUS_BYTE_AMD					0x02

#define LAP_ADV_STATUS_BYTE_EC					0x03
#define LAP_ADV_STATUS_BYTE_INOUT				0x04

#define LAP_ADV_STATUS_BYTE_LOCATION_REQ		0x08
#define LAP_ADV_STATUS_BYTE_SLIMHUB_INFO		0x10
#define LAP_ADV_STATUS_BYTE_CONN_SOSP_WO_MAC	0x40

#define DEFAULT_LAP_ADV_LF_RSSI_VALUE			0x00

/**
 * LAP Advertising Packet Index N
 */
#define LAP_ADV_IDX_EAH0_LENGTH				0
#define LAP_ADV_IDX_EAH1_LENGTH				3
#define LAP_ADV_IDX_STATUS					7
#define LAP_ADV_IDX_PAAR_DEVICE_ID0			8
#define LAP_ADV_IDX_PAAR_DEVICE_ID1			9
#define LAP_ADV_IDX_PAAR_DEVICE_ID2			10
#define LAP_ADV_IDX_PAAR_DEVICE_ID3			11
#define LAP_ADV_IDX_LF_RSSI					12

#if 0
#define LAP_ADV_IDX_ANCHOR_NODE_MAC0		13
#define LAP_ADV_IDX_ANCHOR_NODE_MAC1		14
#define LAP_ADV_IDX_ANCHOR_NODE_MAC2		15
#define LAP_ADV_IDX_ANCHOR_NODE_MAC3		16
#define LAP_ADV_IDX_ANCHOR_NODE_MAC4		17
#define LAP_ADV_IDX_ANCHOR_NODE_MAC5		18
#else
#define LAP_ADV_IDX_LF_SERVICE_ID			13
#define LAP_ADV_IDX_LF_SERVICE_DATA			14
#endif

#define LAP_ADV_IDX_NOTIFICATION_HANDLE		19
#define LAP_ADV_IDX_WRITE_REQUEST_HANDLE	21
#define LAP_ADV_IDX_NOTI_EN_HANDLE			23

#define LAP_DEVICE_TYPE_PAAR_BAND			0x58
/**
 * LAP Message Packet Index N
 */
#define MSG_IDX_PACKET_TYPE			0
#define MSG_IDX_SERVICE_ID			1
#define MSG_IDX_SEQ_NUM				2
#define MSG_IDX_DATA_LEN			3
#define MSG_IDX_CMD					4
#define MSG_IDX_DATA				5

#define DEFAULT_MAX_SCAN_RES				10

#define LAP_ADV_DATA_LEN	25
#define LAP_SCAN_RSP_DATA_LEN	18

#define DEVICE_ID_SOSP_ROUTER		   	      	0x04
#define DEVICE_ID_BITAG                   		0x40
#define DEVICE_ID_PARRWATCH               		0x44
#define DEVICE_ID_IPAD                    		0x4C
#define DEVICE_ID_PAAR_BAND						0x58
#define DEVICE_ID_PAAR_TAG						0x5C
#define DEVICE_ID_TAPWATER_TAG					0x60
#define DEVICE_ID_GAS_TAG						0x64
#define DEVICE_ID_PRINTER                   	0x84
#define DEVICE_ID_DOORLOCK                  	0x88
#define DEVICE_ID_DOORLOG	                  	0x8C
#define DEVICE_ID_TREADMILL                		0x90
#define DEVICE_ID_BIKE                     		0x94
#define DEVICE_ID_STEPPER                   	0x98
#define DEVICE_ID_SENIOR                    	0x9C
#define DEVICE_ID_WEIGHT                    	0xA0
#define DEVICE_ID_COFFEEPORT                	0xB8
#define DEVICE_ID_GASSTOVE                  	0xBC
#define DEVICE_ID_SPHYGMOMETER              	0xC0
#define DEVICE_ID_BLOODGLUCOSE              	0xC4
#define DEVICE_ID_SCALE                     	0xC8
#define DEVICE_ID_PILLBOX                   	0xCC
#define DEVICE_ID_BODYFAT                   	0xD0
#define DEVICE_ID_HOME_APP_TAG					0xD4
#define DEVICE_ID_ENVIRONMENT_TAG				0xD8

typedef struct {
	uint16_t tx_handle;
	uint16_t rx_handle;
	uint16_t cccd_handle;
} uuidhandle;

typedef struct {

	ble_gap_addr_t 	gap_addr;
	uint32_t 		paarID;		// peer device 's device id (4 byte, copyright CSOS)

	uint8_t 		status_byte;
	uint8_t 		LF_dist;
	uint8_t 		RF_rssi;

	uuidhandle 		profile_data;
	bool 			scan_check;
} scanDevRev_t;

typedef struct {
	uint16_t 		connHandle;
	ble_gap_addr_t 	peer_addr;

	uuidhandle 		peer_profile;
	bool			is_connected;

	uint32_t 		peer_paarID;
	uint8_t			device_type;
	uint8_t 		service_type;
} conn_info_t;


uint8_t get_scanDevCnt(void);
void set_scanDevCnt(uint8_t val);

scanDevRev_t* get_scanDevList(void) ;
void clear_scanDevList(void);

conn_info_t * get_connInfo(void);
void clear_connInfo(void);

void add_List_LAP_advdata(ble_gap_evt_adv_report_t* pkt);

conn_info_t* get_LAP_connInfoList(void);

//Start BLE advertising : LIDx
void LAP_start_ble_adv_LIDx();

//Stop BLE advertising : LIDx
void LAP_stop_ble_adv_LIDx();

//Start BLE Scan
void LAP_start_ble_scan(uint8_t* target_paar_id);
//Start BLE connection(Central Role Mode)
void LAP_start_ble_connect(ble_gap_evt_adv_report_t* adv_data);
//Start BLE disconnection(Cenral Role Mode)
void LAP_start_ble_disconnect(uint16_t conn_handle);
//Save UUID handle from ble advertising data(PAAR)
void LAP_save_uuid_handle(ble_gap_evt_adv_report_t* pPkt, paar_uuidhandle* uuid_handle);
//Send ble msg : Central Connection
void LAP_send_ble_msg_central(uint16_t conn_handle, uint16_t handle, uint8_t* msg, uint8_t msg_len);

//Checking LAP ADV Packet
bool is_LAP_adv_packet(LAP_ble_adv_report* pPkt);

//Checking LAP ADV Packet : Location Request
int process_LAP_location_request_packet(LAP_ble_adv_report* pPkt);

//send ble peripheral message : PAAR profile
void LAP_send_ble_msg_peripheral(uint8_t* msg, uint8_t msg_len);
#endif /* APPLICATION_SPB_EXE_SPB_SRC_LAP_API_H_ */
