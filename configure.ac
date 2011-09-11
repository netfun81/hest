AC_INIT(hest, 0.2.0)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CHECK_HEADERS([stdlib.h string.h unistd.h])
AC_CHECK_FUNCS([memset])

AC_PATH_X
AC_CHECK_LIB([X11], [XOpenDisplay])
AC_CHECK_LIB([Xinerama], [XineramaQueryScreens])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_OUTPUT(Makefile hest.1)
