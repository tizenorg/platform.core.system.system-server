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


#include <fcntl.h>

#include "ss_device_plugin.h"
#include "ss_log.h"
#include "include/ss_data.h"

#define DEFAULT_MAX_CPU_FREQ		12000000
#define DEFAULT_MIN_CPU_FREQ		1000000

static int max_cpu_freq_limit = -1;
static int min_cpu_freq_limit = -1;
static int cur_max_cpu_freq = INT_MIN;
static int cur_min_cpu_freq = INT_MAX;

static Eina_List *max_cpu_freq_list;
static Eina_List *min_cpu_freq_list;

struct cpu_freq_entry {
	int pid;
	int freq;
};

static void __set_freq_limit();
static int __is_entry_enble(int pid);
static int __remove_entry_from_max_cpu_freq_list(int pid);
static int __remove_entry_from_min_cpu_freq_list(int pid);
static int __add_entry_to_max_cpu_freq_list(int pid, int freq);
static int __add_entry_to_min_cpu_freq_list(int pid, int freq);
static int __write_max_cpu_freq(int freq);
static int __write_min_cpu_freq(int freq);

int set_max_frequency_action(int argc, char **argv)
{
	int r = -1;
	
	if (argc < 2)
		return -1;

	r = __add_entry_to_max_cpu_freq_list(atoi(argv[0]), atoi(argv[1]));
	if (r < 0) {
		PRT_TRACE_ERR("Add entry failed");
		return -1;
	}

	r = __write_max_cpu_freq(cur_max_cpu_freq);
	if (r < 0) {
		PRT_TRACE_ERR("Write entry failed");
		return -1;
	}

	return 0;
}

int set_min_frequency_action(int argc, char **argv)
{
	int r = -1;
	
	if (argc < 2)
		return -1;

	r = __add_entry_to_min_cpu_freq_list(atoi(argv[0]), atoi(argv[1]));
	if (r < 0) {
		PRT_TRACE_ERR("Add entry failed");
		return -1;
	}
	
	r = __write_min_cpu_freq(cur_min_cpu_freq);
	if (r < 0) {
		PRT_TRACE_ERR("Write entry failed");
		return -1;
	}

	return 0;
}

int release_max_frequency_action(int argc, char **argv)
{
	int r = -1;
	if (argc < 1)
		return -1;
	
	r = __remove_entry_from_max_cpu_freq_list(atoi(argv[0]));
	if (r < 0) {
		PRT_TRACE_ERR("Remove entry failed");
		return -1;
	}

	if (cur_max_cpu_freq == INT_MIN)
		cur_max_cpu_freq = max_cpu_freq_limit;

	r = __write_max_cpu_freq(cur_max_cpu_freq);
	if (r < 0) {
		PRT_TRACE_ERR("Write freq failed");
		return -1;
	}

	return 0;
}

int release_min_frequency_action(int argc, char **argv)
{
	int r = -1;

	if (argc < 1)
		return -1;

	r = __remove_entry_from_min_cpu_freq_list(atoi(argv[0]));
	if (r < 0) {
		PRT_TRACE_ERR("Remove entry failed");
		return -1;
	}

	if (cur_min_cpu_freq == INT_MAX)
		cur_min_cpu_freq = min_cpu_freq_limit;

	r = __write_min_cpu_freq(cur_min_cpu_freq);
	if (r < 0) {
		PRT_TRACE_ERR("Write entry failed");
		return -1;
	}

	return 0;
}

int ss_cpu_handler_init(void)
{
	__set_freq_limit();
	
	ss_action_entry_add_internal(PREDEF_SET_MAX_FREQUENCY, set_max_frequency_action, NULL, NULL);
	ss_action_entry_add_internal(PREDEF_SET_MIN_FREQUENCY, set_min_frequency_action, NULL, NULL);
	ss_action_entry_add_internal(PREDEF_RELEASE_MAX_FREQUENCY, release_max_frequency_action, NULL, NULL);
	ss_action_entry_add_internal(PREDEF_RELEASE_MIN_FREQUENCY, release_min_frequency_action, NULL, NULL);

	return 0;
}

static void __set_freq_limit()
{
	int ret;

	ret = plugin_intf->OEM_sys_get_cpufreq_cpuinfo_max_freq(&max_cpu_freq_limit);
	if (ret < 0) {
		PRT_TRACE_ERR("get cpufreq cpuinfo max readerror: %s", strerror(errno));
		max_cpu_freq_limit = DEFAULT_MAX_CPU_FREQ;
	}

	ret = plugin_intf->OEM_sys_get_cpufreq_cpuinfo_min_freq(&min_cpu_freq_limit);
	if (ret < 0) {
		PRT_TRACE_ERR("get cpufreq cpuinfo min readerror: %s", strerror(errno));
		min_cpu_freq_limit = DEFAULT_MIN_CPU_FREQ;
	}
}

