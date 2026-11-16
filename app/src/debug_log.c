#include "debug_log.h"

#include <stdarg.h>
#include <time.h>

void debug_log(FILE *stream, const char *format, ...)
{
    char timestamp[32];
    time_t now = time(NULL);
    struct tm local_time;

    if (!localtime_r(&now, &local_time) ||
        strftime(timestamp, sizeof timestamp, "%Y-%m-%d %I:%M:%S %p",
                 &local_time) == 0)
        snprintf(timestamp, sizeof timestamp, "unknown time");

    fprintf(stream, "[%s] ", timestamp);

    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
}
