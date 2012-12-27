/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sysman.h>
#include <Ecore_File.h>

#include "ss_log.h"
#include "ss_launch.h"

#define CRASH_PID_MAX 7
#define CRASH_MODE_MAX 2
#define CRASH_TIME_MAX 65
#define CRASH_ARG_NUM 6
#define CRASH_DELIMITER "|"
#define CRASH_VERIFY_MAX 5
#define CRASH_PROCESSNAME_MAX NAME_MAX
#define CRASH_EXEPATH_MAX NAME_MAX
#define CRASH_ARG_MAX (CRASH_PROCESSNAME_MAX + CRASH_EXEPATH_MAX + CRASH_TIME_MAX + CRASH_PID_MAX + CRASH_MODE_MAX + CRASH_VERIFY_MAX)
#define CRASH_NOTI_DIR		"/opt/share/crash"
#define CRASH_NOTI_FILE		"curbs.log"
#define CRASH_NOTI_PATH CRASH_NOTI_DIR"/"CRASH_NOTI_FILE
#define CRASH_WORKER_PATH	"/usr/bin/crash-worker"
#define CRASH_POPUP_PATH	"/usr/apps/org.tizen.crash-popup/bin/crash-popup"

static int noti_fd;
static int add_noti(void);
struct crash_arg
{
	char crash_mode[CRASH_MODE_MAX];
	char crash_processname[CRASH_PROCESSNAME_MAX];
	char crash_timestr[CRASH_TIME_MAX];
	char crash_pid[CRASH_PID_MAX];
	char crash_exepath[CRASH_EXEPATH_MAX];
	char crash_verify[CRASH_VERIFY_MAX];
};

