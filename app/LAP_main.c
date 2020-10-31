/*
 * LAP_main.c
 *
 *  Created on: 2020. 06. 11.
 *      Author: YJPark
 */

#include <ubinos.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <malloc.h>

#include "ble_stack.h"
#include "LAP_api.h"
#include "LAP_main.h"
#include "ble_gap.h"
#include "ble_process.h"
#include "ble_profile.h"

#include "sw_config.h"
#include "cell_management.h"

uint16_t test_target_conn_handle = BLE_CONN_HANDLE_INVALID;
paar_uuidhandle test_target_uuid_handle;
uint8_t test_send_count = 0;

static msgq_pt LAP_msgq;

bool test_ble_connected = false;

//APP_TIMER_DEF(scan_fail_timeout_timer);

bool get_test_ble_connected()
{
    return test_ble_connected;
}

static void process_LAP_scan_timeout()
{
    processing_cell_management();
    printf("Cell Management\r\n");

//	LAP_start_ble_scan(NULL);

//app_timer_stop(scan_fail_timeout_timer);
//app_timer_start(scan_fail_timeout_timer, APP_TIMER_TICKS(5000), NULL);
}

static void processing_LAP_Central_Conn_timeout(LAPEvt_msgt LAP_evt_msg)
{
    cell_management_connect_timeout();

    LAP_start_ble_scan(NULL);
}

static void processing_LAP_Central_Scan_timeout(LAPEvt_msgt LAP_evt_msg)
{
    process_LAP_scan_timeout();
}

static void processing_LAP_Central_Scan_result(LAPEvt_msgt LAP_evt_msg)
{
    //nothing...
}

static void send_packet_central(uint16_t conn_handle, uint16_t handle, paar_packet_t *packet)
{
    uint8_t *temp_packet = (uint8_t*) malloc(PAAR_MAXIMUM_PACKET_SIZE);

    if (temp_packet != NULL)
    {
        memset(temp_packet, 0, PAAR_MAXIMUM_PACKET_SIZE);
        memcpy(temp_packet, packet, PAAR_MAXIMUM_PACKET_SIZE);
        LAP_send_ble_msg_central(conn_handle, handle, temp_packet, PAAR_MAXIMUM_PACKET_SIZE);
    }
}

static void send_ack_central(uint16_t conn_handle, uint16_t handle, paar_packet_t *packet)
{
    uint8_t *temp_packet = (uint8_t*) malloc(PAAR_MAXIMUM_PACKET_SIZE);

    if (temp_packet != NULL)
    {
        memset(temp_packet, 0, PAAR_MAXIMUM_PACKET_SIZE);
        memcpy(temp_packet, packet, PAAR_ACK_PACKET_SIZE);
        LAP_send_ble_msg_central(conn_handle, handle, temp_packet, PAAR_MAXIMUM_PACKET_SIZE);
    }
}

static void send_packet_peripheral(paar_packet_t *packet)
{
    uint8_t *temp_packet = (uint8_t*) malloc(PAAR_MAXIMUM_PACKET_SIZE);

    if (temp_packet != NULL)
    {
        memset(temp_packet, 0, PAAR_MAXIMUM_PACKET_SIZE);
        memcpy(temp_packet, packet, PAAR_MAXIMUM_PACKET_SIZE);
        LAP_send_ble_msg_peripheral(temp_packet, PAAR_MAXIMUM_PACKET_SIZE);
    }
}

static void send_ack_peripheral(paar_packet_t *packet)
{
    uint8_t *temp_packet = (uint8_t*) malloc(PAAR_MAXIMUM_PACKET_SIZE);

    if (temp_packet != NULL)
    {
        memset(temp_packet, 0, PAAR_MAXIMUM_PACKET_SIZE);
        memcpy(temp_packet, packet, PAAR_ACK_PACKET_SIZE);
        LAP_send_ble_msg_peripheral(temp_packet, PAAR_MAXIMUM_PACKET_SIZE);
    }
}

static void send_cccd_handle_enable(uint16_t conn_handle, uint16_t cccd_handle)
{
    uint8_t *temp_packet;

    temp_packet = (uint8_t*) malloc(2);

    memset(temp_packet, 0, 2);

    temp_packet[0] = NRF_NOTI_INDI_ENABLE;		// ble notification msg 데이터
    temp_packet[1] = 0x00;

    printf("BLE send msg : CCCD enable\r\n");

    LAP_send_ble_msg_central(conn_handle, cccd_handle, temp_packet, 2);

}

