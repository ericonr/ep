#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

#include "ep.h"
#include "info_strings.h"

/* print current dir in fish style */
const int fish_style_dir = 1;

static inline int file_match(const struct stat *s1, const struct stat *s2)
{
	return s1->st_dev == s2->st_dev && s1->st_ino == s2->st_ino;
}

void print_pwd(const char *home, char *envpwd)
{
	struct stat st_pwd, st_envpwd, st_home;
	char pwd[PATH_MAX];
	char *mrpwd = getcwd(pwd, sizeof pwd);
	/* const alias to avoid accidentally modifying m(utable)rpwd.
	 * remember to ALWAYS change both variables */
	const char *rpwd = mrpwd;
	if (!rpwd) {
		/* getcwd failed */
		p(unknowndir);
	} else {
		/* if envpwd exists and is valid (starts with '/' and matches current dir),
		 * replace pwd with it */
		if (envpwd && envpwd[0] == '/') {
			/* if we can't stat current dir, bail */
			if (stat(".", &st_pwd)) {
				p(unknowndir);
				return;
			}
			if (!stat(envpwd, &st_envpwd) && file_match(&st_pwd, &st_envpwd)) {
				rpwd = mrpwd = envpwd;
			}
		} else {
			/* invalidate envpwd so it can be used to determine if st_pwd was initialized */
			envpwd = NULL;
		}
		/* strip HOME out if possible */
		if (home) {
			size_t l = strlen(home);
			if (!strncmp(home, rpwd, l) && (rpwd[l] == 0 || rpwd[l] == '/')) {
				/* found HOME in pwd */
				p("~");
				/* advance both pwd pointers */
				rpwd = (mrpwd += l);
				/* check if we are in HOME.
				 * if yes, printing '~' is all we want */
				if (
						/* pwd is only HOME */
						rpwd[0] == 0 ||
						/* current dir is HOME anyway */
						!stat(home, &st_home) &&
						(envpwd || !stat(".", &st_pwd)) && /* only stat(".") if it hasn't happened before */
						file_match(&st_home, &st_pwd))
					return;
			}

			/* starting from here, rpwd always starts with '/'.
			 * it can be a path relative to HOME or an absolute path */

			if (fish_style_dir) {
				char *saveptr;
				const char *tok, *oldtok = NULL;
				while ((tok = strtok_r(mrpwd, "/", &saveptr))) {
					mrpwd = NULL;
					if (!strcmp(tok, "..")) {
						p("/..");
					} else {
						char str[] = {'/', tok[0], 0};
						p(str);
					}

					oldtok = tok;
				}
				/* print the last token in full */
				if (oldtok)
					p(oldtok+1);
				/* if no token was found, we are in root */
				if (mrpwd) {
					p("/");
				}
			} else {
				p(rpwd);
			}
		} else {
			/* HOME is unset */
			p(rpwd);
		}
	}
}
