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


#include <stdlib.h>
#include <stdbool.h>
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
#include <libudev.h>
#include <device-node.h>
#include "queue.h"
#include "log.h"
#include "device-handler.h"
#include "noti.h"
#include "data.h"
#include "sys_pci_noti/sys_pci_noti.h"
#include "predefine.h"
#define BUFF_MAX		255
#define SYS_CLASS_INPUT		"/sys/class/input"
#define USBCON_EXEC_PATH	PREFIX"/bin/usb-server"
#define DEFAULT_USB_INFO_PATH	"/tmp/usb_default"
#define STORE_DEFAULT_USB_INFO	"usb-devices > "DEFAULT_USB_INFO_PATH
#define HDMI_NOT_SUPPORTED	(-1)
#ifdef ENABLE_EDBUS_USE
#include <E_DBus.h>
static E_DBus_Connection *conn;
#endif				/* ENABLE_EDBUS_USE */

typedef enum {
	CB_NOTI_BATT_CHARGE,
	CB_NOTI_BATT_LOW,
	CB_NOTI_BATT_FULL,
	CB_NOTI_MAX
} cb_noti_type;

typedef enum {
	CB_NOTI_OFF	= 0,
	CB_NOTI_ON	= 1
} cb_noti_onoff_type;

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

#define CHANGE_ACTION		"change"
#define ENV_FILTER		"CHGDET"
#define ENV_VALUE_USB		"usb"
#define ENV_VALUE_CHARGER 	"charger"
#define ENV_VALUE_EARJACK 	"earjack"
#define ENV_VALUE_EARKEY 	"earkey"
#define ENV_VALUE_TVOUT 	"tvout"
#define ENV_VALUE_HDMI 		"hdmi"
#define ENV_VALUE_KEYBOARD 	"keyboard"


#define ABNORMAL_POPUP_COUNTER	5

static int input_device_number;

static struct udev_monitor *mon = NULL;
static struct udev *udev = NULL;
static Ecore_Fd_Handler *ufdh = NULL;

static int uevent_control_cb(void *data, Ecore_Fd_Handler *fd_handler);

static int check_lowbat_charge_device(int bInserted)
{
	static int bChargeDeviceInserted = 0;
	int val = -1;
	int bat_state = -1;
	int ret = -1;
	if (bInserted == 1) {
		if (device_get_property(DEVICE_TYPE_POWER, PROP_POWER_CHARGE_NOW, &val) == 0) {
			if (val == 1)
				bChargeDeviceInserted = 1;
			return 0;
		}
	} else if (bInserted == 0) {
		if (device_get_property(DEVICE_TYPE_POWER, PROP_POWER_CHARGE_NOW, &val) == 0) {
			if (val == 0 && bChargeDeviceInserted == 1) {
				bChargeDeviceInserted = 0;
				//low bat popup during charging device removing
				if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, &bat_state) == 0) {
					if(bat_state < VCONFKEY_SYSMAN_BAT_NORMAL
					|| bat_state == VCONFKEY_SYSMAN_BAT_REAL_POWER_OFF) {
						bundle *b = NULL;
						b = bundle_create();
						if(bat_state == VCONFKEY_SYSMAN_BAT_REAL_POWER_OFF)
							bundle_add(b,"_SYSPOPUP_CONTENT_", "poweroff");
						else
							bundle_add(b, "_SYSPOPUP_CONTENT_", "warning");
						ret = syspopup_launch("lowbat-syspopup", b);
						if (ret < 0) {
							_I("popup lauch failed\n");
						}
						bundle_free(b);
					}
				} else {
					_E("failed to get vconf key");
					return -1;
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

	predefine_pm_change_state(LCD_NORMAL);

	/* check charging now */
	ss_lowbat_is_charge_in_now();
	/* check current battery level */
	ss_lowbat_monitor(NULL);
	ss_action_entry_call_internal(PREDEF_USBCON, 0);
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_USB_ONLINE, &val) == 0) {
		_D("jack - usb changed %d",val);
		check_lowbat_charge_device(val);
		if (val==1) {
			snprintf(params, sizeof(params), "%d", CB_NOTI_BATT_CHARGE);
			ss_launch_if_noexist("/usr/bin/sys_device_noti", params);
			_D("usb device notification");
		}
	} else {
		_E("fail to get usb_online status");
	}
}

