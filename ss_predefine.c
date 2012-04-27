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


#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include <dirent.h>
#include <sysman.h>
#include <vconf.h>
#include <pmapi.h>
#include <ITapiPower.h>
#include <TelPower.h>
#include <TapiEvent.h>
#include <TapiCommon.h>
#include <syspopup_caller.h>
#include <sys/reboot.h>
#include <sys/time.h>

#include "ss_log.h"
#include "ss_launch.h"
#include "ss_queue.h"
#include "ss_device_handler.h"
#include "ss_device_plugin.h"
#include "ss_predefine.h"
#include "ss_procmgr.h"
#include "include/ss_data.h"

#define PREDEFINE_SO_DIR	PREFIX"/lib/ss_predefine/"

#define CALL_EXEC_PATH		PREFIX"/bin/call"
#define LOWMEM_EXEC_PATH	PREFIX"/bin/lowmem-popup"
#define LOWBAT_EXEC_PATH	PREFIX"/bin/lowbatt-popup"
#define USBCON_EXEC_PATH	PREFIX"/bin/usb_setting"
#define TVOUT_EXEC_PATH		PREFIX"/bin/tvout-selector"
#define PWROFF_EXEC_PATH	PREFIX"/bin/poweroff-popup"
#define MEMPS_EXEC_PATH	PREFIX"/bin/memps"

/* wait for 5 sec as victim process be dead */
#define WAITING_INTERVAL	5

#define TVOUT_X_BIN		"/usr/bin/xberc"
#define TVOUT_FLAG		0x00000001
#define MEMPS_LOG_FILE		"/var/log/memps"
#define MAX_RETRY		2

#define POWEROFF_DURATION		2
#define POWEROFF_ANIMATION_PATH		"/usr/bin/boot-animation"
#define POWEROFF_NOTI_NAME                     "power_off_start"

#define VCONFKEY_TESTMODE_LOW_BATT_POPUP       "db/testmode/low_batt_popup"
#define WM_READY_PATH                          "/tmp/.wm_ready"
 
#define LOWBAT_OPT_WARNING		1
#define LOWBAT_OPT_POWEROFF		2
#define LOWBAT_OPT_CHARGEERR	3
#define LOWBAT_OPT_CHECK		4

static Ecore_Timer *lowbat_popup_id = NULL;
static int lowbat_popup_option = 0;

static struct timeval tv_start_poweroff;
static void powerdown_ap(TelTapiEvent_t *event, void *data);

static int ss_flags = 0;

static Ecore_Timer *poweroff_timer_id = NULL;
static unsigned int power_subscription_id = 0;

#ifdef NOUSE
int call_predefine_action(int argc, char **argv)
{
	char argstr[128];
	int pid;

	if (argc < 2)
		return -1;

	snprintf(argstr, sizeof(argstr), "-t MT -n %s -i %s", argv[0], argv[1]);
	pid = ss_launch_if_noexist(CALL_EXEC_PATH, argstr);
	if (pid < 0) {
		PRT_TRACE_ERR("call predefine action failed");
		return -1;
	}
	return pid;
}
#endif

