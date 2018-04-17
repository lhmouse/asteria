ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wno-error=unused	\
	-Winvalid-pch -Wwrite-strings -Wsuggest-attribute=noreturn	\
	-Wundef -Wshadow -Wstrict-aliasing=2 -Wstrict-overflow=2 -Wno-error=pragmas -Wswitch-enum	\
	-pipe -fPIC -DPIC
AM_CXXFLAGS = -Wnon-virtual-dtor -pedantic -pedantic-errors -std=c++11
if ENABLE_DEBUG_CHECKS
AM_CPPFLAGS += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
endif

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
	asteria/src/utilities.hpp	\
	asteria/src/variable.hpp	\
	asteria/src/stored_value.hpp	\
	asteria/src/reference.hpp	\
	asteria/src/stored_reference.hpp	\
	asteria/src/exception.hpp	\
	asteria/src/initializer.hpp	\
	asteria/src/expression_node.hpp	\
	asteria/src/expression.hpp	\
	asteria/src/statement.hpp	\
	asteria/src/scope.hpp	\
	asteria/src/recycler.hpp

pkginclude_rocketdir = ${pkgincludedir}/rocket
pkginclude_rocket_HEADERS = \
	asteria/src/rocket/value_ptr.hpp

lib_LTLIBRARIES =	\
	lib/libasteria.la

lib_libasteria_la_SOURCES =	\
	asteria/src/insertable_ostream.cpp	\
	asteria/src/utilities.cpp	\
	asteria/src/variable.cpp	\
	asteria/src/stored_value.cpp	\
	asteria/src/reference.cpp	\
	asteria/src/stored_reference.cpp	\
	asteria/src/exception.cpp	\
	asteria/src/initializer.cpp	\
	asteria/src/expression_node.cpp	\
	asteria/src/expression.cpp	\
	asteria/src/statement.cpp	\
	asteria/src/scope.cpp	\
	asteria/src/recycler.cpp

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
