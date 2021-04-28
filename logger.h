#ifndef VKENV_LOGGER_H
#define VKENV_LOGGER_H

typedef enum
{
  LOG_NONE,
  LOG_ERROR,
  LOG_INFO
} vkenv_LogLevel;

#define logError(TAG, ...) vkenv_log(LOG_ERROR, TAG, __VA_ARGS__)
#define logInfo(TAG, ...) vkenv_log(LOG_INFO, TAG, __VA_ARGS__)

void vkenv_setLogLevel(vkenv_LogLevel log_level);
void vkenv_log(vkenv_LogLevel log_level, const char *tag, const char *format, ...);

#endif // VKENV_LOGGER_H