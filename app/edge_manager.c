/*
 * edge_manager.c
 *
 *  Created on: 2020. 10. 28.
 *      Author: chlee
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

#include "edge_manager.h"

#define PAAR_ID_DEVICE_ID_INDEX 3
#define PAAR_ID_EDGE_ID_INDEX 2

#define EDGE_MANAGER_MAX_BRANCH_CNT 7

#define PACKET_DATA_SCAN_DEPTH_INDEX 0

#define SCAN_RETRY_CNT_MAX 3

static uint8_t edge_manager_branch_count = 0;
static branch_data_t branch_list[EDGE_MANAGER_MAX_BRANCH_CNT];

static uint8_t scan_management_state = SCAN_MANAGEMENT_STATE_IDLE;
static uint8_t scan_order_index = 0;
static uint8_t scan_order_branch_count = 0;
static bool scan_order_flag[EDGE_MANAGER_MAX_BRANCH_CNT];
static uint8_t scan_order[EDGE_MANAGER_MAX_BRANCH_CNT];
static uint8_t scan_done_cnt = 0;

static paar_packet_t current_scan_cmd;

static uint8_t scan_retry_cnt = SCAN_RETRY_CNT_MAX;

mutex_pt edge_mutex;

static uint8_t get_branch_index_by_edge_id(uint8_t edge_id)
{
    int i;

    for (i = 0; i < EDGE_MANAGER_MAX_BRANCH_CNT; i++)
    {
        if (edge_id == branch_list[i].branch_id)
        {
            return i;
        }
    }

    return 0xFF;
}
static void scan_management_init()
{
    scan_management_state = SCAN_MANAGEMENT_STATE_IDLE;
    scan_order_index = 0;
    scan_order_branch_count = 0;
    memset(scan_order_flag, false, EDGE_MANAGER_MAX_BRANCH_CNT);
    memset(scan_order, 0xFF, EDGE_MANAGER_MAX_BRANCH_CNT);
    scan_done_cnt = 0;
}

void edge_manager_init()
{
    int i, r;

    memset(branch_list, 0, sizeof(branch_data_t) * EDGE_MANAGER_MAX_BRANCH_CNT);

    for (i = 0; i < EDGE_MANAGER_MAX_BRANCH_CNT; i++)
    {
        branch_list[i].is_empty = true;
        branch_list[i].scan_done = false;
    }

    scan_management_init();

    r = mutex_create(&edge_mutex);
    if (r != 0)
    {
        printf("fail at mutex_create : edge_mutex");
    }
}

uint8_t edge_manager_branch_add(uint8_t cell_index)
{
    int i;
    uint8_t target_edge_index = 0xFF;
    uint8_t *current_paar_id_ptr;

    mutex_lock(edge_mutex);
    for (i = 0; i < EDGE_MANAGER_MAX_BRANCH_CNT; i++)
    {
        if (branch_list[i].is_empty == true)
        {
            target_edge_index = i;
            break;
        }
    }
    mutex_unlock(edge_mutex);

    if (target_edge_index == 0xFF)
    {
        printf("Edge manager no space\r\n");
        return 0xFF;
    }
    current_paar_id_ptr = cell_management_get_PAAR_ID_by_index(cell_index);
    if (current_paar_id_ptr != NULL)
    {
        if (current_paar_id_ptr[PAAR_ID_DEVICE_ID_INDEX] == EDGE_TAG_DEVICE_ID)
        {
            mutex_lock(edge_mutex);
//			if (current_paar_id_ptr[PAAR_ID_EDGE_ID_INDEX] == EDGE_ID)
//			{
//				printf("Duplicate Edge ID\r\n");
//				return 0xFF;
//			}
            for (i = 0; i < EDGE_MANAGER_MAX_BRANCH_CNT; i++)
            {
                if (branch_list[i].is_empty == false && branch_list[i].branch_id == current_paar_id_ptr[PAAR_ID_EDGE_ID_INDEX])
                {
                    printf("Duplicate Edge ID\r\n");
                    return 0xFF;
                }
            }
            mutex_unlock(edge_mutex);

            cell_management_set_data_authourize(cell_index);

            mutex_lock(edge_mutex);

            branch_list[target_edge_index].is_empty = false;
            branch_list[target_edge_index].scan_done = false;
            branch_list[target_edge_index].cell_index = cell_index;
            branch_list[target_edge_index].conn_handle = get_cell_management_connhandle_by_index(cell_index);
            get_cell_management_profile_data_by_index(cell_index, &branch_list[target_edge_index].profile_data);
            branch_list[target_edge_index].branch_id = current_paar_id_ptr[PAAR_ID_EDGE_ID_INDEX];
            branch_list[target_edge_index].branch_depth = 1;
            branch_list[target_edge_index].connected_edge_cnt = 1;
            memset(branch_list[target_edge_index].connected_edge_list, 0, MAX_CONN_EDGE_LIST_SIZE);
            branch_list[target_edge_index].connected_edge_list[0] = current_paar_id_ptr[PAAR_ID_EDGE_ID_INDEX];

            mutex_unlock(edge_mutex);

            edge_manager_branch_count++;

            printf("Branch Add - ID: 0x%02x Cnt: %d\r\n", branch_list[target_edge_index].branch_id, edge_manager_branch_count);
            return 0;
        }
    }
    return 0xFF;
}

uint8_t edge_manager_branch_remove(uint8_t cell_index)
{
    int i;
    uint8_t found_flag = 0;
    uint8_t target_edge_index = 0xFF;

    mutex_lock(edge_mutex);

    for (i = 0; i < EDGE_MANAGER_MAX_BRANCH_CNT; i++)
    {
        if (branch_list[i].is_empty == false && branch_list[i].cell_index == cell_index)
        {
            found_flag = 1;
            target_edge_index = i;
            break;
        }
    }
    if (found_flag == 0)
    {
        printf("Can't find connected edge\r\n");
        return 0xFF;
    }
    edge_manager_branch_count--;

    printf("Branch Remove - ID: 0x%02x Cnt: %d\r\n", branch_list[target_edge_index].branch_id, edge_manager_branch_count);

    memset(&branch_list[target_edge_index], 0, sizeof(branch_data_t));
    branch_list[target_edge_index].is_empty = true;

    mutex_unlock(edge_mutex);

    return 0;
}

static void edge_manager_scan_management(paar_packet_t *packet)
{
    int i, j;
    paar_data_t *paar_data = (paar_data_t*) packet->data;

    switch (paar_data->cmd)
    {
    case PAAR_EDGE_CMD_SCAN_START:
    {
        packet_scan_cmd_t *packet_scan_cmd = (packet_scan_cmd_t*) packet;

        if (scan_management_state == SCAN_MANAGEMENT_STATE_IDLE)
        {
            packet_scan_cmd->depth -= 1;

            if (packet_scan_cmd->depth == 0)
            {
                task_sleepms(100);

                LAP_start_ble_scan(NULL);
            }
            else
            {
                //todo 하위 노드 정해서 명령 전달
                if (edge_manager_branch_count != 0)
                {
                    scan_management_state = SCAN_MANAGEMENT_STATE_SCANNING;

                    for (j = 0; j < edge_manager_branch_count; j++)
                    {
                        uint8_t target_edge_index = 0xFF;

                        for (i = 0; i < EDGE_MANAGER_MAX_BRANCH_CNT; i++)
                        {
                            if ((branch_list[i].is_empty == false && scan_order_flag[i] == false && branch_list[i].scan_done == false)
                                    && ((branch_list[i].branch_depth < branch_list[target_edge_index].branch_depth) || target_edge_index == 0xFF))
                            {
                                target_edge_index = i;
                                scan_order_flag[i] = true;
                            }
                        }
                        if (target_edge_index != 0xFF)
                        {
                            scan_order[j] = target_edge_index;
                            scan_order_branch_count++;
                        }
                        else
                            break;
                    }
                    memset(&current_scan_cmd, 0, sizeof(paar_packet_t));
                    memcpy(&current_scan_cmd, packet, sizeof(paar_packet_t));

                    send_packet_central(branch_list[scan_order[scan_order_index]].conn_handle, branch_list[scan_order[scan_order_index]].profile_data.rx_handle,
                            &current_scan_cmd);
                    scan_order_index++;
                }
                else
                {
                    packet_edge_conn_report_t packet_edge_conn_report;

                    memset(&packet_edge_conn_report, 0, sizeof(packet_edge_conn_report_t));

                    packet_edge_conn_report.service_ID = EDGE_SERVICE_ID;
                    packet_edge_conn_report.seq = 0x11;
                    packet_edge_conn_report.data_len = 3;
                    packet_edge_conn_report.cmd = PAAR_EDGE_CMD_EDGE_CONN_REPORT;
                    packet_edge_conn_report.scan_done = true;
                    packet_edge_conn_report.num_of_nodes = 1;
                    packet_edge_conn_report.my_edge_id = EDGE_ID;

                    scan_management_state = SCAN_MANAGEMENT_STATE_IDLE;

                    send_packet_peripheral(&packet_edge_conn_report);

                    scan_order_index = 0;
                }
            }
        }
        else
        {
            printf("Processing previous scan cmd\r\n");
        }
        break;
    }

    case PAAR_EDGE_CMD_EDGE_CONN_REPORT:
    {
        packet_edge_conn_report_t *packet_edge_conn_report = (packet_edge_conn_report_t*) packet;
        uint8_t branch_index = get_branch_index_by_edge_id(packet_edge_conn_report->my_edge_id);

        if (branch_index == 0xFF)
        {
            printf("Cannot find branch ID\r\n");
            break;
        }

        branch_list[branch_index].scan_done = packet_edge_conn_report->scan_done;
        if (packet_edge_conn_report->scan_done)
            scan_done_cnt++;
        branch_list[branch_index].connected_edge_cnt = packet_edge_conn_report->num_of_nodes;
        memcpy(branch_list[branch_index].connected_edge_list, &packet_edge_conn_report->my_edge_id, packet_edge_conn_report->num_of_nodes);

        if (scan_order_index < scan_order_branch_count)
        {
            send_packet_central(branch_list[scan_order[scan_order_index]].conn_handle, branch_list[scan_order[scan_order_index]].profile_data.rx_handle,
                    &current_scan_cmd);
            scan_order_index++;
        }
        else
        {
            packet_edge_conn_report_t tmp_packet;
            uint8_t edge_id_index = 0;

            memset(&tmp_packet, 0, sizeof(paar_packet_t));

            tmp_packet.service_ID = EDGE_SERVICE_ID;
            tmp_packet.seq = 0x11;
            tmp_packet.data_len = 1;
            tmp_packet.cmd = PAAR_EDGE_CMD_EDGE_CONN_REPORT;
            if (scan_done_cnt == edge_manager_branch_count)
                tmp_packet.scan_done = true;
            else
                tmp_packet.scan_done = false;
            tmp_packet.my_edge_id = EDGE_ID;
//            for (edge_id_index = 0; edge_id_index < edge_manager_branch_count; edge_id_index++)
//            {
//                tmp_packet.edge_id[edge_id_index] = branch_list[edge_id_index].branch_id;
//            }
            for (i = 0; i < edge_manager_branch_count; i++)
            {
                memcpy(&tmp_packet.edge_id[edge_id_index], branch_list[i].connected_edge_list, branch_list[i].connected_edge_cnt);
                edge_id_index += branch_list[i].connected_edge_cnt;
            }
            tmp_packet.num_of_nodes = edge_id_index;

            send_packet_peripheral(&tmp_packet);

            scan_management_state = SCAN_MANAGEMENT_STATE_IDLE;
            scan_order_index = 0;
        }
        break;
    }
    }
}

void edge_manager_central_data_received(LAPEvt_msgt LAP_evt_msg)
{
    paar_packet_t *packet = (paar_packet_t*) LAP_evt_msg.msg;
    paar_data_t *packet_data = (paar_data_t*) packet->data;

    send_ack_central(LAP_evt_msg.conn_handle, LAP_evt_msg.handle, packet);

    switch (packet_data->cmd)
    {
    case PAAR_EDGE_CMD_EDGE_CONN_REPORT:
        edge_manager_scan_management(packet);
        break;

    case PAAR_EDGE_CMD_DEVICE_CONN_REPORT:

        break;

    case PAAR_EDGE_CMD_ACTIVITY_REPORT:

        break;
    }

}

void edge_manager_peripheral_data_received(LAPEvt_msgt LAP_evt_msg)
{
    paar_packet_t *packet = (paar_packet_t*) LAP_evt_msg.msg;
    paar_data_t *data = (paar_data_t*) packet->data;

    send_ack_peripheral(packet);

    switch (data->cmd)
    {
    case PAAR_EDGE_CMD_SCAN_START:
        edge_manager_scan_management(packet);
        break;

    default:
        break;
    }
}

void edge_manager_scan_timeout()
{
    scan_retry_cnt--;
    if (scan_retry_cnt > 0)
    {
        LAP_start_ble_scan(NULL);
    }
    else
    {
        int i;
        packet_edge_conn_report_t tmp_packet;
        memset(&tmp_packet, 0, sizeof(paar_packet_t));

        tmp_packet.service_ID = EDGE_SERVICE_ID;
        tmp_packet.seq = 0x11;
        tmp_packet.data_len = 1 + edge_manager_branch_count;
        tmp_packet.cmd = PAAR_EDGE_CMD_EDGE_CONN_REPORT;
        if (edge_manager_branch_count == 0)
            tmp_packet.scan_done = true;
        else
            tmp_packet.scan_done = false;
        tmp_packet.num_of_nodes = edge_manager_branch_count;
        tmp_packet.my_edge_id = EDGE_ID;
        for (i = 0; i < edge_manager_branch_count; i++)
        {
            tmp_packet.edge_id[i] = branch_list[i].branch_id;
        }

        send_packet_peripheral(&tmp_packet);

        scan_retry_cnt = SCAN_RETRY_CNT_MAX;
    }
}

void edge_manager_scan_result()
{
    scan_retry_cnt = SCAN_RETRY_CNT_MAX;
}
