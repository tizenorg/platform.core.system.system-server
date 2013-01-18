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


#include <stdio.h>
#include <libintl.h>
#include <locale.h>
#include <vconf.h>
#include <syspopup_caller.h>
#include "ss_log.h"
#include "sys_pci_noti.h"

#define TICKERNOTI_LOCATION_TOP 0

static void pci_noti(pci_noti_type iPCI)
{
	char *lang;
	char *r;
	int ret = -1;
	bundle *b = NULL;
	const int arrSize = 2;
	char str_tickernoti[arrSize];

	lang = vconf_get_str(VCONFKEY_LANGSET);
	if (lang) {
		setenv("LANG", lang, 1);
		setenv("LC_MESSAGES", lang, 1);
		r = setlocale(LC_ALL, "");
		if (r == NULL) {
			setlocale(LC_ALL, lang);
		}
		free(lang);
	}
	bindtextdomain("sys_pci_noti","/usr/share/system-server/sys_pci_noti/res/locale/");
	textdomain("sys_pci_noti");

	b = bundle_create();
	if(b == NULL) {
		PRT_TRACE_ERR("FAIL: bundle_create()");
		return;
	}
	/* set tickernoti style */
	ret = bundle_add(b, "0", "info");
	if (ret != 0)
		goto BUNDLE_ADD_ERR;

	/* set tickernoti text */
	if (iPCI == CB_NOTI_PCI_REMOVED)
		ret = bundle_add(b, "1", _("IDS_COM_POP_KEYBOARD_DISCONNECTED_ABB2"));
	else if (iPCI == CB_NOTI_PCI_INSERTED)
		ret = bundle_add(b, "1", _("IDS_COM_POP_KEYBOARD_CONNECTED_ABB2"));
	if (ret != 0)
		goto BUNDLE_ADD_ERR;
	/* set tickernoti orientation */
	snprintf(str_tickernoti,arrSize,"%d",TICKERNOTI_LOCATION_TOP);
	ret = bundle_add(b, "2", str_tickernoti);
	if (ret != 0)
		goto BUNDLE_ADD_ERR;

	/* set tickernoti timeout */
	ret = bundle_add(b, "3", "3");
	if (ret != 0)
		goto BUNDLE_ADD_ERR;

	/* launch tickernoti */
	ret = syspopup_launch("tickernoti-syspopup", b);
	if (ret < 0)
		PRT_TRACE_ERR("FAIL: syspopup_launch()");

	if(bundle_free(b) != 0)
		PRT_TRACE_ERR("FAIL: bundle_free()");
	return;

BUNDLE_ADD_ERR:
	PRT_TRACE_ERR("FAIL: bundle_add()");
	if (bundle_free(b) != 0)
		PRT_TRACE_ERR("FAIL: bundle_free()");
	return;

}
int main(int argc, char *argv[])
{
	int r = 0;
	int handle = 0;
	int bNoti = -1;
	pci_noti_type cb_type = -1;

	if (argc == 2) {
		cb_type = (pci_noti_type)atoi(argv[1]);
		pci_noti(cb_type);
	}
	else {
		PRT_TRACE_ERR("FAIL param error");
	}

	return 0;
}
