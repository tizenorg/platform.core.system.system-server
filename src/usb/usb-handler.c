/*
 * deviced
 *
 * Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <vconf.h>
#include <device-node.h>

#include "core/log.h"
#include "core/launch.h"
#include "core/data.h"
#include "core/devices.h"
#include "display/poll.h"

#define USBCON_EXEC_PATH	PREFIX"/bin/usb-server"
#define RETRY			3

static void usb_init(void *data)
{
	int val, i = 0, pid;

	_D("check usb connection");
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_USB_ONLINE, &val) == 0) {
		if (val==1) {
			vconf_set_int(VCONFKEY_SYSMAN_USB_STATUS,
					VCONFKEY_SYSMAN_USB_AVAILABLE);
			while (i < RETRY
					&& pm_lock_internal(getpid(), LCD_OFF, STAY_CUR_STATE, 0) == -1) {
				i++;
				sleep(1);
			}
			pid = ss_launch_if_noexist(USBCON_EXEC_PATH, NULL);
			if (pid < 0) {
				_E("usb appl launching failed\n");
				return;
			}
		}
		else if (val==0)
			vconf_set_int(VCONFKEY_SYSMAN_USB_STATUS,VCONFKEY_SYSMAN_USB_DISCONNECTED);
	}
}

const struct device_ops usb_device_ops = {
	.init = usb_init,
};
