#ifndef  LOGGER_H
#define LOGGER_H

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifndef LOG_MAX_ATTRIBUTES
#define LOG_MAX_ATTRIBUTES 8
#endif

#ifndef LOG_MAX_CUSTOM_CALLBACKS
#define LOG_MAX_CUSTOM_CALLBACKS 8
#endif

#define LOG_MAX_CALLBACKS (LOG_MAX_CUSTOM_CALLBACKS + 2)

typedef enum LogLevel {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL
} LogLevel;

typedef struct LogAttr {
  char *key;
  char *value;
} LogAttr;

typedef struct LogEntry {
  const char *fmt;
  va_list args;

  FILE *fd;
  struct tm *time;
  LogLevel level;
  const char *file;
  int line;

  LogAttr attrs[LOG_MAX_ATTRIBUTES];
} LogEntry;

typedef void (*LogCallbackFn)(LogEntry entry); 

typedef struct LogCallback {
  LogCallbackFn fn;
  LogLevel level;
  FILE *fd;
} LogCallback;

typedef struct LogConfig {
  LogLevel level;
  LogAttr attrs[LOG_MAX_ATTRIBUTES];
  LogCallback callbacks[LOG_MAX_CUSTOM_CALLBACKS];
  char *filename;
  bool quiet;
} LogConfig;

typedef struct Logger {
  LogLevel level;
  LogAttr attrs[LOG_MAX_ATTRIBUTES];
  LogCallback callbacks[LOG_MAX_CALLBACKS];
} Logger;

Logger create_logger(LogConfig config);
Logger create_default_logger();
void logger_add_callback(Logger *logger, LogCallbackFn fn, LogLevel level, FILE *fd);
void logger_write_to_file_callback_fn(LogEntry entry);
void logger_write_to_terminal_callback_fn(LogEntry entry);
void logger_log(Logger logger, LogLevel level, const char *file, int line, const char *fmt, ...);

#define log_debug(logger, ...) logger_log(logger, DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(logger, ...) logger_log(logger, INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(logger, ...) logger_log(logger, WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(logger, ...) logger_log(logger, ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(logger, ...) logger_log(logger, FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif

#ifdef LOG_IMPLEMENTATION

static const char *log_level_to_string[] = {
  "DEBUG", "INFO", "WARN", "ERROR", "FATAL" 
};

#ifdef LOG_USE_COLOR
static const char *log_level_to_color[] = {
  "\x1b[94m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[31m"
};
#endif

Logger create_logger(LogConfig config) {
  Logger logger = {
    .level = config.level
  };

  memcpy(logger.attrs, config.attrs, sizeof(config.attrs));
  
  if (config.filename != NULL) {
    FILE *fd = fopen(config.filename, "a");
    assert(fd != NULL && "Something went wrong opening the file");

    logger_add_callback(&logger, logger_write_to_file_callback_fn, logger.level, fd);
  }

  if (!config.quiet) {
    logger_add_callback(&logger,logger_write_to_terminal_callback_fn, logger.level, stdout);
  }

  memcpy(logger.callbacks + 2, config.callbacks, sizeof(config.callbacks));
  
  return logger;
}

Logger create_default_logger() {
  return (Logger){
    .level = DEBUG,
    .callbacks = {
      {
        .fd = stdout,
        .fn = logger_write_to_terminal_callback_fn,
        .level = DEBUG,
      }
    }
  };
}

void logger_add_callback(Logger *logger,LogCallbackFn fn, LogLevel level, FILE *fd) {
  for (int i = 0; i < LOG_MAX_CALLBACKS; i++)  {
    if (!logger->callbacks[i].fn) {
      logger->callbacks[i] = (LogCallback){
        .fn = fn,
        .level = level,
        .fd = fd
      };

      return;
    }
  }
}

void logger_write_to_file_callback_fn(LogEntry entry) {
  char buf[64];
  buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", entry.time)] = '\0';

  fprintf(entry.fd, "%s %-5s %s:%d ", buf, log_level_to_string[entry.level], entry.file, entry.line);
  vfprintf(entry.fd, entry.fmt, entry.args);
  for (int i  = 0; i < LOG_MAX_ATTRIBUTES; i++) {
    LogAttr attr = entry.attrs[i];
    if (attr.key == NULL || attr.value == NULL) {
      continue;
    }
    
    fprintf(entry.fd, " %s=%s", attr.key, attr.value);
  }
  fprintf(entry.fd, "\n");
  fflush(entry.fd);
}

void logger_write_to_terminal_callback_fn(LogEntry entry) {
  char buf[16];
  buf[strftime(buf, sizeof(buf), "%H:%M:%S", entry.time)] = '\0';

#ifdef LOG_USE_COLOR
  fprintf(
    entry.fd, 
    "%s %s%-5s\x1b[0m \x1b[90m%s:%d\x1b[0m ",
    buf, 
    log_level_to_color[entry.level], 
    log_level_to_string[entry.level],
    entry.file, 
    entry.line
  );
#else
  fprintf(
    entry.fd, 
    "%s %-5s %s:%d ",
    buf, 
    log_level_to_string[entry.level], 
    entry.file, 
    entry.line
  );
#endif

  vfprintf(entry.fd, entry.fmt, entry.args);

  for (int i  = 0; i < LOG_MAX_ATTRIBUTES; i++) {
    LogAttr attr = entry.attrs[i];
    if (attr.key == NULL || attr.value == NULL) {
      continue;
    }

  #ifdef LOG_USE_COLOR
    fprintf(entry.fd, " \x1b[90m%s=%s\x1b[0m", attr.key, attr.value);
  #else
    fprintf(entry.fd, " %s=%s", attr.key, attr.value);
  #endif
  }

  fprintf(entry.fd, "\n");
  fflush(entry.fd);
}

void logger_log(Logger logger, LogLevel level, const char *file, int line, const char *fmt, ...) {
  time_t t = time(NULL);

  LogEntry entry = {
    .fmt = fmt,
    
    .time = localtime(&t),
    .level = level,
    .file = file,
    .line = line
  };

  memcpy(entry.attrs, logger.attrs, sizeof(logger.attrs));

  for (int i = 0; i < LOG_MAX_CALLBACKS; i++) {
    LogCallback callback = logger.callbacks[i];
    if (!callback.fn) {
      continue;
    }

    if (entry.level < callback.level) {
      continue;
    }

    entry.fd = callback.fd;
    va_start(entry.args, fmt);
    callback.fn(entry);
    va_end(entry.args);
  }
}

#endif