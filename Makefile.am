ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wall -Wextra -Wconversion -Wsign-conversion  \
  -Winvalid-pch -Wwrite-strings -Wswitch-enum -Wsuggest-attribute=noreturn -Wno-error=pragmas  \
  -Wundef -Wshadow -Wstrict-aliasing=2 -Wstrict-overflow=2 -Wno-error=pragmas  \
  -Wno-error=unused-{function,label,local-typedefs,{,but-set-}{variable,parameter}}  \
  -pipe -fPIC -DPIC -pthread
if enable_werror
AM_CPPFLAGS += -Werror
endif
AM_CXXFLAGS = -std=c++11 -Wnon-virtual-dtor
AM_DEFAULT_SOURCE_EXT = .cpp
LDADD = lib/libasteria.la

%.hpp.gch: %.hpp
	${AM_V_CXX}${CXXCOMPILE} -x c++-header -Wno-error $< -o $@

BUILT_SOURCES =  \
  asteria/src/precompiled.hpp.gch

CLEANFILES =  \
  asteria/src/precompiled.hpp.gch

noinst_LTLIBRARIES =  \
  lib/librocket.la

lib_LTLIBRARIES =  \
  lib/libasteria.la

pkginclude_rocketdir = ${pkgincludedir}/rocket
pkginclude_rocket_HEADERS = \
  asteria/src/rocket/compatibility.h  \
  asteria/src/rocket/preprocessor_utilities.h  \
  asteria/src/rocket/assert.hpp  \
  asteria/src/rocket/throw.hpp  \
  asteria/src/rocket/utilities.hpp  \
  asteria/src/rocket/allocator_utilities.hpp  \
  asteria/src/rocket/insertable_streambuf.hpp  \
  asteria/src/rocket/insertable_istream.hpp  \
  asteria/src/rocket/insertable_ostream.hpp  \
  asteria/src/rocket/insertable_stream.hpp  \
  asteria/src/rocket/variant.hpp  \
  asteria/src/rocket/fill_iterator.hpp  \
  asteria/src/rocket/transparent_comparators.hpp  \
  asteria/src/rocket/unique_handle.hpp  \
  asteria/src/rocket/cow_string.hpp  \
  asteria/src/rocket/cow_vector.hpp  \
  asteria/src/rocket/cow_hashmap.hpp  \
  asteria/src/rocket/refcounted_ptr.hpp

lib_librocket_la_SOURCES =  \
  asteria/src/rocket/assert.cpp  \
  asteria/src/rocket/throw.cpp  \
  asteria/src/rocket/insertable_streambuf.cpp  \
  asteria/src/rocket/insertable_istream.cpp  \
  asteria/src/rocket/insertable_ostream.cpp  \
  asteria/src/rocket/insertable_stream.cpp  \
  asteria/src/rocket/cow_string.cpp

pkginclude_HEADERS =  \
  asteria/src/fwd.hpp  \
  asteria/src/utilities.hpp  \
  asteria/src/abstract_opaque.hpp  \
  asteria/src/shared_opaque_wrapper.hpp  \
  asteria/src/abstract_function.hpp  \
  asteria/src/shared_function_wrapper.hpp  \
  asteria/src/value.hpp  \
  asteria/src/reference_root.hpp  \
  asteria/src/reference_modifier.hpp  \
  asteria/src/reference.hpp  \
  asteria/src/variable.hpp  \
  asteria/src/collector.hpp  \
  asteria/src/exception.hpp  \
  asteria/src/abstract_context.hpp  \
  asteria/src/analytic_context.hpp  \
  asteria/src/executive_context.hpp  \
  asteria/src/global_context.hpp  \
  asteria/src/global_collector.hpp  \
  asteria/src/xpnode.hpp  \
  asteria/src/expression.hpp  \
  asteria/src/statement.hpp  \
  asteria/src/block.hpp  \
  asteria/src/instantiated_function.hpp  \
  asteria/src/parser_error.hpp  \
  asteria/src/token.hpp  \
  asteria/src/token_stream.hpp  \
  asteria/src/parser.hpp

lib_libasteria_la_SOURCES =  \
  asteria/src/utilities.cpp  \
  asteria/src/abstract_opaque.cpp  \
  asteria/src/abstract_function.cpp  \
  asteria/src/value.cpp  \
  asteria/src/reference_root.cpp  \
  asteria/src/reference_modifier.cpp  \
  asteria/src/reference.cpp  \
  asteria/src/variable.cpp  \
  asteria/src/collector.cpp  \
  asteria/src/exception.cpp  \
  asteria/src/abstract_context.cpp  \
  asteria/src/analytic_context.cpp  \
  asteria/src/executive_context.cpp  \
  asteria/src/global_context.cpp  \
  asteria/src/global_collector.cpp  \
  asteria/src/variadic_arguer.cpp  \
  asteria/src/xpnode.cpp  \
  asteria/src/expression.cpp  \
  asteria/src/statement.cpp  \
  asteria/src/block.cpp  \
  asteria/src/instantiated_function.cpp  \
  asteria/src/parser_error.cpp  \
  asteria/src/token.cpp  \
  asteria/src/token_stream.cpp  \
  asteria/src/parser.cpp

lib_libasteria_la_LIBADD =  \
  lib/librocket.la

lib_libasteria_la_LDFLAGS =  \
  -Wl,--no-undefined -no-undefined -version-info 0:0:0

bin_PROGRAMS =  \
  bin/asteria

bin_asteria_SOURCES =  \
  asteria/src/main.cpp

check_PROGRAMS = \
  test/utilities.test  \
  test/value.test  \
  test/variable.test  \
  test/reference.test  \
  test/exception.test  \
  test/executive_context.test  \
  test/expression.test  \
  test/block.test  \
  test/token_stream.test  \
  test/parser.test

TESTS = ${check_PROGRAMS}
