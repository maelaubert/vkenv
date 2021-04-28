#include "logger.h"

#if defined(ANDROID) || defined(__ANDROID__)
// If built with Android NDK tools, use the Android logging system
#include <android/log.h>
void log_error(const char *tag, const char *format, va_list args_list) { __android_log_vprint(ANDROID_LOG_ERROR, tag, format, args_list); }

void log_info(const char *tag, const char *format, va_list args_list) { __android_log_vprint(ANDROID_LOG_INFO, tag, format, args_list); }
#else
#include <stdio.h>
void log_error(const char *tag, const char *format, va_list args_list)
{
  fprintf(stdout, "\033[0;31m");
  fprintf(stdout, "[vkenv:%s] ", tag);
  fprintf(stdout, "\x1b[0m");
  vfprintf(stdout, format, args_list);
  fprintf(stdout, "\n");
  fflush(stdout);
}

void log_info(const char *tag, const char *format, va_list args_list)
{
  fprintf(stdout, "\x1b[32m");
  fprintf(stdout, "[vkenv:%s] ", tag);
  fprintf(stdout, "\x1b[0m");
  vfprintf(stdout, format, args_list);
  fprintf(stdout, "\n");
  fflush(stdout);
}
#endif

static vkenv_LogLevel max_log_level = LOG_INFO;
void vkenv_setLogLevel(vkenv_LogLevel log_level) { max_log_level = log_level; }

void vkenv_log(vkenv_LogLevel log_level, const char *tag, const char *format, ...)
{
  if (log_level <= max_log_level)
  {
    // prepare argument list
    va_list args_list;
    va_start(args_list, format);
    switch (log_level)
    {
    case LOG_ERROR:
      log_error(tag, format, args_list);
      break;
    case LOG_INFO:
      log_info(tag, format, args_list);
      break;
    default:
      break;
    }
    va_end(args_list);
  }
}