static void make_memps_log(char *file, pid_t pid, char *victim_name)
{
	time_t now;
	struct tm *cur_tm;
	char params[4096];
	char new_log[NAME_MAX];
	static pid_t old_pid = 0;
	int ret=-1;

	if (old_pid == pid)
		return;
	old_pid = pid;

	now = time(NULL);
	cur_tm = (struct tm *)malloc(sizeof(struct tm));
	if (cur_tm == NULL) {
		PRT_TRACE_ERR("Fail to memory allocation");
		return;
	}

	if (localtime_r(&now, cur_tm) == NULL) {
		PRT_TRACE_ERR("Fail to get localtime");
		return;
	}

	PRT_TRACE("%s_%s_%d_%.4d%.2d%.2d_%.2d%.2d%.2d.log", file, victim_name,
		 pid, (1900 + cur_tm->tm_year), 1 + cur_tm->tm_mon,
		 cur_tm->tm_mday, cur_tm->tm_hour, cur_tm->tm_min,
		 cur_tm->tm_sec);
	snprintf(new_log, sizeof(new_log),
		 "%s_%s_%d_%.4d%.2d%.2d_%.2d%.2d%.2d.log", file, victim_name,
		 pid, (1900 + cur_tm->tm_year), 1 + cur_tm->tm_mon,
		 cur_tm->tm_mday, cur_tm->tm_hour, cur_tm->tm_min,
		 cur_tm->tm_sec);

	snprintf(params, sizeof(params), "-f %s", new_log);
	ret = ss_launch_evenif_exist(MEMPS_EXEC_PATH, params);
	/* will be removed, just for debugging */
	if(ret > 0) {
		char buf[PATH_MAX];
		FILE *fp;
		snprintf(buf, sizeof(buf), "/proc/%d/oom_adj", ret);
		fp = fopen(buf, "w");
		if (fp != NULL) {
			fprintf(fp, "%d", (-17));
			fclose(fp);
		}
	}
	free(cur_tm);
}

static int lowmem_get_victim_pid()
{
	pid_t pid;
	int fd;

	if (0 > plugin_intf->OEM_sys_get_memnotify_victim_task(&pid)) {
		PRT_TRACE_ERR("Get victim task failed");
		return -1;
	}

	return pid;
}

int lowmem_def_predefine_action(int argc, char **argv)
{
	int pid, ret, oom_adj;
	char appname[PATH_MAX];

	if (argc < 1)
		return -1;

	if (!strcmp(argv[0], OOM_MEM_ACT)) {
		pid = lowmem_get_victim_pid();
		if (pid > 0 && pid != sysman_get_pid(LOWMEM_EXEC_PATH) && pid != sysman_get_pid(MEMPS_EXEC_PATH)) {
			if ((sysman_get_cmdline_name(pid, appname, PATH_MAX)) ==
			    0) {
				PRT_TRACE_EM
				    ("we will kill, lowmem lv2 = %d (%s)\n",
				     pid, appname);
	
				make_memps_log(MEMPS_LOG_FILE, pid, appname);

				if(get_app_oomadj(pid, &oom_adj) < 0) {
					PRT_TRACE_ERR("Failed to get oom_adj");
				}
				PRT_TRACE("%d will be killed with %d oom_adj value", pid, oom_adj);

				kill(pid, SIGTERM);
#ifdef NOUSE
				sleep(WAITING_INTERVAL);
				snprintf(buf, sizeof(buf), "/proc/%d", pid);
				if (access(buf, R_OK) == 0) {
					/* still alive the victim. kill it by force */
					kill(pid, SIGKILL);
				}
#endif

				if (oom_adj >= OOMADJ_BACKGRD_UNLOCKED) {	
					return 0;
				}

				bundle *b = NULL;

				b = bundle_create();
				bundle_add(b, "_APP_NAME_", appname);
				ret = syspopup_launch("lowmem-syspopup", b);
				bundle_free(b);
				if (ret < 0) {
					PRT_TRACE_EM("popup lauch failed\n");
					return -1;
				}
				
				if (set_app_oomadj(ret, OOMADJ_SU) < 0) {	
					PRT_TRACE_ERR("Failed to set oom_adj");
				}
			}
		}
	} else {
		PRT_TRACE_EM("making memps log for low memory\n");
		make_memps_log(MEMPS_LOG_FILE, 1, "LOWMEM_WARNING");
	}

	return 0;
}

