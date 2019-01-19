ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -pipe -pthread -fno-math-errno -fno-trapping-math -finline-limit=2500  \
  -Wall -Wextra -Wconversion -Wsign-conversion -Winvalid-pch  \
  -Wwrite-strings -Wswitch-enum -Wsuggest-attribute=noreturn -Wundef -Wshadow  \
  -Wstrict-aliasing=2 -Wstrict-overflow=2 -Wmissing-field-initializers  \
  -Wno-error=unused-{function,label,local-typedefs,{,but-set-}{variable,parameter}}  \
  -Wno-error={pragmas,invalid-pch,inline,strict-overflow}
AM_CXXFLAGS = -std=c++11 -Wnon-virtual-dtor
AM_DEFAULT_SOURCE_EXT = .cpp
LDADD = lib/libasteria.la

__pch.hpp: ${srcdir}/asteria/src/precompiled.hpp
	${AM_V_GEN}echo "#include \"$$(cygpath -m $< 2>/dev/null || readlink -f $<)\"" >$@
__pch.hpp.gch: __pch.hpp
	${AM_V_CXX}${LTCXXCOMPILE} -x c++-header -Wno-error $< -o $@.o
	${AM_V_GEN}${LIBTOOL} ${AM_V_lt} --tag=CXX --mode=link sh -c 'mv -f "$$1" "$$2"' _ $@.lo $@ -o $@
	${AM_V_GEN}test -z "${EXEEXT}" || rm -f $@${EXEEXT}

BUILT_SOURCES =  \
  __pch.hpp  \
  __pch.hpp.gch

CLEANFILES =  \
  __pch.hpp  \
  __pch.hpp.gch

noinst_HEADERS =  \
  test/_test_init.hpp

noinst_LTLIBRARIES =  \
  lib/librocket.la

lib_LTLIBRARIES =  \
  lib/libasteria.la

pkginclude_rocketdir = ${pkgincludedir}/rocket
pkginclude_rocket_HEADERS =  \
  asteria/src/rocket/compatibility.h  \
  asteria/src/rocket/_compatibility_gcc.h  \
  asteria/src/rocket/_compatibility_clang.h  \
  asteria/src/rocket/_compatibility_msvc.h  \
  asteria/src/rocket/preprocessor_utilities.h  \
  asteria/src/rocket/assert.hpp  \
  asteria/src/rocket/throw.hpp  \
  asteria/src/rocket/utilities.hpp  \
  asteria/src/rocket/allocator_utilities.hpp  \
  asteria/src/rocket/hash_table_utilities.hpp  \
  asteria/src/rocket/insertable_streambuf.hpp  \
  asteria/src/rocket/insertable_istream.hpp  \
  asteria/src/rocket/insertable_ostream.hpp  \
  asteria/src/rocket/insertable_stream.hpp  \
  asteria/src/rocket/variant.hpp  \
  asteria/src/rocket/fill_iterator.hpp  \
  asteria/src/rocket/transparent_comparators.hpp  \
  asteria/src/rocket/unique_handle.hpp  \
  asteria/src/rocket/reference_counter.hpp  \
  asteria/src/rocket/cow_string.hpp  \
  asteria/src/rocket/cow_vector.hpp  \
  asteria/src/rocket/cow_hashmap.hpp  \
  asteria/src/rocket/unique_ptr.hpp  \
  asteria/src/rocket/refcnt_ptr.hpp  \
  asteria/src/rocket/refcnt_object.hpp  \
  asteria/src/rocket/static_vector.hpp  \
  asteria/src/rocket/prehashed_string.hpp  \
  asteria/src/rocket/integer_sequence.hpp  \
  asteria/src/rocket/first_binder.hpp  \
  asteria/src/rocket/ostream_imploder.hpp

lib_librocket_la_SOURCES =  \
  asteria/src/rocket/assert.cpp  \
  asteria/src/rocket/throw.cpp  \
  asteria/src/rocket/insertable_streambuf.cpp  \
  asteria/src/rocket/insertable_istream.cpp  \
  asteria/src/rocket/insertable_ostream.cpp  \
  asteria/src/rocket/insertable_stream.cpp  \
  asteria/src/rocket/cow_string.cpp

lib_librocket_la_CXXFLAGS =  \
  ${AM_CXXFLAGS} -include __pch.hpp

pkginclude_HEADERS =  \
  asteria/src/precompiled.hpp  \
  asteria/src/fwd.hpp  \
  asteria/src/utilities.hpp

pkginclude_syntaxdir = ${pkgincludedir}/syntax
pkginclude_syntax_HEADERS =  \
  asteria/src/syntax/source_location.hpp  \
  asteria/src/syntax/xpnode.hpp  \
  asteria/src/syntax/expression.hpp  \
  asteria/src/syntax/statement.hpp  \
  asteria/src/syntax/block.hpp

