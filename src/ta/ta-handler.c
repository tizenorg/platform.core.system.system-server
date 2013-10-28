/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <pmapi.h>
#include <vconf.h>
#include <sysman.h>
#include <device-node.h>

#include "core/log.h"
#include "core/data.h"

#define RETRY	3

int ss_ta_init()
{
	int val = -1, i = 0, pid;

	_D("check ta connection");
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_TA_ONLINE, &val) == 0) {
		if ( val==1 ) {
			vconf_set_int(VCONFKEY_SYSMAN_CHARGER_STATUS,
					VCONFKEY_SYSMAN_CHARGER_CONNECTED);
			while (i < RETRY
			       && pm_lock_state(LCD_OFF, STAY_CUR_STATE,
						0) == -1) {
				i++;
				sleep(1);
			}
			_D("ta is connected");
		}
		else if ( val==0 )
			vconf_set_int(VCONFKEY_SYSMAN_CHARGER_STATUS,
					VCONFKEY_SYSMAN_CHARGER_DISCONNECTED);
	}
	return 0;
}
