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


#ifndef __SS_LAUNCH_H__
#define __SS_LAUNCH_H__

#define SS_LAUNCH_NICE          0x0002

int ss_launch_if_noexist(const char *execpath, const char *arg, ...);
int ss_launch_evenif_exist(const char *execpath, const char *arg, ...);
int ss_launch_after_kill_if_exist(const char *execpath, const char *arg, ...);

#endif /* __SS_LAUNCH_H__ */