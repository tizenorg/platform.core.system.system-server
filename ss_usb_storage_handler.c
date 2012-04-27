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
#include <glib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <vconf.h>
#include <mntent.h>
#include <limits.h>
#include "ss_device_handler.h"
#include "ss_log.h"

#define BUF_MAX			512     

#define FS_TYPE_CHECKER		"/sbin/fs-type-checker"
#define TEMP_FILE		"/tmp/mountd.tmp"
#define MTAB_FILE		"/etc/mtab"
#define MOUNT_POINT		"/opt/storage/usb"

static int added_noti_value = 0; 
static int removed_noti_value = 0; 

static int __ss_mount_device(char *dev)
{
	if (access(dev, F_OK) != 0) {
		PRT_TRACE_ERR("Failed to find device: DEVICE(%s)", dev);
		return -1;
	}

	int fd = -1;
	int r = -1;
	char *rel_mnt_point;
	char buf_mnt_point[BUF_MAX];

	rel_mnt_point = strrchr(dev, '/');
	if (rel_mnt_point == NULL) {
		PRT_TRACE_ERR("Get Relative Mount Path Failed");
		return -1;
	}

	snprintf(buf_mnt_point, BUF_MAX, "%s%s", MOUNT_POINT, rel_mnt_point);

	/* Make directory to mount */
	r = mkdir(buf_mnt_point, 0755);
	if (r < 0) {
		if (errno == EEXIST) {
			PRT_TRACE("Directory is already exsited: PATH(%s)", buf_mnt_point);
		} else {
			PRT_TRACE_ERR("Make Directory Failed: PATH(%s)", buf_mnt_point);
			return -1;
		}
	}

	/* Mount block device on mount point */
	r = mount(dev, buf_mnt_point, "vfat", 0, "uid=0,gid=0,dmask=0000,fmask=0111,iocharset=iso8859-1,utf8,shortname=mixed");
	if (r < 0) {
		r = rmdir(buf_mnt_point);
		PRT_TRACE_ERR("Mount failed: MOUNT PATH(%s", buf_mnt_point);
		return -1;
	}
	PRT_TRACE("Mount Complete: MOUNT PATH(%s)", buf_mnt_point);

	return 0;
}

static int __ss_unmount_device(char *mnt_point)
{
	if (access(mnt_point, F_OK) != 0) {
		PRT_TRACE_ERR("Failed to find path: MOUNT PATH(%s)", mnt_point);
		return -1;
	}

	int r = -1;

	/* Umount block device */
	r = umount2(mnt_point, MNT_DETACH);
	if (r < 0) {
		PRT_TRACE_ERR("Unmounting is unabled: MOUNT PATH(%s)", mnt_point);
		r = rmdir(mnt_point);
		if (r < 0) {
			PRT_TRACE_ERR("Removing Directory is unabled: PATH(%s)", mnt_point);
		}
		return -1;
	}

	/* Clean up unmounted directory */
	r = rmdir(mnt_point);
	if (r < 0) {
		PRT_TRACE_ERR("Removing Directory is unabled: PATH(%s)", mnt_point);
	}
	PRT_TRACE("Unmount/Remove Complete: MOUNT PATH(%s)", mnt_point);
	
	return 0;
}

static int __ss_usb_storage_added(int argc, char *argv[])
{
	if (argc != 1 || argv[0] == NULL) {
		PRT_TRACE_ERR("Get Vconf Value Failed: KEY(%s)", VCONFKEY_SYSMAN_ADDED_USB_STORAGE);
		return -1;
	}

	int fd = -1;
	int part_num = 0;

	char *buf_dev = argv[0];
	char buf_part_dev[BUF_MAX];
	char *disk_path;
	char *mounted_check;

	/* Check whether mount point directory is exist */
	if (access(MOUNT_POINT, F_OK) < 0) {
		if (mkdir(MOUNT_POINT, 0755) < 0) {
			PRT_TRACE_ERR("Make Mount Directory Failed: DIRECTORY(%s)", MOUNT_POINT);
			return -1;
		}
	}

	/* Mount tmpfs for protecting user data */
	if (mount("tmpfs", MOUNT_POINT, "tmpfs", 0, "") < 0) {
		if (errno != EBUSY) {
			PRT_TRACE_ERR("Failed to mount USB Storage Mount Directory: DIRECTORY(%s)", MOUNT_POINT);
			return -1;
		}
	}

	/* Change permission to avoid to write user data on tmpfs */
	if (chmod(MOUNT_POINT, 0755) < 0) {
		PRT_TRACE_ERR("Failed to change mode: DIRCTORY(%s)", MOUNT_POINT);
		return -1;
	}

	/* Mount a single partition storage device */
	if (__ss_mount_device(buf_dev) < 0) {
		/* Mount a multi partition storage device */
		for (part_num = 1; part_num < 10; part_num++) {
			snprintf(buf_part_dev, BUF_MAX, "%s%d", buf_dev, part_num);
			if (__ss_mount_device(buf_part_dev) < 0) {
				PRT_TRACE("Mounting partition is unabled: PARTITION(%s)", buf_part_dev);
				continue;	
			}
		}
	}

	FILE *file = setmntent(MTAB_FILE, "r");
	struct mntent *mnt_entry;

	/* Check whether block deivce is mounted */
	while (mnt_entry = getmntent(file)) {
		mounted_check =	strstr(mnt_entry->mnt_fsname, buf_dev);
		if (mounted_check != NULL) {
			if (added_noti_value < INT_MAX) {
				++added_noti_value;
			} else {
				added_noti_value = 1;
			}
			/* Broadcast mounting notification */
			if (vconf_set_int(VCONFKEY_SYSMAN_ADDED_USB_STORAGE, added_noti_value) < 0) {
				PRT_TRACE_ERR("Setting vconf value is failed: KEY(%s)", VCONFKEY_SYSMAN_ADDED_USB_STORAGE);
				vconf_set_int(VCONFKEY_SYSMAN_ADDED_USB_STORAGE, -1);
			}

			PRT_TRACE("Setting vconf value: KEY(%s) DEVICE(%s)", VCONFKEY_SYSMAN_ADDED_USB_STORAGE, buf_dev);
			fclose(file);
			return 0;
		}
	}

	/* Failed to mount storage device */
	PRT_TRACE_ERR("Nothing to be mounted: DEVICE(%s)", buf_dev);
	fclose(file);
	return -1;
}

