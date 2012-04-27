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


#ifndef __SS_LOG_H__
#define __SS_LOG_H__

#include <stdio.h>

#if defined(ENABLE_DLOG_OUT)
#define LOG_TAG		"SYSTEM_SERVER"
#include <dlog.h>
#define DLOG_ERR		DLOG_ERROR
#define __LOG(prio, fmt, arg...) \
	do { SLOG(prio, LOG_TAG, fmt, ##arg); } while (0)
#define __LOGD(prio, fmt, arg...) \
	do { SLOG(prio, LOG_TAG, "[%s:%d] "fmt"\n", __FUNCTION__, __LINE__, ##arg); } while (0)
#define __PRT(prio, fmt, arg...) \
	do { fprintf(((D##prio) == DLOG_ERR ? stderr : stdout), fmt"\n", ##arg); } while (0)
#define __PRTD(prio, fmt, arg...) \
	do { \
		fprintf(((D##prio) == DLOG_ERR ? stderr : stdout), \
				"[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
	} while (0)
#else
#include <syslog.h>
#define __LOG(prio, fmt, arg...) \
	do { syslog(prio, fmt, ##arg); } while (0)
#define __LOGD(prio, fmt, arg...) \
	do { syslog(prio, "[%s:%d] "fmt"\n", __FUNCTION__, __LINE__, ##arg); } while (0)
#define __PRT(prio, fmt, arg...) \
	do { fprintf(((prio) == LOG_ERR ? stderr : stdout), fmt"\n", ##arg); } while (0)
#define __PRTD(prio, fmt, arg...) \
	do { \
		fprintf(((prio) == LOG_ERR ? stderr : stdout), \
				"[%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg); \
	} while (0)
#endif

#ifdef DEBUG
extern int g_trace_depth;
#define __PRT_TRACE(prio, fmt, arg...) \
	do { __LOGD(prio, fmt, ##arg); } while (0)
/*	do { \
		int ___i;\
		for(___i=0;___i<g_trace_depth;___i++)\
		{\
			fprintf(stdout, "  "); \
		}\
		fprintf(stdout,\
			"[%s:%d] "fmt"\n", __FUNCTION__, __LINE__,##arg); \
		__LOGD(prio, fmt, ##arg); \
	} while (0) */
#define __PRT_TRACE_ERR(prio, fmt, arg...) \
	do { __LOGD(prio, fmt, ##arg); } while (0)
/*	do { \
		int ___i;\
		for(___i=0;___i<g_trace_depth;___i++)\
		{\
			fprintf(stdout, "  "); \
		}\
		printf(\
			"%c[1;31m[%s:%d] "fmt"%c[0m\n",27, __FUNCTION__, __LINE__,##arg,27); \
		__LOGD(prio, fmt, ##arg); \
	} while (0)*/
#define __PRT_TRACE_EM(prio, fmt, arg...) \
	do { __LOGD(prio, fmt, ##arg); } while (0)
/*	do { \
		int ___i;\
		for(___i=0;___i<g_trace_depth;___i++)\
		{\
			fprintf(stdout, "  "); \
		}\
		printf(\
			"%c[1;34m[%s:%d] "fmt"%c[0m\n",27, __FUNCTION__, __LINE__,##arg,27); \
		__LOGD(prio, fmt, ##arg); \
	} while (0)*/
#endif

#define _NOUT(prio, fmt, arg...) do { } while (0)

#ifdef DEBUG
#  define _LOGD __LOGD
#  define _LOG  __LOGD
#  define _PRTD __PRTD
#  define _PRT  __PRTD
#  define _PRT_TRACE __PRT_TRACE
#  define _PRT_TRACE_ERR __PRT_TRACE_ERR
#  define _PRT_TRACE_EM __PRT_TRACE_EM
#else
#  define _LOGD _NOUT
#  define _LOG  __LOG
#  define _PRTD _NOUT
#  define _PRT  __PRT
#  define _PRT_TRACE _NOUT
#  define _PRT_TRACE_ERR _NOUT
#  define _PRT_TRACE_EM _NOUT
#endif

#define PRT_INFO(fmt, arg...) _PRT(LOG_INFO, fmt, ##arg)
#define PRT_ERR(fmt, arg...) _PRT(LOG_ERR, fmt, ##arg)
#define PRT_DBG(fmt, arg...) _PRTD(LOG_DEBUG, fmt, ##arg)
#define PRT_TRACE(fmt, arg...) _PRT_TRACE(LOG_DEBUG, fmt, ##arg)
#define PRT_TRACE_ERR(fmt, arg...) _PRT_TRACE_ERR(LOG_ERR, fmt, ##arg)
#define PRT_TRACE_EM(fmt, arg...) _PRT_TRACE_EM(LOG_DEBUG, fmt, ##arg)

#if defined(SYSLOG_OUT)
#  define SYSLOG_INFO(fmt, arg...) _LOG(LOG_INFO, fmt, ##arg)
#  define SYSLOG_ERR(fmt, arg...) _LOG(LOG_ERR, fmt, ##arg)
#  define SYSLOG_DBG(fmt, arg...) _LOGD(LOG_DEBUG, fmt, ##arg)
#  define INFO SYSLOG_INFO
#  define ERR SYSLOG_ERR
#  define DBG SYSLOG_DBG
#elif defined(ENABLE_DLOG_OUT)
#  define INFO SLOGI
#  define ERR SLOGE
#  define DBG SLOGD
#else
#  define INFO PRT_INFO
#  define ERR PRT_ERR
#  define DBG PRT_DBG
#endif

#endif
