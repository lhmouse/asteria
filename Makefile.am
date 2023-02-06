ACLOCAL_AMFLAGS = -I m4
END =

## Compiler & linker options
AM_CPPFLAGS = -pipe -Wall -Wextra -Iasteria -U_FORTIFY_SOURCE  \
  -D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE  \
  -f{strict-{aliasing,overflow},merge-all-constants}  \
  -fno-{stack-protector,align-{functions,jumps,loops}}  \
  -fno-{math-errno,trapping-math}  \
  -fno-{devirtualize-speculatively,semantic-interposition,ident}  \
  -Werror={conversion,sign-{compare,conversion},write-strings}  \
  -Werror={return-type,double-promotion,missing-declarations}  \
  -W{missing-field-initializers,suggest-attribute=noreturn,shadow}  \
  -W{switch-enum,unused-{function,label,local-typedefs}}  \
  -Wunused-but-set-{variable,parameter}

AM_CXXFLAGS = -std=c++17 -fvisibility-inlines-hidden  \
  -Wno-redundant-move -Werror={non-virtual-dtor,missing-declarations}

AM_LDFLAGS = -Wl,--no-undefined -no-undefined

AM_DEFAULT_SOURCE_EXT = .cpp
LDADD = lib/libasteria.la
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
