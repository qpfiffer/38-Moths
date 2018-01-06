// vim: noet ts=4 sw=4
#pragma once

typedef enum {
	LOG_INFO,
	LOG_DEBUG,
	LOG_WARN,
	LOG_ERR,
	LOG_FUN,
	LOG_DB
} log_level;

/* xXx DEFINE=m38_log_msg xXx
 * xXx DESCRIPTION=Internal logging utility. xXx
 * xXx RETURNS=Nothin' xXx
 */
void m38_log_msg(log_level level, const char *fmsg, ...);
