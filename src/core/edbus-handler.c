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


#include <stdbool.h>
#include "core/log.h"
#include "core/data.h"
#include "core/edbus-handler.h"
#include "core/common.h"

#define EDBUS_INIT_RETRY_COUNT 5

#define DBUS_REPLY_TIMEOUT	(120 * 1000)
#define RETRY_MAX 5

struct edbus_list{
	char *signal_name;
	E_DBus_Signal_Handler *handler;
};

static struct edbus_object {
	const char *path;
	const char *interface;
	E_DBus_Object *obj;
	E_DBus_Interface *iface;
} edbus_objects[] = {
	/* Add new object & interface here*/
};

static Eina_List *edbus_handler_list;
static int edbus_init_val;
static DBusConnection *conn;
static E_DBus_Connection *edbus_conn;
static DBusPendingCall *edbus_request_name;

static int register_edbus_interface(struct edbus_object *object)
{
	int ret;

	if (!object) {
		_E("object is invalid value!");
		return -1;
	}

	object->obj = e_dbus_object_add(edbus_conn, object->path, NULL);
	if (!object->obj) {
		_E("fail to add edbus obj");
		return -1;
	}

	object->iface = e_dbus_interface_new(object->interface);
	if (!object->iface) {
		_E("fail to add edbus interface");
		return -1;
	}

	e_dbus_object_interface_attach(object->obj, object->iface);

	return 0;
}

E_DBus_Interface *get_edbus_interface(const char *path)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(edbus_objects); i++)
		if (!strcmp(path, edbus_objects[i].path))
			return edbus_objects[i].iface;

	return NULL;
}

pid_t get_edbus_sender_pid(DBusMessage *msg)
{
	const char *sender;
	DBusMessage *send_msg;
	DBusPendingCall *pending;
	DBusMessageIter iter;
	int ret;
	pid_t pid;

	if (!msg) {
		_E("invalid argument!");
		return -1;
	}

	sender = dbus_message_get_sender(msg);
	if (!sender) {
		_E("invalid sender!");
		return -1;
	}

	send_msg = dbus_message_new_method_call(DBUS_SERVICE_DBUS,
				    DBUS_PATH_DBUS,
				    DBUS_INTERFACE_DBUS,
				    "GetConnectionUnixProcessID");
	if (!send_msg) {
		_E("invalid send msg!");
		return -1;
	}

	ret = dbus_message_append_args(send_msg, DBUS_TYPE_STRING,
				    &sender, DBUS_TYPE_INVALID);
	if (!ret) {
		_E("fail to append args!");
		dbus_message_unref(send_msg);
		return -1;
	}

	pending = e_dbus_message_send(edbus_conn, send_msg, NULL, -1, NULL);
	if (!pending) {
		_E("pending is null!");
		dbus_message_unref(send_msg);
		return -1;
	}

	dbus_message_unref(send_msg);

	/* block until reply is received */
	dbus_pending_call_block(pending);

	msg = dbus_pending_call_steal_reply(pending);
	dbus_pending_call_unref(pending);
	if (!msg) {
		_E("reply msg is null!");
		return -1;
	}

	dbus_message_iter_init(msg, &iter);
	dbus_message_iter_get_basic(&iter, &pid);
	dbus_message_unref(msg);

	return pid;
}

static void unregister_edbus_signal_handle(void)
{
	Eina_List *tmp;
	Eina_List *tmp_next;
	struct edbus_list *entry;

	EINA_LIST_FOREACH_SAFE(edbus_handler_list, tmp, tmp_next, entry) {
		if (entry != NULL) {
			e_dbus_signal_handler_del(edbus_conn, entry->handler);
			edbus_handler_list = eina_list_remove(edbus_handler_list, entry);
			free(entry->signal_name);
			free(entry);
		}
	}
}

int register_edbus_signal_handler(const char *path, const char *interface,
		const char *name, E_DBus_Signal_Cb cb)
{
	Eina_List *tmp;
	struct edbus_list *entry;
	E_DBus_Signal_Handler *handler;

	EINA_LIST_FOREACH(edbus_handler_list, tmp, entry) {
		if (entry != NULL && strncmp(entry->signal_name, name, strlen(name)) == 0)
			return -1;
	}

	handler = e_dbus_signal_handler_add(edbus_conn, NULL, path,
				interface, name, cb, NULL);

	if (!handler) {
		_E("fail to add edbus handler");
		return -1;
	}

	entry = malloc(sizeof(struct edbus_list));

	if (!entry) {
		_E("Malloc failed");
		return -1;
	}

	entry->signal_name = strndup(name, strlen(name));

	if (!entry->signal_name) {
		_E("Malloc failed");
		free(entry);
		return -1;
	}

	entry->handler = handler;
	edbus_handler_list = eina_list_prepend(edbus_handler_list, entry);
	if (!edbus_handler_list) {
		_E("eina_list_prepend failed");
		free(entry->signal_name);
		free(entry);
		return -1;
	}
	return 0;
}

