/*
 * deviced
 *
 * Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __LOG_H__
#define __LOG_H__

#ifndef ENABLE_DLOG_OUT
#define ENABLE_DLOG_OUT
#endif

#ifdef ENABLE_DLOG_OUT
#define LOG_TAG	"SYSTEM_SERVER"
#include <dlog.h>
#define _D(fmt, arg...) \
	do { SLOGD(fmt, ##arg); } while(0)
#define _I(fmt, arg...) \
	do { SLOGI(fmt, ##arg); } while(0)
#define _W(fmt, arg...) \
	do { SLOGW(fmt, ##arg); } while(0)
#define _E(fmt, arg...) \
	do { SLOGE(fmt, ##arg); } while(0)
#define _SD(fmt, arg...) \
	do { SECURE_SLOGD(fmt, ##arg); } while(0)
#define _SI(fmt, arg...) \
	do { SECURE_SLOGI(fmt, ##arg); } while(0)
#define _SW(fmt, arg...) \
	do { SECURE_SLOGW(fmt, ##arg); } while(0)
#define _SE(fmt, arg...) \
	do { SECURE_SLOGE(fmt, ##arg); } while(0)
#else
#define _D(x, ...)	do { } while (0)
#define _I(x, ...)	do { } while (0)
#define _W(x, ...)	do { } while (0)
#define _E(x, ...)	do { } while (0)
#define _SD(fmt, args...)	do { } while (0)
#define _SI(fmt, args...)	do { } while (0)
#define _SW(fmt, args...)	do { } while (0)
#define _SE(fmt, args...)	do { } while (0)
#endif

#endif