static void __sync_usb_status(void)
{
	int val = -1;
	int status = -1;
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_USB_ONLINE, &val) != 0 ||
	    vconf_get_int(VCONFKEY_SYSMAN_USB_STATUS,&status) != 0)
		return;
	if ((val == 1 && status == VCONFKEY_SYSMAN_USB_DISCONNECTED) ||
	    (val == 0 && status == VCONFKEY_SYSMAN_USB_AVAILABLE))
		ss_action_entry_call_internal(PREDEF_USBCON, 0);
}

static void ta_chgdet_cb(struct ss_main_data *ad)
{
	int val = -1;
	int ret = -1;
	int bat_state = VCONFKEY_SYSMAN_BAT_NORMAL;
	char params[BUFF_MAX];

	predefine_pm_change_state(LCD_NORMAL);

	/* check charging now */
	ss_lowbat_is_charge_in_now();
	/* check current battery level */
	ss_lowbat_monitor(NULL);

	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_TA_ONLINE, &val) == 0) {
		_D("jack - ta changed %d",val);
		check_lowbat_charge_device(val);
		vconf_set_int(VCONFKEY_SYSMAN_CHARGER_STATUS, val);
		if (val == 0) {
			pm_unlock_state(LCD_OFF, STAY_CUR_STATE);
		} else {
			pm_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
			snprintf(params, sizeof(params), "%d", CB_NOTI_BATT_CHARGE);
			ss_launch_if_noexist("/usr/bin/sys_device_noti", params);
			_D("ta device notification");
		}
		__sync_usb_status();
	}
	else
		_E("failed to get ta status\n");
}

static void earjack_chgdet_cb(struct ss_main_data *ad)
{
	_D("jack - earjack changed\n");
	ss_action_entry_call_internal(PREDEF_EARJACKCON, 0);
}

static void earkey_chgdet_cb(struct ss_main_data *ad)
{
	int val;
	_D("jack - earkey changed\n");
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_EARKEY_ONLINE, &val) == 0)
		vconf_set_int(VCONFKEY_SYSMAN_EARJACKKEY, val);
}

static void tvout_chgdet_cb(struct ss_main_data *ad)
{
	_D("jack - tvout changed\n");
	pm_change_state(LCD_NORMAL);
}

static void hdmi_chgdet_cb(struct ss_main_data *ad)
{
	int val;
	int ret = -1;

	pm_change_state(LCD_NORMAL);
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_HDMI_SUPPORT, &val) == 0) {
		if (val!=1) {
			_E("target is not support HDMI");
			vconf_set_int(VCONFKEY_SYSMAN_HDMI, HDMI_NOT_SUPPORTED);
			return;
		}
	}
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_HDMI_ONLINE, &val) == 0) {
		_D("jack - hdmi changed %d",val);
		vconf_set_int(VCONFKEY_SYSMAN_HDMI,val);
		if(val == 1)
			pm_lock_state(LCD_NORMAL, GOTO_STATE_NOW, 0);
		else
			pm_unlock_state(LCD_NORMAL, PM_SLEEP_MARGIN);
	} else {
		_E("failed to get hdmi_online status");
	}
}

static void keyboard_chgdet_cb(struct ss_main_data *ad)
{
	int val = -1;

	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_KEYBOARD_ONLINE, &val) == 0) {
		_D("jack - keyboard changed %d",val);
		if(val != 1)
			val = 0;
		vconf_set_int(VCONFKEY_SYSMAN_SLIDING_KEYBOARD, val);
	} else {
		vconf_set_int(VCONFKEY_SYSMAN_SLIDING_KEYBOARD, VCONFKEY_SYSMAN_SLIDING_KEYBOARD_NOT_SUPPORTED);
	}
}

