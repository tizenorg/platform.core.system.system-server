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


#include <assert.h>
#include <limits.h>
#include <heynoti.h>
#include <vconf.h>
#include <sysman.h>
#include <fcntl.h>

#include "ss_log.h"
#include "ss_launch.h"
#include "ss_noti.h"
#include "ss_queue.h"
#include "ss_device_plugin.h"
#include "include/ss_data.h"

#define BAT_MON_INTERVAL		30
#define BAT_MON_INTERVAL_MIN		2

#define BATTERY_CHARGING		65535
#define BATTERY_UNKNOWN			-1
#define	BATTERY_FULL			100
#define	BATTERY_NORMAL			100
#define	BATTERY_WARNING_LOW		15
#define	BATTERY_CRITICAL_LOW		5
#define	BATTERY_POWER_OFF		1
#define	BATTERY_REAL_POWER_OFF	0

#define MAX_BATTERY_ERROR		10
#define RESET_RETRY_COUNT		3

#define LOWBAT_EXEC_PATH		PREFIX"/bin/lowbatt-popup"

static int battery_level_table[] = {
	5,			/* BATTERY_LEVEL0 */
	15,			/* 1 */
	25,			/* 2 */
	40,			/* 3 */
	60,			/* 4 */
	80,			/* 5 */
	100,			/* 6 */
};

#define _SYS_LOW_POWER "LOW_POWER"

struct lowbat_process_entry {
	unsigned cur_bat_state;
	unsigned new_bat_state;
	int (*action) (void *);
};

static Ecore_Timer *lowbat_timer;
static int cur_bat_state = BATTERY_UNKNOWN;
static int cur_bat_capacity = -1;

static int bat_err_count = 0;

static int battery_warning_low_act(void *ad);
static int battery_critical_low_act(void *ad);
static int battery_power_off_act(void *ad);

static struct lowbat_process_entry lpe[] = {
	{BATTERY_NORMAL, BATTERY_WARNING_LOW, battery_warning_low_act},
	{BATTERY_WARNING_LOW, BATTERY_CRITICAL_LOW, battery_critical_low_act},
	{BATTERY_CRITICAL_LOW,	BATTERY_POWER_OFF,		battery_critical_low_act},
	{BATTERY_POWER_OFF,		BATTERY_REAL_POWER_OFF,	battery_power_off_act},
	{BATTERY_NORMAL, BATTERY_CRITICAL_LOW, battery_critical_low_act},
	{BATTERY_WARNING_LOW,	BATTERY_POWER_OFF,		battery_critical_low_act},
	{BATTERY_CRITICAL_LOW,	BATTERY_REAL_POWER_OFF,	battery_power_off_act},
	{BATTERY_NORMAL,		BATTERY_POWER_OFF,		battery_critical_low_act},
	{BATTERY_WARNING_LOW,	BATTERY_REAL_POWER_OFF,	battery_power_off_act},
	{BATTERY_NORMAL,		BATTERY_REAL_POWER_OFF,	battery_power_off_act},
};

static int battery_warning_low_act(void *data)
{
	char lowbat_noti_name[NAME_MAX];

	heynoti_get_snoti_name(_SYS_LOW_POWER, lowbat_noti_name, NAME_MAX);
	ss_noti_send(lowbat_noti_name);

	ss_action_entry_call_internal(PREDEF_LOWBAT, 1, WARNING_LOW_BAT_ACT);
	return 0;
}

static int battery_critical_low_act(void *data)
{
	ss_action_entry_call_internal(PREDEF_LOWBAT, 1, CRITICAL_LOW_BAT_ACT);
	return 0;
}

static int battery_power_off_act(void *data)
{
	ss_action_entry_call_internal(PREDEF_LOWBAT, 1,	POWER_OFF_BAT_ACT);
	return 0;
}

static int battery_charge_act(void *data)
{
	return 0;
}

int ss_lowbat_set_charge_on(int onoff)
{
	if(vconf_set_int(VCONFKEY_SYSMAN_BATTERY_CHARGE_NOW, onoff)!=0) {
		PRT_TRACE_ERR("fail to set charge vconf value");
		return -1;
	}
	return 0;
}

int ss_lowbat_is_charge_in_now()
{
	int val = 0;
	if (0 > plugin_intf->OEM_sys_get_battery_charge_now(&val)) {
		PRT_TRACE_ERR("fail to read charge now from kernel");
		ss_lowbat_set_charge_on(0);
		return 0;
	}

	if (val == 1) {
		ss_lowbat_set_charge_on(1);
		return 1;
	} else {
		ss_lowbat_set_charge_on(0);
		return 0;
	}
}

