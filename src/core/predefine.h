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


#ifndef __PREDEFINE_H__
#define __PREDEFINE_H__
#include <bundle.h>

int call_predefine_action(int argc, char **argv);
void ss_predefine_internal_init(void);
void predefine_pm_change_state(unsigned int s_bits);
int predefine_control_launch(char *popup_name, bundle *b, int option);
#endif /* __PREDEFINE_H__ */
