ACLOCAL_AMFLAGS = -I m4
END =

## Compiler options
AM_CPPFLAGS = -pipe -pthread -Wall -Wextra -fPIC -DPIC -Isrc -U_FORTIFY_SOURCE  \
  -DDLL_EXPORT -D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE  \
  -f{strict-{aliasing,overflow},merge-all-constants}  \
  -fno-{math-errno,trapping-math,devirtualize-speculatively,stack-protector}  \
  -fno-semantic-interposition  \
  -Werror={conversion,sign-{compare,conversion},write-strings,return-type}  \
  -Werror={double-promotion,aggressive-loop-optimizations}  \
  -W{switch-enum,suggest-attribute=noreturn,shadow,missing-field-initializers}  \
  -Wunused-{function,label,local-typedefs,{,but-set-}{variable,parameter}}

AM_CXXFLAGS = -std=gnu++14 -Wp,-std=c++11 -fno-gnu-keywords -fvisibility-inlines-hidden  \
  -Wno-redundant-move -Werror={non-virtual-dtor,missing-declarations}

AM_DEFAULT_SOURCE_EXT = .cpp
SUFFIXES = .xipp .xipp.gch
LDADD = lib/libasteria.la

## Initialization
noinst_LIBRARIES =
noinst_LTLIBRARIES =

include_HEADERS =
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
include src/Makefile.inc.am

## Tests
include test/Makefile.inc.am
