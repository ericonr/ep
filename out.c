#include <stdio.h>

#include "ep.h"

/* function to print string */
FILE *out;
void p(const char *s) { fputs(s, out); }

/* function to log stuff */
const enum log_level_value log_level = ERROR;
FILE *outerr;
void e(enum log_level_value l, const char *s, int errcode)
{
	if (l < log_level) {
		return;
	}

	if (errcode) {
		perror(s);
	} else {
		fputs(s, outerr);
		fputs("\n", outerr);
	}
}
