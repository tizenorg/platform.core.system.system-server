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


#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <vconf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <bundle.h>
#include <dirent.h>
#include <libudev.h>
#include <fnmatch.h>
#include "dd-deviced.h"
#include <device-node.h>
#include "queue.h"
#include "log.h"
#include "device-notifier.h"
#include "device-handler.h"
#include "noti.h"
#include "data.h"
#include "predefine.h"
#include "display/poll.h"
#include "devices.h"
#include "sys_pci_noti/sys_pci_noti.h"
#include "udev.h"
#include "common.h"
#include "proc/proc-handler.h"

#define PREDEF_USBCON			"usbcon"
#define PREDEF_EARJACKCON		"earjack_predef_internal"
#define PREDEF_DEVICE_CHANGED		"device_changed"
#define PREDEF_BATTERY_CF_OPENED	"battery_cf_opened"

#define TVOUT_X_BIN			"/usr/bin/xberc"
#define TVOUT_FLAG			0x00000001

#define MOVINAND_MOUNT_POINT		"/opt/media"
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

#define SWITCH_DEVICE_USB 	"usb_cable"


#define ABNORMAL_POPUP_COUNTER	5

static int ss_flags = 0;

static int input_device_number;

static struct udev_monitor *mon = NULL;
static struct udev *udev = NULL;
static Ecore_Fd_Handler *ufdh = NULL;

static struct extcon_device {
	const enum extcon_type type;
	const char *str;
	int fd;
	int count;
} extcon_devices[] = {
	{ EXTCON_TA, "/csa/factory/batt_cable_count", 0, 0},
	{ EXTCON_EARJACK, "/csa/factory/earjack_count", 0, 0},
};

static Eina_Bool uevent_control_cb(void *data, Ecore_Fd_Handler *fd_handler);
extern int battery_power_off_act(void *data);
extern int battery_charge_err_act(void *data);
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
				// TODO : display a popup
			}
			return 0;
		}
	}
	return -1;
}

int extcon_set_count(int index)
{
	int r;
	int ret = 0;
	char buf[BUFF_MAX];

	extcon_devices[index].count++;

	if (extcon_devices[index].fd < 0) {
		_E("cannot open file(%s)", extcon_devices[index].str);
		return -ENOENT;
	}
	lseek(extcon_devices[index].fd, 0, SEEK_SET);
	_I("ext(%d) count %d", index, extcon_devices[index].count);
	snprintf(buf, sizeof(buf), "%d", extcon_devices[index].count);

	r = write(extcon_devices[index].fd, buf, strlen(buf));
	if (r < 0)
		ret = -EIO;
	return ret;
}

static int extcon_get_count(int index)
{
	int fd;
	int r;
	int ret = 0;
	char buf[BUFF_MAX];

	fd = open(extcon_devices[index].str, O_RDWR);
	if (fd < 0)
		return -ENOENT;

	r = read(fd, buf, BUFF_MAX);
	if ((r >= 0) && (r < BUFF_MAX))
		buf[r] = '\0';
	else
		ret = -EIO;

	if (ret != 0) {
		close(fd);
		return ret;
	}
	extcon_devices[index].fd = fd;
	extcon_devices[index].count = atoi(buf);
	_I("get extcon(%d:%x) count %d",
		index, extcon_devices[index].fd, extcon_devices[index].count);

	return ret;
}

static int extcon_create_count(int index)
{
	int fd;
	int r;
	int ret = 0;
	char buf[BUFF_MAX];
	fd = open(extcon_devices[index].str, O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		_E("cannot open file(%s)", extcon_devices[index].str);
		return -ENOENT;
	}
	snprintf(buf, sizeof(buf), "%d", extcon_devices[index].count);
	r = write(fd, buf, strlen(buf));
	if (r < 0)
		ret = -EIO;

	if (ret != 0) {
		close(fd);
		_E("cannot write file(%s)", extcon_devices[index].str);
		return ret;
	}
	extcon_devices[index].fd = fd;
	_I("create extcon(%d:%x) %s",
		index, extcon_devices[index].fd, extcon_devices[index].str);
	return ret;
}

