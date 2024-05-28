#define LOG_USE_COLOR
#define LOG_IMPLEMENTATION
#include "../logger.h"

void default_logger(void) {
  Logger logger = create_default_logger();

  log_debug(logger, "Debug log");
  log_info(logger, "Info log");
  log_warn(logger, "Warn log");
  log_error(logger, "Error log");
  log_fatal(logger, "Fatal log");
}

void logger_from_config(void) {
  LogConfig config = {
    .level = DEBUG,
    
    // it will write the logs into this file
    .filename = "logs.log",

    // it will write the logs to stdout
    .quiet = false,

    .callbacks = {
      {
        // it will write the error logs to stderr
        .level = ERROR,
        .fd = stderr,
        .fn = logger_write_to_terminal_callback_fn,
      }
    },

    // these attributes will be printed in all logs
    .attrs = {
      {
        .key = "machine",
        .value = "HOSTNAME-01"
      }
    }
  };
  Logger logger = create_logger(config);

  // you can add a new callback after the logger creation.
  // it will write the error logs into this file
  logger_add_callback(&logger, logger_write_to_file_callback_fn, ERROR, "errors.log", NULL);

  log_debug(logger, "Debug log");
  log_info(logger, "Info log");
  log_warn(logger, "Warn log");
  log_error(logger, "Error log");
  log_fatal(logger, "Fatal log");
}

int main(void) {
  default_logger();

  return 0;
}