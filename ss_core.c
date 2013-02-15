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


#include <sysman.h>
#include "include/ss_data.h"
#include "ss_queue.h"
#include "ss_log.h"
#include "ss_predefine.h"
#include "ss_core.h"

enum ss_core_cmd_type {
	SS_CORE_ACT_RUN,
	SS_CORE_ACT_CLEAR
};

struct _internal_msg {
	int type;
	int pid;
};

static int core_pipe[2];

static int _ss_core_action_run(void *user_data,
			       struct ss_run_queue_entry *rq_entry)
{
	struct ss_action_entry *act_entry = rq_entry->action_entry;
	int ret;
	char tmp[128];

	rq_entry->state = SS_STATE_RUNNING;
	ret = act_entry->predefine_action(rq_entry->argc, rq_entry->argv);
	if (ret <= 0) {
		if (ret < 0)
			PRT_TRACE_ERR("[SYSMAN] predefine action failed");
		goto fast_done;
	} else {
		snprintf(tmp, sizeof(tmp), "/proc/%d/status", ret);
		if (access(tmp, R_OK) == 0)
			rq_entry->forked_pid = ret;
		else
			goto fast_done;
	}
	return 0;

 fast_done:
	rq_entry->forked_pid = -1;
	rq_entry->state = SS_STATE_DONE;
	ss_core_action_clear(-1);
	return 0;
}

static int core_pipe_cb(void *userdata, Ecore_Fd_Handler * fd_handler)
{
	struct ss_main_data *ad = (struct ss_main_data *)userdata;
	struct _internal_msg p_msg;

	if (!ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ)) {
		PRT_TRACE_ERR
		    ("ecore_main_fd_handler_active_get error , return\n");
		return 1;
	}

	read(core_pipe[0], &p_msg, sizeof(struct _internal_msg));

	switch (p_msg.type) {
	case SS_CORE_ACT_RUN:
		ss_run_queue_run(SS_STATE_INIT, _ss_core_action_run, ad);
		break;
	case SS_CORE_ACT_CLEAR:
		ss_run_queue_del_bypid(p_msg.pid);
		break;
	}
	return 1;
}

int ss_core_action_run()
{
	struct _internal_msg p_msg;

	p_msg.type = SS_CORE_ACT_RUN;
	p_msg.pid = 0;
	write(core_pipe[1], &p_msg, sizeof(struct _internal_msg));

	return 0;
}

int ss_core_action_clear(int pid)
{
	struct _internal_msg p_msg;

	p_msg.type = SS_CORE_ACT_CLEAR;
	p_msg.pid = pid;
	write(core_pipe[1], &p_msg, sizeof(struct _internal_msg));

	return 0;
}

int ss_core_init(struct ss_main_data *ad)
{
	if (pipe(core_pipe) < 0) {
		PRT_TRACE_ERR("pipe cannot create");
		exit(1);
	}

	ecore_main_fd_handler_add(core_pipe[0], ECORE_FD_READ,
				  core_pipe_cb, ad, NULL, NULL);
	return 0;
}
