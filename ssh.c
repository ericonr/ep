#include <stdlib.h>
#include <sys/utsname.h>

#include "ep.h"

void print_ssh(void)
{
	/* expect ssh to have set these variables - we don't need to prompt if not using a tty */
	const char *sshcon = getenv("SSH_CONNECTION"), *sshtty = getenv("SSH_TTY");

	if (sshcon && *sshcon && sshtty && *sshtty) {
		struct utsname u;
		/* don't print anything if uname fails or nodename is empty */
		if (uname(&u) || !u.nodename[0])
			return;

		p("(");
		p(u.nodename);
		p(") ");
	}
}
