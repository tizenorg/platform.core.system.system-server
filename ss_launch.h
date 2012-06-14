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


#ifndef __SS_LAUNCH_H__
#define __SS_LAUNCH_H__

#define SS_LAUNCH_NICE          0x0002

int ss_launch_if_noexist(const char *execpath, const char *arg, ...);
int ss_launch_evenif_exist(const char *execpath, const char *arg, ...);
int ss_launch_after_kill_if_exist(const char *execpath, const char *arg, ...);

#endif /* __SS_LAUNCH_H__ */
