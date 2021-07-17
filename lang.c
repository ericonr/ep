#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <fnmatch.h>

#define NOCOLOR
#include "colors.h"
#include "ep.h"

enum lang_index {
	c_lang,
	go_lang,
	python_lang,
	rust_lang,
	/* misc */
	build_system_lang,
	kicad_lang,
	xbps_src_lang,
	lang_index_n
};

struct lang_check {
	int (*check)(const char *, unsigned char);
	char display[8];
};

#define check_function(lang) lang##_lang_check
#define check_function_decl(lang) static int check_function(lang) (const char *s, unsigned char t)
#define struct_item(lang, d) [lang##_lang] = { .check = check_function(lang), .display = d }

check_function_decl(c);
check_function_decl(go);
check_function_decl(python);
check_function_decl(rust);
check_function_decl(build_system);
check_function_decl(kicad);
check_function_decl(xbps_src);

const struct lang_check l[] = {
	struct_item(c, " ℂ"),
	struct_item(go, " 🐹"),
	struct_item(python, " 🐍"),
	struct_item(rust, " 🦀"),
	struct_item(build_system, " 📦"),
	struct_item(kicad, " ⚡"),
	struct_item(xbps_src, " 😨"),
};

#undef struct_item

/* bitmap of 1<<lang_index */
void print_lang(uint64_t mask) {
	/* only change color if we're going to print something */
	if (mask) fg_color(red);
	for (int i = 0; i < lang_index_n; i++) {
		if (mask & (1 << i)) {
			p(l[i].display);
		}
	}
	if (mask) reset_color();
}

void *lang_thread(void *arg)
{
	/* scan current dir if arg is NULL */
	char *path = arg ? arg : ".";
	uint64_t *mask = calloc(1, sizeof(*mask));
	if (!mask)
		return NULL;

	DIR *d = opendir(path);
	if (!d) {
		/* mask will have been successfully allocated */
		free(mask);
		return NULL;
	}

	struct dirent *item;
	while ((item = readdir(d))) {
		for (int i = 0; i < lang_index_n; i++) {
			/* skip if language has already been detected */
			if (*mask & (1 << i))
				continue;

			*mask |= l[i].check(item->d_name, item->d_type) << i;
		}
	}

	closedir(d);
	return mask;
}

static inline int isfile(unsigned char t) { return t & (DT_REG | DT_LNK); }
static inline int isdir(unsigned char t) { return t & (DT_DIR | DT_LNK); }

check_function_decl(c)
{
	return isfile(t) &&
		(!fnmatch("*.c", s, 0) || !fnmatch("*.h", s, 0) ||
		 !fnmatch("*.cpp", s, 0) || !fnmatch("*.cc", s, 0));
}

check_function_decl(go)
{
	return isfile(t) &&
		(!strcmp("go.mod", s) || !strcmp("go.mod", s) ||
		 !fnmatch("*.go", s, 0));
}

check_function_decl(python)
{
	return isfile(t) && !fnmatch("*.py", s, 0);
}

check_function_decl(rust)
{
	return isfile(t) &&
		(!fnmatch("*.rs", s, 0) || !fnmatch("Cargo.*", s, 0));
}

check_function_decl(build_system)
{
	return isfile(t) &&
		(!strcmp("CMakeLists.txt", s) || !strcmp("meson.build", s) ||
		 !strcmp("configure.ac", s) || !strcmp("Makefile.am", s) || !strcmp("Makefile.in", s));
}

check_function_decl(kicad)
{
	return isfile(t) &&
		(!fnmatch("*-lib-table", s, 0) || !fnmatch("*.kicad_pcb", s, 0) ||
		 !fnmatch("*.drl", s, 0) || !fnmatch("*.gbr", s, 0) ||
		 !fnmatch("*.sch", s, 0)) ||
		isdir(t) && !fnmatch(".pretty", s, 0);
}

check_function_decl(xbps_src)
{
	return isfile(t) && !strcmp("xbps-src", s);
}

#undef check_function
#undef check_function_decl
