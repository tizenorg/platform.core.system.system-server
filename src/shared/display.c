/*
 * deviced
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


#include <stdio.h>
#include <vconf.h>
#include <errno.h>
#include <device-node.h>

#include "log.h"
#include "dd-display.h"

#define DISPLAY_MAX_BRIGHTNESS  100
#define DISPLAY_MIN_BRIGHTNESS  0

API int display_get_count(void)
{
	int val;
	int r;

	r = device_get_property(DEVICE_TYPE_DISPLAY, PROP_DISPLAY_DISPLAY_COUNT, &val);
	if (r < 0)
		return r;

	return val;
}

API int display_get_max_brightness(void)
{
	return DISPLAY_MAX_BRIGHTNESS;
}

API int display_get_min_brightness(void)
{
	return DISPLAY_MIN_BRIGHTNESS;
}

API int display_get_brightness(void)
{
	int val;
	int r;

	r = device_get_property(DEVICE_TYPE_DISPLAY, PROP_DISPLAY_BRIGHTNESS, &val);
	if (r < 0)
		return r;

	return val;
}

API int display_set_brightness_with_setting(int val)
{
	int auto_brt_state;
	int r;

	if (vconf_get_int(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, &auto_brt_state) != 0) {
		_E("Failed to get VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT value");
		errno = EPERM;
		return -1;
	}

	if (auto_brt_state == SETTING_BRIGHTNESS_AUTOMATIC_ON) {
		_D("auto_brightness state is ON, can not change the brightness value");
		return 0;
	}

	r = device_set_property(DEVICE_TYPE_DISPLAY, PROP_DISPLAY_BRIGHTNESS, val);
	if (r < 0)
		return r;

	if (vconf_set_int(VCONFKEY_SETAPPL_LCD_BRIGHTNESS, val) != 0) {
		_E("Failed to set VCONFKEY_SETAPPL_LCD_BRIGHTNESS value");
	}

	if (vconf_set_int(VCONFKEY_PM_CURRENT_BRIGHTNESS, val) != 0) {
		_E("Failed to set VCONFKEY_PM_CURRENT_BRIGHTNESS value");
	}

	return 0;
}

API int display_set_brightness(int val)
{
	int auto_brt_state;
	int r;

	if (vconf_get_int(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, &auto_brt_state) != 0) {
		_E("Failed to get VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT value");
		errno = EPERM;
		return -1;
	}

	vconf_set_int(VCONFKEY_PM_CUSTOM_BRIGHTNESS_STATUS, VCONFKEY_PM_CUSTOM_BRIGHTNESS_ON);
	r = device_set_property(DEVICE_TYPE_DISPLAY, PROP_DISPLAY_BRIGHTNESS, val);
	if (r < 0)
		return r;

	if (auto_brt_state == SETTING_BRIGHTNESS_AUTOMATIC_ON) {
		_D("Auto brightness will be paused");
		vconf_set_int(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, SETTING_BRIGHTNESS_AUTOMATIC_PAUSE);
	}

	if (vconf_set_int(VCONFKEY_PM_CURRENT_BRIGHTNESS, val) != 0) {
		_E("Failed to set VCONFKEY_PM_CURRENT_BRIGHTNESS value");
	}

	return 0;
}

API int display_release_brightness(void)
{
	int bat_state;
	int setting_val;
	int auto_brt_state;
	int charger_state;
	int brt_changed_state;
	int r;

	if (vconf_get_int(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, &bat_state) != 0) {
		_E("Failed to get VCONFKEY_SYSMAN_BATTERY_STATUS_LOW value");
		errno = EPERM;
		return -1;
	}

	if (vconf_get_int(VCONFKEY_SYSMAN_CHARGER_STATUS, &charger_state) != 0) {
		_E("Failed to get VCONFKEY_SYSMAN_CHARGER_STATUS value");
		errno = EPERM;
		return -1;
	}

	if (vconf_get_bool(VCONFKEY_PM_BRIGHTNESS_CHANGED_IN_LPM, &brt_changed_state) != 0) {
		_E("Failed to get VCONFKEY_PM_BRIGHTNESS_CHANGED_IN_LPM value");
		errno = EPERM;
		return -1;
	}

	if (vconf_get_int(VCONFKEY_SETAPPL_LCD_BRIGHTNESS, &setting_val) != 0) {
		_E("Failed to get VCONFKEY_SETAPPL_LCD_BRIGHTNESS value");
		errno = EPERM;
		return -1;
	}

	if (vconf_get_int(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, &auto_brt_state) != 0) {
		_E("Failed to get VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT value");
		errno = EPERM;
		return -1;
	}

	vconf_set_int(VCONFKEY_PM_CUSTOM_BRIGHTNESS_STATUS, VCONFKEY_PM_CUSTOM_BRIGHTNESS_OFF);
	if (bat_state < VCONFKEY_SYSMAN_BAT_WARNING_LOW) {
		_D("can not set brightness for low battery");
		return 0;
	}

	// check dim state
	if (bat_state == VCONFKEY_SYSMAN_BAT_WARNING_LOW &&
		charger_state == VCONFKEY_SYSMAN_CHARGER_DISCONNECTED && !brt_changed_state) {
		_D("batt warning low : brightness is not changed!");
		device_set_property(DEVICE_TYPE_DISPLAY, PROP_DISPLAY_BRIGHTNESS, 0);
		return 0;
	}

	if (auto_brt_state == SETTING_BRIGHTNESS_AUTOMATIC_OFF) {
		device_set_property(DEVICE_TYPE_DISPLAY, PROP_DISPLAY_BRIGHTNESS, setting_val);
	} else if (auto_brt_state == SETTING_BRIGHTNESS_AUTOMATIC_PAUSE) {
		_D("Auto brightness will be enable");
		vconf_set_int(VCONFKEY_SETAPPL_BRIGHTNESS_AUTOMATIC_INT, SETTING_BRIGHTNESS_AUTOMATIC_ON);
	}

	return 0;
}

API int display_get_acl_status(void)
{
	int val;
	int r;

	r = device_get_property(DEVICE_TYPE_DISPLAY, PROP_DISPLAY_ACL_CONTROL, &val);
	if (r < 0)
		return r;

	return val;
}

API int display_set_acl_status(int val)
{
	int r;

	r = device_set_property(DEVICE_TYPE_DISPLAY, PROP_DISPLAY_ACL_CONTROL, val);
	if (r < 0)
		return r;

	return 0;
}
