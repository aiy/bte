/*
 * Copyright (c) 2014 - 2020 <aiy@ferens.net> 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _ULLOG_H_
#define _ULLOG_H_

#include <syslog.h>

#ifdef __cplusplus
extern "C" {
#endif

// log destinations
// STDOUT, STDERR, SYSLOG, ?TEST_OUTPUT_PRODUCER
// STDOUT id default
#define ULLOG_DEST_STDOUT (1 << 0)
#define ULLOG_DEST_STDERR (1 << 1)
#define ULLOG_DEST_SYSLOG (1 << 2)
#ifndef ULLOG_DEST
  #define ULLOG_DEST ULLOG_DEST_STDOUT
#endif
// example to set to STDOUT and STDERR
//#define ULLOG_DEST (ULLOG_DEST_STDOUT | ULLOG_DEST_STDERR)

#define	ULLOG_EMERG	(1 << 0)/* system is unusable */
#define	ULLOG_ALERT	(1 << 1)/* action must be taken immediately */
#define	ULLOG_CRIT	(1 << 2)/* critical conditions */
#define	ULLOG_ERR	(1 << 3)/* error conditions */
#define	ULLOG_WARNING	(1 << 4)/* warning conditions */
#define	ULLOG_NOTICE	(1 << 5)/* normal but significant condition */
#define	ULLOG_INFO	(1 << 6)/* informational */
#define	ULLOG_DEBUG	(1 << 7)/* debug-level messages */
#ifndef ULLOG_LEVEL
  #define ULLOG_LEVEL ULLOG_NOTICE
#endif
#define	__ULLOG_UPTO__(pri)	((1 << ((pri)+1)) - 1)	/* all priorities through pri */

#define ULLOG_FACILITY_LOCAL3 LOG_LOCAL3
// TODO add the rest
#ifndef ULLOG_FACILITY
  #define ULLOG_FACILITY ULLOG_FACILITY_LOCAL3
#endif

#if ULLOG_DEST & ULLOG_DEST_SYSLOG
  #define ullog_init(name) \
    do { \
      openlog(name, 0, ULLOG_FACILITY); \
    } while (0)
  #define ullog_deinit() \
    do { \
      closelog(); \
    } while (0)
#else
  #define ullog_init(name) \
    do{ } while (0) 
  #define ullog_deinit() \
    do{ } while (0) 
#endif 

