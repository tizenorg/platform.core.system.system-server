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


#include <heynoti.h>
#include "ss_log.h"

static int noti_fd;

int ss_noti_getfd()
{
	return noti_fd;
}

int ss_noti_send(char *filename)
{
	return heynoti_publish(filename);
}

int ss_noti_init()
{
	noti_fd = heynoti_init();
	if (noti_fd < 0) {
		PRT_TRACE_ERR("heynoti_init error");
		return -1;
	}

	if (heynoti_attach_handler(noti_fd) < 0) {
		PRT_TRACE_ERR("heynoti_attach_handler error");
		return -1;
	}

	return 0;
}

int ss_noti_add(const char *noti, void (*cb) (void *), void *data)
{
	if (noti_fd < 0)
		return -1;

	return heynoti_subscribe(noti_fd, noti, cb, data);
}
