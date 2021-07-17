/* Érico's prompt
 *
 * I decided I wanted to write my own prompt, so I didn't have to deal with
 * configuring existing ones.
 *
 * Liberties taken:
 *   - assumes stdio is reasonably buffered, so multiple fputs calls aren't expensive
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>
#include <pthread.h>

#include "colors.h"
#include "info_strings.h"
#include "ep.h"

#define PROMPT " ➜ "
#define JOBS " ✦"

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
	int git_launched = 1;
	pthread_t git_handle;
	if (pthread_create(&git_handle, NULL, git_thread, &root_lang_task)) {
		e(INFO, "couldn't create git thread", errno);
		git_launched = 0;
	}

	int pwd_lang_launched = 1;
	pthread_t pwd_lang_handle;
	if (pthread_create(&pwd_lang_handle, NULL, lang_thread, NULL)) {
		e(INFO, "couldn't create lang thread", errno);
		pwd_lang_launched = 0;
	}

	if (chroot)
		p("[chroot] ");

	/* XXX: treat allocation failures as variables not existing in environment */
	const char *home = getenv("HOME");
	char *pwd = getenv("PWD");
	#define cond_strdup(s) s = (s ? strdup(s) : s)
	cond_strdup(home);
	cond_strdup(pwd);
	#undef cond_strdup

	/* show we are on a different machine */
	print_ssh();

	use_color(bcyan, print_pwd(home, pwd));

	/* git status */
	void *git_info = 0;
	if (git_launched) {
		pthread_join(git_handle, &git_info);
		if (git_info)
			print_git(git_info);
	}

	/* programming languages */
	uint64_t pwd_langs = 0, root_langs = 0;
	void *tmp_mask;
	/* if thread returned NULL, assume no language */
	#define read_mask() (tmp_mask ? *(uint64_t *)tmp_mask : 0)
	#define store_and_discard_mask(dst) do{dst = read_mask(); free(tmp_mask);}while(0)
	if (pwd_lang_launched) {
		pthread_join(pwd_lang_handle, &tmp_mask);
		store_and_discard_mask(pwd_langs);
	}
	/* safe to check for launched here because we joined git_handle above */
	if (root_lang_task.launched) {
		pthread_join(root_lang_task.handle, &tmp_mask);
		store_and_discard_mask(root_langs);
	}
	#undef read_mask
	#undef store_and_discard_mask
	print_lang(pwd_langs | root_langs);

	/* we are done with git_info after langs is finished */
	free_git(git_info);

	/* print currently active shell jobs */
	if (shell_jobs) {
		int n = atoi(shell_jobs);
		if (n >= 1) {
			p(JOBS);
			if (n > 1) {
				/* jobs emoji is wide */
				p(" ");
				use_color(blue, p(shell_jobs));
			}
		}
	}

	/* print previous command's duration and exit status */
	if (command_duration >= 5000) {
		const int m = 60;
		const int seconds_in_hour = m*m;
		command_duration /= 1000;

		long long hours = command_duration / seconds_in_hour;
		long long minutes = command_duration / m - hours * m;
		command_duration -= hours * seconds_in_hour + minutes * m;

		char dur[256];
		if (hours) {
			snprintf(dur, sizeof(dur), " %lldh%lldm%llds", hours, minutes, command_duration);
		} else if (minutes) {
			snprintf(dur, sizeof(dur), " %lldm%llds", minutes, command_duration);
		} else {
			snprintf(dur, sizeof(dur), " %llds", command_duration);
		}
		use_color(green, p(dur));
	}
	/* 127 means command not found, that prints a big enough message already */
	if (exit_status && exit_status != 127) {
		char ex[256];
		snprintf(ex, sizeof(ex), " [%d]", exit_status);
		use_color(green, p(ex));
	}

	p(PROMPT);
}
