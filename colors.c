#ifndef NOCOLOR

#include <assert.h>
#include <stdio.h>

#include "colors.h"

void fg_color(enum colors c)
{
	char s[16] = "\x1b[";
	char *counter = s+2;
	int color_offset;

	/* foreground normal colors go from 30 to 37
	 * and bright ones go from 90 to 97 */
	if (c < bblack) {
		*counter++ = '3';
		color_offset = black;
	} else {
		*counter++ = '9';
		color_offset = bblack;
	}
	*counter++ = '0' + (char)(c - color_offset);

	/* terminator char */
	*counter++ = 'm';

	fwrite(s, 1, counter-s, stdout);
}

#endif
