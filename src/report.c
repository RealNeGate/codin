#include <stdarg.h> // va_list, va_start, va_end
#include <stdlib.h> // abort
#include <stdio.h>  // fprintf, fputc

#include "report.h"

noreturn void report_assertion(const char *expression, const char *file, int line) {
	fprintf(stderr, "Assertion failed: %s:%d: %s\n", file, line, expression);
	abort();
}

void report_error(const Source *source, const Location *location, const char *fmt, ...) {
	fprintf(stderr, "%.*s:%d:%d: ERROR ",
		CAST(int,         source->name.size),
		CAST(const char*, source->name.data),
		location->line,
		location->column);
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fputc('\n', stderr);
}

void report_warning(const Source *source, const Location *location, const char *fmt, ...) {
	fprintf(stderr, "%.*s:%d:%d: WARNING ",
		CAST(int,         source->name.size),
		CAST(const char*, source->name.data),
		location->line,
		location->column);
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fputc('\n', stderr);
}