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
#include <sys/mount.h>
#include <errno.h>
#include <vconf.h>
#include <devman.h>
#include <fcntl.h>
#include <dirent.h>
#include <sysman.h>
#include "ss_log.h"
#include "ss_device_handler.h"

#define MOVINAND_DEV		"/dev/mmcblk0p1"
#define FORMAT_MMC		PREFIX"/sbin/mkfs.vfat "
#define FORMAT_MOVINAND		PREFIX"/bin/movi_format.sh"

int mmc_status;

int get_mmcblk_num()
{
	DIR *dp;
	struct dirent *dir;
	struct stat stat;
	char buf[255];
	int fd;
	int r;
	int mmcblk_num;

	if ((dp = opendir("/sys/block")) == NULL) {
		PRT_TRACE_ERR("Can not open directory..\n");
		return -1;
	}
	chdir("/sys/block");

	while (dir = readdir(dp)) {
		memset(&stat, 0, sizeof(struct stat));
		lstat(dir->d_name, &stat);
		if (S_ISDIR(stat.st_mode) || S_ISLNK(stat.st_mode)) {
			if (strncmp(".", dir->d_name, 1) == 0
			    || strncmp("..", dir->d_name, 2) == 0)
				continue;
			if (strncmp("mmcblk", dir->d_name, 6) == 0) {
				snprintf(buf, 255, "/sys/block/%s/device/type",
					 dir->d_name);

				fd = open(buf, O_RDONLY);
				if (fd == -1)
					PRT_TRACE_ERR("%s open error: %s", buf,
						      strerror(errno));
				r = read(fd, buf, 10);
				if ((r >= 0) && (r < 10))
					buf[r] = '\0';
				else
					PRT_TRACE_ERR("%s read error: %s", buf,
						      strerror(errno));
				close(fd);
				if (strncmp("SD", buf, 2) == 0) {
					char *str_mmcblk_num = strndup((dir->d_name) + 6, 1);
					if (str_mmcblk_num == NULL) {
						PRT_TRACE_ERR("Memory Allocation Failed");
						closedir(dp);
						return -1; 
					}
					mmcblk_num =
					    atoi(str_mmcblk_num);

					free(str_mmcblk_num);
					closedir(dp);
					PRT_TRACE("%d \n", mmcblk_num);
					return mmcblk_num;
				}
			}

		}
	}
	closedir(dp);
	PRT_TRACE_ERR("Failed to find mmc block number\n");
	return -1;
}

static int ss_mmc_format_tmp(keynode_t *key_nodes, void *data)
{
	PRT_TRACE_ERR("mmc_format called");
	if (vconf_keynode_get_int(key_nodes) == 1) {
		PRT_TRACE_ERR("format start");
		device_set_property(DEVTYPE_MMC, MMC_PROP_FORMAT, 0);

	}

	return 0;
}

static int ss_mmc_format(keynode_t *key_nodes, void *data)
{
	PRT_TRACE_ERR("mmc format called");
	device_set_property(DEVTYPE_MMC, MMC_PROP_FORMAT, 0);

	return 0;
}

int ss_mmc_unmounted(int argc, char **argv)
{
	int option = -1;

	if (argc < 1) {
		PRT_TRACE_ERR("Option is wong");
		return -1;
	}
	if ((option = atoi(argv[0])) < 0) {
		PRT_TRACE_ERR("Option is wong : %d", option);
		return -1;
	}

	if (umount2(MMC_MOUNT_POINT, option) != 0) {
		PRT_TRACE_ERR("Failed to unmount mmc card\n");
		vconf_set_int(VCONFKEY_SYSMAN_MMC_UNMOUNT,
			      VCONFKEY_SYSMAN_MMC_UNMOUNT_FAILED);
		return -1;

	}
	vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS,
		      VCONFKEY_SYSMAN_MMC_INSERTED_NOT_MOUNTED);
	vconf_set_int(VCONFKEY_SYSMAN_MMC_UNMOUNT,
		      VCONFKEY_SYSMAN_MMC_UNMOUNT_COMPLETED);
	mmc_status = VCONFKEY_SYSMAN_MMC_INSERTED_NOT_MOUNTED;

	return 0;
}

