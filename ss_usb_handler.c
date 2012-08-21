/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.tizenopensource.org/license
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

#include "ss_log.h"
#include "ss_device_plugin.h"
#include "ss_launch.h"
#include "include/ss_data.h"

#define USBCON_EXEC_PATH	PREFIX"/bin/usb_setting"
#define RETRY			3

int ss_usb_init()
{
	int val = -1, i = 0, pid;

	PRT_TRACE("check usb connection");
	if (plugin_intf->OEM_sys_get_jack_usb_online(&val) == 0) {
		if (val==1) {
			vconf_set_int(VCONFKEY_SYSMAN_USB_STATUS,
					VCONFKEY_SYSMAN_USB_AVAILABLE);
			while (i < RETRY
					&& pm_lock_state(LCD_OFF, STAY_CUR_STATE,
						0) == -1) {
				i++;
				sleep(1);
			}
			pid = ss_launch_if_noexist(USBCON_EXEC_PATH, NULL);
			if (pid < 0) {
				PRT_TRACE_ERR("usb appl launching failed\n");
				return -1;
			}
		}
		else if (val==0)
			vconf_set_int(VCONFKEY_SYSMAN_USB_STATUS,VCONFKEY_SYSMAN_USB_DISCONNECTED);
	}

	return 0;
}