static void processing_LAP_Central_Connected(LAPEvt_msgt LAP_evt_msg)
{
    printf("BLE Central connect\r\n");

    uuidhandle temp_uuid_handle;

    int r;
    r = cell_management_current_cccd_handle(&temp_uuid_handle);
    if (r == -1)
    {
        return;
    }

    //save test_connection_handle
    test_target_conn_handle = LAP_evt_msg.conn_handle;

    task_sleep(TEST_SEND_CCCD_MSG_DELAY);

    //send cccd enable
    send_cccd_handle_enable(test_target_conn_handle, temp_uuid_handle.cccd_handle);

    task_sleep(TEST_SEND_CCCD_MSG_DELAY);

    //	//send test msg
    //	send_test_msg_central(test_target_conn_handle, test_target_uuid_handle.rx_handle);

    cell_management_check_connection(LAP_evt_msg.conn_handle);

    task_sleep(200);

    LAP_start_ble_scan(NULL);
}

static void processing_LAP_Central_Disconnected(LAPEvt_msgt LAP_evt_msg)
{
    uint8_t index;
    index = cell_management_search_data_index_by_connhandle(LAP_evt_msg.conn_handle);
    if (index != 0xFF)
    {
        cell_management_data_delete(index);
    }

    printf("BLE Central disconnect\r\n");
}

static void processing_LAP_Central_Data_Received(LAPEvt_msgt LAP_evt_msg)
{
    //todo 하단 노드에서 연결한 디바이스 정보 받아서 라우팅 테이블 생성
    //todo 데이터 받아서 상단 노드로 전달
    printf("data received!!\r\n");
    printf("data packet : ");
    uint8_t i;

    printf("0x");
    for (i = 0; i < LAP_evt_msg.msg_len; i++)
        printf("%02X ", LAP_evt_msg.msg[i]);

    printf("\r\n");
    printf("\r\n");

}

static void processing_LAP_Peripheral_Connected(LAPEvt_msgt LAP_evt_msg)
{
    printf("BLE Peripheral connect\r\n");
}

static void processing_LAP_Peripheral_Disconnected(LAPEvt_msgt LAP_evt_msg)
{

    task_sleep(TEST_ADV_START_DELAY);
    printf("BLE Peripheral disconnect\r\n");

    printf("BLE ADV start\r\n");
    LAP_start_ble_adv_LIDx();
}

static void processing_LAP_Peripheral_Data_Received(LAPEvt_msgt LAP_evt_msg)
{
    //todo Scan 명령 받아서 처리, ACK 송신
    paar_packet_t *packet = (paar_packet_t*) LAP_evt_msg.msg;
    paar_data_t *packet_data = (paar_data_t*) packet->data;

    int i;
    uint8_t *packet_arr = (uint8_t*) packet;
    printf("Received Packet: 0x");
    for (i = 0; i < PAAR_MAXIMUM_PACKET_SIZE; i++)
    {
        if (packet_arr[i] != NULL)
        {
            printf("%02X ", packet_arr[i]);
        }
    }
//	printf("\r\n");
    printf("\r\n");	//print packet

    send_ack_peripheral(packet);

    if (packet_data->cmd == PAAR_EDGE_CMD_SCAN_START)
    {
        packet_data->data[0] -= 1;
        if (packet_data->data[0] == 0)
        {

            task_sleepms(100);

            LAP_start_ble_scan(NULL);
        }
    }
}

static void processing_LAP_Peripheral_CCCD_Enabled(LAPEvt_msgt LAP_evt_msg)
{
    printf("BLE CCCD is enabled. \r\n");

//	task_sleep(TEST_SEND_MSG_DELAY);
//	send_test_msg_peripheral();
}

static void processing_LAP_Peripheral_Adv_Received(LAPEvt_msgt LAP_evt_msg)
{

}

