#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

const char *pathToFileName( const char *path );

#define LOGGER_BUFFLEN 256

#define LOGGER_LOG_FORMAT( format ) "[%s:%u] %s():\t" format "\r\n", pathToFileName( __FILE__ ), __LINE__, __FUNCTION__
#define LOGGER_LOG_FORMAT_INLINE( format ) "[%s:%u] %s():\t" format, pathToFileName( __FILE__ ), __LINE__, __FUNCTION__

#define logln( format, ... ) logger_log_formatted_string( LOGGER_LOG_FORMAT( format ), ##__VA_ARGS__ )

#define log_inline_begin() logger_log_formatted_string( LOGGER_LOG_FORMAT_INLINE( "" ) )
#define log_inline( format, ... ) logger_log_formatted_string( format, ##__VA_ARGS__ )
#define log_inline_end() logger_log_formatted_string( "\r\n" )

#define log_esp( format, ... )

#ifdef debug
#define debugln( format, ... ) logger_log_formatted_string( LOGGER_LOG_FORMAT( format ), ##__VA_ARGS__ )
#define debug_inline_begin() logger_log_formatted_string( LOGGER_LOG_FORMAT_INLINE( "" ) )
#define debug_inline( format, ... ) logger_log_formatted_string( format, ##__VA_ARGS__ )
#define debug_inline_end() logger_log_formatted_string( "\r\n" )
#else
#define debugln( format, ... )
#define debug_inline_begin()
#define debug_inline( format, ... )
#define debug_inline_end()
#endif

void logger_log_formatted_string( const char *format, ... );

#endif