static int extcon_count_init(void)
{
	int i;
	int ret = 0;
	for (i = 0; i < ARRAY_SIZE(extcon_devices); i++) {
		if (extcon_get_count(i) >= 0)
			continue;
		ret = extcon_create_count(i);
		if (ret < 0)
			break;
	}
	return ret;
}

static void usb_chgdet_cb(void *data)
{
	int val = -1;
	int ret = 0;
	char params[BUFF_MAX];

	predefine_pm_change_state(LCD_NORMAL);

	/* check charging now */
	ss_lowbat_is_charge_in_now();
	/* check current battery level */
	ss_lowbat_monitor(NULL);
	action_entry_call_internal(PREDEF_USBCON, 0);

	if (data == NULL)
		ret = device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_USB_ONLINE, &val);
	else
		val = (int)data;
	if (ret == 0) {
		_I("jack - usb changed %d",val);
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
	if ((device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_USB_ONLINE, &val) != 0) ||
	    vconf_get_int(VCONFKEY_SYSMAN_USB_STATUS,&status) != 0)
		return;
	if ((val == 1 && status == VCONFKEY_SYSMAN_USB_DISCONNECTED) ||
	    (val == 0 && status == VCONFKEY_SYSMAN_USB_AVAILABLE))
		action_entry_call_internal(PREDEF_USBCON, 0);
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
		_I("jack - ta changed %d",val);
		check_lowbat_charge_device(val);
		vconf_set_int(VCONFKEY_SYSMAN_CHARGER_STATUS, val);
		if (val == 0) {
			pm_unlock_internal(getpid(), LCD_OFF, STAY_CUR_STATE);
		} else {
			pm_lock_internal(getpid(), LCD_OFF, STAY_CUR_STATE, 0);
			snprintf(params, sizeof(params), "%d", CB_NOTI_BATT_CHARGE);
			ss_launch_if_noexist("/usr/bin/sys_device_noti", params);
			_D("ta device notification");
			device_notify(DEVICE_NOTIFIER_TA, (void *)TRUE);
		}
		__sync_usb_status();
	}
	else
		_E("failed to get ta status");
}

static void earjack_chgdet_cb(struct ss_main_data *ad)
{
	_I("jack - earjack changed");
	action_entry_call_internal(PREDEF_EARJACKCON, 0);
}

static void earkey_chgdet_cb(struct ss_main_data *ad)
{
	int val;
	_I("jack - earkey changed");
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_EARKEY_ONLINE, &val) == 0)
		vconf_set_int(VCONFKEY_SYSMAN_EARJACKKEY, val);
}

static void tvout_chgdet_cb(struct ss_main_data *ad)
{
	_I("jack - tvout changed");
	pm_change_internal(getpid(), LCD_NORMAL);
}

static void hdmi_chgdet_cb(struct ss_main_data *ad)
{
	int val;
	int ret = -1;

	pm_change_internal(getpid(), LCD_NORMAL);
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_HDMI_SUPPORT, &val) == 0) {
		if (val!=1) {
			_I("target is not support HDMI");
			vconf_set_int(VCONFKEY_SYSMAN_HDMI, HDMI_NOT_SUPPORTED);
			return;
		}
	}
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_HDMI_ONLINE, &val) == 0) {
		_I("jack - hdmi changed %d", val);
		vconf_set_int(VCONFKEY_SYSMAN_HDMI,val);
		if(val == 1)
			pm_lock_internal(getpid(), LCD_NORMAL, GOTO_STATE_NOW, 0);
		else
			pm_unlock_internal(getpid(), LCD_NORMAL, PM_SLEEP_MARGIN);
	} else {
		_E("failed to get hdmi_online status");
	}
}

