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
    uint8_t edge_id;
    uint8_t cell_index;
    uint16_t conn_handle;
    uuidhandle profile_data;
    uint8_t branch_depth;
    uint8_t connected_edge_cnt;
    uint8_t connected_edge_list[MAX_CONN_EDGE_LIST_SIZE];
} branch_data_t;

void edge_manager_init();

uint8_t edge_manager_branch_add(uint8_t cell_index);

uint8_t edge_manager_branch_remove(uint8_t cell_index);

#endif /* APP_EDGE_MANAGER_H_ */
