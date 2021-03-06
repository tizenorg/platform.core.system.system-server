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


#include <fcntl.h>
#include <assert.h>
#include <limits.h>
#include <sysman.h>
#include <heynoti.h>
#include <vconf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>

#include "ss_device_plugin.h"
#include "ss_log.h"
#include "ss_noti.h"
#include "ss_queue.h"
#include "include/ss_data.h"

#define DELETE_SM		"sh -c "PREFIX"/bin/delete.sm"

#define MEMNOTIFY_NORMAL	0x0000
#define MEMNOTIFY_LOW		0xfaac
#define MEMNOTIFY_CRITICAL	0xdead
#define MEMNOTIFY_REBOOT	0xb00f

#define _SYS_RES_CLEANUP	"RES_CLEANUP"

#define MEM_THRESHOLD_LV1	60
#define MEM_THRESHOLD_LV2	40


struct lowmem_process_entry {
	unsigned cur_mem_state;
	unsigned new_mem_state;
	int (*action) (void *);
};

static int lowmem_fd = -1;
static int cur_mem_state = MEMNOTIFY_NORMAL;

Ecore_Timer *oom_timer;
#define OOM_TIMER_INTERVAL	5

static int memory_low_act(void *ad);
static int memory_oom_act(void *ad);
static int memory_normal_act(void *ad);

static struct lowmem_process_entry lpe[] = {
	{MEMNOTIFY_NORMAL, MEMNOTIFY_LOW, memory_low_act},
	{MEMNOTIFY_NORMAL, MEMNOTIFY_CRITICAL, memory_oom_act},
	{MEMNOTIFY_LOW, MEMNOTIFY_CRITICAL, memory_oom_act},
	{MEMNOTIFY_CRITICAL, MEMNOTIFY_CRITICAL, memory_oom_act},
	{MEMNOTIFY_LOW, MEMNOTIFY_NORMAL, memory_normal_act},
	{MEMNOTIFY_CRITICAL, MEMNOTIFY_NORMAL, memory_normal_act},

};

unsigned int oom_delete_sm_time = 0;

static int remove_shm()
{
	int maxid, shmid, id;
	struct shmid_ds shmseg;
	struct shm_info shm_info;

	maxid = shmctl(0, SHM_INFO, (struct shmid_ds *)(void *)&shm_info);
	if (maxid < 0) {
		PRT_TRACE_ERR("shared mem error\n");
		return -1;
	}

	for (id = 0; id <= maxid; id++) {
		shmid = shmctl(id, SHM_STAT, &shmseg);
		if (shmid < 0)
			continue;
		if (shmseg.shm_nattch == 0) {
			PRT_TRACE("shared memory killer ==> %d killed\n",
				  shmid);
			shmctl(shmid, IPC_RMID, NULL);
		}
	}
	return 0;
}

static char *convert_to_str(unsigned int mem_state)
{
	char *tmp;
	switch (mem_state) {
	case MEMNOTIFY_NORMAL:
		tmp = "mem normal";
		break;
	case MEMNOTIFY_LOW:
		tmp = "mem low";
		break;
	case MEMNOTIFY_CRITICAL:
		tmp = "mem critical";
		break;
	case MEMNOTIFY_REBOOT:
		tmp = "mem reboot";
		break;
	default:
		assert(0);
	}
	return tmp;
}

static void print_lowmem_state(unsigned int mem_state)
{
	PRT_TRACE("[LOW MEM STATE] %s ==> %s", convert_to_str(cur_mem_state),
		  convert_to_str(mem_state));
}

static int memory_low_act(void *data)
{
	char lowmem_noti_name[NAME_MAX];

	PRT_TRACE("[LOW MEM STATE] memory low state");
	remove_shm();

	heynoti_get_snoti_name(_SYS_RES_CLEANUP, lowmem_noti_name, NAME_MAX);
	ss_noti_send(lowmem_noti_name);
	vconf_set_int(VCONFKEY_SYSMAN_LOW_MEMORY,
		      VCONFKEY_SYSMAN_LOW_MEMORY_SOFT_WARNING);
		
	ss_action_entry_call_internal(PREDEF_LOWMEM, 1, LOW_MEM_ACT);

	return 0;
}

