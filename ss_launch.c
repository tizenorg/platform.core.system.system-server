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
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include "vconf-keys.h"
#include "ss_log.h"
#include "ss_launch.h"

#define MAX_ARGS 255

#define _S(str) ((str == NULL) ? "" : str)

int ss_set_current_lang(void)
{
	char *lang;
	int ret;
	lang = vconf_get_str(VCONFKEY_LANGSET);
	if (lang == NULL)
		return -1;
	ret = setenv("LANG", lang, 1);
	if (ret < 0)
		return -1;
	free(lang);
	return 0;
}


static void prepare_exec(void)
{
	int i;
	int maxfd;
	char buf[MAX_ARGS];
	FILE *fp;

	maxfd = getdtablesize();
	for (i = 3; i < maxfd; i++)
		close(i);

	for (i = 0; i < _NSIG; i++)
		signal(i, SIG_DFL);

	/* RESET oomadj value */
	sprintf(buf,"/proc/%d/oom_adj",getpid());
	fp = fopen(buf, "w");          
	if (fp == NULL)                       
		return;                               
	fprintf(fp, "%d", 0);                  
	fclose(fp);
}

static int parse_cmd(const char *cmdline, char **argv, int max_args)
{
	const char *p;
	char *buf, *bufp;
	int nargs = 0;
	int escape = 0, squote = 0, dquote = 0;

	if (cmdline == NULL || cmdline[0] == '\0')
		return -1;

	bufp = buf = malloc(strlen(cmdline) + 1);
	if (bufp == NULL || buf == NULL)
		return -1;

	p = cmdline;

	while (*p) {
		if (escape) {
			*bufp++ = *p;
			escape = 0;
		} else {
			switch (*p) {
				case '\\':
					escape = 1;
					break;
				case '"':
					if (squote)
						*bufp++ = *p;
					else
						dquote = !dquote;
					break;
				case '\'':
					if (dquote)
						*bufp++ = *p;
					else
						squote = !squote;
					break;
				case ' ':
					if (!squote && !dquote) {
						*bufp = '\0';
						if (nargs < max_args)
							argv[nargs++] = strdup(buf);
						bufp = buf;
						break;
					}
				default:
					*bufp++ = *p;
					break;
			}
		}
		p++;
	}

	if (bufp != buf) {
		*bufp = '\0';
		if (nargs < max_args)
			argv[nargs++] = strdup(buf);
	}

	argv[nargs++] = NULL;

	free(buf);
	return nargs;
}

int launch_app_with_nice(const char *file, char *const argv[], pid_t *pid, int _nice)
{
	int ret;
	int _pid;

	if (file == NULL || access(file, X_OK) != 0) {
		PRT_TRACE_ERR("launch app error: Invalid input");
		errno = EINVAL;
		return -1;
	}

	if (pid && (*pid > 0 && kill(*pid, 0) != -1))
		return *pid;

	_pid = fork();

	if (_pid == -1) {
		PRT_TRACE_ERR("fork error: %s", strerror(errno));
		/* keep errno */
		return -1;
	}

	if (_pid > 0) {     /* parent */
		if (pid)
			*pid = _pid;
		return _pid;
	}

	/* child */
	prepare_exec();

	ret = nice(_nice);

	if (ret == -1 && errno != 0)
		PRT_TRACE_ERR("nice error: %s", strerror(errno));

	ret = execvp(file, argv);

	/* If failed... */
	PRT_TRACE_ERR("exec. error: %s", strerror(errno));
	return -2;
}

int launch_app_cmd_with_nice(const char *cmdline, int _nice)
{
	int i;
	int nargs;
	int ret;
	char *argv[MAX_ARGS + 1];

	nargs = parse_cmd(cmdline, argv, MAX_ARGS + 1);
	if (nargs == -1) {
		PRT_TRACE_ERR("launch app error: Invalid input");
		errno = EINVAL;
		return -1;
	}

	ret = launch_app_with_nice(argv[0], argv, NULL, _nice);

	for (i = 0; i < nargs; i++)
		free(argv[i]);

	return ret;
}

int launch_app_cmd(const char *cmdline)
{
	return launch_app_cmd_with_nice(cmdline, 0);
}

int ss_launch_if_noexist(const char *execpath, const char *arg, ...)
{
	char *buf;
	int pid;
	int nice_value = 0;
	int flag = 0;
	int buf_size = -1;
	va_list argptr;

	if (execpath == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (pid = sysman_get_pid(execpath) > 0)
		return pid;

	va_start(argptr, arg);
	flag = va_arg(argptr, int);

	if (flag & SS_LAUNCH_NICE)
		nice_value = va_arg(argptr, int);

	va_end(argptr);

	ss_set_current_lang();
	arg = _S(arg);

	buf_size = strlen(execpath) + strlen(arg) + 10;
	buf = malloc(buf_size);
	if (buf == NULL) {
		/* Do something for not enought memory error */
		PRT_TRACE_ERR("Malloc failed");
		return -1;
	}

	snprintf(buf, buf_size, "%s %s", execpath, arg);
	//pid = launch_app_cmd_with_nice(buf, nice_value, flag);
	pid = launch_app_cmd_with_nice(buf, nice_value);
	if (pid == -2)
		exit(0);
	free(buf);

	return pid;
}

int ss_launch_evenif_exist(const char *execpath, const char *arg, ...)
{
	char *buf;
	int pid;
	int nice_value = 0;
	int flag = 0;
	int buf_size = -1;

	va_list argptr;

	if (execpath == NULL) {
		errno = EINVAL;
		return -1;
	}

	va_start(argptr, arg);
	flag = va_arg(argptr, int);

	if (flag & SS_LAUNCH_NICE)
		nice_value = va_arg(argptr, int);

	va_end(argptr);

	ss_set_current_lang();

	arg = _S(arg);

	buf_size = strlen(execpath) + strlen(arg) + 10;
	buf = malloc(buf_size);
	if (buf == NULL) {
		/* Do something for not enought memory error */
		PRT_TRACE_ERR("Malloc failed");
		return -1;
	}

	snprintf(buf, buf_size, "%s %s", execpath, arg);
	//pid = launch_app_cmd_with_nice(buf, nice_value, flag);
	pid = launch_app_cmd_with_nice(buf, nice_value);
	if (pid == -2)
		exit(0);
	free(buf);

	return pid;
}

int ss_launch_after_kill_if_exist(const char *execpath, const char *arg, ...)
{
	char *buf;
	int pid;
	int nice_value = 0;
	int flag = 0;
	int buf_size = -1;
	int exist_pid;
	va_list argptr;

	if (execpath == NULL) {
		errno = EINVAL;
		return -1;
	}

	if ((exist_pid = sysman_get_pid(execpath)) > 0)
		kill(exist_pid, SIGTERM);

	va_start(argptr, arg);
	flag = va_arg(argptr, int);

	if (flag & SS_LAUNCH_NICE)
		nice_value = va_arg(argptr, int);

	va_end(argptr);

	ss_set_current_lang();

	arg = _S(arg);

	buf_size = strlen(execpath) + strlen(arg) + 10;
	buf = malloc(buf_size);
	if (buf == NULL) {
		/* Do something for not enought memory error */
		PRT_TRACE_ERR("Malloc Failed");
		return -1;
	}

	snprintf(buf, buf_size, "%s %s", execpath, arg);
	//pid = launch_app_cmd_with_nice(buf, nice_value, flag);
	pid = launch_app_cmd_with_nice(buf, nice_value);
	if (pid == -2)
		exit(0);
	free(buf);

	return pid;

}
