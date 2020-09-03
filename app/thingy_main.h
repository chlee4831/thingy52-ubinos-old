/*
 * FE_main.h
 *
 *  Created on: 2019. 8. 27.
 *      Author: YJPark
 */

#ifndef APP_THINGY_MAIN_H_
#define APP_THINGY_MAIN_H_


#include <stdint.h>
//#include "ble_gap.h"
//
//#include "ble_stack.h"

typedef struct
{
	uint8_t event;
	uint8_t state;
	uint8_t* msg;
}mainEvt_msgt;

// event type
enum{
	THINGY_EVT_TEST_TIMEOUT = 0,
	THINGY_EVT_BUTTON,
	THINGY_EVT_ENV_PRESSURE,
	THINGY_EVT_ENV_HUMIDITY,
	THINGY_EVT_ENV_TEMPERATURE,
	THINGY_EVT_ENV_GAS,
	THINGY_EVT_ENV_COLOR,
};

enum{
	ADL_STATE_EMPTY = 0,
};

void thingy_main_event_send(uint8_t evt, uint8_t state, uint8_t * msg);

void thingy_main_task_init(void);

#endif /* ADL_MAIN_H_ */