static void keyboard_chgdet_cb(struct ss_main_data *ad)
{
	int val = -1;

	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_KEYBOARD_ONLINE, &val) == 0) {
		_I("jack - keyboard changed %d", val);
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
		_I("mmc removed");
		ss_mmc_removed();
	} else {
		_I("mmc added");
		if (inserted)
			return;
		inserted = true;
		_D("mmc added");
		ret = ss_mmc_inserted();
		if (ret == -1) {
			vconf_get_int(VCONFKEY_SYSMAN_MMC_MOUNT,&val);
			// TODO : display a popup
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
	int ret;
	char params[BUFF_MAX];
	static int bat_full_noti = 0;
	static int present_status = 1;

	ss_lowbat_monitor(NULL);

	if (device_get_property(DEVICE_TYPE_POWER, PROP_POWER_CHARGE_NOW, &charge_now) != 0 ||
	    device_get_property(DEVICE_TYPE_POWER, PROP_POWER_CAPACITY, &capacity) != 0)
		_E("fail to get battery node value");
	if (charge_now == 0 && capacity == 0) {
		_I("target will be shut down");
		battery_power_off_act(NULL);
		return;
	}

	ret = device_get_property(DEVICE_TYPE_POWER, PROP_POWER_PRESENT, &val);
	if (ret != 0)
		_E("fail to get battery present value");
	if (val == 0 && present_status == 1) {
		present_status = 0;
		_I("battery cf is opened");
		if (charge_now)
			action_entry_call_internal(PREDEF_BATTERY_CF_OPENED, 0);
	}

	if (val == 1 && present_status == 0) {
		present_status = 1;
		_I("battery cf is closed again");
	}

	if (device_get_property(DEVICE_TYPE_POWER, PROP_POWER_HEALTH, &val) == 0) {
		if (val==BATTERY_OVERHEAT || val==BATTERY_COLD) {
			_I("Battery health status is not good (%d)", val);

			if (__check_abnormal_popup_launch() != 0)
				return;

			if (device_get_property(DEVICE_TYPE_POWER, PROP_POWER_CAPACITY, &val) == 0 && val <= 0)
				battery_power_off_act(NULL);
			else
				battery_charge_err_act(NULL);
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
	int status;
	int ret = vconf_get_int(VCONFKEY_SYSMAN_USB_HOST_STATUS, &status);
	if (ret != 0) {
		_E("vconf get failed(VCONFKEY_SYSMAN_USB_HOST_STATUS)");
		return ;
	}

	if(VCONFKEY_SYSMAN_USB_HOST_CONNECTED == status) {
		int pid = ss_launch_if_noexist(USBCON_EXEC_PATH, NULL);
		if (pid < 0) {
			_E("usb-server launching failed");
			return;
		}
	}
}

static void usb_host_add_cb()
{
	int status;
	int ret = vconf_get_int(VCONFKEY_SYSMAN_USB_HOST_STATUS, &status);
	if (ret != 0) {
		_E("vconf get failed ()");
		return;
	}

	if (-1 == status) { /* '-1' means that USB host mode is not loaded yet */
		_D("This usb device is connected defaultly");

		ret = system(STORE_DEFAULT_USB_INFO);
		_D("Return value of usb-devices: %d\n", ret);
		if (0 != access(DEFAULT_USB_INFO_PATH, F_OK)) {
			ret = system(STORE_DEFAULT_USB_INFO);
			_D("Return value of usb-devices: %d\n", ret);
		}
	}
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
		return -EINVAL;
	}

	mon = udev_monitor_new_from_netlink(udev, "kernel");
	if (mon == NULL) {
		_E("error udev_monitor create");
		uevent_control_stop(-1);
		return -EINVAL;
	}

	if (udev_monitor_set_receive_buffer_size(mon, 1024) != 0) {
		_E("fail to set receive buffer size");
		return -EINVAL;
	}

	if (udev_monitor_filter_add_match_subsystem_devtype(mon, "platform", NULL) < 0 ||
		udev_monitor_filter_add_match_subsystem_devtype(mon, "input", NULL) < 0) {
		_E("error apply subsystem filter");
		uevent_control_stop(-1);
		return -1;
	}

	ufd = udev_monitor_get_fd(mon);
	if (ufd == -1) {
		_E("error udev_monitor_get_fd");
		uevent_control_stop(ufd);
		return -EINVAL;
	}

	ufdh = ecore_main_fd_handler_add(ufd, ECORE_FD_READ, uevent_control_cb, NULL, NULL, NULL);
	if (!ufdh) {
		_E("error ecore_main_fd_handler_add");
		uevent_control_stop(ufd);
		return -EINVAL;
	}

	if (udev_monitor_enable_receiving(mon) < 0) {
		_E("error unable to subscribe to udev events");
		uevent_control_stop(ufd);
		return -EINVAL;
	}

	return 0;
}

static Eina_Bool uevent_control_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
	struct udev_device *dev = NULL;
	struct udev_list_entry *list_entry = NULL;
	const char *env_name = NULL;
	const char *env_value = NULL;
	const char *devpath;
	const char *devnode;
	const char *action;
	int ufd = -1;
	int ret = -1;
	int i, len;

	if (!ecore_main_fd_handler_active_get(fd_handler,ECORE_FD_READ))
		goto out;
	if ((ufd = ecore_main_fd_handler_fd_get(fd_handler)) == -1)
		goto out;
	if ((dev = udev_monitor_receive_device(mon)) == NULL)
		goto out;

	env_name = udev_device_get_subsystem(dev);
	if (strncmp(env_name, INPUT_SUBSYSTEM, strlen(INPUT_SUBSYSTEM)) == 0) {
		char *devpath = udev_device_get_devpath(dev);
		/* check new input device */
		if (!fnmatch(INPUT_PATH, devpath, 0)) {
			action = udev_device_get_action(dev);
			devnode = udev_device_get_devnode(dev);
			if (!strcmp(action, ADD))
				device_notify(DEVICE_NOTIFIER_INPUT_ADD, (void *)devnode);
			else if (!strcmp(action, REMOVE))
				device_notify(DEVICE_NOTIFIER_INPUT_REMOVE, (void *)devnode);
			udev_device_unref(dev);
			uevent_control_stop(ufd);
			uevent_control_start();
			return EINA_TRUE;
		}
	}

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
		goto out;
	}

	_I("UEVENT DETECTED (%s)", env_value);
	action_entry_call_internal(PREDEF_DEVICE_CHANGED,1,env_value);

	udev_device_unref(dev);
	uevent_control_stop(ufd);
	uevent_control_start();
out:
	return EINA_TRUE;
}

