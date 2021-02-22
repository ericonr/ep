#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ep.h"
#include "info_strings.h"

/* print current dir in fish style */
const int fish_style_dir = 1;

void print_pwd(const char *home)
{
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
}