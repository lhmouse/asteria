ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wno-error=unused	\
	-Winvalid-pch -Wwrite-strings -Wsuggest-attribute=noreturn	\
	-Wundef -Wshadow -Wstrict-aliasing=2 -Wstrict-overflow=2 -Wno-error=pragmas -Wswitch-enum	\
	-pipe -fPIC -DPIC
AM_CXXFLAGS = -Wnon-virtual-dtor -pedantic -pedantic-errors -std=c++11
LDADD = lib/libasteria.la

%.hpp.gch: %.hpp
	${CXX} -x c++-header ${DEFS} -I. ${AM_CPPFLAGS} ${CPPFLAGS} ${AM_CXXFLAGS} ${CXXFLAGS} -Wno-error $< -o $@

BUILT_SOURCES =	\
	asteria/src/precompiled.hpp.gch

CLEANFILES =	\
	asteria/src/precompiled.hpp.gch

pkginclude_rocketdir = ${pkgincludedir}/rocket
pkginclude_rocket_HEADERS = \
	asteria/src/rocket/compatibility.hpp	\
	asteria/src/rocket/assert.hpp	\
	asteria/src/rocket/exchange.hpp	\
	asteria/src/rocket/insertable_streambuf.hpp	\
	asteria/src/rocket/insertable_ostream.hpp	\
	asteria/src/rocket/value_ptr.hpp	\
	asteria/src/rocket/variant.hpp	\
	asteria/src/rocket/nullptr_filler.hpp

pkginclude_HEADERS =	\
	asteria/src/fwd.hpp	\
	asteria/src/sptr_fmt.hpp	\
	asteria/src/utilities.hpp	\
	asteria/src/opaque_base.hpp	\
	asteria/src/function_base.hpp	\
	asteria/src/parameter.hpp	\
	asteria/src/variable.hpp	\
	asteria/src/stored_value.hpp	\
	asteria/src/reference.hpp	\
	asteria/src/stored_reference.hpp	\
	asteria/src/exception.hpp	\
	asteria/src/initializer.hpp	\
	asteria/src/expression_node.hpp	\
	asteria/src/expression.hpp	\
	asteria/src/statement.hpp	\
	asteria/src/block.hpp	\
	asteria/src/instantiated_function.hpp	\
	asteria/src/simple_function.hpp	\
	asteria/src/local_variable.hpp	\
	asteria/src/scope.hpp	\
	asteria/src/recycler.hpp	\
	asteria/src/executor.hpp

lib_LTLIBRARIES =	\
	lib/libasteria.la

lib_libasteria_la_SOURCES =	\
	asteria/src/rocket/assert.cpp	\
	asteria/src/rocket/insertable_streambuf.cpp	\
	asteria/src/rocket/insertable_ostream.cpp	\
	asteria/src/utilities.cpp	\
	asteria/src/opaque_base.cpp	\
	asteria/src/function_base.cpp	\
	asteria/src/parameter.cpp	\
	asteria/src/variable.cpp	\
	asteria/src/stored_value.cpp	\
	asteria/src/reference.cpp	\
	asteria/src/stored_reference.cpp	\
	asteria/src/exception.cpp	\
	asteria/src/initializer.cpp	\
	asteria/src/expression_node.cpp	\
	asteria/src/expression.cpp	\
	asteria/src/statement.cpp	\
	asteria/src/block.cpp	\
	asteria/src/instantiated_function.cpp	\
	asteria/src/simple_function.cpp	\
	asteria/src/local_variable.cpp	\
	asteria/src/scope.cpp	\
	asteria/src/recycler.cpp	\
	asteria/src/executor.cpp

lib_libasteria_la_LDFLAGS =	\
	-Wl,--no-undefined -no-undefined -version-info 0:0:0

bin_PROGRAMS =	\
	bin/asteria

bin_asteria_SOURCES = asteria/src/main.cpp

check_PROGRAMS = \
	tmp/block	\
	tmp/exception	\
	tmp/expression	\
	tmp/recycler	\
	tmp/reference	\
	tmp/scope	\
	tmp/utilities	\
	tmp/variable

tmp_block_SOURCES        = test/block.cpp
tmp_exception_SOURCES    = test/exception.cpp
tmp_expression_SOURCES   = test/expression.cpp
tmp_recycler_SOURCES     = test/recycler.cpp
tmp_reference_SOURCES    = test/reference.cpp
tmp_scope_SOURCES        = test/scope.cpp
tmp_utilities_SOURCES    = test/utilities.cpp
tmp_variable_SOURCES     = test/variable.cpp

TESTS = ${check_PROGRAMS}
