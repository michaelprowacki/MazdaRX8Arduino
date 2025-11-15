#include "logger.h"

char _logger_ser_buf[LOGGER_BUFFLEN];
unsigned int _logger_ser_buf_idx = 0;

const char *pathToFileName( const char *path ) {
    size_t i = 0;
    size_t pos = 0;
    char *p = (char *)path;
    while ( *p ) {
        i++;
        if ( *p == '/' || *p == '\\' ) {
            pos = i;
        }
        p++;
    }
    return path + pos;
}
