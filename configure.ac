AC_INIT([asteria], [0.1], [lh_mouse@126.com], [asteria], [https://github.com/lhmouse/asteria])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AC_LANG([C++])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])
LT_INIT([disable-static])

AC_ARG_ENABLE([werror], AS_HELP_STRING([--disable-werror], [disable -Werror]))
AM_CONDITIONAL([ENABLE_WERROR], [test "${enable_werror}" != "no"])

AC_ARG_ENABLE([debug-logs], AS_HELP_STRING([--enable-debug-logs], [output debug logs to stderr]))
AM_CONDITIONAL([ENABLE_DEBUG_LOGS], [test "${enable_debug_logs}" == "yes"])
AM_COND_IF([ENABLE_DEBUG_LOGS], [
  AC_DEFINE([ASTERIA_ENABLE_DEBUG_LOGS], [1], [Define to 1 to enable debug logs.])
])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
