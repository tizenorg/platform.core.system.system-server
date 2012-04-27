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
#include <fcntl.h>
#include <heynoti.h>
#include <TapiCommon.h>

#include "ss_log.h"
#include "ss_core.h"
#include "ss_sig_handler.h"
#include "ss_device_handler.h"
#include "ss_pmon_handler.h"
#include "ss_sysnoti.h"
#include "ss_noti.h"
#include "ss_queue.h"
#include "ss_predefine.h"
#include "ss_bs.h"
#include "ss_procmgr.h"
#include "ss_timemgr.h"
#include "ss_cpu_handler.h"
#include "ss_device_plugin.h"
#include "include/ss_data.h"

static void fini(struct ss_main_data *ad)
{
}

static void init_ad(struct ss_main_data *ad)
{
	memset(ad, 0x0, sizeof(struct ss_main_data));
}

static void writepid(char *pidpath)
{
	FILE *fp;

	fp = fopen(pidpath, "w");
	if (fp != NULL) {
		fprintf(fp, "%d", getpid());
		fclose(fp);
	}
}

static void system_server_init(struct ss_main_data *ad)
{
	if (0 > _ss_devman_plugin_init()) {
		PRT_TRACE_ERR("Device Manager Plugin Initialize failed");
		exit (-1);
	}

	/* telephony initialize */
	int ret = 0;
	ret = tel_init();
	if (ret != TAPI_API_SUCCESS) {
		PRT_TRACE_ERR("tapi init error : [0x%x]", ret);
	}
	/* Register Telephony Client */
	ret = tel_register_app_name("slp.system.server");
	if (ret != TAPI_API_SUCCESS) {
		PRT_TRACE_ERR("tapi app register error : [0x%x]", ret);
	}

	ad->sysnoti_fd = ss_sysnoti_init();
	if (ss_noti_init() < 0)
		PRT_TRACE_ERR("init noti error");

	ss_queue_init();
	ss_core_init(ad);
	ss_signal_init();
	ss_predefine_internal_init();
	ss_process_manager_init();
	ss_time_manager_init();
	ss_cpu_handler_init();

	ss_lowmem_init(ad);
	ss_lowbat_init(ad);
	ss_usb_init();
	ss_ta_init();
	ss_pmon_init(ad);
	ss_device_change_init(ad);
	ss_mmc_init();
	ss_bs_init();
	_ss_usb_storage_init();
}

#define SS_PIDFILE_PATH		"/var/run/.system_server.pid"

static int system_main(int argc, char **argv)
{
	struct ss_main_data ad;

	init_ad(&ad);
	ad.noti_fd = heynoti_init();
	heynoti_attach_handler(ad.noti_fd);
	system_server_init(&ad);

	ecore_main_loop_begin();

	fini(&ad);
	ecore_shutdown();

	return 0;
}

static int elm_main(int argc, char **argv)
{
	return system_main(argc, argv);
}

int main(int argc, char **argv)
{
	writepid(SS_PIDFILE_PATH);
	ecore_init();
	return elm_main(argc, argv);
}
