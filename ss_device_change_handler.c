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


#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <vconf.h>
#include <sysman.h>
#include <pmapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <syspopup_caller.h>
#include <aul.h>
#include <bundle.h>
#include <dirent.h>

#include "ss_queue.h"
#include "ss_log.h"
#include "ss_device_handler.h"
#include "ss_device_plugin.h"
#include "ss_noti.h"
#include "include/ss_data.h"
#include "sys_device_noti/sys_device_noti.h"

#define BUFF_MAX		255
#define SYS_CLASS_INPUT		"/sys/class/input"
#define USBCON_EXEC_PATH	PREFIX"/bin/usb-server"
#define DEFAULT_USB_INFO_PATH	"/tmp/usb_default"
#define STORE_DEFAULT_USB_INFO	"usb-devices > "DEFAULT_USB_INFO_PATH

struct input_event {
	long dummy[2];
	unsigned short type;
	unsigned short code;
	int value;
};

enum snd_jack_types {
	SND_JACK_HEADPHONE = 0x0001,
	SND_JACK_MICROPHONE = 0x0002,
	SND_JACK_HEADSET = SND_JACK_HEADPHONE | SND_JACK_MICROPHONE,
	SND_JACK_LINEOUT = 0x0004,
	SND_JACK_MECHANICAL = 0x0008,	/* If detected separately */
	SND_JACK_VIDEOOUT = 0x0010,
	SND_JACK_AVOUT = SND_JACK_LINEOUT | SND_JACK_VIDEOOUT,
};


static int input_device_number;
static int check_lowbat_charge_device(int bInserted)
{
	static int bChargeDeviceInserted = 0;
	int val = -1;
	int bat_state = -1;
	int ret = -1;
	if (bInserted == 1) {
		if (plugin_intf->OEM_sys_get_battery_charge_now(&val) == 0) {
			if (val == 1)
				bChargeDeviceInserted = 1;
			return 0;
		}
	} else if (bInserted == 0) {
		if (plugin_intf->OEM_sys_get_battery_charge_now(&val) == 0) {
			if (val == 0 && bChargeDeviceInserted == 1) {
				bChargeDeviceInserted = 0;
				//low bat popup during charging device removing
				vconf_get_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, &bat_state);
				if(bat_state < VCONFKEY_SYSMAN_BAT_NORMAL) {
					bundle *b = NULL;
					b = bundle_create();
					if(bat_state == VCONFKEY_SYSMAN_BAT_POWER_OFF)
						bundle_add(b,"_SYSPOPUP_CONTENT_", "poweroff");
					else
						bundle_add(b, "_SYSPOPUP_CONTENT_", "warning");
					ret = syspopup_launch("lowbat-syspopup", b);
					if (ret < 0) {
						PRT_TRACE_EM("popup lauch failed\n");
					}
					bundle_free(b);
				}
			}
			return 0;
		}
	}
	return -1;
}

static void usb_chgdet_cb(struct ss_main_data *ad)
{
	int val = -1;
	char params[BUFF_MAX];
	PRT_TRACE("jack - usb changed\n");
	pm_change_state(LCD_NORMAL);
	/* check charging now */
	ss_lowbat_is_charge_in_now();
	/* check current battery level */
	ss_lowbat_monitor(NULL);
	ss_action_entry_call_internal(PREDEF_USBCON, 0);

	if (plugin_intf->OEM_sys_get_jack_usb_online(&val)==0) {
		check_lowbat_charge_device(val);
		if (val==1) {
			snprintf(params, sizeof(params), "%d", CB_NOTI_BATT_CHARGE);
			ss_launch_if_noexist("/usr/bin/sys_device_noti", params);
			PRT_TRACE("usb device notification");
		}
	}
}

