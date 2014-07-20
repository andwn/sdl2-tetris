#include "logsys.h"

#include <stdio.h>

int logLevel = TRACE;

FILE *logfile;

char *levelStr[7] = {
	"[ALL]",
	"[TRACE]",
	"[DEBUG]",
	"[INFO]",
	"[WARNING]",
	"[ERROR]",
	"[FATAL]"
};

void log_open(const char *filename) {
	logfile = fopen(filename, "w");
	if(logfile == NULL) {
		log_msgf(ERROR, "Unable to create log \"%s\".\n", filename);
	}
}

void log_close() {
	if(logfile == NULL) return;
	log_msgf(DEBUG, "Closing log file.\n");
	fclose(logfile);
}
/*
void log_msg(int level, const char *msg) {
	if(logfile == NULL || logLevel > level) return;
	fprintf(logfile, "%s %s", levelStr[level], msg);
}
*/
void log_msgf(int level, const char *format, ...) {
	if(logfile == NULL || logLevel > level) return;
	fprintf(logfile, "%s ", levelStr[level]);
	va_list args;
	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);
}
