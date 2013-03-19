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

enum {
	PLAY_HAPTIC = 0,
	STOP_HAPTIC,
	LEVEL_HAPTIC,
};

struct haptic_node {
	int pid;
	int level;
	int play;
};

static GList *haptic_head;

static int add_node(struct haptic_node *node)
{
	haptic_head = g_list_append(haptic_head, node);
}

static int delete_node(struct haptic_node *node)
{
	haptic_head = g_list_remove(haptic_head, node);
}

static struct haptic_node *find_node(int pid)
{
	GList *elem;
	struct haptic_node *node = NULL;

	for (elem = haptic_head; elem; elem = elem->next) {
		node = elem->data;
		if (node->pid == pid) {
			return node;
		}
	}

	node = NULL;
	return node;
}

static int haptic_play(int pid)
{
	struct haptic_node *node;
	int r;

	node = malloc(sizeof(struct haptic_node));
	if (node == NULL) {
		PRT_TRACE_ERR("malloc fail");
		return -1;
	}

	node->pid = pid;
	node->level = 0;
	node->play = 1;

	add_node(node);

	r = device_set_property(DEVICE_TYPE_VIBRATOR, PROP_VIBRATOR_ENABLE, 1);
	if (r < 0) {
		PRT_TRACE_ERR("set enable fail");
		return -1;
	}

	return 0;
}

static void check_play_state(gpointer data, gpointer user_data)
{
	struct haptic_node *node = (struct haptic_node*)data;
	int *play = (int*)user_data;

	*play += node->play;
	PRT_TRACE_ERR("node pid : %d, level : %d, play : %d", node->pid, node->level, node->play);
}

static int haptic_stop(int pid)
{
	struct haptic_node *node;
	int play = 0;
	int r;

	node = find_node(pid);
	if (node == NULL) {
		PRT_TRACE_ERR("find_node(%d) fail", pid);
		return -1;
	}

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

static void get_current_level(gpointer data, gpointer user_data)
{
	struct haptic_node *node = (struct haptic_node*)data;
	int *sum = (int*)user_data;

	PRT_TRACE_ERR("node pid : %d, level : %d, play : %d", node->pid, node->level, node->play);
	if (node->play == 1) {
		PRT_TRACE_ERR("node->play : %d, sum : %d", node->play, *sum);
		*sum += node->level;
	}
}

static int haptic_change_level(int pid, int level)
{
	struct haptic_node *node;
	int sum = 0;
	int r;

	PRT_TRACE_ERR("pid : %d, level : %d", pid, level);

	node = find_node(pid);
	if (node == NULL) {
		PRT_TRACE_ERR("find_node(%d) fail", pid);
		return -1;
	}

	node->level = level;

	g_list_foreach(haptic_head, get_current_level, &sum);
	PRT_TRACE_ERR("current sum level : %d", sum);

	r = device_set_property(DEVICE_TYPE_VIBRATOR, PROP_VIBRATOR_LEVEL, sum);
	if (r < 0) {
		PRT_TRACE_ERR("set level fail");
		return -1;
	}

	return 0;
}

int haptic_def_predefine_action(int argc, char **argv)
{
	int i;
	int pid;
	int mode;

	PRT_TRACE_ERR("argc : %d", argc);
	for (i = 0; i < argc; ++i)
		PRT_TRACE_ERR("[%2d] %s", i, argv[i]);

	if (argc != 3) {
		PRT_TRACE_ERR("Haptic predefine action failed");
		return -1;
	}

	pid = atoi(argv[0]);
	mode = atoi(argv[1]);
	PRT_TRACE_ERR("pid : %d, mode : %d", pid, mode);

	switch(mode) {
	case PLAY_HAPTIC:
		haptic_play(pid);
		break;
	case STOP_HAPTIC:
		haptic_stop(pid);
		break;
	case LEVEL_HAPTIC:
		haptic_change_level(pid, atoi(argv[2]));
		break;
	default:
		break;
	}

	return 0;
}
