#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <fnmatch.h>

#include "ep.h"

enum lang_index {
	c_lang,
	//cpp_lang,
	//python_lang,
	//go_lang,
	lang_index_n
};
static int c_lang_check(const char *, unsigned char);

struct lang_check {
	int (*check)(const char *, unsigned char);
	char display[8];
};

const struct lang_check l[] = {
	[c_lang] = { .check = c_lang_check, .display = " C" },
};

/* bitmap of 1<<lang_index */
void print_lang(uint64_t mask) {
	for (int i = 0; i < lang_index_n; i++) {
		if (mask & (1 << i)) {
			p(l[i].display);
		}
	}
}

void *lang_thread(void *arg)
{
	/* scan current dir if arg is NULL */
	char *path = arg ? arg : ".";
	uint64_t *mask = calloc(1, sizeof(mask));
	if (!mask)
		return NULL;

	DIR *d = opendir(path);
	if (!d)
		return NULL;

	struct dirent *item;
	while ((item = readdir(d))) {
		for (int i = 0; i < lang_index_n; i++) {
			/* skip if language has already been detected */
			if (*mask & (1 << i))
				continue;

			*mask |= l[i].check(item->d_name, item->d_type) << i;
		}
	}

	return mask;
}

static inline int isfile(unsigned char t) { return t & (DT_REG | DT_LNK); }

static int c_lang_check(const char *s, unsigned char t)
{
	return isfile(t) && !fnmatch("*.c", s, 0);
}
