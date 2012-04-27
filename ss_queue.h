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


#ifndef __SS_QUEUE_H__
#define __SS_QUEUE_H__

#include "ss_sysnoti.h"

struct ss_action_entry {
	int owner_pid;
	void *handle;
	char *type;
	char *path;
	int (*predefine_action) ();
	int (*ui_viewable) ();
	int (*is_accessable) (int caller_pid);
};

enum ss_run_state {
	SS_STATE_INIT,
	SS_STATE_RUNNING,
	SS_STATE_DONE
};

struct ss_run_queue_entry {
	enum ss_run_state state;
	struct ss_action_entry *action_entry;
	int forked_pid;
	int argc;
	char *argv[SYSMAN_MAXARG];
};

int ss_action_entry_add_internal(char *type,
				 int (*predefine_action) (),
				 int (*ui_viewable) (),
				 int (*is_accessable) (int));
int ss_action_entry_add(struct sysnoti *msg);
int ss_action_entry_call_internal(char *type, int argc, ...);
int ss_action_entry_call(struct sysnoti *msg, int argc, char **argv);

int ss_run_queue_run(enum ss_run_state state,
		     int (*run_func) (void *, struct ss_run_queue_entry *),
		     void *user_data);

struct ss_run_queue_entry *ss_run_queue_find_bypid(int pid);
int ss_run_queue_add(struct ss_action_entry *act_entry, int argc, char **argv);
int ss_run_queue_del(struct ss_run_queue_entry *entry);
int ss_run_queue_del_bypid(int pid);

void ss_queue_init();

#endif /* __SS_QUEUE_H__ */
