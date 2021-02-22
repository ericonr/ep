#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "ep.h"

static char *git_branch_name, *git_status, *git_root;
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

static void read_line_from_command(const char *cmd, struct rlfc_data *d)
{
	FILE *f = popen(cmd, "re");
	if (!f)
		return;

	d->l = getline(&d->line, &d->n, f);
	d->status = pclose(f);

	if (d->l > 0) {
		d->line[d->l-1] = 0;
	}
}

static void *get_git_status(void *);
static void *get_git_root(void *);

void *git_thread(void *arg)
{
	struct threaded_task *root_lang_task = arg;
	/* only knows how to launch this task */
	if (root_lang_task->task != task_launch_root_lang) root_lang_task = NULL;

	struct rlfc_data c = { 0 };
	read_line_from_command("git rev-parse --abbrev-ref HEAD 2>/dev/null", &c);

	if (!c.status) {
		/* if git exits with 0, we are in a repo */

		if (c.l > 0) {
			/* TODO: treat case where it reads HEAD */

			/* line ownserhip goes to outside this function */
			git_branch_name = c.line;
		}
	} else {
		return NULL;
	}

	/* since we are in a repo, read git status;
	 * if we add more stuff to do in repos, launch more threads */

	pthread_t status_handle, root_handle;
	if (pthread_create(&status_handle, NULL, get_git_status, NULL))
		goto status_create_error;
	if (pthread_create(&root_handle, NULL, get_git_root, NULL))
		goto root_create_error;

	void *local_git_root;
	pthread_join(root_handle, &local_git_root);
	if (root_lang_task) {
		root_lang_task->launched = !pthread_create(&root_lang_task->handle, NULL, lang_thread, local_git_root);
	}
root_create_error:
	pthread_join(status_handle, NULL);
status_create_error:
	return NULL;
}

struct statuses {
	size_t c;
	char s[64];
	char format[3];
	char prefix;
};
enum status_index { added, deleted, modified_unstaged, modified_staged, modified_both, untracked, status_index_n };

static void *get_git_status(void *arg)
{
	(void)arg;

	FILE *f = popen("git status --porcelain=v1 -z 2>/dev/null", "re");
	if (!f)
		return NULL;

	struct statuses g[status_index_n] = {
		[added] = { .format = "A ", .prefix = '+' },
		[deleted] = { .format = "D ", .prefix = '-' },
		[modified_unstaged] = { .format = " M", .prefix = '~' },
		[modified_staged] = { .format = "M ", .prefix = '>' },
		[modified_both] = { .format = "MM", .prefix = '~' },
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

	/* cheat to display things closer to a "userful" measure */
	g[added].c += g[modified_staged].c + g[modified_both].c;
	g[modified_unstaged].c += g[modified_both].c;

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

static void *get_git_root(void *arg)
{
	(void)arg;

	struct rlfc_data c = { 0 };
	read_line_from_command("git rev-parse --show-toplevel 2>/dev/null", &c);

	if (c.l > 0)
		git_root = c.line;

	return git_root;
}