static int __is_entry_enable(int pid)
{
	char pid_path[PATH_MAX];
	
	snprintf(pid_path, PATH_MAX, "/proc/%d", pid);
	if (access(pid_path, F_OK) < 0) {
		return 0;
	}

	return 1;
}

static int __remove_entry_from_max_cpu_freq_list(int pid)
{
	Eina_List *tmp;
	Eina_List *tmp_next;
	struct cpu_freq_entry *entry;

	cur_max_cpu_freq = INT_MIN;

	EINA_LIST_FOREACH_SAFE(max_cpu_freq_list, tmp, tmp_next, entry) {
		if (entry != NULL) {
			if ((!__is_entry_enable(entry->pid)) || (entry->pid == pid)) {
				max_cpu_freq_list = eina_list_remove(max_cpu_freq_list, entry);
				free(entry);
				continue;
			}

			if (entry->freq > cur_max_cpu_freq) {
				cur_max_cpu_freq = entry->freq;
			}
		}
	}

	return 0;
}

static int __remove_entry_from_min_cpu_freq_list(int pid)
{
	Eina_List *tmp;
	Eina_List *tmp_next;
	struct cpu_freq_entry *entry;

	cur_min_cpu_freq = INT_MAX;

	EINA_LIST_FOREACH_SAFE(min_cpu_freq_list, tmp, tmp_next, entry) {
		if (entry != NULL) {
			if ((!__is_entry_enable(entry->pid)) || (entry->pid == pid)) {
				min_cpu_freq_list = eina_list_remove(min_cpu_freq_list, entry);
				free(entry);
				continue;
			}

			if (entry->freq < cur_min_cpu_freq) {
				cur_min_cpu_freq = entry->freq;
			}

		}
	}

	return 0;
}

static int __add_entry_to_max_cpu_freq_list(int pid, int freq)
{
	int r = -1;
	struct cpu_freq_entry *entry;
	
	r = __remove_entry_from_max_cpu_freq_list(pid);
	if (r < 0) {
		PRT_TRACE_ERR("Remove duplicated entry failed");
	}

	if (freq > cur_max_cpu_freq) {
		cur_max_cpu_freq = freq;
	}

	entry = malloc(sizeof(struct cpu_freq_entry));
	if (!entry) {
		PRT_TRACE_ERR("Malloc failed");
		return -1;
	}
	
	entry->pid = pid;
	entry->freq = freq;

	max_cpu_freq_list = eina_list_prepend(max_cpu_freq_list, entry);
	if (!max_cpu_freq_list) {
		PRT_TRACE_ERR("eina_list_prepend failed");
		return -1;
	}

	return 0;
}

static int __add_entry_to_min_cpu_freq_list(int pid, int freq)
{
	int r = -1;
	struct cpu_freq_entry *entry;
	
	r = __remove_entry_from_min_cpu_freq_list(pid);
	if (r < 0) {
		PRT_TRACE_ERR("Remove duplicated entry failed");
	}

	if (freq < cur_min_cpu_freq) {
		cur_min_cpu_freq = freq;
	}

	entry = malloc(sizeof(struct cpu_freq_entry));
	if (!entry) {
		PRT_TRACE_ERR("Malloc failed");
		return -1;
	}
	
	entry->pid = pid;
	entry->freq = freq;

	min_cpu_freq_list = eina_list_prepend(min_cpu_freq_list, entry);
	if (!min_cpu_freq_list) {
		PRT_TRACE_ERR("eina_list_prepend failed");
		return -1;
	}
	
	return 0;
}

static int __write_max_cpu_freq(int freq)
{
	int ret;

	ret = plugin_intf->OEM_sys_set_cpufreq_scaling_max_freq(freq);
	if (ret < 0) {
		PRT_TRACE_ERR("set cpufreq max freq write error: %s", strerror(errno));
		return -1;
	}
	
	return 0;
}

static int __write_min_cpu_freq(int freq)
{
	int ret;

	ret = plugin_intf->OEM_sys_set_cpufreq_scaling_min_freq(freq);
	if (ret < 0) {
		PRT_TRACE_ERR("set cpufreq min freq write error: %s", strerror(errno));
		return -1;
	}
	
	return 0;
}
