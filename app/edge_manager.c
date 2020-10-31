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

static uint8_t edge_manager_branch_count = 0;
static branch_data_t branch_list[EDGE_MANAGER_MAX_BRANCH_CNT];

mutex_pt edge_mutex;

void edge_manager_init()
{
    int i, r;

    memset(branch_list, 0, sizeof(branch_data_t) * EDGE_MANAGER_MAX_BRANCH_CNT);

    for (i = 0; i < EDGE_MANAGER_MAX_BRANCH_CNT; i++)
    {
        branch_list[i].is_empty = true;
    }

    r = mutex_create(&edge_mutex);
    if (r != 0)
    {
        printf("fail at mutex_create : edge_mutex");
    }
}

uint8_t edge_manager_branch_add(uint8_t cell_index)
{
    int i;
    uint8_t target_edge_index;
    uint8_t *current_paar_id_ptr;

    mutex_lock(edge_mutex);
    for (i = 0; i < EDGE_MANAGER_MAX_BRANCH_CNT; i++)
    {
        if (branch_list[i].is_empty == true)
            target_edge_index = i;
    }
    mutex_unlock(edge_mutex);

    current_paar_id_ptr = cell_management_get_PAAR_ID_by_index(cell_index);
    if (current_paar_id_ptr != NULL)
    {
        if (current_paar_id_ptr[PAAR_ID_DEVICE_ID_INDEX] == EDGE_TAG_DEVICE_ID)
        {
            cell_management_set_data_authourize(cell_index);

            mutex_lock(edge_mutex);

            branch_list[target_edge_index].is_empty = false;
            branch_list[target_edge_index].cell_index = cell_index;
            branch_list[target_edge_index].conn_handle = get_cell_management_connhandle_by_index(cell_index);
            get_cell_management_profile_data_by_index(cell_index, &branch_list[target_edge_index].profile_data);
            branch_list[target_edge_index].edge_id = current_paar_id_ptr[PAAR_ID_EDGE_ID_INDEX];
            branch_list[target_edge_index].branch_depth = 1;
            branch_list[target_edge_index].connected_edge_cnt = 1;
            memset(branch_list[target_edge_index].connected_edge_list, 0, MAX_CONN_EDGE_LIST_SIZE);
            branch_list[target_edge_index].connected_edge_list[0] = current_paar_id_ptr[PAAR_ID_EDGE_ID_INDEX];

            mutex_unlock(edge_mutex);

            edge_manager_branch_count++;

            printf("Branch Add - ID: 0x%02x Cnt: %d\r\n", branch_list[target_edge_index].edge_id, edge_manager_branch_count);
            return 0;
        }
    }
    return 0xFF;
}

uint8_t edge_manager_branch_remove(uint8_t cell_index)
{
    int i;
    uint8_t target_edge_index;

    mutex_lock(edge_mutex);

    for (i = 0; i < EDGE_MANAGER_MAX_BRANCH_CNT; i++)
    {
        if (branch_list[i].is_empty == false && branch_list[i].cell_index == cell_index)
        {
            target_edge_index = i;
            break;
        }
    }

    edge_manager_branch_count--;

    printf("Branch Remove - ID: 0x%02x Cnt: %d\r\n", branch_list[target_edge_index].edge_id, edge_manager_branch_count);

    memset(&branch_list[target_edge_index], 0, sizeof(branch_data_t));
    branch_list[target_edge_index].is_empty = true;

    mutex_unlock(edge_mutex);

    edge_manager_branch_count--;

    return 0;
}

void edge_manager_peripheral_data_received(LAPEvt_msgt LAP_evt_msg)
{
    paar_packet_t *packet = (paar_packet_t*) LAP_evt_msg.msg;
    paar_data_t *packet_data = (paar_data_t*) packet->data;
    uint8_t target_edge_index = 0;

    int i;
    uint8_t *packet_arr = (uint8_t*) packet;
    printf("Received Packet: 0x");
    for (i = 0; i < PAAR_MAXIMUM_PACKET_SIZE; i++)
    {
        printf("%02X ", packet_arr[i]);
    }
//	printf("\r\n");
    printf("\r\n");	//print packet

    send_ack_peripheral(packet);

    switch (packet_data->cmd)
    {
    case PAAR_EDGE_CMD_SCAN_START:
        packet_data->data[PACKET_DATA_SCAN_DEPTH_INDEX] -= 1;
        if (packet_data->data[PACKET_DATA_SCAN_DEPTH_INDEX] == 0)
        {

            task_sleepms(100);

            LAP_start_ble_scan(NULL);
        }
        else
        {
            //todo 하위 노드 정해서 명령 전달
            if (edge_manager_branch_count != 0)
            {
                for (i = 0; i < EDGE_MANAGER_MAX_BRANCH_CNT; i++)
                {
                    if (branch_list[i].is_empty == false && branch_list[i].branch_depth > branch_list[target_edge_index].branch_depth)
                    {
                        target_edge_index = i;
                    }
                }
                send_packet_central(branch_list[target_edge_index].conn_handle, branch_list[target_edge_index].profile_data.rx_handle, packet);
                printf("Send Packet: 0x");
                for (i = 0; i < PAAR_MAXIMUM_PACKET_SIZE; i++)
                {
                    printf("%02X ", packet_arr[i]);
                }
                printf("\r\n");	//print packet
            }
        }
        break;

    default:
        break;
    }
}