static void ta_chgdet_cb(struct ss_main_data *ad)
{
	PRT_TRACE("jack - ta changed\n");
	pm_change_state(LCD_NORMAL);
	/* check charging now */
	ss_lowbat_is_charge_in_now();
	/* check current battery level */
	ss_lowbat_monitor(NULL);

	int val = -1;
	int ret = -1;
	int bat_state = VCONFKEY_SYSMAN_BAT_NORMAL;
	char params[BUFF_MAX];

	if (plugin_intf->OEM_sys_get_jack_charger_online(&val) == 0) {
		check_lowbat_charge_device(val);
		vconf_set_int(VCONFKEY_SYSMAN_CHARGER_STATUS, val);
		if (val == 0) {
			pm_unlock_state(LCD_OFF, STAY_CUR_STATE);
		} else {
			pm_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
			snprintf(params, sizeof(params), "%d", CB_NOTI_BATT_CHARGE);
			ss_launch_if_noexist("/usr/bin/sys_device_noti", params);
			PRT_TRACE("ta device notification");
		}
	}
	else
		PRT_TRACE_ERR("failed to get ta status\n");
}

static void earjack_chgdet_cb(struct ss_main_data *ad)
{
	PRT_TRACE("jack - earjack changed\n");
	ss_action_entry_call_internal(PREDEF_EARJACKCON, 0);
}

static void earkey_chgdet_cb(struct ss_main_data *ad)
{
	int val;
	PRT_TRACE("jack - earkey changed\n");

	if (plugin_intf->OEM_sys_get_jack_earkey_online(&val) == 0)
		vconf_set_int(VCONFKEY_SYSMAN_EARJACKKEY, val);
}

static void tvout_chgdet_cb(struct ss_main_data *ad)
{
	PRT_TRACE("jack - tvout changed\n");
	pm_change_state(LCD_NORMAL);
}

static void hdmi_chgdet_cb(struct ss_main_data *ad)
{
	PRT_TRACE("jack - hdmi changed\n");
	int val;
	pm_change_state(LCD_NORMAL);
	if (plugin_intf->OEM_sys_get_jack_hdmi_online(&val) == 0)
		if(val == 1)
			pm_lock_state(LCD_NORMAL, GOTO_STATE_NOW, 0);
		else
			pm_unlock_state(LCD_NORMAL, PM_SLEEP_MARGIN);
}

static void keyboard_chgdet_cb(struct ss_main_data *ad)
{
	int val = -1;
	int ret = -1;
	PRT_TRACE("jack - keyboard changed\n");

	ret = plugin_intf->OEM_sys_get_jack_keyboard_online(&val);
	if( ret == 0) {
		if(val != 1)
			val = 0;
		vconf_set_int(VCONFKEY_SYSMAN_SLIDING_KEYBOARD, val);
	} else {
		vconf_set_int(VCONFKEY_SYSMAN_SLIDING_KEYBOARD, VCONFKEY_SYSMAN_SLIDING_KEYBOARD_NOT_SUPPORTED);
	}
}

static void mmc_chgdet_cb(void *data)
{
	if (data == NULL) {
		PRT_TRACE("mmc removed");
		ss_mmc_removed();
	} else {
		PRT_TRACE("mmc added");
		ss_mmc_inserted();
	}
}

static void ums_unmount_cb(void *data)
{
	umount(MOVINAND_MOUNT_POINT);
}

static void charge_cb(struct ss_main_data *ad)
{
	int val = -1;
	char params[BUFF_MAX];
	static int bat_full_noti = 0;
	ss_lowbat_monitor(NULL);
	if (plugin_intf->OEM_sys_get_battery_health(&val) == 0) {
		if (val==BATTERY_OVERHEAT || val==BATTERY_COLD) {
			PRT_TRACE_ERR("Battery health status is not good (%d)", val);
			ss_action_entry_call_internal(PREDEF_LOWBAT, 1, CHARGE_ERROR_ACT);
			return;
		}
	} else {
		PRT_TRACE_ERR("failed to get battery health status");
	}
	plugin_intf->OEM_sys_get_battery_charge_full(&val);
	if (val==0) {
		if (bat_full_noti==1) {
			snprintf(params, sizeof(params), "%d %d", CB_NOTI_BATT_FULL, CB_NOTI_OFF);
			ss_launch_if_noexist("/usr/bin/sys_device_noti", params);
		}
		bat_full_noti = 0;
	} else {
		if (val==1 && bat_full_noti==0) {
			bat_full_noti = 1;
			PRT_TRACE("battery full noti");
			snprintf(params, sizeof(params), "%d %d", CB_NOTI_BATT_FULL, CB_NOTI_ON);
			ss_launch_if_noexist("/usr/bin/sys_device_noti", params);
		}
	}
}