pkginclude_runtimedir = ${pkgincludedir}/runtime
pkginclude_runtime_HEADERS =  \
  asteria/src/runtime/refcnt_base.hpp  \
  asteria/src/runtime/abstract_opaque.hpp  \
  asteria/src/runtime/abstract_function.hpp  \
  asteria/src/runtime/value.hpp  \
  asteria/src/runtime/reference_root.hpp  \
  asteria/src/runtime/reference_modifier.hpp  \
  asteria/src/runtime/reference.hpp  \
  asteria/src/runtime/variable.hpp  \
  asteria/src/runtime/variable_hashset.hpp  \
  asteria/src/runtime/abstract_variable_callback.hpp  \
  asteria/src/runtime/collector.hpp  \
  asteria/src/runtime/traceable_exception.hpp  \
  asteria/src/runtime/abstract_context.hpp  \
  asteria/src/runtime/analytic_context.hpp  \
  asteria/src/runtime/executive_context.hpp  \
  asteria/src/runtime/function_analytic_context.hpp  \
  asteria/src/runtime/function_executive_context.hpp  \
  asteria/src/runtime/global_context.hpp  \
  asteria/src/runtime/generational_collector.hpp  \
  asteria/src/runtime/variadic_arguer.hpp  \
  asteria/src/runtime/reference_stack.hpp  \
  asteria/src/runtime/reference_dictionary.hpp  \
  asteria/src/runtime/instantiated_function.hpp

pkginclude_compilerdir = ${pkgincludedir}/compiler
pkginclude_compiler_HEADERS =  \
  asteria/src/compiler/parser_error.hpp  \
  asteria/src/compiler/token.hpp  \
  asteria/src/compiler/token_stream.hpp  \
  asteria/src/compiler/parser.hpp  \
  asteria/src/compiler/simple_source_file.hpp

pkginclude_librarydir = ${pkgincludedir}/library
pkginclude_library_HEADERS =  \
  asteria/src/library/argument_sentry.hpp

lib_libasteria_la_SOURCES =  \
  asteria/src/utilities.cpp  \
  asteria/src/syntax/source_location.cpp  \
  asteria/src/syntax/xpnode.cpp  \
  asteria/src/syntax/expression.cpp  \
  asteria/src/syntax/statement.cpp  \
  asteria/src/syntax/block.cpp  \
  asteria/src/runtime/refcnt_base.cpp  \
  asteria/src/runtime/abstract_opaque.cpp  \
  asteria/src/runtime/abstract_function.cpp  \
  asteria/src/runtime/value.cpp  \
  asteria/src/runtime/reference_root.cpp  \
  asteria/src/runtime/reference_modifier.cpp  \
  asteria/src/runtime/reference.cpp  \
  asteria/src/runtime/variable.cpp  \
  asteria/src/runtime/variable_hashset.cpp  \
  asteria/src/runtime/abstract_variable_callback.cpp  \
  asteria/src/runtime/collector.cpp  \
  asteria/src/runtime/traceable_exception.cpp  \
  asteria/src/runtime/abstract_context.cpp  \
  asteria/src/runtime/analytic_context.cpp  \
  asteria/src/runtime/executive_context.cpp  \
  asteria/src/runtime/function_analytic_context.cpp  \
  asteria/src/runtime/function_executive_context.cpp  \
  asteria/src/runtime/global_context.cpp  \
  asteria/src/runtime/generational_collector.cpp  \
  asteria/src/runtime/variadic_arguer.cpp  \
  asteria/src/runtime/reference_stack.cpp  \
  asteria/src/runtime/reference_dictionary.cpp  \
  asteria/src/runtime/instantiated_function.cpp  \
  asteria/src/compiler/parser_error.cpp  \
  asteria/src/compiler/token.cpp  \
  asteria/src/compiler/token_stream.cpp  \
  asteria/src/compiler/parser.cpp  \
  asteria/src/compiler/simple_source_file.cpp  \
  asteria/src/library/argument_sentry.cpp

lib_libasteria_la_CXXFLAGS =  \
  ${AM_CXXFLAGS} -include __pch.hpp

lib_libasteria_la_LIBADD =  \
  lib/librocket.la

lib_libasteria_la_LDFLAGS =  \
  -Wl,--no-undefined -no-undefined -version-info 0:0:0

bin_PROGRAMS =  \
  bin/asteria

bin_asteria_SOURCES =  \
  asteria/src/main.cpp

check_PROGRAMS =  \
  test/utilities.test  \
  test/value.test  \
  test/variable.test  \
  test/reference.test  \
  test/traceable_exception.test  \
  test/executive_context.test  \
  test/expression.test  \
  test/block.test  \
  test/token_stream.test  \
  test/parser.test  \
  test/simple_source_file.test  \
  test/garbage_collection.test  \
  test/varg.test

TESTS = ${check_PROGRAMS}
