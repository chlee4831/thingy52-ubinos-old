/*
 * edge_manager.h
 *
 *  Created on: 2020. 10. 28.
 *      Author: chlee
 */

#ifndef APP_EDGE_MANAGER_H_
#define APP_EDGE_MANAGER_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_CONN_EDGE_LIST_SIZE 12

typedef struct
{
    bool is_empty;
    bool scan_done;
    uint8_t branch_id;
    uint8_t cell_index;
    uint16_t conn_handle;
    uuidhandle profile_data;
    uint8_t branch_depth;
    uint8_t connected_edge_cnt;
    uint8_t connected_edge_list[MAX_CONN_EDGE_LIST_SIZE];
} branch_data_t;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t service_ID;
    uint8_t seq;        //4bit:4bit - seq_no:total
    uint8_t data_len;
    uint8_t cmd;
} packet_ack_t;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t service_ID;
    uint8_t seq;        //4bit:4bit - seq_no:total
    uint8_t data_len;
    uint8_t cmd;
    uint8_t depth;
} packet_scan_cmd_t;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t service_ID;
    uint8_t seq;        //4bit:4bit - seq_no:total
    uint8_t data_len;
    uint8_t cmd;
    uint8_t scan_done;
    uint8_t num_of_nodes;
    uint8_t my_edge_id;
    uint8_t edge_id[12];
} packet_edge_conn_report_t;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t service_ID;
    uint8_t seq;        //4bit:4bit - seq_no:total
    uint8_t data_len;
    uint8_t cmd;
    uint8_t edge_id;
    uint8_t num_of_devices;
    uint8_t device_id[7];
} packet_dev_conn_reoprt_t;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t service_ID;
    uint8_t seq;        //4bit:4bit - seq_no:total
    uint8_t data_len;
    uint8_t cmd;
    uint8_t src_edge_id;
    uint8_t src_device_id;
    uint8_t service_id;
    uint8_t act_data[11];
} packet_activity_report_t;

enum
{
    PAAR_EDGE_CMD_SCAN_START = 1,
    PAAR_EDGE_CMD_EDGE_CONN_REPORT,
    PAAR_EDGE_CMD_DEVICE_CONN_REPORT,
    PAAR_EDGE_CMD_ACTIVITY_REPORT,
};

enum
{
    SCAN_MANAGEMENT_STATE_IDLE,
    SCAN_MANAGEMENT_STATE_SCANNING,
};

void edge_manager_init();

uint8_t edge_manager_branch_add(uint8_t cell_index);

uint8_t edge_manager_branch_remove(uint8_t cell_index);

void edge_manager_central_data_received(LAPEvt_msgt LAP_evt_msg);

void edge_manager_peripheral_data_received(LAPEvt_msgt LAP_evt_msg);

#endif /* APP_EDGE_MANAGER_H_ */
