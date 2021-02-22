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

#include "info_strings.h"

#define PROMPT " ➜ "

/* function to print string */
FILE *out;
static inline void p(const char *s) { fputs(s, out); }

/* print current dir in fish style */
const int fish_style_dir = 1;

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	out = stdout;

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
				/* XXX: log error */
				return 1;
				break;
		}
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
