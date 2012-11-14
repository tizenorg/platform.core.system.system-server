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
#ifndef __SYS_PCI_NOTI_H__
#define __SYS_PCI_NOTI_H__

#ifndef _
#define _(str) gettext(str)
#endif

#ifndef gettext_noop
#define gettext_noop(str) (str)
#endif

#ifndef N_
#define N_(str) gettext_noop(str)
#endif

typedef enum {
	CB_NOTI_PCI_REMOVED = 0,
	CB_NOTI_PCI_INSERTED
}pci_noti_type;

#endif /* __SYS_PCI_NOTI_H__ */
