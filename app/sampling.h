/*
 * sampling_main.h
 *
 *  Created on: 2020. 9. 15.
 *      Author: chlee
 */

#ifndef APP_SAMPLING_H_
#define APP_SAMPLING_H_


#include <stdint.h>

#include "tes.h"



/***************************************************************************************************************/

typedef struct
{
	uint8_t event;
	uint8_t state;
	uint8_t* msg;
}smpEvt_msgt;

// event type
enum {
	SAMP_EVT_ENV = 0,
	SAMP_EVT_MOTION,
	SAMP_EVT_UI,
};

enum {
	ENV_STATE_TEMP_HUMID = 0,
	ENV_STATE_PRESSURE,
	ENV_STATE_GAS,
	ENV_STATE_COLOR,
};

void sampling_event_send(uint8_t evt, uint8_t state, uint8_t * msg);

void sampling_task_init(void);
/***************************************************************************************************************/



#endif /* APP_SAMPLING_H_ */
