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


#include <glib.h>
#include <device-node.h>

#include "ss_log.h"
#include "ss_predefine.h"
#include "include/ss_data.h"

#ifndef MERGE_BTW_APPLICATIONS
#define MERGE_BTW_APPLICATIONS
#endif

enum {
	OPEN = 0,
	CLOSE,
	PLAY,
	ONESHOT,
	STOP,
	LEVEL,
};

struct haptic_node {
	int handle;
	int level;
	int play;
};

static GList *haptic_head;

static int add_node(struct haptic_node *node)
{
	haptic_head = g_list_append(haptic_head, node);
	return 0;
}

static int delete_node(struct haptic_node *node)
{
	haptic_head = g_list_remove(haptic_head, node);
	return 0;
}

static struct haptic_node *find_node(int handle)
{
	GList *elem;
	struct haptic_node *node;

	for (elem = haptic_head; elem; elem = elem->next) {
		node = elem->data;
		if (node->handle == handle) {
			return node;
		}
	}

	return NULL;
}
#ifndef MERGE_BTW_APPLICATIONS
static void stop_all_device(gpointer data, gpointer user_data)
{
	struct haptic_node *node = (struct haptic_node*)data;

	node->level = 0;
	node->play = 0;
	PRT_TRACE_ERR("node handle : %d, level : %d, play : %d", node->handle, node->level, node->play);
}
#endif
static int haptic_play(int handle)
{
	struct haptic_node *node;
	int r;

	PRT_TRACE("handle : %d", handle);
	node = find_node(handle);
	if (node == NULL) {
		PRT_TRACE_ERR("find_node(%d) fail", handle);
		return -1;
	}
#ifndef MERGE_BTW_APPLICATIONS
	g_list_foreach(haptic_head, stop_all_device, NULL);
#endif
	r = device_set_property(DEVICE_TYPE_VIBRATOR, PROP_VIBRATOR_ENABLE, 1);
	if (r < 0) {
		PRT_TRACE_ERR("set enable fail");
		return -1;
	}

	node->level = 0;
	node->play = 1;
	return 0;
}

static int haptic_oneshot(int handle, int duration, int level)
{
	struct haptic_node *node;
	int r;

	PRT_TRACE("handle : %d", handle);
	node = find_node(handle);
	if (node == NULL) {
		PRT_TRACE_ERR("find_node(%d) fail", handle);
		return 0;
	}

	r = device_set_property(DEVICE_TYPE_VIBRATOR, PROP_VIBRATOR_LEVEL, level);
	if (r < 0) {
		PRT_TRACE_ERR("set level fail");
		return -1;
	}

	r = device_set_property(DEVICE_TYPE_VIBRATOR, PROP_VIBRATOR_ONESHOT, duration);
	if (r < 0) {
		PRT_TRACE_ERR("set oneshot fail");
		return -1;
	}

	return 0;
}

static void check_play_state(gpointer data, gpointer user_data)
{
	struct haptic_node *node = (struct haptic_node*)data;
	int *play = (int*)user_data;

	*play += node->play;
	PRT_TRACE_ERR("node handle : %d, level : %d, play : %d", node->handle, node->level, node->play);
}

