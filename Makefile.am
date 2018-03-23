ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wno-error=unused-parameter	\
	-Winvalid-pch -Wno-missing-field-initializers -Wwrite-strings -Wsuggest-attribute=noreturn	\
	-Wundef -Wshadow -Wstrict-aliasing=2 -Wstrict-overflow=2 -Wno-error=pragmas	\
	-pipe -fPIC -DPIC -std=c++11
AM_CXXFLAGS = -Wnon-virtual-dtor

pkginclude_HEADERS =	\
	asteria/src/parser.hpp

bin_PROGRAMS =	\
	bin/asteria

bin_asteria_SOURCES =	\
	asteria/src/main.cpp

bin_asteria_LDADD =	\
	lib/libasteria.la

lib_LTLIBRARIES =	\
	lib/libasteria.la

lib_libasteria_la_SOURCES =	\
	asteria/src/parser.cpp

lib_libasteria_la_LDFLAGS =	\
	-Wl,--no-undefined -version-info 0:0:0

lib_libasteria_la_LIBADD =	\
	${AM_LIBS}