int usbcon_def_predefine_action(int argc, char **argv)
{
	int pid, val = -1;

	if (plugin_intf->OEM_sys_get_jack_usb_online(&val) == 0) {
		if (val == 0) {
			vconf_set_int(VCONFKEY_SYSMAN_USB_STATUS,
				      VCONFKEY_SYSMAN_USB_DISCONNECTED);
			pm_unlock_state(LCD_OFF, STAY_CUR_STATE);
			return 0;
		}

		vconf_set_int(VCONFKEY_SYSMAN_USB_STATUS,
			      VCONFKEY_SYSMAN_USB_AVAILABLE);
		pm_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
		pid = ss_launch_if_noexist(USBCON_EXEC_PATH, NULL);
		if (pid < 0) {
			PRT_TRACE_ERR("usb predefine action failed\n");
			return -1;
		}
		return pid;
	}
	PRT_TRACE_ERR("failed to get usb status\n");
	return -1;
}

int earjackcon_def_predefine_action(int argc, char **argv)
{
	int val;

	/* TODO 
	 * if we can recognize which type of jack is changed,
	 * should move following code for TVOUT to a new action function */
	PRT_TRACE_EM("earjack_normal predefine action\n");
	/*
	   if(device_get_property(DEVTYPE_JACK,JACK_PROP_TVOUT_ONLINE,&val) == 0)
	   {
	   if (val == 1 &&
	   vconf_get_int(VCONFKEY_SETAPPL_TVOUT_TVSYSTEM_INT, &val) == 0) {
	   PRT_TRACE_EM("Start TV out %s\n", (val==0)?"NTSC":(val==1)?"PAL":"Unsupported");
	   switch (val) {
	   case 0:              // NTSC
	   ss_launch_after_kill_if_exist(TVOUT_X_BIN, "-connect 2");
	   break;
	   case 1:              // PAL
	   ss_launch_after_kill_if_exist(TVOUT_X_BIN, "-connect 3");
	   break;
	   default:
	   PRT_TRACE_ERR("Unsupported TV system type:%d \n", val);
	   return -1;
	   }
	   // UI clone on
	   ss_launch_evenif_exist(TVOUT_X_BIN, "-clone 1");
	   ss_flags |= TVOUT_FLAG;
	   return vconf_set_int(VCONFKEY_SYSMAN_EARJACK, VCONFKEY_SYSMAN_EARJACK_TVOUT);
	   }
	   else {
	   if(val == 1) {
	   PRT_TRACE_EM("TV type is not set. Please set the TV type first.\n");
	   return -1;
	   }
	   if (ss_flags & TVOUT_FLAG) {
	   PRT_TRACE_EM("TV out Jack is disconnected.\n");
	   // UI clone off
	   ss_launch_after_kill_if_exist(TVOUT_X_BIN, "-clone 0");
	   ss_flags &= ~TVOUT_FLAG;
	   return vconf_set_int(VCONFKEY_SYSMAN_EARJACK, VCONFKEY_SYSMAN_EARJACK_REMOVED);
	   }
	   // Current event is not TV out event. Fall through 
	   }
	   }
	 */
	if (plugin_intf->OEM_sys_get_jack_earjack_online(&val) == 0) {
		return vconf_set_int(VCONFKEY_SYSMAN_EARJACK, val);
	}

	return -1;
}

int lowbat_popup(void *data)
{
	int ret = -1, state = 0;
	ret = vconf_get_int("memory/boot-animation/finished", &state);
	if (state == 1 || ret != 0) {
		bundle *b = NULL;
		b = bundle_create();
		if(lowbat_popup_option == LOWBAT_OPT_WARNING) {
			bundle_add(b, "_SYSPOPUP_CONTENT_", "warning");
		} else if(lowbat_popup_option == LOWBAT_OPT_POWEROFF) {
			bundle_add(b, "_SYSPOPUP_CONTENT_", "poweroff");
		} else if(lowbat_popup_option == LOWBAT_OPT_CHARGEERR) {
			bundle_add(b, "_SYSPOPUP_CONTENT_", "chargeerr");
		} else {
			bundle_add(b, "_SYSPOPUP_CONTENT_", "check");
		}

		ret = syspopup_launch("lowbat-syspopup", b);
		if (ret < 0) {
			PRT_TRACE_EM("popup lauch failed\n");
			bundle_free(b);
			return 1;
		}
			lowbat_popup_id = NULL;
		lowbat_popup_option = 0;
		bundle_free(b);
	} else {
		PRT_TRACE_EM("boot-animation running yet");
		return 1;
	}

	return 0;
}

