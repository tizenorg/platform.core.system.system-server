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
#ifndef __SYS_DEVICE_NOTI_H__
#define __SYS_DEVICE_NOTI_H__

typedef enum {
	CB_NOTI_BATT_CHARGE,
	CB_NOTI_BATT_LOW,
	CB_NOTI_BATT_FULL,
	CB_NOTI_MAX
}cb_noti_type;
typedef enum {
	CB_NOTI_OFF	= 0,
	CB_NOTI_ON	= 1
}cb_noti_onoff_type;

#endif /* __SYS_DEVICE__NOTI_H__ */
