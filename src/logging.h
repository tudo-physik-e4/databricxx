// Copyright (C) 2011-2014 Oliver Schulz <oschulz@mpp.mpg.de>

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


#ifndef DBRX_LOGGING_H
#define DBRX_LOGGING_H

#include <stdint.h>
#include <string>
#include <stdarg.h>


namespace dbrx {


enum LogLevel {
	LL_ALL   = 0,
	LL_TRACE = 10,
	LL_DEBUG = 20,
	LL_INFO  = 30,
	LL_WARN  = 40,
	LL_ERROR = 50,
	LL_OFF   = 0xffffffffl
};

extern LogLevel g_logLevel;

inline LogLevel log_level() { return g_logLevel; }
void log_level(LogLevel level);
void log_level(const char* level);
void log_level(const std::string &level);

LogLevel string2LogLevel(const char *level);
const char* logLevel2String(LogLevel level);

void logLevelName(LogLevel level);

inline bool log_enabled(LogLevel level) { return level >= g_logLevel; }

inline void log_to(std::ostream &stream);

void v_log_generic_impl(const char* tag, const char *fmt, va_list argp);

void log_trace_impl(const char *fmt, ...);
void log_debug_impl(const char *fmt, ...);
void log_info_impl(const char *fmt, ...);
void log_warn_impl(const char *fmt, ...);
void log_error_impl(const char *fmt, ...);

inline void log_nothing_impl() {}


class TmpLogLevel {
protected:
	LogLevel m_storedLogLevel;

public:
	TmpLogLevel(LogLevel level) {
		m_storedLogLevel = log_level();
		log_level(level);
	}

	virtual ~TmpLogLevel()
		{ log_level(m_storedLogLevel); }
};


} // namespace dbrx


#define log_trace(...) ((dbrx::LL_TRACE >= dbrx::g_logLevel) ? dbrx::log_trace_impl(__VA_ARGS__) : dbrx::log_nothing_impl())
#define log_debug(...) ((dbrx::LL_DEBUG >= dbrx::g_logLevel) ? dbrx::log_debug_impl(__VA_ARGS__) : dbrx::log_nothing_impl())
#define log_info(...) ((dbrx::LL_INFO >= dbrx::g_logLevel) ? dbrx::log_info_impl(__VA_ARGS__) : dbrx::log_nothing_impl())
#define log_warn(...) ((dbrx::LL_WARN >= dbrx::g_logLevel) ? dbrx::log_warn_impl(__VA_ARGS__) : dbrx::log_nothing_impl())
#define log_error(...) ((dbrx::LL_ERROR >= dbrx::g_logLevel) ? dbrx::log_error_impl(__VA_ARGS__) : dbrx::log_nothing_impl())


#endif // DBRX_LOGGING_H