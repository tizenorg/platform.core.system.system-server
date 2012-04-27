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
#include <sys/types.h>
#include <sys/wait.h>
#include "ss_core.h"

#define PRT_TRACE_ERR(format, args...) do { \
	char buf[255];\
	snprintf(buf, 255, format, ##args);\
	write(2, buf, strlen(buf));\
} while (0);

#define PRT_TRACE(format, args...) do { \
	char buf[255];\
	snprintf(buf, 255, format, ##args);\
	write(1, buf, strlen(buf));\
} while (0);

static struct sigaction sig_child_old_act;
static struct sigaction sig_pipe_old_act;

static void sig_child_handler(int signo, siginfo_t *info, void *data)
{
	pid_t pid;
	int status;

	pid = waitpid(info->si_pid, &status, 0);
	if (pid == -1) {
		PRT_TRACE_ERR("SIGCHLD received\n");
		return;
	}

	PRT_TRACE("sig child actend call - %d\n", info->si_pid);

	ss_core_action_clear(info->si_pid);
}

static void sig_pipe_handler(int signo, siginfo_t *info, void *data)
{

}

void ss_signal_init()
{
	struct sigaction sig_act;

	sig_act.sa_handler = NULL;
	sig_act.sa_sigaction = sig_child_handler;
	sig_act.sa_flags = SA_SIGINFO;
	sigemptyset(&sig_act.sa_mask);
	sigaction(SIGCHLD, &sig_act, &sig_child_old_act);

	sig_act.sa_handler = NULL;
	sig_act.sa_sigaction = sig_pipe_handler;
	sig_act.sa_flags = SA_SIGINFO;
	sigemptyset(&sig_act.sa_mask);
	sigaction(SIGPIPE, &sig_act, &sig_pipe_old_act);
}
