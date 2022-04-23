ACLOCAL_AMFLAGS = -I m4
NOTHING =

## Compiler options
AM_CPPFLAGS = -pipe -pthread -Wall -Wextra -fPIC -DPIC -DDLL_EXPORT  \
  -Iasteria/src -Iasteria/src/runtime -I${srcdir}/asteria/src  \
  -U_FORTIFY_SOURCE -D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE  \
  -f{strict-{aliasing,overflow},merge-all-constants}  \
  -fno-{math-errno,trapping-math,devirtualize-speculatively,stack-protector}  \
  -Werror={conversion,sign-{compare,conversion},write-strings,return-type}  \
  -Werror={double-promotion,aggressive-loop-optimizations}  \
  -W{invalid-pch,switch-enum,suggest-attribute=noreturn,shadow,missing-field-initializers}  \
  -Wunused-{function,label,local-typedefs,{,but-set-}{variable,parameter}}

AM_CXXFLAGS = -include precompiled.w.hpp -std=gnu++14 -Wp,-std=c++11  \
  -fno-gnu-keywords -fvisibility-inlines-hidden  \
  -Wno-redundant-move  \
  -Werror={non-virtual-dtor,missing-declarations}

AM_DEFAULT_SOURCE_EXT = .cpp
LDADD = lib/libasteria.la

## Pre-compiled header
BUILT_SOURCES =  \
  precompiled.w.hpp  \
  precompiled.w.hpp.gch  \
  ${NOTHING}

CLEANFILES =  \
  precompiled.w.hpp  \
  precompiled.w.hpp.gch  \
  precompiled.w.hpp.gch.lo  \
  ${NOTHING}

precompiled.w.hpp: ${srcdir}/asteria/src/precompiled.hpp
	## Build the wrapper header, which contains the absolute path to the plain header.
	${AM_V_GEN}echo "#include \"$$(cygpath -m $< 2>/dev/null || readlink -f $<)\"" >$@

precompiled.w.hpp.gch: precompiled.w.hpp config.h
	## Compile the header. This must use LTCXXCOMPILE due to some nasty PIC flags...
	${AM_V_CXX}${LTCXXCOMPILE} -x c++-header -Wno-error $< -o $@.o
	## ..., but it causes the 'real' output to go into './libs/' and leaves a text file outside,
	## so move it to the correct location, ...
	@${LIBTOOL} ${AM_V_lt} --tag=CXX --mode=link sh -c 'mv -f "$$1" "$$2"' _ $@.lo $@ -o $@
	## ..., then libtool creates another text file as the 'executable', so delete it.
	@test -z "${EXEEXT}" || rm -f $@${EXEEXT}

## Initialization
EXTRA_DIST =  \
  asteria/doc/operator-precedence.txt  \
  asteria/doc/standard-library.txt  \
  asteria/doc/syntax.txt  \
  asteria/doc/examples.txt  \
  asteria/doc/asteria.nanorc  \
  ${NOTHING}

noinst_LIBRARIES =
noinst_LTLIBRARIES =
TESTS = ${check_PROGRAMS}

include_HEADERS =
lib_LIBRARIES =
lib_LTLIBRARIES =
bin_PROGRAMS =

check_HEADERS =
check_LIBRARIES =
check_LTLIBRARIES =
check_PROGRAMS =

## Programs and libraries
include asteria/rocket/Makefile.inc.am
include asteria/src/Makefile.inc.am

## Tests
include asteria/test/Makefile.inc.am
