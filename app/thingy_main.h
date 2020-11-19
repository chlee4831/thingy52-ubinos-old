/*
 * FE_main.h
 *
 *  Created on: 2019. 8. 27.
 *      Author: YJPark
 */

#ifndef APP_THINGY_MAIN_H_
#define APP_THINGY_MAIN_H_

#include <stdint.h>

typedef struct
{
    uint8_t event;
    uint8_t state;
    uint8_t *msg;
} mainEvt_msgt;

enum
{
    THINGY_MAIN_EVT_BUTTON,
};

void thingy_main_event_send(uint8_t evt, uint8_t state, uint8_t *msg);

void thingy_main_task_init(void);

#endif /* THINGY_MAIN_H_ */
