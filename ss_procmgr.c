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
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

#include <sysman.h>
#include "include/ss_data.h"
#include "ss_queue.h"
#include "ss_log.h"

#define LIMITED_PROCESS_OOMADJ 15

#ifdef NOUSE
typedef struct _node {
	pid_t pid;
	struct _node *next;
} Node;

static Node *head = NULL;

static Node *find_node(pid_t pid)
{
	Node *t = head;

	while (t != NULL) {
		if (t->pid == pid)
			break;
		t = t->next;
	}
	return t;
}

static Node *add_node(pid_t pid)
{
	Node *n;

	n = (Node *) malloc(sizeof(Node));
	if (n == NULL) {
		PRT_TRACE_ERR("Not enough memory, add cond. fail");
		return NULL;
	}

	n->pid = pid;
	n->next = head;
	head = n;

	return n;
}

static int del_node(Node *n)
{
	Node *t;
	Node *prev;

	if (n == NULL)
		return 0;

	t = head;
	prev = NULL;
	while (t != NULL) {
		if (t == n) {
			if (prev != NULL)
				prev->next = t->next;
			else
				head = head->next;
			free(t);
			break;
		}
		prev = t;
		t = t->next;
	}
	return 0;
}
#endif

int get_app_oomadj(int pid, int *oomadj)
{
	if (pid < 0)
		return -1;

	char buf[PATH_MAX];
	FILE *fp = NULL;

	snprintf(buf, sizeof(buf), "/proc/%d/oom_adj", pid);
	fp = fopen(buf, "r");
	if (fp == NULL)
		return -1;
	if (fgets(buf, PATH_MAX, fp) == NULL) {
		fclose(fp);
		return -1;
	}
	(*oomadj) = atoi(buf);
	fclose(fp);
	return 0;
}

int set_app_oomadj(pid_t pid, int new_oomadj)
{
	char buf[PATH_MAX];
	FILE *fp;
	int old_oomadj;
	char exe_name[PATH_MAX];

	if (sysman_get_cmdline_name(pid, exe_name, PATH_MAX) < 0)
		snprintf(exe_name, sizeof(exe_name), "Unknown (maybe dead)");

	snprintf(buf, sizeof(buf), "/proc/%d/oom_adj", pid);
	fp = fopen(buf, "r");
	if (fp == NULL)
		return -1;
	if (fgets(buf, PATH_MAX, fp) == NULL) {
		fclose(fp);
		return -1;
	}
	old_oomadj = atoi(buf);
	fclose(fp);
	PRT_TRACE_EM("Process %s, pid %d, old_oomadj %d", exe_name, pid,
		     old_oomadj);

	if (old_oomadj < OOMADJ_APP_LIMIT)
		return 0;

	PRT_TRACE_EM("Process %s, pid %d, new_oomadj %d", exe_name, pid,
		     new_oomadj);
	snprintf(buf, sizeof(buf), "/proc/%d/oom_adj", pid);
	fp = fopen(buf, "w");
	if (fp == NULL)
		return -1;

	fprintf(fp, "%d", new_oomadj);
	fclose(fp);

	return 0;
}

int set_oomadj_action(int argc, char **argv)
{
	int pid = -1;
	int new_oomadj = -20;

	if (argc < 2)
		return -1;
	if ((pid = atoi(argv[0])) < 0 || (new_oomadj = atoi(argv[1])) <= -20)
		return -1;

	char buf[255];
	FILE *fp;

	PRT_TRACE_EM("OOMADJ_SET : pid %d, new_oomadj %d", pid, new_oomadj);
	snprintf(buf, sizeof(buf), "/proc/%d/oom_adj", pid);
	fp = fopen(buf, "w");
	if (fp == NULL)
		return -1;
	fprintf(fp, "%d", new_oomadj);
	fclose(fp);

	return 0;
}

