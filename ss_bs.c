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

#define BSNOTI_DIR		"/opt/bs"
#define BSNOTI_FILE		"curbs.log"
#define BSNOTI_FULL_PATH	BSNOTI_DIR"/"BSNOTI_FILE

#define CRASH_WORKER_PATH	"/usr/apps/org.tizen.crash-worker/bin/crash-worker"

static int noti_fd;
static int add_noti(void);

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

	fd = open(buf, O_CREAT, S_IRUSR | S_IWUSR);
	close(fd);
	snprintf(buf, sizeof(buf), "chmod 666 %s/%s", path, file);
	system(buf);
	snprintf(buf, sizeof(buf), "chown root:app %s/%s", path, file);
	system(buf);

	return 0;
}

static void launch_crash_worker(void *data)
{
	FILE *fp;
	char bsfile_name[NAME_MAX], bs_color[MAX_INPUT];
	char args[NAME_MAX + MAX_INPUT];
	int ret = -1, i;

	fp = fopen((char *)data, "r");
	if (fp == NULL) {
		return;
	}
	/* launch bs process */
	while (fgets(args, NAME_MAX + MAX_INPUT, fp) != NULL) {
		/* add rule for log */
		if (args[strlen(args) - 1] != '\n') {
			PRT_TRACE_ERR("bsfile log must be terminated with new line character\n");
			break;
		}
		/* change last caracter from \n to \0 */
		args[strlen(args) - 1] = '\0';
		for (i = 0; i < NAME_MAX + MAX_INPUT; i++) {
			if (args[i] == ' ') {
				if (i >= NAME_MAX - 1) {
					PRT_TRACE_ERR("bsfile name is over 254. 255(NAME_MAX) - 1(NULL Termination)\n");
					break;
				}
				strncpy(bsfile_name, args, i);
				bsfile_name[i] = '\0';
				strncpy(bs_color, args + i + 1, MAX_INPUT);
				bs_color[MAX_INPUT - 1] = '\0';
				snprintf(args, sizeof(args), "%s %s",
					 bsfile_name, bs_color);
				PRT_TRACE("bsfile_name(size %d): %s\nargs: %s\n", i, bsfile_name, bs_color, args);
				ret = ss_launch_evenif_exist (CRASH_WORKER_PATH, args);
				break;
			}
		}
		if (ret < 0)
			break;
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

static Ecore_File_Monitor *bs_file_monitor;

static Ecore_File_Monitor_Cb __bs_file_cb(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
	switch (event) {
	case ECORE_FILE_EVENT_DELETED_DIRECTORY:
	case ECORE_FILE_EVENT_DELETED_SELF:
		if (0 > make_noti_file(BSNOTI_DIR, BSNOTI_FILE)) {
			launch_crash_worker((void *)path);
		}
		break;
	case ECORE_FILE_EVENT_MODIFIED:
	default:
		launch_crash_worker((void *)path);
		break;
	}

	return;
}

int ss_bs_init(void)
{
	if (0 > make_noti_file(BSNOTI_DIR, BSNOTI_FILE)) {
		PRT_TRACE_ERR("make_noti_file() failed");
		launch_crash_worker((void *)BSNOTI_FULL_PATH);
	}

	if (0 == ecore_file_init()) {
		PRT_TRACE_ERR("ecore_file_init() failed");
		launch_crash_worker((void *)BSNOTI_FULL_PATH);
	}

	bs_file_monitor = ecore_file_monitor_add(BSNOTI_FULL_PATH,(void *) __bs_file_cb, NULL);
	if (!bs_file_monitor) {
		PRT_TRACE_ERR("ecore_file_monitor_add() failed");
		launch_crash_worker((void *)BSNOTI_FULL_PATH);
		return -1;
	}

	return 0;
}
