/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
#define FS_VFAT_MOUNT_OPT  "uid=0,gid=0,dmask=0000,fmask=0111,iocharset=iso8859-1,utf8,shortname=mixed,smackfsroot=*,smackfsdef=*"

int mmc_status;
int ss_mmc_inserted(void);
static int __umount_fs(void);

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

	while ((dir = readdir(dp)) != NULL) {
		memset(&stat, 0, sizeof(struct stat));
		if(lstat(dir->d_name, &stat) < 0) {continue;}
		if (S_ISDIR(stat.st_mode) || S_ISLNK(stat.st_mode)) {
			if (strncmp(".", dir->d_name, 1) == 0
			    || strncmp("..", dir->d_name, 2) == 0)
				continue;
			if (strncmp("mmcblk", dir->d_name, 6) == 0) {
				snprintf(buf, 255, "/sys/block/%s/device/type",
					 dir->d_name);

				fd = open(buf, O_RDONLY);
				if (fd == -1) {
					PRT_TRACE_ERR("%s open error: %s", buf,
						      strerror(errno));
					continue;
				}
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

static int ss_mmc_format(keynode_t *key_nodes, void *data)
{
	PRT_TRACE_ERR("mmc format called");
	__umount_fs();
	vconf_set_int(VCONFKEY_SYSMAN_MMC_FORMAT_PROGRESS, VCONFKEY_SYSMAN_MMC_FORMAT_PROGRESS_NOW);
	device_set_property(DEVTYPE_MMC, MMC_PROP_FORMAT, 0);
	vconf_set_int(VCONFKEY_SYSMAN_MMC_FORMAT_PROGRESS, VCONFKEY_SYSMAN_MMC_FORMAT_PROGRESS_NONE);
	ss_mmc_inserted();
	return 0;
}

int ss_mmc_unmounted(int argc, char **argv)
{
	int option = -1;
	int mmc_err = 0;
	if (argc < 1) {
		PRT_TRACE_ERR("Option is wong");
		return -1;
	}
	if ((option = atoi(argv[0])) < 0) {
		PRT_TRACE_ERR("Option is wong : %d", option);
		return -1;
	}

	if (umount2(MMC_MOUNT_POINT, option) != 0) {
		mmc_err = errno;
		PRT_TRACE_ERR("Failed to unmount mmc card\n");
		vconf_set_int(VCONFKEY_SYSMAN_MMC_UNMOUNT,
			      VCONFKEY_SYSMAN_MMC_UNMOUNT_FAILED);
		vconf_set_int(VCONFKEY_SYSMAN_MMC_ERR_STATUS,
				mmc_err);
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
	/* mmc card mount */
	ss_mmc_inserted();

	ss_action_entry_add_internal(PREDEF_MOUNT_MMC, ss_mmc_inserted, NULL, NULL);
	ss_action_entry_add_internal(PREDEF_UNMOUNT_MMC, ss_mmc_unmounted, NULL, NULL);
	ss_action_entry_add_internal(PREDEF_FORMAT_MMC, ss_mmc_format, NULL, NULL);
	return 0;
}

static int __mount_fs(char *path, const char *fs_name, const char *mount_data)
{
	int ret, retry = 0;
	do {
		if ((ret = mount(path, MMC_MOUNT_POINT, fs_name, 0, mount_data)) == 0) {
			PRT_TRACE("Mounted mmc card\n");
			return 0;
		}
		usleep(100000);
	} while (ret == -1 && errno == ENOENT && retry++ < 10);

	return errno;
}

static int __mmc_mount_fs(void)
{
	char buf[NAME_MAX];
	char params[NAME_MAX];
	int fd;
	int blk_num;

	if (access(MMC_MOUNT_POINT, R_OK) != 0) {
		if (mkdir(MMC_MOUNT_POINT, 0755) < 0) {
			PRT_TRACE_ERR("Make Directory is failed");
			return errno;
		}
	}

	blk_num = get_mmcblk_num();
	if (blk_num  == -1) {
		PRT_TRACE_ERR("fail to check mmc block");
		return -1;
	}

	snprintf(buf, sizeof(buf), "%s%dp1", MMC_DEV, blk_num);
	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		PRT_TRACE_ERR("can't open the '%s': %s", buf, strerror(errno));
		snprintf(buf, sizeof(buf), "%s%d", MMC_DEV, blk_num);
	}
	close(fd);

		if (__mount_fs(buf, "vfat", FS_VFAT_MOUNT_OPT) != 0)
			return errno;
	return 0;
}

int ss_mmc_inserted(void)
{
	int r = 0;

	if (mmc_status == VCONFKEY_SYSMAN_MMC_MOUNTED) {
		PRT_DBG("Mmc is already mounted.\n");
		vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS, VCONFKEY_SYSMAN_MMC_MOUNTED);
		vconf_set_int(VCONFKEY_SYSMAN_MMC_MOUNT, VCONFKEY_SYSMAN_MMC_MOUNT_ALREADY);

		return -1;
	}

	r = __mmc_mount_fs();
	if (r == 0)
		goto mount_complete;
	else if (r == -1)
		goto mount_fail;
	vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS, VCONFKEY_SYSMAN_MMC_INSERTED_NOT_MOUNTED);
	vconf_set_int(VCONFKEY_SYSMAN_MMC_MOUNT, VCONFKEY_SYSMAN_MMC_MOUNT_FAILED);
	vconf_set_int(VCONFKEY_SYSMAN_MMC_ERR_STATUS, errno);
	mmc_status = VCONFKEY_SYSMAN_MMC_INSERTED_NOT_MOUNTED;
	PRT_TRACE_ERR("Failed to mount mmc card\n");
	return -1;

mount_complete:
	vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS, VCONFKEY_SYSMAN_MMC_MOUNTED);
	vconf_set_int(VCONFKEY_SYSMAN_MMC_MOUNT, VCONFKEY_SYSMAN_MMC_MOUNT_COMPLETED);
	mmc_status = VCONFKEY_SYSMAN_MMC_MOUNTED;
	return 0;
mount_fail:
	vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS, VCONFKEY_SYSMAN_MMC_REMOVED);
	vconf_set_int(VCONFKEY_SYSMAN_MMC_MOUNT, VCONFKEY_SYSMAN_MMC_MOUNT_FAILED);
	vconf_set_int(VCONFKEY_SYSMAN_MMC_ERR_STATUS, VCONFKEY_SYSMAN_MMC_EINVAL);
	mmc_status = VCONFKEY_SYSMAN_MMC_REMOVED;
	return 0;
}

static int __umount_fs(void)
{
	int ret;
	if ((ret = umount2(MMC_MOUNT_POINT, MNT_DETACH)) != 0) {
		PRT_TRACE_ERR("Failed to unmount mmc card\n");
	}
	mmc_status = VCONFKEY_SYSMAN_MMC_REMOVED;
	return ret;
}

int ss_mmc_removed()
{
	int mmc_err = 0;
	vconf_set_int(VCONFKEY_SYSMAN_MMC_STATUS, VCONFKEY_SYSMAN_MMC_REMOVED);
	mmc_err = __umount_fs();
	vconf_set_int(VCONFKEY_SYSMAN_MMC_ERR_STATUS, mmc_err);

	return 0;
}
