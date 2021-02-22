#ifndef EP_H
#define EP_H

#include <stdio.h>

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

#endif
