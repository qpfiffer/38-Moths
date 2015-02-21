// vim: noet ts=4 sw=4
#pragma once

typedef enum {
	LOG_INFO,
	LOG_WARN,
	LOG_ERR,
	LOG_FUN,
	LOG_DB
} log_level;

void log_msg(log_level level, const char *fmsg, ...);
