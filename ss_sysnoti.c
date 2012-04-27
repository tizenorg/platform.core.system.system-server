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


#include <sysman.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include "include/ss_data.h"
#include "ss_log.h"
#include "ss_queue.h"

#define SYSNOTI_SOCKET_PATH "/tmp/sn"

enum sysnoti_cmd {
	ADD_SYSMAN_ACTION,
	CALL_SYSMAN_ACTION
};

static void print_sysnoti_msg(const char *title, struct sysnoti *msg)
{
	int i;
	char exe_name[PATH_MAX];

	if (sysman_get_cmdline_name(msg->pid, exe_name, PATH_MAX) < 0)
		snprintf(exe_name, sizeof(exe_name), "Unknown (maybe dead)");

	PRT_TRACE_ERR("=====================");
	PRT_TRACE_ERR("pid : %d", msg->pid);
	PRT_TRACE_ERR("process name : %s", exe_name);
	PRT_TRACE_ERR("cmd : %d", msg->cmd);
	PRT_TRACE_ERR("type : %s", msg->type);
	PRT_TRACE_ERR("path : %s", msg->path);
	for (i = 0; i < msg->argc; i++)
		PRT_TRACE_ERR("arg%d : %s", i, msg->argv[i]);
	PRT_TRACE_ERR("=====================");
}

static inline int recv_int(int fd)
{
	int val, r = -1;
	while(1) {
		r = read(fd, &val, sizeof(int));
		if (r < 0) {
			if(errno == EINTR) {
				PRT_TRACE_ERR("Re-read for error(EINTR)");
				continue;
			}
			else {
				PRT_TRACE_ERR("Read fail for int");
				return -1;
			}
		} else {
			return val;
		}
	}
}

static inline char *recv_str(int fd)
{
	int len, r = -1;
	char *str;

	while(1) {
		r = read(fd, &len, sizeof(int));
		if (r < 0) {
			if(errno == EINTR) {
				PRT_TRACE_ERR("Re-read for error(EINTR)");
				continue;
			}
			else {
				PRT_TRACE_ERR("Read fail for str length");
				return NULL;
			}
		} else
			break;
	}

	if (len <= 0) {
		PRT_TRACE_ERR("str is null");
		return NULL;
	}

	if (len >= INT_MAX) {
		PRT_TRACE_ERR("size is over INT_MAX");
		return NULL;
	}

	str = (char *)malloc(len + 1);
	if (str == NULL) {
		PRT_TRACE_ERR("Not enough memory");
		return NULL;
	}

	while(1) {
		r = read(fd, str, len);
		if(r < 0) {
			if(errno == EINTR) {
				PRT_TRACE_ERR("Re-read for error(EINTR)");
				continue;                       
			}                                               
			else {      
				PRT_TRACE_ERR("Read fail for str");
				return NULL;
			}                                                           
		} else
			break;
	}
	str[len] = 0;

	return str;
}

static int read_message(int fd, struct sysnoti *msg)
{
	int i;

	msg->pid = recv_int(fd);
	msg->cmd = recv_int(fd);
	msg->type = recv_str(fd);
	msg->path = recv_str(fd);
	msg->argc = recv_int(fd);

	for (i = 0; i < msg->argc; i++)
		msg->argv[i] = recv_str(fd);

	return 0;
}

static inline void internal_free(char *str)
{
	if (!str)
		free(str);
}

static inline void free_message(struct sysnoti *msg)
{
	internal_free(msg->type);
	internal_free(msg->path);
	free(msg);
}

static int sysnoti_cb(void *data, Ecore_Fd_Handler * fd_handler)
{
	int fd;
	struct sysnoti *msg;
	int ret = -1;
	struct sockaddr_un client_address;
	int client_sockfd;
	int client_len;

	if (!ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ)) {
		PRT_TRACE_ERR
		    ("ecore_main_fd_handler_active_get error , return\n");
		return 1;
	}

	fd = ecore_main_fd_handler_fd_get(fd_handler);

	msg = malloc(sizeof(struct sysnoti));
	if (msg == NULL) {
		PRT_TRACE_ERR("%s : Not enough memory", __FUNCTION__);
		return 1;
	}

	client_len = sizeof(client_address);
	client_sockfd =
	    accept(fd, (struct sockaddr *)&client_address,
		   (socklen_t *)&client_len);
	if(client_sockfd == -1) {
		PRT_TRACE_ERR("socket accept error");
		return 1;
	}

	if (read_message(client_sockfd, msg) < 0) {
		PRT_TRACE_ERR("%s : recv error msg", __FUNCTION__);
		free(msg);
		write(client_sockfd, &ret, sizeof(int));
		close(client_sockfd);
		return 1;
	}

	print_sysnoti_msg(__FUNCTION__, msg);
	if (msg->argc > SYSMAN_MAXARG) {
		PRT_TRACE_ERR("%s : error argument", __FUNCTION__);
		free_message(msg);
		write(client_sockfd, &ret, sizeof(int));
		close(client_sockfd);
		return 1;
	}

	switch (msg->cmd) {
#ifdef NOUSE
	case ADD_SYSMAN_ACTION:
		ret = ss_action_entry_add(msg);
		PRT_TRACE_ERR("%s : ss_action_entry_add : %d", __FUNCTION__,ret);
		break;
#endif
	case CALL_SYSMAN_ACTION:
		ret = ss_action_entry_call(msg, msg->argc, msg->argv);
		break;
	default:
		ret = -1;
	}

	write(client_sockfd, &ret, sizeof(int));
	close(client_sockfd);

	free_message(msg);

	return 1;
}

static int ss_sysnoti_server_init(void)
{
	int fd;
	struct sockaddr_un serveraddr;

	if (access(SYSNOTI_SOCKET_PATH, F_OK) == 0)
		unlink(SYSNOTI_SOCKET_PATH);

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		PRT_ERR("%s: socket create failed\n", __FUNCTION__);
		return -1;
	}

	if((fsetxattr(fd, "security.SMACK64IPOUT", "@", 2, 0)) < 0 ) {
		PRT_ERR("%s: Socket SMACK labeling failed\n", __FUNCTION__);
		if(errno != EOPNOTSUPP)
			return -1;
	}

	if((fsetxattr(fd, "security.SMACK64IPIN", "*", 2, 0)) < 0 ) {
		PRT_ERR("%s: Socket SMACK labeling failed\n", __FUNCTION__);
		if(errno != EOPNOTSUPP)
			return -1;
	}

	bzero(&serveraddr, sizeof(struct sockaddr_un));
	serveraddr.sun_family = AF_UNIX;
	strncpy(serveraddr.sun_path, SYSNOTI_SOCKET_PATH,
		sizeof(serveraddr.sun_path));

	if (bind(fd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr)) <
	    0) {
		PRT_ERR("%s: socket bind failed\n", __FUNCTION__);
		return -1;
	}

	if (chmod(SYSNOTI_SOCKET_PATH, (S_IRWXU | S_IRWXG | S_IRWXO)) < 0)	/* 0777 */
		PRT_ERR("failed to change the socket permission");

	listen(fd, 5);

	PRT_INFO("socket create & listen ok\n");

	return fd;
}

int ss_sysnoti_init(void)
{
	int fd;
	fd = ss_sysnoti_server_init();
	ecore_main_fd_handler_add(fd, ECORE_FD_READ, sysnoti_cb, NULL, NULL,
				  NULL);
	return fd;
}
