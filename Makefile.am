ACLOCAL_AMFLAGS = -I m4
NOTHING =

# Compiler options
AM_CPPFLAGS = -pipe -pthread -Wall -Wextra  \
  -D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE -D_WIN32_WINNT=0x0600  \
  -f{inline-limit=2400,strict-{aliasing,overflow},merge-all-constants}  \
  -fno-{math-errno,trapping-math,devirtualize-speculatively}  \
  -Werror={{,sign-}conversion,write-strings,return-type,double-promotion}  \
  -W{invalid-pch,switch-enum,suggest-attribute=noreturn,undef,shadow,missing-field-initializers}  \
  -Wunused-{function,label,local-typedefs,{,but-set-}{variable,parameter}}

AM_CXXFLAGS = -include .pch.hpp -std=gnu++14 -Wp,-std=c++11 -fno-gnu-keywords  \
  -Wzero-as-null-pointer-constant -Wno-redundant-move  \
  -Werror={non-virtual-dtor,missing-declarations}

AM_DEFAULT_SOURCE_EXT = .cpp
LDADD = lib/libasteria.la

# Pre-compiled header
BUILT_SOURCES =  \
  .pch.hpp  \
  .pch.hpp.gch  \
  ${NOTHING}

CLEANFILES =  \
  .pch.hpp  \
  .pch.hpp.gch  \
  .pch.hpp.gch.lo  \
  ${NOTHING}

.pch.hpp: ${srcdir}/asteria/src/precompiled.hpp
	${AM_V_GEN}echo "#include \"$$(cygpath -m $< 2>/dev/null || readlink -f $<)\"" >$@

.pch.hpp.gch: .pch.hpp config.h
	${AM_V_CXX}${LTCXXCOMPILE} -x c++-header -Wno-error $< -o $@.o
	@${LIBTOOL} ${AM_V_lt} --tag=CXX --mode=link sh -c 'mv -f "$$1" "$$2"' _ $@.lo $@ -o $@
	@test -z "${EXEEXT}" || rm -f $@${EXEEXT}

# Initialization
EXTRA_DIST =  \
  asteria/doc/operator-precedence.txt  \
  asteria/doc/standard-library.txt  \
  asteria/doc/syntax.txt  \
  asteria/doc/examples.txt  \
  ${NOTHING}

noinst_LIBRARIES =
noinst_LTLIBRARIES =
TESTS = ${check_PROGRAMS}

pkginclude_HEADERS =
lib_LIBRARIES =
lib_LTLIBRARIES =
bin_PROGRAMS =

check_HEADERS =
check_LTRARIES =
check_LIBRARIES =
check_PROGRAMS =

# Programs and libraries
include asteria/rocket/Makefile.inc.am
include asteria/src/Makefile.inc.am

# Tests
include asteria/test/Makefile.inc.am
