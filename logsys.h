#ifndef TETRIS_LOGSYS
#define TETRIS_LOGSYS

#include <stdarg.h>

// Log levels
enum {
	ALL,     // Everything
	TRACE,   // Excessive debugging
	DEBUG,   // Verbose information
	INFO,    // Information
	WARNING, // Potential problem
	ERROR,   // Something went wrong
	FATAL    // Crash
};

// Opens a log file to write to
void log_open(const char *filename);

// Closes log file if one is open
void log_close();

// Log simple message
//void log_msg(int level, const char *msg);

// Log with formatting, syntax like fprintf
void log_msgf(int level, const char *format, ...);

#endif