static int haptic_stop(int handle)
{
	struct haptic_node *node;
	int play = 0;
	int r;

	PRT_TRACE("handle : %d", handle);
	node = find_node(handle);
	if (node == NULL) {
		PRT_TRACE_ERR("find_node(%d) fail", handle);
		return -1;
	}

	node->level = 0;
	node->play = 0;

	g_list_foreach(haptic_head, check_play_state, &play);
	PRT_TRACE_ERR("play state : %d", play);

	if (!play) {
		PRT_TRACE_ERR("not playing anymore, will be stop");
		r = device_set_property(DEVICE_TYPE_VIBRATOR, PROP_VIBRATOR_ENABLE, 0);
		if (r < 0) {
			PRT_TRACE_ERR("set enable fail");
			return -1;
		}
	}

	return 0;
}
#ifdef MERGE_BTW_APPLICATIONS
static void get_current_level(gpointer data, gpointer user_data)
{
	struct haptic_node *node = (struct haptic_node*)data;
	int *sum = (int*)user_data;

	PRT_TRACE_ERR("node handle : %d, level : %d, play : %d", node->handle, node->level, node->play);
	if (node->play == 1) {
		PRT_TRACE_ERR("node->play : %d, sum : %d", node->play, *sum);
		*sum += node->level;
	}
}
#endif
static int haptic_change_level(int handle, int level)
{
	struct haptic_node *node;
	int sum = 0;
	int r;

	PRT_TRACE_ERR("handle : %d, level : %d", handle, level);
	node = find_node(handle);
	if (node == NULL) {
		PRT_TRACE_ERR("find_node(%d) fail", handle);
		return -1;
	}

	node->level = level;
#ifdef MERGE_BTW_APPLICATIONS
	g_list_foreach(haptic_head, get_current_level, &sum);
	PRT_TRACE_ERR("current sum level : %d", sum);
#else
	sum = level;
	if (!node->play) {
		PRT_TRACE_ERR("This handle is stoped by another handle");
		return 0;
	}
#endif

	r = device_set_property(DEVICE_TYPE_VIBRATOR, PROP_VIBRATOR_LEVEL, sum);
	if (r < 0) {
		PRT_TRACE_ERR("set level fail");
		return -1;
	}

	return 0;
}

static int haptic_open(int handle)
{
	struct haptic_node *node;

	PRT_TRACE("handle : %d", handle);
	node = malloc(sizeof(struct haptic_node));
	if (node == NULL) {
		PRT_TRACE_ERR("malloc fail");
		return -1;
	}

	node->handle = handle;
	node->level = 0;
	node->play = 0;

	add_node(node);
	return 0;
}

static int haptic_close(int handle)
{
	struct haptic_node *node;
	int play = 0;
	int r;

	PRT_TRACE("handle : %d", handle);
	node = find_node(handle);
	if (node == NULL) {
		PRT_TRACE_ERR("find_node(%d) fail", handle);
		return -1;
	}

	node->level = 0;
	node->play = 0;

	delete_node(node);
	free(node);

	g_list_foreach(haptic_head, check_play_state, &play);
	PRT_TRACE_ERR("play state : %d", play);

	if (!play) {
		PRT_TRACE_ERR("not playing anymore, will be stop");
		r = device_set_property(DEVICE_TYPE_VIBRATOR, PROP_VIBRATOR_ENABLE, 0);
		if (r < 0) {
			PRT_TRACE_ERR("set enable fail");
			return -1;
		}
	}

	return 0;
}

int haptic_def_predefine_action(int argc, char **argv)
{
	int i;
	int pid;
	int mode;
	int handle;

	PRT_TRACE_ERR("argc : %d", argc);
	for (i = 0; i < argc; ++i)
		PRT_TRACE_ERR("[%2d] %s", i, argv[i]);

	if (argc <= 0 || argc > 5) {
		PRT_TRACE_ERR("Haptic predefine action failed");
		return -1;
	}

	pid = atoi(argv[0]);
	mode = atoi(argv[1]);
	handle = atoi(argv[2]);
	PRT_TRACE_ERR("pid : %d, mode : %d", pid, mode);

	switch(mode) {
	case OPEN:
		return haptic_open(handle);
	case CLOSE:
		return haptic_close(handle);
	case PLAY:
		return haptic_play(handle);
	case ONESHOT:
		return haptic_oneshot(handle, atoi(argv[3]), atoi(argv[4]));
	case STOP:
		return haptic_stop(handle);
	case LEVEL:
		return haptic_change_level(handle, atoi(argv[3]));
	default:
		break;
	}

	return -1;
}