static int __ss_usb_storage_removed(int argc, char *argv[])
{
	if (argc != 1 || argv[0] == NULL) {
		PRT_TRACE_ERR("Get Vonf Value Failed: KEY(%s)", VCONFKEY_SYSMAN_REMOVED_USB_STORAGE);
		return -1;
	}

	int fd = -1;
	int part_num = 0;

	char *buf_dev_name = argv[0];
	char buf_mnt_point[BUF_MAX];
	char *mounted_check;

	snprintf(buf_mnt_point, BUF_MAX, "%s/%s", MOUNT_POINT, buf_dev_name);

	if (__ss_unmount_device(buf_mnt_point) == 0) {
		umount2(MOUNT_POINT, MNT_DETACH);

		if (removed_noti_value < INT_MAX) {
			++removed_noti_value;
		} else {
			removed_noti_value = 1;
		}
		if (vconf_set_int(VCONFKEY_SYSMAN_REMOVED_USB_STORAGE, removed_noti_value) < 0) {
			PRT_TRACE_ERR("Setting vconf value is failed: KEY(%s)", VCONFKEY_SYSMAN_REMOVED_USB_STORAGE);
			vconf_set_int(VCONFKEY_SYSMAN_ADDED_USB_STORAGE, -1);
		}

		PRT_TRACE("Setting vconf value: KEY(%s) DEVICE(%s)", VCONFKEY_SYSMAN_REMOVED_USB_STORAGE, buf_dev_name);
		return 0;
	}

	FILE *file = setmntent(MTAB_FILE, "r");
	struct mntent *mnt_entry;

	while (mnt_entry = getmntent(file)) {
		mounted_check =	strstr(mnt_entry->mnt_dir, buf_mnt_point);
		if (mounted_check != NULL) {
			if (__ss_unmount_device(mnt_entry->mnt_dir) < 0) {
				PRT_TRACE_ERR("Unmount Failed: MOUNT PATH(%s)", mnt_entry->mnt_dir);
			}
			PRT_TRACE("Unmount Success: MOUNT PATH(%s)", mnt_entry->mnt_dir);
		}
	}
	fclose(file);

	umount2(MOUNT_POINT, MNT_DETACH);

	if (removed_noti_value < INT_MAX) {
		++removed_noti_value;
	} else {
		removed_noti_value = 1;
	}
	if (vconf_set_int(VCONFKEY_SYSMAN_REMOVED_USB_STORAGE, removed_noti_value) < 0) {
		PRT_TRACE_ERR("Setting vconf value is failed: KEY(%s)", VCONFKEY_SYSMAN_REMOVED_USB_STORAGE);
		vconf_set_int(VCONFKEY_SYSMAN_ADDED_USB_STORAGE, -1);
	}

	PRT_TRACE("Setting vconf value: KEY(%s) DEVICE(%s)", VCONFKEY_SYSMAN_REMOVED_USB_STORAGE, buf_dev_name);
	return 0;
}

int _ss_usb_storage_init()
{
	ss_action_entry_add_internal(PREDEF_USB_STORAGE_ADD, __ss_usb_storage_added, NULL, NULL);
	ss_action_entry_add_internal(PREDEF_USB_STORAGE_REMOVE, __ss_usb_storage_removed, NULL, NULL);

	return 0;
}