int lowbat_def_predefine_action(int argc, char **argv)
{
	int ret, state=0;
	char argstr[128];
	char* option = NULL;

	if (argc < 1)
		return -1;

	if(lowbat_popup_id != NULL) {
		ecore_timer_del(lowbat_popup_id);
		lowbat_popup_id = NULL;
	}

	bundle *b = NULL;
	b = bundle_create();
	if(!strcmp(argv[0],WARNING_LOW_BAT_ACT) || !strcmp(argv[0],CRITICAL_LOW_BAT_ACT)) {
		bundle_add(b, "_SYSPOPUP_CONTENT_", "warning");
		lowbat_popup_option = LOWBAT_OPT_WARNING;
	} else if(!strcmp(argv[0],POWER_OFF_BAT_ACT)) {
		bundle_add(b, "_SYSPOPUP_CONTENT_", "poweroff");
		lowbat_popup_option = LOWBAT_OPT_POWEROFF;
	} else if(!strcmp(argv[0],CHARGE_ERROR_ACT)) {
		bundle_add(b, "_SYSPOPUP_CONTENT_", "chargeerr");
		lowbat_popup_option = LOWBAT_OPT_CHARGEERR;
	} else {
		bundle_add(b, "_SYSPOPUP_CONTENT_", "check");
		lowbat_popup_option = LOWBAT_OPT_CHECK;
	}

	ret = vconf_get_int("memory/boot-animation/finished", &state);
	if (state == 1 || ret != 0) {
		ret = syspopup_launch("lowbat-syspopup", b);
		if (ret < 0) {
			PRT_TRACE_EM("popup lauch failed\n");
			bundle_free(b);
			lowbat_popup_option = 0;
			return -1;
		}
	} else {
		PRT_TRACE_EM("boot-animation running yet");
		lowbat_popup_id = ecore_timer_add(1, lowbat_popup, NULL);
	}
	bundle_free(b);
	return 0;
}

Eina_Bool powerdown_ap_by_force(void *data)
{
	struct timeval now;
	int poweroff_duration = POWEROFF_DURATION;
	char *buf;

	/* Getting poweroff duration */
	buf = getenv("PWROFF_DUR");
	if (buf != NULL && strlen(buf) < 1024)
		poweroff_duration = atoi(buf);
	if (poweroff_duration < 0 || poweroff_duration > 60)
		poweroff_duration = POWEROFF_DURATION;

	gettimeofday(&now, NULL);
	/* Waiting until power off duration and displaying animation */
	while (now.tv_sec - tv_start_poweroff.tv_sec < poweroff_duration) {
		usleep(100000);
		gettimeofday(&now, NULL);
	}

	PRT_TRACE("Power off by force\n");
	kill(-1, SIGTERM);
	/* give a chance to be terminated for each process */
	sleep(1);
	sync();
	reboot(RB_POWER_OFF);
	return EINA_TRUE;
}

static void powerdown_ap(TelTapiEvent_t *event, void *data)
{
	struct timeval now;
	int poweroff_duration = POWEROFF_DURATION;
	char *buf;

	if (poweroff_timer_id) {
		ecore_timer_del(poweroff_timer_id);
		poweroff_timer_id = NULL;
	}
	if (power_subscription_id) {
		tel_deregister_event(power_subscription_id);
		power_subscription_id = 0;
	}
	PRT_TRACE("Power off \n");

	/* Getting poweroff duration */
	buf = getenv("PWROFF_DUR");
	if (buf != NULL && strlen(buf) < 1024)
		poweroff_duration = atoi(buf);
	if (poweroff_duration < 0 || poweroff_duration > 60)
		poweroff_duration = POWEROFF_DURATION;

	gettimeofday(&now, NULL);
	/* Waiting until power off duration and displaying animation */
	while (now.tv_sec - tv_start_poweroff.tv_sec < poweroff_duration) {
		usleep(100000);
		gettimeofday(&now, NULL);
	}

	kill(-1, SIGTERM);
	/* give a chance to be terminated for each process */
	sleep(1);
	sync();
	reboot(RB_POWER_OFF);
}

