/*
 * LAP_main.h
 *
 *  Created on: 2020. 06. 11.
 *      Author: YJPark
 */

#ifndef APPLICATION_GASTAG_EXE_GASTAG_SRC_LAP_MAIN_H_
#define APPLICATION_GASTAG_EXE_GASTAG_SRC_LAP_MAIN_H_

#include <stdint.h>

typedef struct
{
    uint8_t event;
    uint8_t status;
    uint16_t conn_handle;
    uint16_t handle;
    uint32_t msg_len;
    uint8_t *msg;
} LAPEvt_msgt;

#define PAAR_ACK_PACKET_SIZE 4

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t cmd;
    uint8_t data[14];
} paar_data_t;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t service_ID;
    uint8_t seq;		//4bit:4bit - seq_no:total
    uint8_t data_len;
    uint8_t data[15];
} paar_packet_t;

enum
{
    LAP_CENTRAL_EVT = 0,
    LAP_PERIPHERAL_EVT,
    LAP_LIDx_EVT,
    LAP_PNIP_EVT,
    LAP_AMD_EVT,
};

enum
{
    LAP_CENTRAL_ST_SCAN_RESULT = 0,
    LAP_CENTRAL_ST_SCAN_TIMEOUT,
    LAP_CENTRAL_ST_CONN_TIMEOUT,
    LAP_CENTRAL_ST_SCAN_ADV_REPORT,
    LAP_CENTRAL_ST_CONNECTED,
    LAP_CENTRAL_ST_DISCONNECTED,
    LAP_CENTRAL_ST_DATA_RECEIVED,
};

enum
{
    LAP_PERIPHERAL_ST_CONNECTED = 0,
    LAP_PERIPHERAL_ST_DISCONNECTED,
    LAP_PERIPHERAL_ST_DATA_RECEIVED,
    LAP_PERIPHERAL_ST_CCCD_ENABLED,
    LAP_PERIPHERAL_ST_ADV_RECEIVED,
};

enum
{
    LAP_EVT_LIDX = 0,
    LAP_EVT_PNIP,
    LAP_EVT_AMD,

    EC_EVT_CENTRAL,
    EC_EVT_PERIPHERAL,
};

enum
{
    EC_STATUS_ADV_REPORT,
    EC_STATUS_CONNECTED,
    EC_STATUS_DISCONNECTED,

    EC_STATUS_DATA_RECEIVE,
    EC_STATUS_ENTRANCE_INFO,

};

#define EDGE_SERVICE_ID 0X60

enum
{
    PAAR_EDGE_CMD_SCAN_START = 1,
    PAAR_EDGE_CMD_EDGE_CONN_REPORT,
    PAAR_EDGE_CMD_DEVICE_CONN_REPORT,
    PAAR_EDGE_CMD_ACTIVITY_REPORT,
};

#define LAP_EVENT_HANDLE_NULL				0
#define LAP_EVENT_MSG_LEN_NULL				0

void LAP_main_task_init(void);

int LAP_event_send(uint8_t evt, uint8_t state, uint16_t conn_handle, uint16_t handle, uint32_t msg_len, uint8_t *msg);

bool get_test_ble_connected();

void send_packet_central(uint16_t conn_handle, uint16_t handle, paar_packet_t *packet);
void send_ack_central(uint16_t conn_handle, uint16_t handle, paar_packet_t *packet);

void send_packet_peripheral(paar_packet_t *packet);
void send_ack_peripheral(paar_packet_t *packet);

#endif /* APPLICATION_GASTAG_EXE_GASTAG_SRC_LAP_MAIN_H_ */
