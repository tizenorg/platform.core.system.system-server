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


#ifndef __EDBUS_HANDLE_H__
#define __EDBUS_HANDLE_H__

#include <E_DBus.h>

#define BUS_NAME		"org.tizen.system.deviced"
#define OBJECT_PATH		"/Org/Tizen/System/DeviceD"
#define INTERFACE_NAME		BUS_NAME

struct dbus_byte {
	char *data;
	int size;
};

struct edbus_method {
	const char *member;
	const char *signature;
	const char *reply_signature;
	E_DBus_Method_Cb func;
};

int register_edbus_method(const char *path, const struct edbus_method *edbus_methods, int size);
int register_edbus_signal_handler(const char *path, const char *interface,
		const char *name, E_DBus_Signal_Cb cb);
E_DBus_Interface *get_edbus_interface(const char *path);
pid_t get_edbus_sender_pid(DBusMessage *msg);
int broadcast_edbus_signal(const char *path, const char *interface,
		const char *name, const char *sig, char *param[]);
DBusMessage *send_edbus_method_sync(const char *dest, const char *path,
		const char *interface, const char *method,
		const char *sig, char *param[]);

void edbus_init(void *data);
void edbus_exit(void *data);

#endif /* __EDBUS_HANDLE_H__ */