static void usb_host_chgdet_cb(keynode_t *in_key, struct ss_main_data *ad)
{
	PRT_TRACE("ENTER: usb_host_chgdet_cb()");
	int status;
	int ret = vconf_get_int(VCONFKEY_SYSMAN_USB_HOST_STATUS, &status);
	if (ret != 0) {
		PRT_TRACE_ERR("vconf get failed(VCONFKEY_SYSMAN_USB_HOST_STATUS)\n");
		return ;
	}

	if(VCONFKEY_SYSMEN_USB_HOST_CONNECTED == status) {
		int pid = ss_launch_if_noexist(USBCON_EXEC_PATH, NULL);
		if (pid < 0) {
			PRT_TRACE("usb-server launching failed\n");
			return;
		}
	}
	PRT_TRACE("EXIT: usb_host_chgdet_cb()");
}

static void usb_host_add_cb()
{
	PRT_TRACE("ENTER: usb_host_add_cb()\n");
	int status;
	int ret = vconf_get_int(VCONFKEY_SYSMAN_USB_HOST_STATUS, &status);
	if (ret != 0) {
		PRT_TRACE("vconf get failed ()\n");
		return;
	}

	if (-1 == status) { /* '-1' means that USB host mode is not loaded yet */
		PRT_TRACE("This usb device is connected defaultly\n");

		ret = system(STORE_DEFAULT_USB_INFO);
		PRT_TRACE("Return value of usb-devices: %d\n", ret);
		if (0 != access(DEFAULT_USB_INFO_PATH, F_OK)) {
			ret = system(STORE_DEFAULT_USB_INFO);
			PRT_TRACE("Return value of usb-devices: %d\n", ret);
		}
	}
	PRT_TRACE("EXIT: usb_host_add_cb()\n");
}

int ss_device_change_init(struct ss_main_data *ad)
{
	/* for simple noti change cb */
	ss_noti_add("device_usb_chgdet", (void *)usb_chgdet_cb, (void *)ad);
	ss_noti_add("device_ta_chgdet", (void *)ta_chgdet_cb, (void *)ad);
	ss_noti_add("device_earjack_chgdet", (void *)earjack_chgdet_cb, (void *)ad);
	ss_noti_add("device_earkey_chgdet", (void *)earkey_chgdet_cb, (void *)ad);
	ss_noti_add("device_tvout_chgdet", (void *)tvout_chgdet_cb, (void *)ad);
	ss_noti_add("device_hdmi_chgdet", (void *)hdmi_chgdet_cb, (void *)ad);
	ss_noti_add("device_keyboard_chgdet", (void *)keyboard_chgdet_cb, (void *)ad);

	ss_noti_add("device_usb_host_add", (void *)usb_host_add_cb, (void *)ad);
	ss_noti_add("mmcblk_add", (void *)mmc_chgdet_cb, (void *)1);
	ss_noti_add("mmcblk_remove", (void *)mmc_chgdet_cb, NULL);

	ss_noti_add("unmount_ums", (void *)ums_unmount_cb, NULL);
	ss_noti_add("device_charge_chgdet", (void *)charge_cb, (void *)ad);

	if (vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS, usb_host_chgdet_cb, NULL) < 0) {
		PRT_TRACE_ERR("vconf key notify failed(VCONFKEY_SYSMAN_USB_HOST_STATUS)");
	}

	}

	/* set initial state for devices */
	input_device_number = 0;
	keyboard_chgdet_cb(NULL);
	hdmi_chgdet_cb(NULL);
	system(STORE_DEFAULT_USB_INFO);

	return 0;
}
