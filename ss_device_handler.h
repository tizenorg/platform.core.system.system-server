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


#ifndef __SS_DEVICE_HANDLER_H__
#define __SS_DEVICE_HANDLER_H__

#include "include/ss_data.h"

/* MMC functions */
int ss_mmc_init();
int ss_mmc_inserted();
int ss_mmc_removed();

/* USB Storage */
int _ss_usb_storage_init(void);

/* Battery functions */
int ss_lowbat_init(struct ss_main_data *ad);
int ss_lowbat_is_charge_in_now();
int ss_lowbat_set_charge_on(int onoff);
int ss_lowbat_monitor(void *data);

/* Low memory functions */
int ss_lowmem_init(struct ss_main_data *ad);

/* USB functions */
int ss_usb_init();

/* TA functions */
int ss_ta_init();

/* device change init */
int ss_device_change_init(struct ss_main_data *ad);

#endif /* __SS_DEVICE_HANDLER_H__ */