static int memory_oom_act(void *data)
{
	unsigned int cur_time;
	char lowmem_noti_name[NAME_MAX];

	PRT_TRACE("[LOW MEM STATE] memory oom state");
	cur_time = time(NULL);
	PRT_TRACE("cur=%d, old=%d, cur-old=%d", cur_time, oom_delete_sm_time,
		  cur_time - oom_delete_sm_time);
	if (cur_time - oom_delete_sm_time > 15) {
		remove_shm();
		oom_delete_sm_time = cur_time;
		/* Also clean up unreturned memory of applications */
		heynoti_get_snoti_name(_SYS_RES_CLEANUP, lowmem_noti_name,
				       NAME_MAX);
		ss_noti_send(lowmem_noti_name);
	}
	vconf_set_int(VCONFKEY_SYSMAN_LOW_MEMORY,
		      VCONFKEY_SYSMAN_LOW_MEMORY_HARD_WARNING);

	ss_action_entry_call_internal(PREDEF_LOWMEM, 1, OOM_MEM_ACT);

	return 1;
}

static int memory_normal_act(void *data)
{
	PRT_TRACE("[LOW MEM STATE] memory normal state");
	vconf_set_int(VCONFKEY_SYSMAN_LOW_MEMORY,
		      VCONFKEY_SYSMAN_LOW_MEMORY_NORMAL);
	return 0;
}

static int lowmem_process(unsigned int mem_state, void *ad)
{
	int i;
	for (i = 0; i < sizeof(lpe) / sizeof(struct lowmem_process_entry); i++) {
		if ((cur_mem_state == lpe[i].cur_mem_state)
		    && (mem_state == lpe[i].new_mem_state)) {

			if(oom_timer != NULL) {
				ecore_timer_del(oom_timer);
				oom_timer = NULL;
			}
			lpe[i].action(ad);
			if(mem_state == MEMNOTIFY_CRITICAL) 
				oom_timer = ecore_timer_add(OOM_TIMER_INTERVAL,lpe[i].action, ad);
			return 0;
		}
	}
	return 0;
}

static unsigned int lowmem_read(int fd)
{
	unsigned int mem_state;
	read(fd, &mem_state, sizeof(mem_state));
	return mem_state;
}

static int lowmem_cb(void *data, Ecore_Fd_Handler * fd_handler)
{
	int fd;
	struct ss_main_data *ad = (struct ss_main_data *)data;
	unsigned int mem_state;

	if (!ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ)) {
		PRT_TRACE_ERR
		    ("ecore_main_fd_handler_active_get error , return\n");
		return 1;
	}

	fd = ecore_main_fd_handler_fd_get(fd_handler);

	mem_state = lowmem_read(fd);
	print_lowmem_state(mem_state);
	lowmem_process(mem_state, ad);
	cur_mem_state = mem_state;

	return 1;
}

static int set_threshold()
{
	if (0 > plugin_intf->OEM_sys_set_memnotify_threshold_lv1(MEM_THRESHOLD_LV1)) {
		PRT_TRACE_ERR("Set memnorify threshold lv1 failed");
		return -1;
	}

	if (0 > plugin_intf->OEM_sys_set_memnotify_threshold_lv2(MEM_THRESHOLD_LV2)) {
		PRT_TRACE_ERR("Set memnorify threshold lv2 failed");
		return -1;
	}

	return 0;
}

int ss_lowmem_init(struct ss_main_data *ad)
{
	char lowmem_dev_node[PATH_MAX];

	if (0 > plugin_intf->OEM_sys_get_memnotify_node(lowmem_dev_node)) {
		PRT_TRACE_ERR("Low memory handler fd init failed");
		return -1;
	}

	lowmem_fd = open(lowmem_dev_node, O_RDONLY);
	if (lowmem_fd < 0) {
		PRT_TRACE_ERR("ss_lowmem_init fd open failed");
		return -1;
	}

	oom_timer = NULL;
	ecore_main_fd_handler_add(lowmem_fd, ECORE_FD_READ, lowmem_cb, ad, NULL,
				  NULL);
	if (set_threshold() < 0) {
		PRT_TRACE_ERR("Setting lowmem threshold is failed");
		return -1;
	}

	return 0;
}
