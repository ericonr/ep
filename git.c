#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ep.h"

static char *git_branch_name, *git_status;
void print_git(void)
{
	if (git_branch_name) {
		p(" ");
		p(git_branch_name);

		if (git_status) {
			p(" [");
			p(git_status);
			p("]");
		}
	}
}

struct rlfc_data {
	char *line;
	size_t n;
	ssize_t l;
	int status;
};

static void read_line_from_command(const char *c, struct rlfc_data *d)
{
	FILE *f = popen("git rev-parse --abbrev-ref HEAD 2>/dev/null", "re");
	if (!f)
		return;

	d->l = getline(&d->line, &d->n, f);
	d->status = pclose(f);
}

static char *get_git_status(void);

void *git_thread(void *arg)
{
	struct rlfc_data c = { 0 };
	read_line_from_command("git rev-parse --abbrev-ref HEAD 2>/dev/null", &c);

	if (!c.status) {
		/* if git exits with 0, we are in a repo */

		if (c.l > 0) {
			/* TODO: treat case where it reads HEAD */
			c.line[c.l-1] = 0;
			/* line ownserhip goes to outside this function */
			git_branch_name = c.line;
		}
	} else {
		return NULL;
	}

	/* since we are in a repo, read git status;
	 * if we add more stuff to do in repos, launch more threads */

	get_git_status();

	return NULL;
}

static char *get_git_status(void)
{
	size_t modified = 0;

	FILE *f = popen("git status --porcelain=v1 -z 2>/dev/null", "re");
	if (!f)
		return NULL;

	char *line = NULL;
	size_t n = 0;
	ssize_t l;
	while ((l = getdelim(&line, &n, 0, f)) != -1) {
		modified += l>4 && line[1] == 'M';
	}
	free(line);

	if (!pclose(f)) {
		asprintf(&git_status, "~%zu", modified);
	}

	return git_status;
}
