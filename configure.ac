AC_INIT([Asteria], [0.1], [lh_mouse@126.com], [asteria], [https://github.com/lhmouse/asteria])
AC_CONFIG_SRCDIR([asteria/src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AC_LANG([C++])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])
LT_INIT([disable-static])

AC_ARG_ENABLE([debug-logs], AS_HELP_STRING([--enable-debug-logs], [output debug logs to stderr]))
AM_CONDITIONAL([enable_debug_logs], [test "${enable_debug_logs}" == "yes"])
AM_COND_IF([enable_debug_logs], [
  AC_DEFINE([ASTERIA_ENABLE_DEBUG_LOGS], [1], [Define to 1 to enable debug logs.])
])

AC_ARG_ENABLE([debug-checks], AS_HELP_STRING([--enable-debug-checks], [enable assertions]))
AM_CONDITIONAL([enable_debug_checks], [test "${enable_debug_checks}" == "yes"])
AM_COND_IF([enable_debug_checks], [
  AC_DEFINE([_GLIBCXX_DEBUG], [1], [Define to 1 to enable debug checks of libstdc++.])
  AC_DEFINE([_LIBCPP_DEBUG], [1], [Define to 1 to enable debug checks of libc++.])
  AC_DEFINE([_DEBUG], [1], [Define to 1 to enable debug checks of MSVC standard library.])
])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
