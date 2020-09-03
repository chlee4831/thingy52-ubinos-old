/*
 Copyright (C) 2016 Sung Ho Park, Yu Jin Park
 Contact: ubinos.org@gmail.com

 This file is part of the exe_helloworld component of the Ubinos.

 GNU General Public License Usage
 This file may be used under the terms of the GNU
 General Public License version 3.0 as published by the Free Software
 Foundation and appearing in the file license_gpl3.txt included in the
 packaging of this file. Please review the following information to
 ensure the GNU General Public License version 3.0 requirements will be
 met: http://www.gnu.org/copyleft/gpl.html.

 GNU Lesser General Public License Usage
 Alternatively, this file may be used under the terms of the GNU Lesser
 General Public License version 2.1 as published by the Free Software
 Foundation and appearing in the file license_lgpl.txt included in the
 packaging of this file. Please review the following information to
 ensure the GNU Lesser General Public License version 2.1 requirements
 will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.

 Commercial Usage
 Alternatively, licensees holding valid commercial licenses may
 use this file in accordance with the commercial license agreement
 provided with the software or, alternatively, in accordance with the
 terms contained in a written agreement between you and rightful owner.
 */

#include <ubinos.h>

#include <stdio.h>
#include <stdint.h>
#include <thingy_main.h>
#include "ble_stack.h"
#include "ble_process.h"
#include "LAP_main.h"
#include "LAP_api.h"

int appmain(int argc, char * argv[]) {

	printf("\n\n\n\r");
	printf("=======================================================\n\r");
	printf("Thingy52 (build time: %s %s)\n\r", __TIME__, __DATE__);
	printf("=======================================================\n\r");

	//Set Pin9, Pin10 NFC ANT -> GPIO
	//Disable NFC function
	if ((NRF_UICR->NFCPINS & UICR_NFCPINS_PROTECT_Msk)
			== (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos)) {
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		}
		NRF_UICR->NFCPINS &= ~UICR_NFCPINS_PROTECT_Msk;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		}
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		}
		NVIC_SystemReset();
	}

	//BLE stack task init
	BLE_stack_task_init();

	//BLE process task init
	BLE_process_task_init();

	//LAP protocol task init
	LAP_main_task_init();

	//main task init
	thingy_main_task_init();

	//Kernel Scheduler Start
	ubik_comp_start();

	return 0;
}


