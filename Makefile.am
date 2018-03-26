ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wno-error=unused	\
	-Winvalid-pch -Wno-missing-field-initializers -Wwrite-strings -Wsuggest-attribute=noreturn	\
	-Wundef -Wshadow -Wstrict-aliasing=2 -Wstrict-overflow=2 -Wno-error=pragmas -Wswitch-enum	\
	-pipe -fPIC -DPIC
AM_CXXFLAGS = -Wnon-virtual-dtor -pedantic -pedantic-errors -std=c++11

%.hpp.gch: %.hpp
	${CXX} -x c++-header ${DEFS} -I. ${AM_CPPFLAGS} ${CPPFLAGS} ${AM_CXXFLAGS} ${CXXFLAGS} -Wno-error $< -o $@

BUILT_SOURCES =	\
	asteria/src/precompiled.hpp.gch

CLEANFILES =	\
	asteria/src/precompiled.hpp.gch

pkginclude_HEADERS =	\
	asteria/src/type_tuple.hpp	\
	asteria/src/fwd.hpp	\
	asteria/src/insertable_ostream.hpp	\
	asteria/src/misc.hpp	\
	asteria/src/variable.hpp	\
	asteria/src/statement.hpp

lib_LTLIBRARIES =	\
	lib/libasteria.la

lib_libasteria_la_SOURCES =	\
	asteria/src/fwd.cpp	\
	asteria/src/insertable_ostream.cpp	\
	asteria/src/misc.cpp	\
	asteria/src/variable.cpp	\
	asteria/src/statement.cpp

lib_libasteria_la_LDFLAGS =	\
	-Wl,--no-undefined -version-info 0:0:0

lib_libasteria_la_LIBADD =	\
	${AM_LIBS}

bin_PROGRAMS =	\
	bin/asteria

bin_asteria_SOURCES =	\
	asteria/src/main.cpp

bin_asteria_LDADD =	\
	lib/libasteria.la