static void mmc_chgdet_cb(void *data)
{
	static bool first, inserted;
	int mmc_status;
	int ret = -1;
	int val = -1;

	/* at first time, this part will be judge mmc is already inserted or not. */
	if (!first) {
		vconf_get_int(VCONFKEY_SYSMAN_MMC_STATUS, &mmc_status);
		if (mmc_status == VCONFKEY_SYSMAN_MMC_MOUNTED)
			inserted = true;
		first = true;
	}

	if (data == NULL) {
		/* when removed mmc, emul kernel notify twice
		 * So this code ignores second event */
		if (!inserted)
			return;
		inserted = false;
		_D("mmc removed");
		ss_mmc_removed();
	} else {
		/* when inserted mmc, emul kernel notify twice(insert, changed)
		 * So this code ignores second event */
		if (inserted)
			return;
		inserted = true;
		_D("mmc added");
		ret = ss_mmc_inserted();
		if (ret == -1) {
			vconf_get_int(VCONFKEY_SYSMAN_MMC_MOUNT,&val);
			if (val == VCONFKEY_SYSMAN_MMC_MOUNT_FAILED) {
				bundle *b = NULL;
				b = bundle_create();
				if (b == NULL) {
					_E("error bundle_create()");
					return;
				}
				bundle_add(b, "_SYSPOPUP_CONTENT_", "mounterr");
				ret = syspopup_launch("mmc-syspopup", b);
				if (ret < 0) {
					_E("popup launch failed");
				}
				bundle_free(b);
			} else if (val == VCONFKEY_SYSMAN_MMC_MOUNT_COMPLETED) {
				bundle *b = NULL;
				b = bundle_create();
				if (b == NULL) {
					_E("error bundle_create()");
					return;
				}
				bundle_add(b, "_SYSPOPUP_CONTENT_", "mountrdonly");
				ret = syspopup_launch("mmc-syspopup", b);
				if (ret < 0) {
					_E("popup launch failed");
				}
				bundle_free(b);
			}
		}
	}
}

static void ums_unmount_cb(void *data)
{
	umount(MOVINAND_MOUNT_POINT);
}

static int __check_abnormal_popup_launch(void)
{
	static int noti_count = 0;
	if (noti_count >= ABNORMAL_POPUP_COUNTER) {
		noti_count = 0;
		return 0;
	} else {
		noti_count++;
		return -EAGAIN;
	}
}

