ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -Wextra -Wconversion -Wsign-conversion -Wno-error=unused	\
	-Winvalid-pch -Wwrite-strings -Wsuggest-attribute=noreturn -Wswitch-enum -Wundef -Wshadow	\
	-Wstrict-aliasing=2 -Wstrict-overflow=2 -Wno-error=pragmas -Wno-error=unknown-warning-option	\
	-pipe -fPIC -DPIC -pedantic -pedantic-errors
if ENABLE_WERROR
AM_CPPFLAGS += -Werror
endif
AM_CXXFLAGS = -std=c++11 -Wnon-virtual-dtor
AM_DEFAULT_SOURCE_EXT = .cpp
LDADD = lib/libasteria.la

%.hpp.gch: %.hpp
	${AM_V_CXX}${CXX} -x c++-header ${DEFS} -I. ${AM_CPPFLAGS} ${CPPFLAGS} ${AM_CXXFLAGS} ${CXXFLAGS} -Wno-error $< -o $@

BUILT_SOURCES =	\
	asteria/src/precompiled.hpp.gch

CLEANFILES =	\
	asteria/src/precompiled.hpp.gch

pkginclude_rocketdir = ${pkgincludedir}/rocket
pkginclude_rocket_HEADERS = \
	asteria/src/rocket/compatibility.h	\
	asteria/src/rocket/preprocessor_utilities.h	\
	asteria/src/rocket/assert.hpp	\
	asteria/src/rocket/throw.hpp	\
	asteria/src/rocket/utilities.hpp	\
	asteria/src/rocket/allocator_utilities.hpp	\
	asteria/src/rocket/insertable_streambuf.hpp	\
	asteria/src/rocket/insertable_istream.hpp	\
	asteria/src/rocket/insertable_ostream.hpp	\
	asteria/src/rocket/insertable_iostream.hpp	\
	asteria/src/rocket/variant.hpp	\
	asteria/src/rocket/fill_iterator.hpp	\
	asteria/src/rocket/unique_handle.hpp	\
	asteria/src/rocket/cow_string.hpp	\
	asteria/src/rocket/cow_vector.hpp	\
	asteria/src/rocket/cow_hashmap.hpp

pkginclude_HEADERS =	\
	asteria/src/fwd.hpp	\
	asteria/src/utilities.hpp	\
	asteria/src/opaque_base.hpp	\
	asteria/src/function_base.hpp	\
	asteria/src/value.hpp	\
	asteria/src/parser_result.hpp	\
	asteria/src/token.hpp

lib_LTLIBRARIES =	\
	lib/libasteria.la

lib_libasteria_la_SOURCES =	\
	asteria/src/rocket/assert.cpp	\
	asteria/src/rocket/throw.cpp	\
	asteria/src/rocket/insertable_streambuf.cpp	\
	asteria/src/rocket/insertable_istream.cpp	\
	asteria/src/rocket/insertable_ostream.cpp	\
	asteria/src/rocket/insertable_iostream.cpp	\
	asteria/src/rocket/cow_string.cpp	\
	asteria/src/utilities.cpp	\
	asteria/src/opaque_base.cpp	\
	asteria/src/function_base.cpp	\
	asteria/src/value.cpp	\
	asteria/src/token.cpp

lib_libasteria_la_LDFLAGS =	\
	-Wl,--no-undefined -no-undefined -version-info 0:0:0

bin_PROGRAMS =	\
	bin/asteria

bin_asteria_SOURCES =	\
	asteria/src/main.cpp

check_PROGRAMS = \
	test/utilities.test	\
	test/value.test	\
	test/token.test

TESTS = ${check_PROGRAMS}