static int is_running_process(pid_t pid)
{
	char buf[PATH_MAX + 1];
	snprintf(buf, sizeof(buf), "/proc/%d", pid);
	if (!access(buf, R_OK))
		return 1;
	return 0;
}
static int make_noti_file(const char *path, const char *file)
{
	PRT_TRACE("Make Noti File");
	int fd;
	char buf[PATH_MAX];

	/* make a directory */
	if (access(path, F_OK) == -1) {
		snprintf(buf, sizeof(buf), "mkdir -p %s", path);
		system(buf);
		snprintf(buf, sizeof(buf), "chown root:app %s", path);
		system(buf);
	}

	snprintf(buf, sizeof(buf), "%s/%s", path, file);

	if (access(buf, F_OK) == 0)
		return -1;

	if ((fd = open(buf, O_CREAT, S_IRUSR | S_IWUSR)) < 0)
		return -1;
	close(fd);
	snprintf(buf, sizeof(buf), "chmod 666 %s/%s", path, file);
	system(buf);
	snprintf(buf, sizeof(buf), "chown root:app %s/%s", path, file);
	system(buf);

	return 0;
}
static int crash_arg_parser(char *linebuffer, struct crash_arg *arg)
{
	char *ptr = NULL;
	int verify_num = 0;
	int verify_arg_num = 0;

	if (linebuffer == NULL || arg == NULL) {
		PRT_TRACE_ERR("crash_arg_parser input arguments is NULL\n");
		return -1;
	}
	ptr = strtok(linebuffer, CRASH_DELIMITER);
	if (ptr == NULL) {
		PRT_TRACE_ERR("can't strtok linebuffer ptr(%s)\n", ptr);
		return -1;
	}
	snprintf(arg->crash_mode, CRASH_MODE_MAX, "%s",  ptr);
	ptr = strtok(NULL, CRASH_DELIMITER);
	if (ptr == NULL) {
		PRT_TRACE_ERR("can't strtok linebuffer ptr(%s)\n", ptr);
		return -1;
	}
	snprintf(arg->crash_processname, CRASH_PROCESSNAME_MAX, "%s",  ptr);
	ptr = strtok(NULL, CRASH_DELIMITER);
	if (ptr == NULL) {
		PRT_TRACE_ERR("can't strtok linebuffer ptr(%s)\n", ptr);
		return -1;
	}
	snprintf(arg->crash_timestr, CRASH_TIME_MAX, "%s", ptr);
	ptr = strtok(NULL, CRASH_DELIMITER);
	if (ptr == NULL) {
		PRT_TRACE_ERR("can't strtok linebuffer ptr(%s)\n", ptr);
		return -1;
	}
	snprintf(arg->crash_pid, CRASH_PID_MAX, "%s", ptr);
	ptr = strtok(NULL, CRASH_DELIMITER);
	if (ptr == NULL) {
		PRT_TRACE_ERR("can't strtok linebuffer ptr(%s)\n", ptr);
		return -1;
	}
	snprintf(arg->crash_exepath, CRASH_EXEPATH_MAX, "%s", ptr);
	ptr = strtok(NULL, CRASH_DELIMITER);
	if (ptr == NULL) {
		PRT_TRACE_ERR("can't strtok linebuffer ptr(%s)\n", ptr);
		return -1;
	}
	snprintf(arg->crash_verify, CRASH_VERIFY_MAX, "%s", ptr);
	verify_num = strlen(arg->crash_processname) + strlen(arg->crash_exepath);
	verify_arg_num = atoi(arg->crash_verify);
	PRT_TRACE("vnum %d vanum %d\n", verify_num, verify_arg_num);
	if (verify_num == verify_arg_num)
		return 1;
	else
		return 0;
}
static void launch_crash_worker(void *data)
{
	static int popup_pid = 0;
	FILE *fp;
	int ret = -1;
	int len = 0;
	char linebuffer[CRASH_ARG_MAX] = {0,};
	char crash_worker_args[CRASH_ARG_MAX] = {0,};
	struct crash_arg parsing_arg;
	fp = fopen((char *)data, "r");
	if (fp == NULL) {
		return;
	}
	/* launch crash process */
	while (fgets(linebuffer, CRASH_ARG_MAX, fp) != NULL) {
		len = strlen(linebuffer);
		if (!len || linebuffer[len - 1] != '\n') {
			PRT_TRACE_ERR("crash inoti msg  must be terminated with new line character\n");
			break;
		}
		/* change last caracter from \n to \0 */
		linebuffer[strlen(linebuffer) - 1] = '\0';
		if (crash_arg_parser(linebuffer, &parsing_arg) != 1)
			continue;
		snprintf(crash_worker_args, sizeof(crash_worker_args), "%s %s %s %s %s",
				parsing_arg.crash_mode, parsing_arg.crash_processname,
				parsing_arg.crash_timestr, parsing_arg.crash_pid, parsing_arg.crash_exepath);
		PRT_TRACE("crash_worker args(%s)\n", crash_worker_args);
		PRT_TRACE("(%s%s%s)\n", parsing_arg.crash_mode,
				parsing_arg.crash_processname, parsing_arg.crash_timestr);
		ret = ss_launch_evenif_exist (CRASH_WORKER_PATH, crash_worker_args);
		if (ret > 0) {
			char buf[PATH_MAX];
			FILE *fpAdj;
			snprintf(buf, sizeof(buf), "/proc/%d/oom_adj", ret);
			fpAdj = fopen(buf, "w");
			if (fpAdj != NULL) {
				fprintf(fpAdj, "%d", (-17));
				fclose(fpAdj);
			}
		}
		if (!is_running_process(popup_pid))
			popup_pid = ss_launch_evenif_exist (CRASH_POPUP_PATH, parsing_arg.crash_processname);

		if (popup_pid < 0) {
			PRT_TRACE_ERR("popup failed)\n");
			break;
		}
	}
	fclose(fp);

	if (ret != -1) {
		fp = fopen((char *)data, "w");
		if (fp == NULL) {
			return;
		}
		fclose(fp);
	}

	return;
}

static Ecore_File_Monitor *crash_file_monitor;

static Ecore_File_Monitor_Cb __crash_file_cb(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
	switch (event) {
	case ECORE_FILE_EVENT_DELETED_DIRECTORY:
	case ECORE_FILE_EVENT_DELETED_SELF:
		if (0 > make_noti_file(CRASH_NOTI_DIR, CRASH_NOTI_FILE)) {
			launch_crash_worker((void *)path);
		}
		break;
	case ECORE_FILE_EVENT_MODIFIED:
	default:
		launch_crash_worker((void *)path);
		break;
	}

	return NULL;
}

int ss_bs_init(void)
{
	if (0 > make_noti_file(CRASH_NOTI_DIR, CRASH_NOTI_FILE)) {
		PRT_TRACE_ERR("make_noti_file() failed");
		launch_crash_worker((void *)CRASH_NOTI_PATH);
	}

	if (0 == ecore_file_init()) {
		PRT_TRACE_ERR("ecore_file_init() failed");
		launch_crash_worker((void *)CRASH_NOTI_PATH);
	}

	crash_file_monitor = ecore_file_monitor_add(CRASH_NOTI_PATH,(void *) __crash_file_cb, NULL);
	if (!crash_file_monitor) {
		PRT_TRACE_ERR("ecore_file_monitor_add() failed");
		launch_crash_worker((void *)CRASH_NOTI_PATH);
		return -1;
	}

	return 0;
}
