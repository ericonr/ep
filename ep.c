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

/* function to print string */
FILE *out;
static inline void p(const char *s) { fputs(s, out); }

/* function to log stuff */
enum log_level_value { DEBUG, INFO, WARN, ERROR };
const enum log_level_value log_level = ERROR;
FILE *outerr;
static void e(enum log_level_value l, const char *s, int errcode)
{
	if (l < log_level) {
		return;
	}

	if (errcode) {
		perror(s);
	} else {
		fputs(s, outerr);
		fputs("\n", outerr);
	}
}

/* print current dir in fish style */
const int fish_style_dir = 1;

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
	pthread_t git_handle;
	if (pthread_create(&git_handle, NULL, get_git_branch_name, NULL)) {
		e(WARN, "couldn't create git thread", errno);
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

	/* deal with printing current path */
	char *pwd = getcwd(NULL, 0);
	char *rpwd = NULL;
	if (!pwd || !(rpwd = realpath(pwd, NULL))) {
		/* getcwd or realpath failed */
		p(unknowndir);
	} else {
		/* strip HOME out if possible */
		if (home) {
			size_t l = strlen(home);
			if (!strncmp(home, rpwd, l)) {
				/* found HOME in pwd */
				p("~");
				rpwd += l;
				if (*rpwd) {
					if (fish_style_dir) {
						/* short and sweet way of malloc-ing enough memory */
						char *frpwd = strdup(rpwd);

						/* rpwd starts with a slash */
						const char *c = rpwd, *co;
						char *n = frpwd;
						for (; c; co = c, c = strchr(co+1, '/')) {
							*n++ = '/';
							*n++ = *(c+1);
						}
						/* copy last path completely */
						strcpy(--n, co+1);
						p(frpwd);
					} else {
						p(rpwd);
					}
				}
			} else {
				/* HOME wasn't in rpwd */
				p(rpwd);
			}
		} else {
			/* HOME is unset */
			p(rpwd);
		}
	}

	/* git status */
	pthread_join(git_handle, NULL);
	if (git_branch_name) {
		p(" ");
		p(git_branch_name);
	}

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
