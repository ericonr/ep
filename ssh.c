#include <stdlib.h>
#include <sys/utsname.h>

#include "ep.h"

void print_ssh(void)
{
	const char *sshcon, *sshtty;

	/* expect ssh to have set these variables - we don't need to prompt if not using a tty.
	 * we do the conditional like this because getenv is not guaranteed to be re-entrant */
	if ((sshcon = getenv("SSH_CONNECTION")) && *sshcon &&
			(sshtty = getenv("SSH_TTY")) && *sshtty) {
		struct utsname u;
		/* don't print anything if uname fails or nodename is empty */
		if (uname(&u) || !u.nodename[0])
			return;

		p("(");
		p(u.nodename);
		p(") ");
	}
}
