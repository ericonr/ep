#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "colors.h"
#include "ep.h"

struct git_info {
	char *git_branch_name, *git_status, *git_root;
};

void print_git(void *arg)
{
	struct git_info *git_info = arg;

	if (git_info->git_branch_name) {
		p(" ");
		use_color(magenta, p(git_info->git_branch_name));

		fg_color(blue);
		if (git_info->git_status) {
			p(" [");
			p(git_info->git_status);
			p("]");
		}
		reset_color();
	}
}

void free_git(void *arg)
{
	struct git_info *git_info = arg;

	if (git_info) {
		free(git_info->git_branch_name);
		free(git_info->git_status);
		free(git_info->git_root);
		free(git_info);
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

	struct git_info *git_info;
	if (!c.status && c.l > 0) {
		/* if git exits with 0 and outputs a string with content, we are in a repo */
		/* TODO: treat case where it reads HEAD */

		git_info = calloc(1, sizeof *git_info);
		/* if we can't even allocate that, bail out */
		if (!git_info)
			return NULL;
		/* line ownserhip goes to outside this function */
		git_info->git_branch_name = c.line;
	} else {
		return NULL;
	}

	/* since we are in a repo, read git status;
	 * if we add more stuff to do in repos, launch more threads */

	pthread_t status_handle, root_handle;
	if (pthread_create(&status_handle, thread_a, get_git_status, git_info))
		goto status_create_error;
	if (pthread_create(&root_handle, thread_a, get_git_root, git_info))
		goto root_create_error;

	pthread_join(root_handle, NULL);
	if (root_lang_task && git_info->git_root) {
		root_lang_task->launched = !pthread_create(&root_lang_task->handle, thread_a, lang_thread, git_info->git_root);
	}
root_create_error:
	pthread_join(status_handle, NULL);
status_create_error:
	return git_info;
}

static void *get_git_status(void *arg)
{
	struct git_info *git_info = arg;

	FILE *f = popen("timeout 0.8s git status --porcelain=v1 -z 2>/dev/null", "re");
	if (!f)
		return NULL;

	typedef uint_fast16_t t_format;
	#define FORMAT(s) (((t_format)(unsigned char)s[0]) | (((t_format)(unsigned char)s[1]) << CHAR_BIT))
	enum status_index { added, deleted, modified_unstaged, modified_staged, modified_both, untracked, status_index_n };
	struct statuses {
		size_t c;
		char s[32];
		t_format format;
		char prefix;
	} g[status_index_n] = {
		[added] = { .format = FORMAT("A "), .prefix = '+' },
		[deleted] = { .format = FORMAT("D "), .prefix = '-' },
		[modified_unstaged] = { .format = FORMAT(" M"), .prefix = '~' },
		[modified_staged] = { .format = FORMAT("M "), .prefix = '>' },
		[modified_both] = { .format = FORMAT("MM"), .prefix = '~' },
		[untracked] = { .format = FORMAT("??"), .prefix = '+' },
	};

	char *line = NULL;
	size_t n = 0;
	ssize_t l;
	while ((l = getdelim(&line, &n, 0, f)) != -1) {
		if (l > 4) {
			for (int i = 0; i < status_index_n; i++) {
				g[i].c += FORMAT(line) == g[i].format;
			}
		}
	}
	free(line);
	#undef FORMAT

	/* cheat to display things closer to a "userful" measure */
	g[added].c += g[modified_staged].c + g[modified_both].c;
	g[modified_unstaged].c += g[modified_both].c;

	if (!pclose(f)) {
		for (int i = 0; i < status_index_n; i++) {
			if (g[i].c) {
				snprintf(g[i].s, sizeof(g[i].s), "%c%zu", g[i].prefix, g[i].c);
			}
		}
		if (asprintf(&git_info->git_status, "%s%s%s", g[added].s, g[deleted].s, g[modified_unstaged].s) == 0) {
			/* if string is empty, don't display it */
			free(git_info->git_status);
			git_info->git_status = NULL;
		}
	}

	return git_info->git_status;
}

static void *get_git_root(void *arg)
{
	struct git_info *git_info = arg;

	struct rlfc_data c = { 0 };
	read_line_from_command("git rev-parse --show-toplevel 2>/dev/null", &c);

	if (c.l > 0)
		git_info->git_root = c.line;

	return git_info->git_root;
}