static void charge_cb(struct ss_main_data *ad)
{
	int val = -1;
	int charge_now = -1;
	int capacity = -1;
	char params[BUFF_MAX];
	static int bat_full_noti = 0;

	ss_lowbat_monitor(NULL);

	if (device_get_property(DEVICE_TYPE_POWER, PROP_POWER_CHARGE_NOW, &charge_now) != 0 ||
	    device_get_property(DEVICE_TYPE_POWER, PROP_POWER_CAPACITY, &capacity) != 0)
		_E("fail to get battery node value");
	if (charge_now == 0 && capacity == 0) {
		_E("target will be shut down");
		ss_action_entry_call_internal(PREDEF_LOWBAT, 1, POWER_OFF_BAT_ACT);
		return;
	}

	if (device_get_property(DEVICE_TYPE_POWER, PROP_POWER_HEALTH, &val) == 0) {
		if (val==BATTERY_OVERHEAT || val==BATTERY_COLD) {
			_E("Battery health status is not good (%d)", val);

			if (__check_abnormal_popup_launch() != 0)
				return;

			if (device_get_property(DEVICE_TYPE_POWER, PROP_POWER_CAPACITY, &val) == 0 && val <= 0)
				ss_action_entry_call_internal(PREDEF_LOWBAT, 1, POWER_OFF_BAT_ACT);
			else
				ss_action_entry_call_internal(PREDEF_LOWBAT, 1, CHARGE_ERROR_ACT);
			return;
		}
	} else {
		_E("failed to get battery health status");
	}
	device_get_property(DEVICE_TYPE_POWER, PROP_POWER_CHARGE_FULL, &val);
	if (val==0) {
		if (bat_full_noti==1) {
			snprintf(params, sizeof(params), "%d %d", CB_NOTI_BATT_FULL, CB_NOTI_OFF);
			ss_launch_if_noexist("/usr/bin/sys_device_noti", params);
		}
		bat_full_noti = 0;
	} else {
		if (val==1 && bat_full_noti==0) {
			bat_full_noti = 1;
			_D("battery full noti");
			snprintf(params, sizeof(params), "%d %d", CB_NOTI_BATT_FULL, CB_NOTI_ON);
			ss_launch_if_noexist("/usr/bin/sys_device_noti", params);
		}
	}
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

static void usb_host_chgdet_cb(keynode_t *in_key, struct ss_main_data *ad)
{
	_D("ENTER: usb_host_chgdet_cb()");
	int status;
	int ret = vconf_get_int(VCONFKEY_SYSMAN_USB_HOST_STATUS, &status);
	if (ret != 0) {
		_E("vconf get failed(VCONFKEY_SYSMAN_USB_HOST_STATUS)\n");
		return ;
	}

	if(VCONFKEY_SYSMAN_USB_HOST_CONNECTED == status) {
		int pid = ss_launch_if_noexist(USBCON_EXEC_PATH, NULL);
		if (pid < 0) {
			_D("usb-server launching failed\n");
			return;
		}
	}
	_D("EXIT: usb_host_chgdet_cb()");
}

static void usb_host_add_cb()
{
	_D("ENTER: usb_host_add_cb()\n");
	int status;
	int ret = vconf_get_int(VCONFKEY_SYSMAN_USB_HOST_STATUS, &status);
	if (ret != 0) {
		_D("vconf get failed ()\n");
		return;
	}

	if (-1 == status) { /* '-1' means that USB host mode is not loaded yet */
		_D("This usb device is connected defaultly\n");

		ret = system(STORE_DEFAULT_USB_INFO);
		_D("Return value of usb-devices: %d\n", ret);
		if (0 != access(DEFAULT_USB_INFO_PATH, F_OK)) {
			ret = system(STORE_DEFAULT_USB_INFO);
			_D("Return value of usb-devices: %d\n", ret);
		}
	}
	_D("EXIT: usb_host_add_cb()\n");
}

static int uevent_control_stop(int ufd)
{
	if (ufdh) {
		ecore_main_fd_handler_del(ufdh);
		ufdh = NULL;
	}
	if (ufd >= 0) {
		close(ufd);
		ufd = -1;
	}
	if (mon) {
		udev_monitor_unref(mon);
		mon = NULL;
	}
	if (udev) {
		udev_unref(udev);
		udev = NULL;
	}
	return 0;
}

static int uevent_control_start(void)
{
	int ufd = -1;

	udev = udev_new();
	if (!udev) {
		_E("error create udev");
		return -1;
	}

	mon = udev_monitor_new_from_netlink(udev, "kernel");
	if (mon == NULL) {
		_E("error udev_monitor create");
		uevent_control_stop(-1);
		return -1;
	}

	udev_monitor_set_receive_buffer_size(mon, 1024);
	if (udev_monitor_filter_add_match_subsystem_devtype(mon, "platform", NULL) < 0) {
		_E("error apply subsystem filter");
		uevent_control_stop(-1);
		return -1;
	}

	ufd = udev_monitor_get_fd(mon);
	if (ufd == -1) {
		_E("error udev_monitor_get_fd");
		uevent_control_stop(ufd);
		return -1;
	}

	ufdh = ecore_main_fd_handler_add(ufd, ECORE_FD_READ, uevent_control_cb, NULL, NULL, NULL);
	if (!ufdh) {
		_E("error ecore_main_fd_handler_add");
		uevent_control_stop(ufd);
		return -1;
	}

	if (udev_monitor_enable_receiving(mon) < 0) {
		_E("error unable to subscribe to udev events");
		uevent_control_stop(ufd);
		return -1;
	}

	return 0;
}

static int uevent_control_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	struct udev_device *dev = NULL;
	struct udev_list_entry *list_entry = NULL;
	char *env_name = NULL;
	char *env_value = NULL;
	int ufd = -1;
	int ret = -1;

	if (!ecore_main_fd_handler_active_get(fd_handler,ECORE_FD_READ))
		return -1;
	if ((ufd = ecore_main_fd_handler_fd_get(fd_handler)) == -1)
		return -1;
	if ((dev = udev_monitor_receive_device(mon)) == NULL)
		return -1;

	udev_list_entry_foreach(list_entry,udev_device_get_properties_list_entry(dev)) {
		env_name = udev_list_entry_get_name(list_entry);
		if (strncmp(env_name, ENV_FILTER, strlen(ENV_FILTER)) == 0) {
			env_value = udev_list_entry_get_value(list_entry);
			ret = 0;
			break;
		}
	}

	if (ret != 0) {
		udev_device_unref(dev);
		return -1;
	}

	_D("UEVENT DETECTED (%s)",env_value);
	ss_action_entry_call_internal(PREDEF_DEVICE_CHANGED,1,env_value);

	udev_device_unref(dev);
	uevent_control_stop(ufd);
	uevent_control_start();

	return 0;
}