int changed_device_def_predefine_action(int argc, char **argv)
{
	int state;

	if (argc ==2 && argv[0] != NULL && argv[1] != NULL) {
		state = atoi(argv[1]);
		goto switch_device;
	}

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

	switch_device:
		if (strncmp(argv[0], SWITCH_DEVICE_USB, strlen(SWITCH_DEVICE_USB)) == 0)
			usb_chgdet_cb((void *)state);
		return 0;
}

int usbcon_def_predefine_action(int argc, char **argv)
{
	int pid;
	int val = -1;
	int ret = -1;
	int bat_state = VCONFKEY_SYSMAN_BAT_NORMAL;

	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_USB_ONLINE, &val) == 0) {
		if (val == 0) {
			vconf_set_int(VCONFKEY_SYSMAN_USB_STATUS,
				      VCONFKEY_SYSMAN_USB_DISCONNECTED);
			pm_unlock_internal(getpid(), LCD_OFF, STAY_CUR_STATE);
			return 0;
		}

		if ( vconf_get_int(VCONFKEY_SYSMAN_USB_STATUS, &val) == 0 && val == VCONFKEY_SYSMAN_USB_AVAILABLE)
			return 0;

		vconf_set_int(VCONFKEY_SYSMAN_USB_STATUS,
			      VCONFKEY_SYSMAN_USB_AVAILABLE);
		pm_lock_internal(getpid(), LCD_OFF, STAY_CUR_STATE, 0);
		pid = ss_launch_if_noexist(USBCON_EXEC_PATH, NULL);
		if (pid < 0) {
			_E("usb predefine action failed\n");
			return -1;
		}
		return pid;
	}
	_E("failed to get usb status\n");
	return -1;
}

int earjackcon_def_predefine_action(int argc, char **argv)
{
	int val;

	_E("earjack_normal predefine action");
	if (device_get_property(DEVICE_TYPE_EXTCON, PROP_EXTCON_EARJACK_ONLINE, &val) == 0) {
		if (CONNECTED(val))
			extcon_set_count(EXTCON_EARJACK);
		return vconf_set_int(VCONFKEY_SYSMAN_EARJACK, val);
	}

	return -1;
}

static int battery_def_cf_opened_actioin(int argc, char **argv)
{
	// TODO : display a popup
out:
	return -EINVAL;
}