int check_and_set_old_backgrd()
{
	int pid = -1;
	DIR *dp;
	struct dirent *dentry;
	FILE *fp;
	char buf[PATH_MAX];
	int token =0;

	dp = opendir("/proc");
	if (!dp) {
		PRT_TRACE_EM("BACKGRD MANAGE : fail to open /proc : %s", strerror(errno));
		return -1;
	}

	while ((dentry = readdir(dp)) != NULL) {
		if (!isdigit(dentry->d_name[0]))
			continue;

		pid = atoi(dentry->d_name);

		snprintf(buf, sizeof(buf), "/proc/%d/oom_adj", pid);
		fp = fopen(buf, "r");
		if (fp == NULL)
			continue;
		if (fgets(buf, sizeof(buf), fp) == NULL) {
			fclose(fp);
			continue;
		}
		if(atoi(buf) == OOMADJ_BACKGRD_UNLOCKED) {
			fclose(fp);
			PRT_TRACE("BACKGRD MANAGE : process wchich has OOMADJ_BACKGRD_UNLOCKED exists");
			break;
		}
		fclose(fp);
	}
	closedir(dp);

	if(dentry != NULL ) {
		int cur_oom=-1;
		dp = opendir("/proc");
		if (!dp) {
			PRT_TRACE_EM("BACKGRD MANAGE : fail to open /proc : %s", strerror(errno));
			return -1;
		}
		while ((dentry = readdir(dp)) != NULL) {

			if (!isdigit(dentry->d_name[0]))
				continue;

			pid = atoi(dentry->d_name);

			snprintf(buf, sizeof(buf), "/proc/%d/oom_adj", pid);
			fp = fopen(buf, "r+");
			if (fp == NULL)
				continue;
			if (fgets(buf, sizeof(buf), fp) == NULL) {
				fclose(fp); 
				continue;
			}                          
			cur_oom = atoi(buf);
			if(cur_oom >= LIMITED_PROCESS_OOMADJ) {
				PRT_TRACE("BACKGRD MANAGE : kill the process %d (oom_adj %d)", pid, cur_oom);
				kill(pid, SIGTERM);
			}
			else if(cur_oom >= OOMADJ_BACKGRD_UNLOCKED) {
				PRT_TRACE("BACKGRD MANAGE : process %d set oom_adj %d (before %d)", pid, cur_oom+1, cur_oom);
				fprintf(fp, "%d", ++cur_oom);
			}
			fclose(fp);
		}
		closedir(dp);
	}
	return 0;
}

int set_active_action(int argc, char **argv)
{
	int pid = -1;
	int ret = 0;
	int oomadj = 0;

	if (argc < 1)
		return -1;
	if ((pid = atoi(argv[0])) < 0)
		return -1;

	if (get_app_oomadj(pid, &oomadj) < 0)
		return -1;

	switch (oomadj) {
	case OOMADJ_FOREGRD_LOCKED:
	case OOMADJ_BACKGRD_LOCKED:
	case OOMADJ_SU:
		ret = 0;
		break;
	case OOMADJ_FOREGRD_UNLOCKED:
		ret = set_app_oomadj((pid_t) pid, OOMADJ_FOREGRD_LOCKED);
		break;
	case OOMADJ_BACKGRD_UNLOCKED:
		ret = set_app_oomadj((pid_t) pid, OOMADJ_BACKGRD_LOCKED);
		break;
	case OOMADJ_INIT:
		ret = set_app_oomadj((pid_t) pid, OOMADJ_BACKGRD_LOCKED);
		break;
	default:
		if(oomadj > OOMADJ_BACKGRD_UNLOCKED) {
			ret = set_app_oomadj((pid_t) pid, OOMADJ_BACKGRD_LOCKED);
		} else {
			PRT_TRACE_EM("Unknown oomadj value (%d) !", oomadj);
			ret = -1;
		}
		break;
	}
	return ret;
}

