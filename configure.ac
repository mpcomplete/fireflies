# Process this file with autoconf to produce a configure script.
AC_INIT(fireflies, 2.08, mpcomplete@gmail.com)
AC_CONFIG_SRCDIR([src/arrow.cc])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_SUBDIRS([libgfx])

sys=`uname`

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

AC_ARG_ENABLE(screensaver, AC_HELP_STRING([--enable-screensaver],
    [Compile as a Windows screensaver (default=yes)]))

# Checks for SDL (optional)
AC_CHECK_PROG([SDL_CONF], [sdl-config], [yes])

case $sys in
CYGWIN*|cygwin*|MINGW*|mingw*)
    if test "$enable_screensaver" = "no"; then
	OPT_LIBS="-mconsole -mwindows"
	OPT_OBJS="main.o canvas_base.o"
	PROGRAM="fireflies.exe"
	BINDIR="./"
	if test -z "$SDL_CONF"; then
	    AC_MSG_ERROR([need SDL to compile as non-screensaver])
	fi
    else
	SDL_CONF="";
	OPT_LIBS="-lscrnsave -lmingw32 -lgdi32 -mwindows"
	OPT_OBJS="winsaver.o resource.o"
	PROGRAM="fireflies.scr"
	BINDIR='C:\Windows\'
    fi

    CFLAGS="${CFLAGS} -DWIN32"
    GL_LIBS="-lopengl32 -lglu32 -lglew32"
    ;;
*)
# FIXME: I should use a more sophisticated path detection method.
    for try in /usr/lib/xscreensaver /usr/lib/X11/xscreensaver /usr/X11R6/lib/xscreensaver /usr/libexec/xscreensaver; do
	    test -d $try && BINDIR=$try
    done
    for try in /usr/share/xscreensaver/config /usr/share/control-center/screensavers/; do
	    test -d $try && CONFDIR=$try
    done
    for try in /usr/lib/X11/app-defaults/XScreenSaver /etc/X11/app-defaults/XScreenSaver; do
	    test -f $try && RCFILE=$try
    done

    OPT_LIBS="-lX11"
    OPT_OBJS="main.o canvas_base.o"
    PROGRAM="fireflies"

    AC_CHECK_LIB([GL], [glXSwapBuffers],\
	[AC_DEFINE([HAVE_GLX], [1], [Define to compile with GLX support.])
	OPT_OBJS="$OPT_OBJS canvas_glx.o"],\
	[AC_CHECK_LIB([MesaGL], [glXSwapBuffers],\
	    [AC_DEFINE([HAVE_GLX], [1], [Define to compile with GLX support.])
	    OPT_OBJS="$OPT_OBJS canvas_glx.o"])])

    AC_CHECK_LIB([GL], [glBegin], \
	[GL_LIBS="-lGL -lGLU -lGLEW"], \
	[AC_CHECK_LIB([MesaGL], [glBegin], \
	    [GL_LIBS="-lMesaGL -lMesaGLU"], \
	    [AC_MSG_ERROR([cannot find GL libraries])])])

    ;;
esac

if test -n "$SDL_CONF"; then
    AC_DEFINE([HAVE_SDL], [1], [Define to compile with SDL support.])
    SDL_LIBS="\`sdl-config --libs\`"
    SDL_CFLAGS="\`sdl-config --cflags\`"
    OPT_OBJS="$OPT_OBJS canvas_sdl.o"
fi

AC_ARG_WITH(bindir, AC_HELP_STRING([--with-bindir],
    [Where to put the binary (default=/usr/lib/xscreensaver)]),
    BINDIR=$withval)
AC_ARG_WITH(confdir, AC_HELP_STRING([--with-confdir],
    [Where to put the xscreensaver config (default=/usr/share/xscreensaver/config)]),
    CONFDIR=$withval)

AC_SUBST(BINDIR)
AC_SUBST(CONFDIR)
AC_SUBST(RCFILE)
AC_SUBST(OPT_LIBS)
AC_SUBST(OPT_OBJS)
AC_SUBST(PROGRAM)

AC_SUBST(SDL_LIBS)
AC_SUBST(SDL_CFLAGS)

AC_SUBST(GL_LIBS)

AC_OUTPUT(Make.include installit fireflies.spec)
chmod +x installit