static void pci_keyboard_add_cb(struct ss_main_data *ad)
{
	char params[BUFF_MAX];
	_D("pci- keyboard inserted\n");
	pm_change_internal(getpid(), LCD_NORMAL);

	snprintf(params, sizeof(params), "%d", CB_NOTI_PCI_INSERTED);
	ss_launch_if_noexist("/usr/bin/sys_pci_noti", params);

}
static void pci_keyboard_remove_cb(struct ss_main_data *ad)
{
	char params[BUFF_MAX];
	_D("pci- keyboard removed\n");
	pm_change_internal(getpid(), LCD_NORMAL);

	snprintf(params, sizeof(params), "%d", CB_NOTI_PCI_REMOVED);
	ss_launch_if_noexist("/usr/bin/sys_pci_noti", params);
}

static void device_change_init(void *data)
{
	if (extcon_count_init() != 0)
		_E("fail to init extcon files");
	action_entry_add_internal(PREDEF_USBCON, usbcon_def_predefine_action, NULL, NULL);
	action_entry_add_internal(PREDEF_EARJACKCON, earjackcon_def_predefine_action, NULL, NULL);
	action_entry_add_internal(PREDEF_BATTERY_CF_OPENED, battery_def_cf_opened_actioin, NULL, NULL);
	action_entry_add_internal(PREDEF_DEVICE_CHANGED, changed_device_def_predefine_action, NULL, NULL);

	if (uevent_control_start() != 0) {
		_E("fail uevent control init");
		return;
	}

	/* for simple noti change cb */
	ss_noti_add("device_usb_chgdet", (void *)usb_chgdet_cb, NULL);
	ss_noti_add("device_ta_chgdet", (void *)ta_chgdet_cb, data);
	ss_noti_add("device_earjack_chgdet", (void *)earjack_chgdet_cb, data);
	ss_noti_add("device_earkey_chgdet", (void *)earkey_chgdet_cb, data);
	ss_noti_add("device_tvout_chgdet", (void *)tvout_chgdet_cb, data);
	ss_noti_add("device_hdmi_chgdet", (void *)hdmi_chgdet_cb, data);
	ss_noti_add("device_keyboard_chgdet", (void *)keyboard_chgdet_cb, data);
	ss_noti_add("device_usb_host_add", (void *)usb_host_add_cb, data);
	ss_noti_add("mmcblk_add", (void *)mmc_chgdet_cb, (void *)1);
	ss_noti_add("mmcblk_remove", (void *)mmc_chgdet_cb, NULL);
	ss_noti_add("unmount_ums", (void *)ums_unmount_cb, NULL);
	ss_noti_add("device_charge_chgdet", (void *)charge_cb, data);
	ss_noti_add("device_pci_keyboard_add", (void *)pci_keyboard_add_cb, data);
	ss_noti_add("device_pci_keyboard_remove", (void *)pci_keyboard_remove_cb, data);

	if (vconf_notify_key_changed(VCONFKEY_SYSMAN_USB_HOST_STATUS,
		(void *)usb_host_chgdet_cb, NULL) < 0) {
		_E("vconf key notify failed(VCONFKEY_SYSMAN_USB_HOST_STATUS)");
	}

	/* check and set earjack init status */
	earjackcon_def_predefine_action(0, NULL);
	/* dbus noti change cb */
#ifdef ENABLE_EDBUS_USE
	e_dbus_init();
	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (!conn)
		_E("check system dbus running!\n");

	e_dbus_signal_handler_add(conn, NULL, "/system/uevent/xxxxx",
				  "system.uevent.xxxxx",
				  "Change", cb_xxxxx_signaled, data);
#endif				/* ENABLE_EDBUS_USE */

	/* set initial state for devices */
	input_device_number = 0;
	keyboard_chgdet_cb(NULL);
	hdmi_chgdet_cb(NULL);
	system(STORE_DEFAULT_USB_INFO);
}

static void device_change_exit(void *data)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(extcon_devices); i++) {
		if (extcon_devices[i].fd <= 0)
			continue;
		close(extcon_devices[i].fd);
	}

}

const struct device_ops change_device_ops = {
	.init = device_change_init,
	.exit = device_change_exit,
};