int poweroff_def_predefine_action(int argc, char **argv)
{
	int ret;

	heynoti_publish(POWEROFF_NOTI_NAME);

	pm_change_state(LCD_NORMAL);
	system("/etc/rc.d/rc.shutdown &");
	sync();

	gettimeofday(&tv_start_poweroff, NULL);
	ret =
	    tel_register_event(TAPI_EVENT_POWER_PHONE_OFF,
			       &power_subscription_id,
			       (TelAppCallback) & powerdown_ap, NULL);
	if (ret != TAPI_API_SUCCESS) {
		PRT_TRACE_ERR
		    ("tel_register_event is not subscribed. error %d\n", ret);
		powerdown_ap_by_force(NULL);
		return 0;
	}

	ret = tel_process_power_command(TAPI_PHONE_POWER_OFF);
	if (ret != TAPI_API_SUCCESS) {
		PRT_TRACE_ERR("tel_process_power_command() error %d\n", ret);
		powerdown_ap_by_force(NULL);
		return 0;
	}

	poweroff_timer_id = ecore_timer_add(15, powerdown_ap_by_force, NULL);
	return 0;
}

int entersleep_def_predefine_action(int argc, char **argv)
{
	int ret;

	pm_change_state(LCD_NORMAL);
	sync();

	/* flight mode
	 * TODO - add check, cb, etc... 
	 * should be checked wirh telephony part */
	ret = tel_set_flight_mode(TAPI_POWER_FLIGHT_MODE_ENTER);
	PRT_TRACE_ERR("request for changing into flight mode : %d\n", ret);

	system("/etc/rc.d/rc.entersleep");
	pm_change_state(POWER_OFF);

	return 0;
}

int leavesleep_def_predefine_action(int argc, char **argv)
{
	int ret;

	pm_change_state(LCD_NORMAL);
	sync();

	/* flight mode
	 * TODO - add check, cb, etc... 
	 * should be checked wirh telephony part */
	ret = tel_set_flight_mode(TAPI_POWER_FLIGHT_MODE_LEAVE);
	PRT_TRACE_ERR("request for changing into flight mode : %d\n", ret);

	return 0;
}

static void restart_ap(TelTapiEvent_t *event, void *data);

Eina_Bool restart_ap_ecore(void *data)
{
	restart_ap(NULL, (void *)-1);
	return EINA_TRUE;
}

static void restart_ap(TelTapiEvent_t *event, void *data)
{
	struct timeval now;
	int poweroff_duration = POWEROFF_DURATION;
	char *buf;

	if (poweroff_timer_id) {
		ecore_timer_del(poweroff_timer_id);
		poweroff_timer_id = NULL;
	}
	if (power_subscription_id) {
		tel_deregister_event(power_subscription_id);
		power_subscription_id = 0;
	}

	PRT_INFO("Restart\n");
	sync();

	buf = getenv("PWROFF_DUR");
	if (buf != NULL && strlen(buf) < 1024)
		poweroff_duration = atoi(buf);
	if (poweroff_duration < 0 || poweroff_duration > 60)
		poweroff_duration = POWEROFF_DURATION;
	gettimeofday(&now, NULL);
	while (now.tv_sec - tv_start_poweroff.tv_sec < poweroff_duration) {
		usleep(100000);
		gettimeofday(&now, NULL);
	}

	reboot(RB_AUTOBOOT);
}