int ss_mmc_init()
{
#ifdef NOUSE
	/* movinand mount */
	if (access(MOVINAND_MOUNT_POINT, R_OK) != 0)
		mkdir(MOVINAND_MOUNT_POINT, 0755);

	if (mount
	    (MOVINAND_DEV, MOVINAND_MOUNT_POINT, "vfat", 0,
	     "uid=0,gid=0,dmask=0000,fmask=0000,iocharset=iso8859-1,utf8,shortname=mixed")
	    < 0) {
		PRT_TRACE_ERR
		    ("Failed to mount. Format movinand as rfs file system. "
		     MOVINAND_DEV "\n");
		system(FORMAT_MOVINAND);
		mount(MOVINAND_DEV, MOVINAND_MOUNT_POINT, "rfs", 0,
		      "uid=0,gid=0,dmask=0000,fmask=0000,iocharset=iso8859-1,utf8,shortname=mixed");
	}
#endif
	/* mmc card mount */
	ss_mmc_inserted();

	vconf_notify_key_changed("memory/mmc/format", (void *)ss_mmc_format_tmp,
				 NULL);

	ss_action_entry_add_internal(PREDEF_MOUNT_MMC, ss_mmc_inserted, NULL,
				     NULL);
	ss_action_entry_add_internal(PREDEF_UNMOUNT_MMC, ss_mmc_unmounted, NULL,
				     NULL);
	ss_action_entry_add_internal(PREDEF_FORMAT_MMC, ss_mmc_format, NULL,
				     NULL);
	return 0;
}

#ifdef NOUSE
static int set_mount_opt(char *buf)
{
	snprintf(buf, NAME_MAX, "external,iocharset=cp949");
	return 0;
}
#endif

int ss_mmc_inserted()
{
	char buf[NAME_MAX];
	int blk_num, ret, retry = 0;
	char opt[NAME_MAX];
	char *popt = opt;

	if (mmc_status == VCONFKEY_SYSMAN_MMC_MOUNTED) {
		PRT_DBG("Mmc is already mounted.\n");
		vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS,
			      VCONFKEY_SYSMAN_MMC_MOUNTED);
		vconf_set_int(VCONFKEY_SYSMAN_MMC_MOUNT,
			      VCONFKEY_SYSMAN_MMC_MOUNT_ALREADY);
		return -1;
	}

	if (access(MMC_MOUNT_POINT, R_OK) != 0)
		mkdir(MMC_MOUNT_POINT, 0755);

	if ((blk_num = get_mmcblk_num()) == -1) {
		vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS,
			      VCONFKEY_SYSMAN_MMC_REMOVED);
		vconf_set_int(VCONFKEY_SYSMAN_MMC_MOUNT,
			      VCONFKEY_SYSMAN_MMC_MOUNT_FAILED);
		mmc_status = VCONFKEY_SYSMAN_MMC_REMOVED;
		return 0;
	}
	popt = NULL;

	snprintf(buf, sizeof(buf), "%s%d", MMC_DEV, blk_num);
	if (mount
	    (buf, MMC_MOUNT_POINT, "vfat", 0,
	     "uid=0,gid=0,dmask=0000,fmask=0111,iocharset=iso8859-1,utf8,shortname=mixed")
	    == 0) {
		PRT_DBG("Mounted mmc card\n");
		vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS,
			      VCONFKEY_SYSMAN_MMC_MOUNTED);
		vconf_set_int(VCONFKEY_SYSMAN_MMC_MOUNT,
			      VCONFKEY_SYSMAN_MMC_MOUNT_COMPLETED);
		mmc_status = VCONFKEY_SYSMAN_MMC_MOUNTED;
		return 0;
	}
	do {
		snprintf(buf, sizeof(buf), "%s%dp1", MMC_DEV, blk_num);
		if ((ret =
		     mount(buf, MMC_MOUNT_POINT, "vfat", 0,
			   "uid=0,gid=0,dmask=0000,fmask=0111,iocharset=iso8859-1,utf8,shortname=mixed"))
		    == 0) {
			PRT_DBG("Mounted mmc card partition 1(%s)\n", buf);
			vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS,
				      VCONFKEY_SYSMAN_MMC_MOUNTED);
			vconf_set_int(VCONFKEY_SYSMAN_MMC_MOUNT,
				      VCONFKEY_SYSMAN_MMC_MOUNT_COMPLETED);
			mmc_status = VCONFKEY_SYSMAN_MMC_MOUNTED;
			return 0;
		}
		usleep(100000);
	} while (ret == -1 && errno == ENOENT && retry++ < 10);

	vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS,
		      VCONFKEY_SYSMAN_MMC_INSERTED_NOT_MOUNTED);
	vconf_set_int(VCONFKEY_SYSMAN_MMC_MOUNT,
		      VCONFKEY_SYSMAN_MMC_MOUNT_FAILED);
	mmc_status = VCONFKEY_SYSMAN_MMC_INSERTED_NOT_MOUNTED;
	PRT_TRACE_ERR("Failed to mount mmc card\n");
	return -1;
}

int ss_mmc_removed()
{
	vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS, VCONFKEY_SYSMAN_MMC_REMOVED);

	if (umount2(MMC_MOUNT_POINT, MNT_DETACH) != 0) {
		PRT_TRACE_ERR("Failed to unmount mmc card\n");
	}
	mmc_status = VCONFKEY_SYSMAN_MMC_REMOVED;

	return 0;
}
