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

#include "ss_queue.h"
#include "ss_log.h"
#include "ss_device_handler.h"
#include "ss_device_plugin.h"
#include "ss_noti.h"
#include "include/ss_data.h"

#define BUFF_MAX					255
#define SYS_CLASS_INPUT					"/sys/class/input"

#ifdef ENABLE_EDBUS_USE
#include <E_DBus.h>
static E_DBus_Connection *conn;
#endif				/* ENABLE_EDBUS_USE */

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

static void usb_chgdet_cb(struct ss_main_data *ad)
{
	PRT_TRACE("jack - usb changed\n");
	pm_change_state(LCD_NORMAL);
	/* check charging now */
	ss_lowbat_is_charge_in_now();
	/* check current battery level */
	ss_lowbat_monitor(NULL);
	ss_action_entry_call_internal(PREDEF_USBCON, 0);
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

	if (plugin_intf->OEM_sys_get_jack_charger_online(&val) == 0) {
		if (val == 0) 
			pm_unlock_state(LCD_OFF, STAY_CUR_STATE);
		else
			pm_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
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
	pm_change_state(LCD_NORMAL);
}

static void cradle_chgdet_cb(struct ss_main_data *ad)
{
	PRT_TRACE("jack - cradle changed\n");
#ifdef CRADLE_SUPPORT
	int val;
	ss_lowbat_is_charge_in_now();
	if (plugin_intf->OEM_sys_get_jack_cradle_online(&val) == 0)
		vconf_set_int(VCONFKEY_SYSMAN_CRADLE_STATUS, !!val);
#endif
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

	if (plugin_intf->OEM_sys_get_battery_health(&val) == 0) {
		if (val != BATTERY_GOOD) {
			PRT_TRACE_ERR("Battery health status is not good (%d)", val);
			ss_action_entry_call_internal(PREDEF_LOWBAT, 1, CHARGE_ERROR_ACT);
			return;
		}
	} else {
		PRT_TRACE_ERR("failed to get battery health status");
}

	ss_action_entry_call_internal(PREDEF_LOWBAT, 1, CHARGE_CHECK_ACT);
}

#ifdef ENABLE_EDBUS_USE
static void cb_xxxxx_signaled(void *data, DBusMessage * msg)
{
	char *args;
	DBusError err;
	struct ss_main_data *ad;

	ad = data;

	dbus_error_init(&err);
	if (dbus_message_get_args
	    (msg, &err, DBUS_TYPE_STRING, &args, DBUS_TYPE_INVALID)) {
		if (!strcmp(args, "action")) ;	/* action */
	}

	return;
}
#endif				/* ENABLE_EDBUS_USE */

static void __usb_storage_cb(keynode_t *key, void *data)
{
	char *vconf_value;

	if (data == NULL) {
		PRT_TRACE("USB Storage removed");
 		vconf_value = vconf_get_str(VCONFKEY_INTERNAL_REMOVED_USB_STORAGE);
		ss_action_entry_call_internal(PREDEF_USB_STORAGE_REMOVE, 1, vconf_value);
	} else {
		PRT_TRACE("USB Storage added");
 		vconf_value = vconf_get_str(VCONFKEY_INTERNAL_ADDED_USB_STORAGE);
		ss_action_entry_call_internal(PREDEF_USB_STORAGE_ADD, 1, vconf_value);
	}
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
	ss_noti_add("device_cradle_chgdet", (void *)cradle_chgdet_cb, (void *)ad);
	ss_noti_add("device_keyboard_chgdet", (void *)keyboard_chgdet_cb, (void *)ad);
	ss_noti_add("mmcblk_add", (void *)mmc_chgdet_cb, (void *)1);
	ss_noti_add("mmcblk_remove", (void *)mmc_chgdet_cb, NULL);

	ss_noti_add("unmount_ums", (void *)ums_unmount_cb, NULL);
	ss_noti_add("device_charge_chgdet", (void *)charge_cb, (void *)ad);

	if (vconf_notify_key_changed(VCONFKEY_INTERNAL_ADDED_USB_STORAGE, (void *)__usb_storage_cb, (void *)1) < 0) {
		PRT_TRACE_ERR("Vconf notify key chaneged failed: KEY(%s)", VCONFKEY_SYSMAN_ADDED_USB_STORAGE);
	}

	if (vconf_notify_key_changed(VCONFKEY_INTERNAL_REMOVED_USB_STORAGE, (void *)__usb_storage_cb, NULL) < 0) {
		PRT_TRACE_ERR("Vconf notify key chaneged failed: KEY(%s)", VCONFKEY_SYSMAN_REMOVED_USB_STORAGE);
	}

	/* dbus noti change cb */
#ifdef ENABLE_EDBUS_USE
	e_dbus_init();
	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (!conn)
		PRT_TRACE_ERR("check system dbus running!\n");

	e_dbus_signal_handler_add(conn, NULL, "/system/uevent/xxxxx",
				  "system.uevent.xxxxx",
				  "Change", cb_xxxxx_signaled, ad);
#endif				/* ENABLE_EDBUS_USE */

	cradle_chgdet_cb(NULL);

	return 0;
}
