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

struct statuses {
	size_t c;
	char s[64];
	char format[3];
	char prefix;
};
enum status_index { added, deleted, modified_unstaged, modified_staged, untracked, status_index_n };

static char *get_git_status(void)
{
	FILE *f = popen("git status --porcelain=v1 -z 2>/dev/null", "re");
	if (!f)
		return NULL;

	struct statuses g[status_index_n] = {
		[added] = { .format = "A ", .prefix = '+' },
		[deleted] = { .format = "D ", .prefix = '-' },
		[modified_unstaged] = { .format = " M", .prefix = '~' },
		[modified_staged] = { .format = "M ", .prefix = '>' },
		[untracked] = { .format = "??", .prefix = '+' },
	};

	char *line = NULL;
	size_t n = 0;
	ssize_t l;
	while ((l = getdelim(&line, &n, 0, f)) != -1) {
		if (l > 4) {
			for (int i = 0; i < status_index_n; i++) {
				g[i].c += !strncmp(g[i].format, line, 2);
			}
		}
	}
	free(line);

	/* cheat to display added+modified_staged in one place */
	g[added].c += g[modified_staged].c;

	if (!pclose(f)) {
		for (int i = 0; i < status_index_n; i++) {
			if (g[i].c) {
				snprintf(g[i].s, sizeof(g[i].s), "%c%zu", g[i].prefix, g[i].c);
			}
		}
		if (asprintf(&git_status, "%s%s%s", g[added].s, g[deleted].s, g[modified_unstaged].s) == 0) {
			/* if string is empty, don't display it */
			free(git_status);
			git_status = NULL;
		}
	}

	return git_status;
}
