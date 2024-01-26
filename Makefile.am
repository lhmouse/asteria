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

ACLOCAL_AMFLAGS = -I m4
END =

## Compiler & linker options
AM_CPPFLAGS = -pipe -Wall -Wextra -Iasteria -U_FORTIFY_SOURCE  \
  @libpcre2_8_CFLAGS@ @libcrypto_CFLAGS@ @libz_CFLAGS@ -D_FILE_OFFSET_BITS=64  \
  -D_POSIX_C_SOURCE=200809 -D_DEFAULT_SOURCE -D_DARWIN_C_SOURCE  \
  -f{strict-{aliasing,overflow},merge-all-constants,rounding-math}  \
  -fno-{stack-protector,align-{functions,jumps,loops},semantic-interposition}  \
  -fno-{devirtualize-speculatively,ident,math-errno,trapping-math}  \
  -Werror={conversion,sign-{compare,conversion},write-strings}  \
  -Werror={return-type,double-promotion,missing-declarations}  \
  -W{missing-field-initializers,suggest-attribute=noreturn,shadow,switch-enum}  \
  -Wunused-{function,label,local-typedefs,but-set-{variable,parameter}}

AM_CXXFLAGS = @asm_CFLAGS@ @asan_CFLAGS@ -fno-elide-constructors  \
  -std=c++17 -f{visibility-inlines-hidden,nothrow-opt,no-enforce-eh-specs}  \
  -Wno-redundant-move -Wzero-as-null-pointer-constant -Werror=non-virtual-dtor

AM_LDFLAGS = -no-undefined
LDADD = lib/libasteria.la ${lib_libasteria_la_LIBADD}
AM_DEFAULT_SOURCE_EXT = .cpp
SUFFIXES = .xipp

## Initialization
noinst_LIBRARIES =
noinst_LTLIBRARIES =

nobase_include_HEADERS =
lib_LIBRARIES =
lib_LTLIBRARIES =
bin_PROGRAMS =

check_HEADERS =
check_LIBRARIES =
check_LTLIBRARIES =
check_PROGRAMS =

BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
TESTS = ${check_PROGRAMS}

## Documentation
include doc/Makefile.inc.am

## Programs and libraries
include rocket/Makefile.inc.am
include asteria/Makefile.inc.am

## Tests
include test/Makefile.inc.am
