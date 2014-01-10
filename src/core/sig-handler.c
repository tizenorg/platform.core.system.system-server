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


#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vconf.h>
#include "core.h"
#include "core/edbus-handler.h"
#include "display/poll.h"
#include "devices.h"

#define _E(format, args...) do { \
	char buf[255];\
	snprintf(buf, 255, format, ##args);\
	write(2, buf, strlen(buf));\
} while (0);

#define _D(format, args...) do { \
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
		_E("SIGCHLD received\n");
		return;
	}

	_D("sig child actend call - %d\n", info->si_pid);

	ss_core_action_clear(info->si_pid);
}

static void sig_pipe_handler(int signo, siginfo_t *info, void *data)
{

}

static void signal_init(void *data)
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

const struct device_ops signal_device_ops = {
	.init = signal_init,
};
