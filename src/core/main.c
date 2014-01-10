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


#include <stdio.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include "log.h"
#include "data.h"
#include "devices.h"

#define SS_PIDFILE_PATH		"/var/run/.system_server.pid"

static void init_ad(struct ss_main_data *ad)
{
	memset(ad, 0x0, sizeof(struct ss_main_data));
}

static void writepid(char *pidpath)
{
	FILE *fp;

	fp = fopen(pidpath, "w");
	if (fp != NULL) {
		fprintf(fp, "%d", getpid());
		fclose(fp);
	}
}

static void sig_quit(int signo)
{
	_E("received SIGTERM signal %d", signo);
}

static int system_main(int argc, char **argv)
{
	struct ss_main_data ad;

	init_ad(&ad);
	devices_init(&ad);
	signal(SIGTERM, sig_quit);
	ecore_main_loop_begin();

	devices_exit(&ad);
	ecore_shutdown();
	return 0;
}

int main(int argc, char **argv)
{
	writepid(SS_PIDFILE_PATH);
	ecore_init();
	return system_main(argc, argv);
}