static int lowbat_process(int bat_percent, void *ad)
{
	int new_bat_capacity;
	int new_bat_state;
	int vconf_state = -1;
	int bat_full = -1;
	int i, ret = 0;
	int val = 0;
	new_bat_capacity = bat_percent;
	if (new_bat_capacity < 0)
		return -1;
	if (new_bat_capacity != cur_bat_capacity) {
		PRT_TRACE("[BAT_MON] cur = %d new = %d", cur_bat_capacity, new_bat_capacity);
		if (vconf_set_int(VCONFKEY_SYSMAN_BATTERY_CAPACITY, new_bat_capacity) == 0)
			cur_bat_capacity = new_bat_capacity;
	}

	if (0 > vconf_get_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, &vconf_state)) {
		PRT_TRACE_ERR("vconf_get_int() failed");
		return -1;
	}

	if (new_bat_capacity <= BATTERY_REAL_POWER_OFF) {
		if (0 > plugin_intf->OEM_sys_get_battery_charge_now(&val)) {
			PRT_TRACE_ERR("fail to read charge now from kernel");
		}
		PRT_TRACE("charge_now status %d",val);
		if (val == 1) {
			new_bat_state = BATTERY_POWER_OFF;
			if (vconf_state != VCONFKEY_SYSMAN_BAT_POWER_OFF)
				ret=vconf_set_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, VCONFKEY_SYSMAN_BAT_POWER_OFF);
		} else {
			new_bat_state = BATTERY_REAL_POWER_OFF;
			if (vconf_state != VCONFKEY_SYSMAN_BAT_REAL_POWER_OFF)
				ret=vconf_set_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, VCONFKEY_SYSMAN_BAT_REAL_POWER_OFF);
		}
	} else if (new_bat_capacity <= BATTERY_POWER_OFF) {
		new_bat_state = BATTERY_POWER_OFF;
		if (vconf_state != VCONFKEY_SYSMAN_BAT_POWER_OFF)
			ret=vconf_set_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, VCONFKEY_SYSMAN_BAT_POWER_OFF);
	} else if (new_bat_capacity <= BATTERY_CRITICAL_LOW) {
		new_bat_state = BATTERY_CRITICAL_LOW;
		if (vconf_state != VCONFKEY_SYSMAN_BAT_CRITICAL_LOW)
			ret=vconf_set_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, VCONFKEY_SYSMAN_BAT_CRITICAL_LOW);
	} else if (new_bat_capacity <= BATTERY_WARNING_LOW) {
		new_bat_state = BATTERY_WARNING_LOW;
		if (vconf_state != VCONFKEY_SYSMAN_BAT_WARNING_LOW)
			ret=vconf_set_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, VCONFKEY_SYSMAN_BAT_WARNING_LOW);
	} else {
		new_bat_state = BATTERY_NORMAL;
		if (new_bat_capacity == BATTERY_FULL) {
			plugin_intf->OEM_sys_get_battery_charge_full(&bat_full);
			if (bat_full == 1) {
				if (vconf_state != VCONFKEY_SYSMAN_BAT_FULL)
				ret=vconf_set_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, VCONFKEY_SYSMAN_BAT_FULL);
			} else {
				if (vconf_state != VCONFKEY_SYSMAN_BAT_NORMAL)
					ret=vconf_set_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, VCONFKEY_SYSMAN_BAT_NORMAL);
			}
		} else {
			if (vconf_state != VCONFKEY_SYSMAN_BAT_NORMAL)
				ret=vconf_set_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, VCONFKEY_SYSMAN_BAT_NORMAL);
		}
	}

	if(ret < 0)
		return -1;

	ss_lowbat_is_charge_in_now();

	if (cur_bat_state == new_bat_state) {
		return 0;
	}

	if (cur_bat_state == BATTERY_UNKNOWN) {
		for (i = 0;
		     i < sizeof(lpe) / sizeof(struct lowbat_process_entry);
		     i++) {
			if (new_bat_state == lpe[i].new_bat_state) {
				lpe[i].action(ad);
				cur_bat_state = new_bat_state;
				return 0;
			}
		}
	} else {
		for (i = 0;
		     i < sizeof(lpe) / sizeof(struct lowbat_process_entry);
		     i++) {
			if ((cur_bat_state == lpe[i].cur_bat_state)
			    && (new_bat_state == lpe[i].new_bat_state)) {
				lpe[i].action(ad);
				cur_bat_state = new_bat_state;
				return 0;
			}
		}
	}
	PRT_TRACE("[BATMON] Unknown battery state cur:%d new:%d",cur_bat_state,new_bat_state);
	cur_bat_state = new_bat_state;
	return -1;
}

static int lowbat_read()
{
	int bat_percent;

	plugin_intf->OEM_sys_get_battery_capacity(&bat_percent);

	return bat_percent;
}

int ss_lowbat_monitor(void *data)
{
	struct ss_main_data *ad = (struct ss_main_data *)data;
	int bat_percent;

	bat_percent = lowbat_read();
	if (bat_percent < 0) {
		ecore_timer_interval_set(lowbat_timer, BAT_MON_INTERVAL_MIN);
		bat_err_count++;
		if (bat_err_count > MAX_BATTERY_ERROR) {
			PRT_TRACE_ERR
			    ("[BATMON] Cannot read battery gage. stop read fuel gage");
			return 0;
		}
		return 1;
	}
	if (bat_percent > 100)
		bat_percent = 100;

	if (lowbat_process(bat_percent, ad) < 0)
		ecore_timer_interval_set(lowbat_timer, BAT_MON_INTERVAL_MIN);
	else
		ecore_timer_interval_set(lowbat_timer, BAT_MON_INTERVAL);

	return 1;
}

static int wakeup_cb(keynode_t *key_nodes, void *data)
{
	int pm_state = 0;

	if ((pm_state =
	     vconf_keynode_get_int(key_nodes)) == VCONFKEY_PM_STATE_LCDOFF)
		ss_lowbat_monitor(NULL);

	return 0;
}

/* for debugging (request by kernel) */
static int check_battery()
{
	int r;
	int ret = -1;

	if (0 > plugin_intf->OEM_sys_get_battery_present(&ret)) {
		PRT_TRACE_ERR("[BATMON] battery check : %d", ret);
	}
	PRT_TRACE("[BATMON] battery check : %d", ret);

	return ret;
}

int ss_lowbat_init(struct ss_main_data *ad)
{
	int i;

	/* need check battery */
	lowbat_timer =
	    ecore_timer_add(BAT_MON_INTERVAL_MIN, ss_lowbat_monitor, ad);
	ss_lowbat_is_charge_in_now();

	vconf_notify_key_changed(VCONFKEY_PM_STATE, (void *)wakeup_cb, NULL);

	return 0;
}
