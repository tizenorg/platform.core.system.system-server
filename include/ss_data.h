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


#ifndef __SS_DATA_H__
#define __SS_DATA_H__

#include <Ecore.h>
#include <unistd.h>

enum {
	WIN_CREATE = 0,
	WIN_SHOW,
	WIN_DELETE,
	WIN_MAX
};

#define FM_RADIO_APP		"FM_radio"
#define MULTIMEDIA_APP		"music"
#define BLUETOOTH_APP		"bluetooth"
#define VOICERECORDER_APP	"voicerecorder"

#define MMC_DEV			"/dev/mmcblk"

#define VCONFKEY_INTERNAL_ADDED_USB_STORAGE 	"memory/Device/usbhost/added_storage_uevent"
#define VCONFKEY_INTERNAL_REMOVED_USB_STORAGE	"memory/Device/usbhost/removed_storage_uevent"

#define PREDEF_CALL			"call"
#define PREDEF_LOWMEM			"lowmem"
#define PREDEF_LOWBAT			"lowbat"
#define PREDEF_USBCON			"usbcon"
#define PREDEF_ENTERSLEEP		"entersleep"
#define PREDEF_LEAVESLEEP		"leavesleep"
#define PREDEF_POWEROFF			"poweroff"
#define PREDEF_REBOOT			"reboot"
#define PREDEF_PWROFF_POPUP		"pwroff-popup"
#define PREDEF_BACKGRD			"backgrd"
#define PREDEF_FOREGRD			"foregrd"
#define PREDEF_ACTIVE			"active"
#define PREDEF_USB_STORAGE_ADD		"usbstorage-add"
#define PREDEF_USB_STORAGE_REMOVE	"usbstorage-remove"
#define PREDEF_INACTIVE			"inactive"

#define OOMADJ_SET			"oomadj_set"
#define LOW_MEM_ACT			"low_mem_act"
#define OOM_MEM_ACT			"oom_mem_act"

#define WARNING_LOW_BAT_ACT		"warning_low_bat_act"
#define CRITICAL_LOW_BAT_ACT		"critical_low_bat_act"
#define POWER_OFF_BAT_ACT		"power_off_bat_act"
#define CHARGE_BAT_ACT			"charge_bat_act"
#define CHARGE_CHECK_ACT			"charge_check_act"
#define CHARGE_ERROR_ACT			"charge_error_act"

#define PREDEF_EARJACKCON		"earjack_predef_internal"

#define PREDEF_SET_DATETIME		"set_datetime"
#define PREDEF_SET_TIMEZONE		"set_timezone"

#define PREDEF_MOUNT_MMC		"mountmmc"
#define PREDEF_UNMOUNT_MMC		"unmountmmc"
#define PREDEF_FORMAT_MMC		"formatmmc"

#define PREDEF_SET_MAX_FREQUENCY	"set_max_frequency"     
#define PREDEF_SET_MIN_FREQUENCY	"set_min_frequency"     
#define PREDEF_RELEASE_MAX_FREQUENCY	"release_max_frequency" 
#define PREDEF_RELEASE_MIN_FREQUENCY	"release_min_frequency" 

#define OOMADJ_SU                       (-17)
#define OOMADJ_INIT                     (-16)
#define OOMADJ_FOREGRD_LOCKED           (-15)
#define OOMADJ_FOREGRD_UNLOCKED         (-10)
#define OOMADJ_BACKGRD_LOCKED           (-5)
#define OOMADJ_BACKGRD_UNLOCKED         (1)

#define OOMADJ_APP_LIMIT		(-16)

#define MOVINAND_MOUNT_POINT		"/opt/media"
#define MMC_MOUNT_POINT			"/opt/storage/sdcard"

struct ui_contention_info {

};

struct ss_main_data {
	int sysnoti_fd;
	int noti_fd;
};

#endif /* __SS_DATA_H__ */
