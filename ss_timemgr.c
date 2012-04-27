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


#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <pmapi.h>
#include <vconf.h>

#include <sysman.h>
#include "include/ss_data.h"
#include "ss_queue.h"
#include "ss_log.h"

#include <time.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <fcntl.h>

#include <ITapiMisc.h>

static const char default_rtc0[] = "/dev/rtc0";
static const char default_rtc1[] = "/dev/rtc1";
static const char default_localtime[] = "/opt/etc/localtime";

char *substring(const char *str, size_t begin, size_t len)
{
	if (str == 0 || strlen(str) == 0 || strlen(str) < begin
	    || strlen(str) < (begin + len))
		return 0;

	return strndup(str + begin, len);
}

int handle_timezone(char *str)
{
	int ret;
	struct stat sts;
	const char *sympath = default_localtime;

	if (str == NULL)
		return -1;
	const char *tzpath = str;

	PRT_TRACE("TZPATH = %s\n", tzpath);

	/* unlink current link
	 * eg. rm /opt/etc/localtime */
	if (stat(sympath, &sts) == -1 && errno == ENOENT) {
		/* DO NOTHING */
	} else {
		ret = unlink(sympath);
		if (ret < 0) {
			PRT_TRACE("unlink error : [%d]%s\n", ret,
				  strerror(errno));
			return -1;
		} else
			PRT_TRACE("unlink success\n");

	}

	/* symlink new link
	 * eg. ln -s /usr/share/zoneinfo/Asia/Seoul /opt/etc/localtime */
	ret = symlink(tzpath, sympath);
	if (ret < 0) {
		PRT_TRACE("symlink error : [%d]%s\n", ret, strerror(errno));
		return -1;
	} else
		PRT_TRACE("symlink success\n");

	tzset();
	return 0;
}

/*
 * TODO : error handling code should be added here.
 */
int handle_date(char *str)
{
	long int tmp = 0;
	time_t timet = 0;
	time_t before = 0;

	if (str == NULL)
		return -1;

	tmp = (long int)atoi(str);
	timet = (time_t) tmp;

	PRT_TRACE("ctime = %s", ctime(&timet));
	vconf_set_int(VCONFKEY_SYSTEM_TIMECHANGE, timet);

	return 0;
}

int set_datetime_action(int argc, char **argv)
{
	int ret = 0;
	unsigned int pm_state;
	if (argc < 1)
		return -1;
	if (vconf_get_int(VCONFKEY_PM_STATE, &ret) != 0)
		PRT_TRACE("Fail to get vconf value for pm state\n");
	if (ret == 1)
		pm_state = 0x1;
	else if (ret == 2)
		pm_state = 0x2;
	else
		pm_state = 0x4;

	pm_lock_state(pm_state, STAY_CUR_STATE, 0);
	ret = handle_date(argv[0]);
	pm_unlock_state(pm_state, STAY_CUR_STATE);
	return ret;
}

int set_timezone_action(int argc, char **argv)
{
	int ret;
	unsigned int pm_state;
	if (argc < 1)
		return -1;
	if (vconf_get_int(VCONFKEY_PM_STATE, &ret) != 0)
		PRT_TRACE("Fail to get vconf value for pm state\n");
	if (ret == 1)
		pm_state = 0x1;
	else if (ret == 2)
		pm_state = 0x2;
	else
		pm_state = 0x4;

	pm_lock_state(pm_state, STAY_CUR_STATE, 0);
	ret = handle_timezone(argv[0]);
	pm_unlock_state(pm_state, STAY_CUR_STATE);
	return ret;
}

int ss_time_manager_init(void)
{
	ss_action_entry_add_internal(PREDEF_SET_DATETIME, set_datetime_action,
				     NULL, NULL);
	ss_action_entry_add_internal(PREDEF_SET_TIMEZONE, set_timezone_action,
				     NULL, NULL);
	return 0;
}
