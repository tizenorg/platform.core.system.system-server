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


#ifndef __SS_PROCMGR_H__
#define __SS_PROCMGR_H__

int ss_process_manager_init(void);

int get_app_oomadj(int pid, int *oomadj);
int set_app_oomadj(int pid, int new_oomadj);

#endif /* __SS_PROCMGR_H__ */
