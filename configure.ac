## Copyright (C) 2023 by LH_Mouse <lh_mouse@126.com>
##
## Permission to use, copy, modify, and/or distribute this
## software for any purpose with or without fee is hereby granted.
##
## THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
## WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
## THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
## CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
## LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
## NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
## CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

## AC_INIT(package, version, [bug-report], [tarname], [url])
AC_INIT([asteria], [master], [lh_mouse@126.com], [asteria], [https://github.com/lhmouse/asteria])
AC_LANG([C++])
AC_CONFIG_SRCDIR([asteria/simple_script.cpp])
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
AC_CHECK_HEADERS([uchar.h])
AM_ICONV_LINK()
PKG_CHECK_MODULES([libz], [zlib >= 1.2.9], [], [AC_MSG_ERROR([zlib >= 1.2.9 required])])
PKG_CHECK_MODULES([libpcre2_8], [libpcre2-8], [], [AC_MSG_ERROR([PCRE2 required])])
PKG_CHECK_MODULES([libcrypto], [libcrypto], [], [AC_MSG_ERROR([OpenSSL required])])

## Check whether REPL is to be built
AC_ARG_ENABLE([repl], AS_HELP_STRING([--disable-repl], [do not build the REPL]))
AM_CONDITIONAL([enable_repl], [test "${enable_repl}" != "no"])
AM_COND_IF([enable_repl], [
  PKG_CHECK_MODULES([libedit], [libedit], [],
    [AC_MSG_ERROR([Editline library not found; if you don't need the REPL, pass `--disable-repl`])])
])

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
AS_VAR_SET([sanitizer_flags])
AM_CONDITIONAL([enable_address_sanitizer], [test "${enable_sanitizer}" == "address"])
AM_COND_IF([enable_address_sanitizer], [
  AC_CHECK_LIB([asan], [__asan_report_error], [], [AC_MSG_ERROR([address sanitizer not found])])
  AC_CHECK_LIB([ubsan], [__ubsan_on_report], [], [AC_MSG_ERROR([undefined behavior sanitizer not found])])
  AC_DEFINE([POSEIDON_ENABLE_ADDRESS_SANITIZER], [1], [Define to 1 to enable address sanitizer.])
  AS_VAR_APPEND([sanitizer_flags], [" -fsanitize=address,undefined"])
])
AM_CONDITIONAL([enable_thread_sanitizer], [test "${enable_sanitizer}" == "thread"])
AM_COND_IF([enable_thread_sanitizer], [
  AC_CHECK_LIB([tsan], [__tsan_on_report], [], [AC_MSG_ERROR([thread sanitizer not found])])
  AC_DEFINE([POSEIDON_ENABLE_THREAD_SANITIZER], [1], [Define to 1 to enable thread sanitizer.])
  AS_VAR_APPEND([sanitizer_flags], [" -fsanitize=thread"])
])

AC_CHECK_DECL([__APPLE__], [
  # macOS...
  AC_MSG_WARN([Apple systems are nonsense and receive no standard support.])
], [
  # Set host-specific options
  AC_CHECK_DECL([__i386__], AS_VAR_SET([host_asm_opt], ["-masm=intel -mavx -mfpmath=sse"]))
  AC_CHECK_DECL([__amd64__], AS_VAR_SET([host_asm_opt], ["-masm=intel -mavx"]))
])

## Finish
AC_SUBST([abi_major])
AC_SUBST([abi_minor])
AC_SUBST([abi_suffix])
AC_SUBST([sanitizer_flags])
AC_SUBST([host_asm_opt])

AC_CONFIG_FILES([Makefile asteria/version.h])
AC_OUTPUT