int changed_device_def_predefine_action(int argc, char **argv)
{
	if (argc != 1 || argv[0] == NULL) {
		_E("param is failed");
		return -1;
	}

	if (strncmp(argv[0], ENV_VALUE_USB, strlen(ENV_VALUE_USB)) == 0)
		usb_chgdet_cb(NULL);
	if (strncmp(argv[0], ENV_VALUE_CHARGER, strlen(ENV_VALUE_CHARGER)) == 0)
		ta_chgdet_cb(NULL);
	if (strncmp(argv[0], ENV_VALUE_EARJACK, strlen(ENV_VALUE_EARJACK)) == 0)
		earjack_chgdet_cb(NULL);
	if (strncmp(argv[0], ENV_VALUE_EARKEY, strlen(ENV_VALUE_EARKEY)) == 0)
		earkey_chgdet_cb(NULL);
	if (strncmp(argv[0], ENV_VALUE_TVOUT, strlen(ENV_VALUE_TVOUT)) == 0)
		tvout_chgdet_cb(NULL);
	if (strncmp(argv[0], ENV_VALUE_HDMI, strlen(ENV_VALUE_HDMI)) == 0)
		hdmi_chgdet_cb(NULL);
	if (strncmp(argv[0], ENV_VALUE_KEYBOARD, strlen(ENV_VALUE_KEYBOARD)) == 0)
		keyboard_chgdet_cb(NULL);

	return 0;
}

static void pci_keyboard_add_cb(struct ss_main_data *ad)
{
	char params[BUFF_MAX];
	_D("pci- keyboard inserted\n");
	pm_change_state(LCD_NORMAL);

	snprintf(params, sizeof(params), "%d", CB_NOTI_PCI_INSERTED);
	ss_launch_if_noexist("/usr/bin/sys_pci_noti", params);

}
static void pci_keyboard_remove_cb(struct ss_main_data *ad)
{
	char params[BUFF_MAX];
	_D("pci- keyboard removed\n");
	pm_change_state(LCD_NORMAL);

	snprintf(params, sizeof(params), "%d", CB_NOTI_PCI_REMOVED);
	ss_launch_if_noexist("/usr/bin/sys_pci_noti", params);
}
int ss_device_change_init(struct ss_main_data *ad)
{
	ss_action_entry_add_internal(PREDEF_DEVICE_CHANGED, changed_device_def_predefine_action, NULL, NULL);

	if (uevent_control_start() == -1) {
		_E("fail uevent control init");
		return -1;
	}
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

	ss_noti_add("device_pci_keyboard_add", (void *)pci_keyboard_add_cb, (void *)ad);
	ss_noti_add("device_pci_keyboard_remove", (void *)pci_keyboard_remove_cb, (void *)ad);

	if (vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS, usb_host_chgdet_cb, NULL) < 0) {
		_E("vconf key notify failed(VCONFKEY_SYSMAN_USB_HOST_STATUS)");
	}
	/* dbus noti change cb */
#ifdef ENABLE_EDBUS_USE
	e_dbus_init();
	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (!conn)
		_E("check system dbus running!\n");

	e_dbus_signal_handler_add(conn, NULL, "/system/uevent/xxxxx",
				  "system.uevent.xxxxx",
				  "Change", cb_xxxxx_signaled, ad);
#endif				/* ENABLE_EDBUS_USE */

	/* set initial state for devices */
	input_device_number = 0;
	keyboard_chgdet_cb(NULL);
	hdmi_chgdet_cb(NULL);
	system(STORE_DEFAULT_USB_INFO);

	return 0;
}
