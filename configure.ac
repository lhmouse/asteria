AC_INIT([Asteria], [0.1], [lh_mouse@126.com], [asteria], [https://github.com/lhmouse/asteria])
AC_CONFIG_SRCDIR([asteria/src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AC_LANG([C++])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])
LT_INIT([disable-static])

## Check for required libraries
AC_CHECK_LIB([rt], [clock_gettime], [], [AC_MSG_ERROR(clock and time library not found)])
AC_CHECK_LIB([pcre2-8], [pcre2_compile_8], [], [AC_MSG_ERROR(PCRE2 library not found)])

## Set up optional features
AC_ARG_ENABLE([debug-checks], AS_HELP_STRING([--enable-debug-checks], [enable assertions]))
AM_CONDITIONAL([enable_debug_checks], [test "${enable_debug_checks}" == "yes"])
AM_COND_IF([enable_debug_checks], [
  AC_DEFINE([_GLIBCXX_DEBUG], [1], [Define to 1 to enable debug checks of libstdc++.])
  AC_DEFINE([_LIBCPP_DEBUG], [1], [Define to 1 to enable debug checks of libc++.])
  AC_DEFINE([_DEBUG], [1], [Define to 1 to enable debug checks of MSVC standard library.])
])

AC_ARG_ENABLE([sanitizer], AS_HELP_STRING([--enable-sanitizer=address|thread],
  [enable sanitizer (address sanitizer and thread sanitizer cannot be enabled at the same time)]))
AM_CONDITIONAL([enable_address_sanitizer], [test "${enable_sanitizer}" == "address"])
AM_COND_IF([enable_address_sanitizer], [
  AC_CHECK_LIB([asan], [__asan_report_error], [], [AC_MSG_ERROR([address sanitizer not found])])
  AC_DEFINE([POSEIDON_ENABLE_ADDRESS_SANITIZER], [1], [Define to 1 to enable address sanitizer.])
])
AM_CONDITIONAL([enable_thread_sanitizer], [test "${enable_sanitizer}" == "thread"])
AM_COND_IF([enable_thread_sanitizer], [
  AC_CHECK_LIB([tsan], [__tsan_on_report], [], [AC_MSG_ERROR([thread sanitizer not found])])
  AC_DEFINE([POSEIDON_ENABLE_THREAD_SANITIZER], [1], [Define to 1 to enable thread sanitizer.])
])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
