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


#include <notification.h>
#include <heynoti.h>
#include "core/common.h"
#include "log.h"
#include "data.h"
#include "devices.h"

static int noti_fd;

int ss_noti_getfd()
{
	return noti_fd;
}

int ss_noti_send(char *filename)
{
	return heynoti_publish(filename);
}

int ss_noti_add(const char *noti, void (*cb) (void *), void *data)
{
	if (noti_fd < 0)
		return -1;

	return heynoti_subscribe(noti_fd, noti, cb, data);
}

static void noti_init(void *data)
{
	struct ss_main_data *ad = (struct ss_main_data*)data;

	if ((ad->noti_fd = heynoti_init()) < 0) {
		_E("Hey Notification Initialize failed");
		return;
	}
	if (heynoti_attach_handler(ad->noti_fd) != 0) {
		_E("fail to attach hey noti handler");
		return;
	}

	noti_fd = heynoti_init();
	if (noti_fd < 0) {
		_E("heynoti_init error");
		return;
	}

	if (heynoti_attach_handler(noti_fd) < 0) {
		_E("heynoti_attach_handler error");
		return;
	}
}

static void noti_exit(void *data)
{
	struct ss_main_data *ad = (struct ss_main_data*)data;

	heynoti_close(ad->noti_fd);
	heynoti_close(noti_fd);
}

const struct device_ops noti_device_ops = {
	.init = noti_init,
	.exit = noti_exit,
};

int notification_send(char *title, char *str, char *event_str, int resp)
{
	notification_error_e err = NOTIFICATION_ERROR_NONE;
	notification_h noti = NULL;
	bundle *b = NULL;

	noti = notification_create(NOTIFICATION_TYPE_NOTI);
	if (noti == NULL) {
		_E("Failed to create notification \n");
		return -1;
	}

	err = notification_set_pkgname(noti, SYSTEM_SERVER_APP_NAME);
	if (err != NOTIFICATION_ERROR_NONE) {
		_E("Unable to set pkgname \n");
		return -1;
	}

	err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, title, NULL, NOTIFICATION_TYPE_NONE);
	if (err != NOTIFICATION_ERROR_NONE) {
		_E("Unable to set notification title \n");
		return -1;
	}

	err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, str, NULL, NOTIFICATION_TYPE_NONE);
	if (err != NOTIFICATION_ERROR_NONE) {
		_E("Unable to set notification content \n");
		return -1;
	}

	 /* for responses */
	if (resp) {
		b = bundle_create();
		if (strcmp(event_str, "poweroff") == 0)
			bundle_add(b, "buttons", "Shutdown,Cancel");
		notification_set_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_RESPONDING, NULL, NULL, b);
	}

	err = notification_insert(noti, NULL);
	if (err != NOTIFICATION_ERROR_NONE) {
		_E("Unable to insert notification \n");
		return -1;
	}

	 /* for responses */
	if (resp) {
		err = notification_wait_response(noti, 0, &resp, NULL);
		if (err != NOTIFICATION_ERROR_NONE) {
			_E("Unable to wait for a notification response \n");
			return -1;
		}
		return resp;
	}

	return 0;
}
