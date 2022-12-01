## AC_INIT(package, version, [bug-report], [tarname], [url])
AC_INIT([asteria], [master], [lh_mouse@126.com], [asteria], [https://github.com/lhmouse/asteria])
AC_LANG([C++])
AC_CONFIG_SRCDIR([src/simple_script.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CXX

LT_INIT([disable-static])
LT_LANG([C++])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])

## Define ABI information
AS_VAR_SET([abi_major], [1])
AS_VAR_SET([abi_minor], [0])
AS_VAR_SET([abi_suffix], [beta.0])

## Check for required libraries
AC_CHECK_LIB([rt], [clock_gettime], [], [AC_MSG_WARN(librt not found; proceeding without it)])
AC_CHECK_LIB([z], [crc32], [], [AC_MSG_ERROR(zlib not found)])
AC_CHECK_LIB([pcre2-8], [pcre2_compile_8], [], [AC_MSG_ERROR(PCRE2 not found)])
AC_CHECK_LIB([crypto], [MD5_Init], [], [AC_MSG_ERROR(OpenSSL not found)])
AC_CHECK_LIB([edit], [el_init], [], [AC_MSG_ERROR(Editline library not found)])
AM_ICONV_LINK()

## Check for assertions
AC_ARG_ENABLE([debug-checks], AS_HELP_STRING([--enable-debug-checks], [enable assertions]))
AM_CONDITIONAL([enable_debug_checks], [test "${enable_debug_checks}" == "yes"])
AM_COND_IF([enable_debug_checks], [
  AC_DEFINE([_GLIBCXX_DEBUG], 1, [Define to 1 to enable debug checks of libstdc++.])
  AC_DEFINE([_LIBCPP_DEBUG], 1, [Define to 1 to enable debug checks of libc++.])
  AC_DEFINE([_DEBUG], 1, [Define to 1 to enable debug checks of MSVC standard library.])
])

## Check for pre-compiled headers
AC_ARG_ENABLE([pch], AS_HELP_STRING([--disable-pch], [do not use pre-compiled headers]))
AM_CONDITIONAL([enable_pch], [test "${enable_pch}" != "no"])

## Check for sanitizers
AC_ARG_ENABLE([sanitizer], AS_HELP_STRING([--enable-sanitizer=address|thread],
  [enable sanitizer (address sanitizer and thread sanitizer cannot be enabled at the same time)]))

AM_CONDITIONAL([enable_address_sanitizer], [test "${enable_sanitizer}" == "address"])
AM_COND_IF([enable_address_sanitizer], [
  AC_CHECK_LIB([asan], [__asan_report_error], [], [AC_MSG_ERROR([address sanitizer not found])])
  AC_DEFINE([ASTERIA_ENABLE_ADDRESS_SANITIZER], 1, [Define to 1 to enable address sanitizer.])
])

AM_CONDITIONAL([enable_thread_sanitizer], [test "${enable_sanitizer}" == "thread"])
AM_COND_IF([enable_thread_sanitizer], [
  AC_CHECK_LIB([tsan], [__tsan_on_report], [], [AC_MSG_ERROR([thread sanitizer not found])])
  AC_DEFINE([ASTERIA_ENABLE_THREAD_SANITIZER], 1, [Define to 1 to enable thread sanitizer.])
])

## Finish
AC_SUBST([abi_major])
AC_SUBST([abi_minor])
AC_SUBST([abi_suffix])

AC_CONFIG_FILES([Makefile src/version.h])
AC_OUTPUT