static int append_variant(DBusMessageIter *iter, const char *sig, char *param[])
{
	char *ch;
	int i;
	int int_type;
	uint64_t int64_type;
	DBusMessageIter arr;
	struct dbus_byte *byte;

	if (!sig || !param)
		return 0;

	for (ch = (char*)sig, i = 0; *ch != '\0'; ++i, ++ch) {
		switch (*ch) {
		case 'i':
			int_type = atoi(param[i]);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &int_type);
			break;
		case 'u':
			int_type = strtoul(param[i], NULL, 10);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &int_type);
			break;
		case 't':
			int64_type = atoll(param[i]);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &int64_type);
			break;
		case 's':
			dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param[i]);
			break;
		case 'a':
			++i, ++ch;
			switch (*ch) {
			case 'y':
				dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &arr);
				byte = (struct dbus_byte*)param[i];
				dbus_message_iter_append_fixed_array(&arr, DBUS_TYPE_BYTE, &(byte->data), byte->size);
				dbus_message_iter_close_container(iter, &arr);
				break;
			default:
				break;
			}
			break;
		default:
			return -EINVAL;
		}
	}

	return 0;
}

int broadcast_edbus_signal(const char *path, const char *interface,
		const char *name, const char *sig, char *param[])
{
	DBusMessage *msg;
	DBusMessageIter iter;
	int r;

	msg = dbus_message_new_signal(path, interface, name);
	if (!msg) {
		_E("fail to allocate new %s.%s signal", interface, name);
		return -EPERM;
	}

	dbus_message_iter_init_append(msg, &iter);
	r = append_variant(&iter, sig, param);
	if (r < 0) {
		_E("append_variant error(%d)", r);
		return -EPERM;
	}

	e_dbus_message_send(edbus_conn, msg, NULL, -1, NULL);

	dbus_message_unref(msg);
	return 0;
}

DBusMessage *send_edbus_method_sync(const char *dest, const char *path,
		const char *interface, const char *method,
		const char *sig, char *param[])
{
	DBusConnection *conn;
	DBusMessage *msg;
	DBusMessageIter iter;
	DBusMessage *reply;
	DBusError err;
	int i, r;

	conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (!conn) {
		_E("dbus_bus_get error");
		return NULL;
	}

	msg = dbus_message_new_method_call(dest, path, interface, method);
	if (!msg) {
		_E("dbus_message_new_method_call(%s:%s-%s)", path, interface, method);
		return NULL;
	}

	dbus_message_iter_init_append(msg, &iter);
	r = append_variant(&iter, sig, param);
	if (r < 0) {
		_E("append_variant error(%d)", r);
		dbus_message_unref(msg);
		return NULL;
	}

	for (i = 0; i < RETRY_MAX; ++i) {
		dbus_error_init(&err);

		reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_REPLY_TIMEOUT, &err);
		if (reply)
			break;

		if (dbus_error_is_set(&err)) {
			_E("dbus_connection_send error(%s:%s) : retry..%d", err.name, err.message, i);
			dbus_error_free(&err);
		}
	}

	dbus_message_unref(msg);
	return reply;
}

int register_edbus_method(const char *path, const struct edbus_method *edbus_methods, int size)
{
	E_DBus_Interface *iface;
	int ret;
	int i;

	iface = get_edbus_interface(path);

	if (!iface) {
		_E("fail to get edbus interface!");
		return -ENODEV;
	}

	for (i = 0; i < size; i++) {
		ret = e_dbus_interface_method_add(iface,
				    edbus_methods[i].member,
				    edbus_methods[i].signature,
				    edbus_methods[i].reply_signature,
				    edbus_methods[i].func);
		if (!ret) {
			_E("fail to add method %s!", edbus_methods[i].member);
			return -EINVAL;
		}
	}

	return 0;
}

void edbus_init(void *data)
{
	int retry = EDBUS_INIT_RETRY_COUNT;
	int i, ret;

	while (--retry) {
		edbus_init_val = e_dbus_init();
		if (edbus_init_val)
			break;
		if (!retry) {
			_E("fail to init edbus");
			return;
		}
	}

	retry = EDBUS_INIT_RETRY_COUNT;
	while (--retry) {
		conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
		if (conn)
			break;
		if (!retry) {
			_E("fail to get dbus");
			goto out1;
		}
	}

	retry = EDBUS_INIT_RETRY_COUNT;
	while (--retry) {
		edbus_conn = e_dbus_connection_setup(conn);
		if (edbus_conn)
			break;
		if (!retry) {
			_E("fail to get edbus");
			goto out2;
		}
	}

	retry = EDBUS_INIT_RETRY_COUNT;
	while (--retry) {
		edbus_request_name = e_dbus_request_name(edbus_conn, BUS_NAME, 0, NULL, NULL);
		if (edbus_request_name)
			break;
		if (!retry) {
			_E("fail to request edbus name");
			goto out3;
		}
	}

	for (i = 0; i < ARRAY_SIZE(edbus_objects); i++) {
		ret = register_edbus_interface(&edbus_objects[i]);
		if (ret < 0) {
			_E("fail to add obj & interface for %s",
				    edbus_objects[i].interface);
			return;
		}
		_D("add new obj for %s", edbus_objects[i].interface);
	}
	return;

out3:
	e_dbus_connection_close(edbus_conn);
out2:
	dbus_connection_set_exit_on_disconnect(conn, FALSE);
out1:
	e_dbus_shutdown();
}

void edbus_exit(void *data)
{
	unregister_edbus_signal_handle();
	e_dbus_connection_close(edbus_conn);
	e_dbus_shutdown();
}
