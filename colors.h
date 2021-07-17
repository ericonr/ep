#ifndef COLORS_H
#define COLORS_H

/* uses the same ordering as the color codes themselves */
enum colors {
	black,
	red,
	green,
	yellow,
	blue,
	magenta,
	cyan,
	white,

	/* bright colors */
	bblack,
	bred,
	bgreen,
	byellow,
	bblue,
	bmagenta,
	bcyan,
	bwhite,
};

#ifdef NOCOLOR
# define fg_color(c) do{}while(0)
#else
void fg_color(enum colors);
#endif

#define reset_color() fg_color(white)
#define use_color(c, x) do{ fg_color(c); x; reset_color(); }while(0);

#endif
