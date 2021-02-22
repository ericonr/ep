/* Érico's prompt
 *
 * I decided I wanted to write my own prompt, so I didn't have to deal with
 * configuring existing ones.
 *
 * Liberties taken:
 *   - will crash with segfault if allocations fail; NULL is special only when meaningful beyond allocation
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
	int chroot = 0;
	int c;
	while((c = getopt(argc, argv, "cj:")) != -1) {
		switch (c) {
			case 'c':
				chroot = 1;
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
	pthread_join(pwd_lang_handle, NULL);
	if (root_lang_task.launched) pthread_join(root_lang_task.handle, NULL);
	print_lang();

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

	p(PROMPT);
}