int restart_def_predefine_action(int argc, char **argv)
{
	int ret;

	pm_change_state(LCD_NORMAL);
	system("/etc/rc.d/rc.shutdown &");
	sync();

	gettimeofday(&tv_start_poweroff, NULL);

	ret =
	    tel_register_event(TAPI_EVENT_POWER_PHONE_OFF,
			       &power_subscription_id,
			       (TelAppCallback) & restart_ap, NULL);
	if (ret != TAPI_API_SUCCESS) {
		PRT_TRACE_ERR
		    ("tel_register_event is not subscribed. error %d\n", ret);
		restart_ap(NULL, (void *)-1);
		return 0;
	}

	ret = tel_process_power_command(TAPI_PHONE_POWER_OFF);
	if (ret != TAPI_API_SUCCESS) {
		PRT_TRACE_ERR("tel_process_power_command() error %d\n", ret);
		restart_ap(NULL, (void *)-1);
		return 0;
	}

	poweroff_timer_id = ecore_timer_add(15, restart_ap_ecore, NULL);
	return 0;
}

int launching_predefine_action(int argc, char **argv)
{
	int ret;

	if (argc < 0)
		return -1;

	/* current just launching poweroff-popup */
	ret = syspopup_launch("poweroff-syspopup", NULL);
	if (ret < 0) {
		PRT_TRACE_ERR("poweroff popup predefine action failed");
		return -1;
	}
	return 0;
}

static void ss_action_entry_load_from_sodir()
{
	DIR *dp;
	struct dirent *dentry;
	struct sysnoti *msg;
	char *ext;
	char tmp[128];

	dp = opendir(PREDEFINE_SO_DIR);
	if (!dp) {
		ERR("fail open %s", PREDEFINE_SO_DIR);
		return;
	}

	msg = malloc(sizeof(struct sysnoti));
	if (msg == NULL) {
		ERR("Malloc failed");
		closedir(dp);
		return;
	}

	msg->pid = getpid();

	while ((dentry = readdir(dp)) != NULL) {
		if ((ext = strstr(dentry->d_name, ".so")) == NULL)
			continue;

		snprintf(tmp, sizeof(tmp), "%s/%s", PREDEFINE_SO_DIR,
			 dentry->d_name);
		msg->path = tmp;
		*ext = 0;
		msg->type = &(dentry->d_name[3]);
		ss_action_entry_add(msg);
	}
	free(msg);

	closedir(dp);
}

void ss_predefine_internal_init(void)
{
#ifdef NOUSE
	ss_action_entry_add_internal(PREDEF_CALL, call_predefine_action, NULL,
				     NULL);
#endif
	ss_action_entry_add_internal(PREDEF_LOWMEM, lowmem_def_predefine_action,
				     NULL, NULL);
	ss_action_entry_add_internal(PREDEF_LOWBAT, lowbat_def_predefine_action,
				     NULL, NULL);
	ss_action_entry_add_internal(PREDEF_USBCON, usbcon_def_predefine_action,
				     NULL, NULL);
	ss_action_entry_add_internal(PREDEF_EARJACKCON,
				     earjackcon_def_predefine_action, NULL,
				     NULL);
	ss_action_entry_add_internal(PREDEF_POWEROFF,
				     poweroff_def_predefine_action, NULL, NULL);
	ss_action_entry_add_internal(PREDEF_PWROFF_POPUP,
				     launching_predefine_action, NULL, NULL);
	ss_action_entry_add_internal(PREDEF_ENTERSLEEP,
				     entersleep_def_predefine_action, NULL,
				     NULL);
	ss_action_entry_add_internal(PREDEF_LEAVESLEEP,
				     leavesleep_def_predefine_action, NULL,
				     NULL);
	ss_action_entry_add_internal(PREDEF_REBOOT,
				     restart_def_predefine_action, NULL, NULL);

	ss_action_entry_load_from_sodir();

	/* check and set earjack init status */
	earjackcon_def_predefine_action(0, NULL);
}
