#ifndef EP_H
#define EP_H

#include <stdio.h>
#include <pthread.h>
#include <signal.h>

enum task_identity {
	task_launch_root_lang,
};

struct threaded_task {
	pthread_t handle;
	sig_atomic_t launched;
	enum task_identity task;
};

/* from out.c */
extern FILE *out, *outerr;
enum log_level_value { DEBUG, INFO, WARN, ERROR };
extern const enum log_level_value log_level;
void p(const char *);
void e(enum log_level_value, const char *, int);

/* from path.c */
extern const int fish_style_dir;
void print_pwd(const char *);

/* from git.c */
void *git_thread(void *);
void print_git(void);

/* from lang.c */
void *lang_thread(void *);
void print_lang(void);

#endif
