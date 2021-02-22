/* Érico's prompt
 *
 * I decided I wanted to write my own prompt, so I didn't have to deal with
 * configuring existing ones.
 *
 * Liberties taken:
 *   - might crash with segfault if some allocations fail; NULL is special only when meaningful beyond allocation
 *   - assumes stdio is reasonably buffered, so multiple fputs calls aren't expensive
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>
#include <pthread.h>

#include "info_strings.h"
#include "ep.h"

#define PROMPT " ➜ "

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	out = stdout;
	outerr = stderr;

	char *shell_jobs = NULL;
	long long command_duration = 0;
	int chroot = 0, exit_status = 0;
	int c;
	while((c = getopt(argc, argv, "cd:e:j:")) != -1) {
		switch (c) {
			case 'c':
				chroot = 1;
				break;
			case 'd':
				command_duration = strtoll(optarg, NULL, 10);
				break;
			case 'e':
				/* won't print anything if optarg can't be parsed into a number */
				exit_status = atoi(optarg);
				break;
			case 'j':
				shell_jobs = optarg;
				break;
			default:
				e(ERROR, "invalid command line option", 0);
				return 1;
				break;
		}
	}

	/* start threads for long(er) running steps */
	struct threaded_task root_lang_task = { .task = task_launch_root_lang };
	pthread_t git_handle;
	if (pthread_create(&git_handle, NULL, git_thread, &root_lang_task)) {
		e(ERROR, "couldn't create git thread", errno);
		return 1;
	}

	pthread_t pwd_lang_handle;
	if (pthread_create(&pwd_lang_handle, NULL, lang_thread, NULL)) {
		e(ERROR, "couldn't create lang thread", errno);
		return 1;
	}

	if (chroot)
		p("[chroot] ");

	const char *home = getenv("HOME");
	const char *hostname = getenv("HOSTNAME");

	/* show we are on a different machine */
	if (hostname) {
		p(hostname);
		p(" ");
	}

	print_pwd(home);

	/* git status */
	pthread_join(git_handle, NULL);
	print_git();

	/* programming languages */
	uint64_t pwd_langs = 0, root_langs = 0;
	void *tmp_mask;
	pthread_join(pwd_lang_handle, &tmp_mask);
	/* if thread returned NULL, assume no language */
	#define read_mask() (tmp_mask ? *(uint64_t *)tmp_mask : 0)
	pwd_langs = read_mask();
	/* safe to check for launched here because we joined git_handle above */
	if (root_lang_task.launched) {
		pthread_join(root_lang_task.handle, &tmp_mask);
		root_langs = read_mask();
	}
	#undef read_mask
	print_lang(pwd_langs | root_langs);

	/* print currently active shell jobs */
	if (shell_jobs) {
		int n = atoi(shell_jobs);
		if (n >= 1) {
			p(" ✦");
			if (n > 1) {
				/* jobs emoji is wide */
				p(" ");
				p(shell_jobs);
			}
		}
	}

	/* print previous command's duration and exit status */
	if (command_duration >= 5000) {
		char dur[256];
		/* TODO: pretty print with hours and minutes? */
		snprintf(dur, sizeof(dur), " %llds", command_duration/1000);
		p(dur);
	}
	/* 127 means command not found, that prints a big enough message already */
	if (exit_status && exit_status != 127) {
		char ex[256];
		snprintf(ex, sizeof(ex), " [%d]", exit_status);
		p(ex);
	}

	p(PROMPT);
}
