#include <stdio.h>

#include "ep.h"

char *git_branch_name;

void *get_git_branch_name(void *arg)
{
	FILE *f = popen("git rev-parse --abbrev-ref HEAD 2>/dev/null", "re");
	if (!f) {
		return NULL;
	}

	char *line = NULL;
	size_t n = 0;
	ssize_t l = getline(&line, &n, f);

	if (!pclose(f)) {
		/* if git exits with 0, we are in a repo */

		if (l > 0) {
			/* TODO: treat case where it reads HEAD */
			line[l-1] = 0;
			/* line ownserhip goes to outside this function */
			git_branch_name = line;
		}
	}

	return NULL;
}
