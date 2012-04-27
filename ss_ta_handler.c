/* 
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * This file is part of system-server
 * Written by DongGi Jang <dg0402.jang@samsung.com>
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with SAMSUNG ELECTRONICS.
 *
 * SAMSUNG make no representations or warranties about the suitability
 * of the software, either express or implied, including but not limited
 * to the implied warranties of merchantability, fitness for a particular
 * purpose, or non-infringement. SAMSUNG shall not be liable for any
 * damages suffered by licensee as a result of using, modifying or
 * distributing this software or its derivatives.
*/


#include <pmapi.h>
#include <vconf.h>
#include <sysman.h>

#include "ss_device_plugin.h"
#include "ss_log.h"
#include "include/ss_data.h"

#define RETRY	3

int ss_ta_init()
{
	int val = -1, i = 0, pid;

	PRT_TRACE("check ta connection");
	if (plugin_intf->OEM_sys_get_jack_charger_online(&val) == 0) {
		if (val==1) {
			while (i < RETRY
			       && pm_lock_state(LCD_OFF, STAY_CUR_STATE,
						0) == -1) {
				i++;
				sleep(1);
			}
			PRT_TRACE("ta is connected");
		}
	}
	return 0;
}