static void processing_LAP_Central_event(LAPEvt_msgt LAP_evt_msg)
{
    switch (LAP_evt_msg.status)
    {
    case LAP_CENTRAL_ST_SCAN_TIMEOUT:
        processing_LAP_Central_Scan_timeout(LAP_evt_msg);
        break;
    case LAP_CENTRAL_ST_CONN_TIMEOUT:
        processing_LAP_Central_Conn_timeout(LAP_evt_msg);
        break;
    case LAP_CENTRAL_ST_SCAN_RESULT:
        processing_LAP_Central_Scan_result(LAP_evt_msg);
        break;
    case LAP_CENTRAL_ST_CONNECTED:
        processing_LAP_Central_Connected(LAP_evt_msg);
        break;
    case LAP_CENTRAL_ST_DISCONNECTED:
        processing_LAP_Central_Disconnected(LAP_evt_msg);
        break;
    case LAP_CENTRAL_ST_DATA_RECEIVED:
        processing_LAP_Central_Data_Received(LAP_evt_msg);
        break;
    }
}

static void processing_LAP_Peripheral_event(LAPEvt_msgt LAP_evt_msg)
{
    switch (LAP_evt_msg.status)
    {
    case LAP_PERIPHERAL_ST_CONNECTED:
        processing_LAP_Peripheral_Connected(LAP_evt_msg);
        break;
    case LAP_PERIPHERAL_ST_DISCONNECTED:
        processing_LAP_Peripheral_Disconnected(LAP_evt_msg);
        break;
    case LAP_PERIPHERAL_ST_DATA_RECEIVED:
        processing_LAP_Peripheral_Data_Received(LAP_evt_msg);
        break;
    case LAP_PERIPHERAL_ST_CCCD_ENABLED:
        processing_LAP_Peripheral_CCCD_Enabled(LAP_evt_msg);
        break;
    case LAP_PERIPHERAL_ST_ADV_RECEIVED:
        processing_LAP_Peripheral_Adv_Received(LAP_evt_msg);
        break;
    }
}

void processing_LAP_LIDx_event(LAPEvt_msgt LAP_evt_msg)
{

}

void processing_LAP_PNIP_event(LAPEvt_msgt LAP_evt_msg)
{

}

void processing_LAP_AMD_event(LAPEvt_msgt LAP_evt_msg)
{

}

void LAP_Protocol_start_operation()
{
    LAP_start_ble_adv_LIDx();
}

void scan_fail_timer_handler()
{
    LAP_start_ble_scan(NULL);
}

void LAP_main_task(void *arg)
{
    int r;
    LAPEvt_msgt LAP_evt_msg;

    ble_stack_init_wait();

    LAP_Protocol_start_operation();

    cell_management_data_init();

    for (;;)
    {
        r = msgq_receive(LAP_msgq, (unsigned char*) &LAP_evt_msg);
        if (0 != r)
        {
            logme("fail at msgq_receive\r\n");
        }
        else
        {
            switch (LAP_evt_msg.event)
            {
            case LAP_CENTRAL_EVT:
                processing_LAP_Central_event(LAP_evt_msg);
                break;
            case LAP_PERIPHERAL_EVT:
                processing_LAP_Peripheral_event(LAP_evt_msg);
                break;

            case LAP_LIDx_EVT:
                processing_LAP_LIDx_event(LAP_evt_msg);
                break;
            case LAP_PNIP_EVT:
                processing_LAP_PNIP_event(LAP_evt_msg);
                break;
            case LAP_AMD_EVT:
                processing_LAP_AMD_event(LAP_evt_msg);
                break;
            }

            if (LAP_evt_msg.msg != NULL)
            {
                free(LAP_evt_msg.msg);
            }
        }
    }
}

void LAP_main_task_init(void)
{
    int r;

    r = msgq_create(&LAP_msgq, sizeof(LAPEvt_msgt), 20);
    if (0 != r)
    {
        printf("fail at msgq create\r\n");
    }

    r = task_create(NULL, LAP_main_task, NULL, task_gethighestpriority() - 2, 1024, NULL);
    if (r != 0)
    {
        printf("== LAP_main_task failed \n\r");
    }
    else
    {
        printf("== LAP_main_task created \n\r");
    }
}

int LAP_event_send(uint8_t evt, uint8_t state, uint16_t conn_handle, uint16_t handle, uint32_t msg_len, uint8_t *msg)
{
    LAPEvt_msgt lap_msg;

    lap_msg.event = evt;
    lap_msg.status = state;
    lap_msg.handle = handle;
    lap_msg.conn_handle = conn_handle;
    lap_msg.msg_len = msg_len;
    lap_msg.msg = msg;

    return msgq_send(LAP_msgq, (unsigned char*) &lap_msg);
}