int set_inactive_action(int argc, char **argv)
{
	int pid = -1;
	int ret = 0;
	int oomadj = 0;

	if (argc < 1)
		return -1;
	if ((pid = atoi(argv[0])) < 0)
		return -1;

	if (get_app_oomadj(pid, &oomadj) < 0)
		return -1;

	switch (oomadj) {
	case OOMADJ_FOREGRD_UNLOCKED:
	case OOMADJ_BACKGRD_UNLOCKED:
	case OOMADJ_SU:
		ret = 0;
		break;
	case OOMADJ_FOREGRD_LOCKED:
		ret = set_app_oomadj((pid_t) pid, OOMADJ_FOREGRD_UNLOCKED);
		break;
	case OOMADJ_BACKGRD_LOCKED:
		check_and_set_old_backgrd();
		ret = set_app_oomadj((pid_t) pid, OOMADJ_BACKGRD_UNLOCKED);
		break;
	case OOMADJ_INIT:
		check_and_set_old_backgrd();
		ret = set_app_oomadj((pid_t) pid, OOMADJ_BACKGRD_UNLOCKED);
		break;
	default:
		if(oomadj > OOMADJ_BACKGRD_UNLOCKED) {
			ret = 0;
		} else {
			PRT_TRACE_EM("Unknown oomadj value (%d) !", oomadj);
			ret = -1;
		}
		break;

	}
	return ret;
}

int set_foregrd_action(int argc, char **argv)
{
	int pid = -1;
	int ret = 0;
	int oomadj = 0;

	if (argc < 1)
		return -1;
	if ((pid = atoi(argv[0])) < 0)
		return -1;

	if (get_app_oomadj(pid, &oomadj) < 0)
		return -1;

	switch (oomadj) {
	case OOMADJ_FOREGRD_LOCKED:
	case OOMADJ_FOREGRD_UNLOCKED:
	case OOMADJ_SU:
		ret = 0;
		break;
	case OOMADJ_BACKGRD_LOCKED:
		ret = set_app_oomadj((pid_t) pid, OOMADJ_FOREGRD_LOCKED);
		break;
	case OOMADJ_BACKGRD_UNLOCKED:
		ret = set_app_oomadj((pid_t) pid, OOMADJ_FOREGRD_UNLOCKED);
		break;
	case OOMADJ_INIT:
		ret = set_app_oomadj((pid_t) pid, OOMADJ_FOREGRD_UNLOCKED);
		break;
	default:
		if(oomadj > OOMADJ_BACKGRD_UNLOCKED) {
			ret = set_app_oomadj((pid_t) pid, OOMADJ_FOREGRD_UNLOCKED);
		} else {                
			PRT_TRACE_EM("Unknown oomadj value (%d) !", oomadj);
			ret = -1;                                       
		}
		break;

	}
	return ret;
}

int set_backgrd_action(int argc, char **argv)
{
	int pid = -1;
	int ret = 0;
	int oomadj = 0;

	if (argc < 1)
		return -1;
	if ((pid = atoi(argv[0])) < 0)
		return -1;

	if (get_app_oomadj(pid, &oomadj) < 0)
		return -1;

	switch (oomadj) {
	case OOMADJ_BACKGRD_LOCKED:
	case OOMADJ_BACKGRD_UNLOCKED:
	case OOMADJ_SU:
		ret = 0;
		break;
	case OOMADJ_FOREGRD_LOCKED:
		ret = set_app_oomadj((pid_t) pid, OOMADJ_BACKGRD_LOCKED);
		break;
	case OOMADJ_FOREGRD_UNLOCKED:
		check_and_set_old_backgrd();
		ret = set_app_oomadj((pid_t) pid, OOMADJ_BACKGRD_UNLOCKED);
		break;
	case OOMADJ_INIT:
		check_and_set_old_backgrd();
		ret = set_app_oomadj((pid_t) pid, OOMADJ_BACKGRD_UNLOCKED);
		break;
	default:
		if(oomadj > OOMADJ_BACKGRD_UNLOCKED) {
			ret = 0;
		} else {                
			PRT_TRACE_EM("Unknown oomadj value (%d) !", oomadj);
			ret = -1;                                       
		}
		break;
	}
	return ret;
}

int ss_process_manager_init(void)
{
	ss_action_entry_add_internal(PREDEF_FOREGRD, set_foregrd_action, NULL,
				     NULL);
	ss_action_entry_add_internal(PREDEF_BACKGRD, set_backgrd_action, NULL,
				     NULL);
	ss_action_entry_add_internal(PREDEF_ACTIVE, set_active_action, NULL,
				     NULL);
	ss_action_entry_add_internal(PREDEF_INACTIVE, set_inactive_action, NULL,
				     NULL);
	ss_action_entry_add_internal(OOMADJ_SET, set_oomadj_action, NULL, NULL);
	return 0;
}
