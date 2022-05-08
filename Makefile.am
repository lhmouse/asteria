ACLOCAL_AMFLAGS = -I m4
NOTHING =

## Compiler options
AM_CPPFLAGS = -pipe -pthread -Wall -Wextra -fPIC -DPIC -Isrc -U_FORTIFY_SOURCE  \
  -DDLL_EXPORT -D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE  \
  -f{strict-{aliasing,overflow},merge-all-constants}  \
  -fno-{math-errno,trapping-math,devirtualize-speculatively,stack-protector}  \
  -fno-semantic-interposition  \
  -Werror={conversion,sign-{compare,conversion},write-strings,return-type}  \
  -Werror={double-promotion,aggressive-loop-optimizations}  \
  -W{invalid-pch,switch-enum,suggest-attribute=noreturn,shadow,missing-field-initializers}  \
  -Wunused-{function,label,local-typedefs,{,but-set-}{variable,parameter}}

AM_CXXFLAGS = -include precompiled.xipp -std=gnu++14 -Wp,-std=c++11  \
  -fno-gnu-keywords -fvisibility-inlines-hidden  \
  -Wno-redundant-move  \
  -Werror={non-virtual-dtor,missing-declarations}

AM_DEFAULT_SOURCE_EXT = .cpp
SUFFIXES = .xipp .xipp.gch
LDADD = lib/libasteria.la

BUILT_SOURCES =  \
  precompiled.xipp  \
  precompiled.xipp.gch  \
  ${NOTHING}

CLEANFILES =  \
  precompiled.xipp  \
  precompiled.xipp.gch  \
  precompiled.xipp.gch.lo  \
  ${NOTHING}

## Custome rules
%.xipp: ${srcdir}/src/%.ipp
	## Build the wrapper header, which contains the absolute path to the plain header.
	${AM_V_GEN}echo "#include \"$$(readlink -f $<)\"" >$@

%.xipp.gch: %.xipp src/version.h config.h
	## Compile the header. This must use LTCXXCOMPILE due to some nasty PIC flags...
	${AM_V_CXX}${LTCXXCOMPILE} -x c++-header -Wno-error $< -o $@.o
	## ..., but it causes the 'real' output to go into './libs/' and leaves a text file
	## outside, so move it to the correct location, ...
	@${LIBTOOL} ${AM_V_lt} --tag=CXX --mode=link sh -c 'mv -f "$$1" "$$2"' _ $@.lo $@ -o $@
	## ..., then libtool creates another text file as the 'executable', so delete it.
	@test -z "${EXEEXT}" || rm -f $@${EXEEXT}

## Initialization
EXTRA_DIST =  \
  doc/operator-precedence.txt  \
  doc/standard-library.txt  \
  doc/syntax.txt  \
  doc/examples.txt  \
  doc/asteria.nanorc  \
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
include rocket/Makefile.inc.am
include src/Makefile.inc.am

## Tests
include test/Makefile.inc.am