#define __ULLOG_EMERG(fmt, args...) \
	do { \
		if (ULLOG_DEST & ULLOG_DEST_STDOUT) { \
			printf("emerg:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); printf("\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_STDERR) { \
			fprintf(stderr, "emerg:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); fprintf(stderr, "\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_SYSLOG) { \
    	syslog(ULLOG_EMERG | ULLOG_FACILITY, "%s,%d,%s(): " fmt,  __FILE__, __LINE__, __func__, ##args); \
		} \
	} while (0) 

#define __ULLOG_ALERT(fmt, args...) \
	do { \
		if (ULLOG_DEST & ULLOG_DEST_STDOUT) { \
			printf("alert:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); printf("\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_STDERR) { \
			fprintf(stderr, "alert:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); fprintf(stderr, "\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_SYSLOG) { \
    	syslog(ULLOG_ALERT | ULLOG_FACILITY, "%s,%d,%s(): " fmt,  __FILE__, __LINE__, __func__, ##args); \
		} \
	} while (0) 

#define __ULLOG_CRIT(fmt, args...) \
	do { \
		if (ULLOG_DEST & ULLOG_DEST_STDOUT) { \
			printf("crit:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); printf("\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_STDERR) { \
			fprintf(stderr, "crit:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); fprintf(stderr, "\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_SYSLOG) { \
    	syslog(ULLOG_CRIT | ULLOG_FACILITY, "%s,%d,%s(): " fmt,  __FILE__, __LINE__, __func__, ##args); \
		} \
	} while (0) 

#define __ULLOG_ERR(fmt, args...) \
	do { \
		if (ULLOG_DEST & ULLOG_DEST_STDOUT) { \
			printf("err:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); printf("\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_STDERR) { \
			fprintf(stderr, "err:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); fprintf(stderr, "\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_SYSLOG) { \
    	syslog(ULLOG_ERR | ULLOG_FACILITY, "%s,%d,%s(): " fmt,  __FILE__, __LINE__, __func__, ##args); \
		} \
	} while (0) 

#define __ULLOG_WARNING(fmt, args...) \
	do { \
		if (ULLOG_DEST & ULLOG_DEST_STDOUT) { \
			printf("warning:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); printf("\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_STDERR) { \
			fprintf(stderr, "warning:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); fprintf(stderr, "\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_SYSLOG) { \
    	syslog(ULLOG_WARNING | ULLOG_FACILITY, "%s,%d,%s(): " fmt,  __FILE__, __LINE__, __func__, ##args); \
		} \
	} while (0) 

#define __ULLOG_NOTICE(fmt, args...) \
	do { \
		if (ULLOG_DEST & ULLOG_DEST_STDOUT) { \
			printf("notice:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); printf("\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_STDERR) { \
			fprintf(stderr, "notice:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); fprintf(stderr, "\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_SYSLOG) { \
    	syslog(ULLOG_NOTICE | ULLOG_FACILITY, "%s,%d,%s(): " fmt,  __FILE__, __LINE__, __func__, ##args); \
		} \
	} while (0) 

#define __ULLOG_INFO(fmt, args...) \
	do { \
		if (ULLOG_DEST & ULLOG_DEST_STDOUT) { \
			printf("info:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); printf("\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_STDERR) { \
			fprintf(stderr, "info:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); fprintf(stderr, "\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_SYSLOG) { \
    	syslog(ULLOG_INFO | ULLOG_FACILITY, "%s,%d,%s(): " fmt,  __FILE__, __LINE__, __func__, ##args); \
		} \
	} while (0) 

#define __ULLOG_DEBUG(fmt, args...) \
	do { \
		if (ULLOG_DEST & ULLOG_DEST_STDOUT) { \
			printf("debug:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); printf("\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_STDERR) { \
			fprintf(stderr, "debug:  %s:%d:%s(): " fmt, \
			__FILE__, __LINE__, __func__, ##args); fprintf(stderr, "\n"); \
		} \
		if (ULLOG_DEST & ULLOG_DEST_SYSLOG) { \
    	syslog(ULLOG_DEBUG | ULLOG_FACILITY, "%s,%d,%s(): " fmt,  __FILE__, __LINE__, __func__, ##args); \
		} \
	} while (0) 

#define ullog(lvl, fmt, args...) \
	do { \
		if (lvl <= ULLOG_EMERG && lvl <= ULLOG_LEVEL) { \
			__ULLOG_EMERG(fmt, ##args); \
		} else if (lvl <= ULLOG_ALERT && lvl <= ULLOG_LEVEL) { \
			__ULLOG_ALERT(fmt, ##args); \
		} else if (lvl <= ULLOG_CRIT && lvl <= ULLOG_LEVEL) { \
			__ULLOG_CRIT(fmt, ##args); \
		} else if (lvl <= ULLOG_ERR && lvl <= ULLOG_LEVEL) { \
			__ULLOG_ERR(fmt, ##args); \
		} else if (lvl <= ULLOG_WARNING && lvl <= ULLOG_LEVEL) { \
			__ULLOG_WARNING(fmt, ##args); \
		} else if (lvl <= ULLOG_NOTICE && lvl <= ULLOG_LEVEL) { \
			__ULLOG_NOTICE(fmt, ##args); \
		} else if (lvl <= ULLOG_INFO && lvl <= ULLOG_LEVEL) { \
			__ULLOG_INFO(fmt, ##args); \
		} else if (lvl <= ULLOG_DEBUG && lvl <= ULLOG_LEVEL) { \
			__ULLOG_DEBUG(fmt, ##args); \
		} else { \
    	do{ } while (0); \
		} \
	} while (0) 

#define ullog_emerg(fmt, args...) \
	if (ULLOG_EMERG <= ULLOG_LEVEL) { \
		__ULLOG_EMERG(fmt, ##args); \
	} else { \
   	do{ } while (0); \
	} \

#define ullog_alert(fmt, args...) \
	if (ULLOG_ALERT <= ULLOG_LEVEL) { \
		__ULLOG_ALERT(fmt, ##args); \
	} else { \
   	do{ } while (0); \
	} \

#define ullog_crit(fmt, args...) \
	if (ULLOG_CRIT <= ULLOG_LEVEL) { \
		__ULLOG_CRIT(fmt, ##args); \
	} else { \
   	do{ } while (0); \
	} \

#define ullog_err(fmt, args...) \
	if (ULLOG_ERR <= ULLOG_LEVEL) { \
		__ULLOG_ERR(fmt, ##args); \
	} else { \
   	do{ } while (0); \
	} \

#define ullog_warn(fmt, args...) \
	if (ULLOG_WARNING <= ULLOG_LEVEL) { \
		__ULLOG_WARNING(fmt, ##args); \
	} else { \
   	do{ } while (0); \
	} \

#define ullog_notice(fmt, args...) \
	if (ULLOG_NOTICE <= ULLOG_LEVEL) { \
		__ULLOG_NOTICE(fmt, ##args); \
	} else { \
   	do{ } while (0); \
	} \

#define ullog_info(fmt, args...) \
	if (ULLOG_INFO <= ULLOG_LEVEL) { \
		__ULLOG_INFO(fmt, ##args); \
	} else { \
   	do{ } while (0); \
	} \

#define ullog_debug(fmt, args...) \
	if (ULLOG_DEBUG <= ULLOG_LEVEL) { \
		__ULLOG_DEBUG(fmt, ##args); \
	} else { \
   	do{ } while (0); \
	} \

#if 0
#if (ULLOG_DEST_STDOUT > 0) && (ULLOG_DEST_STDERR > 0) && (ULLOG_DEST_SYSLOG > 0)
  #define ullog(lvl, fmt, args...) \
  do { \
    printf("%s:%d:%s(): " fmt, \
      __FILE__, __LINE__, __func__, ##args); printf("\n"); \
    fprintf(stderr, "%s:%d:%s(): " fmt, \
      __FILE__, __LINE__, __func__, ##args); printf("\n"); \
    syslog(lvl, "%s:%d:%s(): " fmt, \
      __FILE__, __LINE__, __func__, ##args); \
  } while (0)
#elif (ULLOG_DEST_STDOUT > 0) && (ULLOG_DEST_STDERR > 0) && (ULLOG_DEST_SYSLOG == 0)
  #define ullog(lvl, fmt, args...) \
  do { \
    printf("%s:%d:%s(): " fmt, \
      __FILE__, __LINE__, __func__, ##args); printf("\n"); \
    fprintf(stderr, "%s:%d:%s(): " fmt, \
      __FILE__, __LINE__, __func__, ##args); printf("\n"); \
  } while (0)
#elif (ULLOG_DEST_STDOUT > 0) && (ULLOG_DEST_STDERR == 0) && (ULLOG_DEST_SYSLOG > 0)
  #define ullog(lvl, fmt, args...) \
  do { \
    printf("%s:%d:%s(): " fmt, \
      __FILE__, __LINE__, __func__, ##args); printf("\n"); \
    syslog(lvl, "%s:%d:%s(): " fmt, \
      __FILE__, __LINE__, __func__, ##args); \
  } while (0)
#elif (ULLOG_DEST_STDOUT > 0) && (ULLOG_DEST_STDERR == 0) && (ULLOG_DEST_SYSLOG == 0)
  #define ullog(lvl, fmt, args...) \
  do { \
    printf("%s:%d:%s(): " fmt, \
      __FILE__, __LINE__, __func__, ##args); printf("\n"); \
  } while (0)
#else
  #define ullog(lvl, fmt, args...) \
    do{ } while (0) 
#endif
#endif

#if 0
#if (ULLOG_DEST & ULLOG_DEST_STDERR) 
  #define ullog(fmt, args...) \
    do { \
      fprintf(stderr, "%s:%d:%s(): " fmt, \
        __FILE__, __LINE__, __func__, ##args); printf("\n"); \
    } while (0)
#endif
#endif


#if 0
#if defined(DEBUG) && DEBUG > 0
 #if defined(SYSLOG) && SYSLOG > 0
  #define PRINTF(fmt, args...) \
  do { \
    syslog(LOG_DEBUG | LOG_LOCAL3, "ul: %s,%d,%s(): " fmt,  __FILE__, __LINE__, __func__, ##args); \
  } while (0)
 #else
  #define PRINTF(fmt, args...) do { printf("%s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args); printf("\n"); } while (0)
 #endif
#else
 #define PRINTF(fmt, args...) do{ } while ( false )
#endif
#endif

#if 0
#define ullog(level, format, ...)                                   \
    do {                                                                   \
        lc_log_internal(__FUNCTION__, __FILE__, __LINE__,                  \
                        0, 0, level | LOG_LOCAL3, "ul: " format, ## __VA_ARGS__);              \
    } while(0)

#define ullog_debug(format, ...)                                   \
    do {                                                                   \
        lc_log_internal(__FUNCTION__, __FILE__, __LINE__,                  \
                LCO_SOURCE_INFO, 0, LOG_DEBUG | LOG_LOCAL3, "ul: " format, ## __VA_ARGS__); \
    } while(0)

#endif


#if 0
// old implementation

#define S_DEBUG_VERBOSE 6
#define S_DEBUG 5
#define S_INFO 4
#define S_WARNING 3
#define S_ERROR 2
#define S_CRITICAL 1

#define LOGGER_MAX_FILES                  5
#define LOGGER_MAX_FILE_SZ                1048576
#include "ulfile.h"


typedef struct LOG_PROPERTIES
{
  pthread_mutex_t g_mtx_log;
  int g_maxFilesNum;
  int g_maxFileSz;
  char g_logName[MAX_FILEPATH_LEN];
  FILE *g_fp;
  int g_fd;
  int g_log_flags;
  int g_log_level;
} LOG_PROPERTIES_T;


#define INIT_LOGGER(name)     logger_Init("log", name, LOGGER_MAX_FILES, LOGGER_MAX_FILE_SZ)

LOG_PROPERTIES_T * logger_init(void);

int logger_Init(const char *filePath,
                const char *fileName,
                unsigned int maxFilesNum,
                unsigned int maxFileSz);

int logger_InitSyslog(const char *ident, int sylogOpt, int syslogFacility);

int logger_SetLevel(int sev);
void logger_AddStderr();
void logger_Log(int sev, const char *msg, ...);
int logger_GetLogLevel();
int logger_SetHistory(unsigned int maxFiles, unsigned int maxFileBytes);
int logger_open(LOG_PROPERTIES_T *pLogProperties);
void logger_close(LOG_PROPERTIES_T *pLogProperties);



#define LOG logger_Log

#define LOG_FULL_FMT(fmt) "("__FILE__":%d) " fmt"\n", __LINE__

// These expansions are here because I cannot get a variable length macro
// to work in win32 SDK
#define X_CRITICAL(fmt) S_CRITICAL, LOG_FULL_FMT(fmt)
#define X_ERROR(fmt) S_ERROR, LOG_FULL_FMT(fmt)
#define X_WARNING(fmt) S_WARNING, LOG_FULL_FMT(fmt)
#define X_INFO(fmt) S_INFO, fmt"\n"
#define X_DEBUG(fmt) S_DEBUG, fmt"\n"
#define X_DEBUGV(fmt) S_DEBUG_VERBOSE, fmt"\n"

#define LOG_MSG(sv, fmt, args...) \
                if(sv == S_INFO) \
                  LOG(sv, fmt"\n", ##args); \
                else \
                  LOG(sv, "("__FILE__":%d) "fmt"\n", __LINE__, ## args)
#endif

#ifdef __cplusplus
}  
#endif //#ifdef __cplusplus

#endif /* _ULLOG_H_ */
