ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -pipe -pthread -Wall -Wextra \
  -D_POSIX_SOURCE -D_DEFAULT_SOURCE -D_GNU_SOURCE -D_WIN32_WINNT=0x0600  \
  -f{inline-limit=2400,strict-{aliasing,overflow},merge-all-constants}  \
  -fno-{math-errno,trapping-math,devirtualize-speculatively}  \
  -Werror={{,sign-}conversion,write-strings,return-type}  \
  -W{invalid-pch,switch-enum,suggest-attribute=noreturn,undef,shadow,missing-field-initializers}  \
  -Wunused-{function,label,local-typedefs,{,but-set-}{variable,parameter}}
AM_CXXFLAGS = -std=c++14 -Wnon-virtual-dtor -Wzero-as-null-pointer-constant -Wmissing-declarations
AM_DEFAULT_SOURCE_EXT = .cpp
LDADD = lib/libasteria.la

.pch.hpp: ${srcdir}/asteria/src/precompiled.hpp
	${AM_V_GEN}echo "#include \"$$(cygpath -m $< 2>/dev/null || readlink -f $<)\"" >$@

.pch.hpp.gch: .pch.hpp config.h
	${AM_V_CXX}${LTCXXCOMPILE} -x c++-header -Wno-error $< -o $@.o
	@${LIBTOOL} ${AM_V_lt} --tag=CXX --mode=link sh -c 'mv -f "$$1" "$$2"' _ $@.lo $@ -o $@
	@test -z "${EXEEXT}" || rm -f $@${EXEEXT}

BUILT_SOURCES =  \
  .pch.hpp  \
  .pch.hpp.gch

MOSTLYCLEANFILES =  \
  .pch.hpp  \
  .pch.hpp.gch  \
  .pch.hpp.gch.lo

noinst_LTLIBRARIES =  \
  lib/librocket.la

lib_LTLIBRARIES =  \
  lib/libasteria.la

pkginclude_rocketdir = ${pkgincludedir}/rocket
pkginclude_rocket_HEADERS =  \
  asteria/rocket/compiler.h  \
  asteria/rocket/preprocessor_utilities.h  \
  asteria/rocket/assert.hpp  \
  asteria/rocket/throw.hpp  \
  asteria/rocket/utilities.hpp  \
  asteria/rocket/allocator_utilities.hpp  \
  asteria/rocket/hash_table_utilities.hpp  \
  asteria/rocket/variant.hpp  \
  asteria/rocket/fill_iterator.hpp  \
  asteria/rocket/unique_handle.hpp  \
  asteria/rocket/reference_counter.hpp  \
  asteria/rocket/cow_string.hpp  \
  asteria/rocket/cow_stringbuf.hpp  \
  asteria/rocket/cow_istringstream.hpp  \
  asteria/rocket/cow_ostringstream.hpp  \
  asteria/rocket/cow_stringstream.hpp  \
  asteria/rocket/cow_vector.hpp  \
  asteria/rocket/cow_hashmap.hpp  \
  asteria/rocket/unique_ptr.hpp  \
  asteria/rocket/refcnt_ptr.hpp  \
  asteria/rocket/refcnt_object.hpp  \
  asteria/rocket/prehashed_string.hpp  \
  asteria/rocket/static_vector.hpp  \
  asteria/rocket/optional.hpp  \
  asteria/rocket/array.hpp

pkginclude_rocket_platformdir = ${pkgincludedir}/rocket/platform
pkginclude_rocket_platform_HEADERS =  \
  asteria/rocket/platform/compiler_clang.h  \
  asteria/rocket/platform/compiler_gcc.h  \
  asteria/rocket/platform/compiler_msvc.h

lib_librocket_la_SOURCES =  \
  asteria/rocket/assert.cpp  \
  asteria/rocket/throw.cpp  \
  asteria/rocket/cow_string.cpp  \
  asteria/rocket/cow_stringbuf.cpp  \
  asteria/rocket/cow_istringstream.cpp  \
  asteria/rocket/cow_ostringstream.cpp  \
  asteria/rocket/cow_stringstream.cpp

lib_librocket_la_CXXFLAGS =  \
  ${AM_CXXFLAGS} -include .pch.hpp

pkginclude_HEADERS =  \
  asteria/src/precompiled.hpp  \
  asteria/src/fwd.hpp  \
  asteria/src/utilities.hpp

pkginclude_lldsdir = ${pkgincludedir}/llds
pkginclude_llds_HEADERS =  \
  asteria/src/llds/air_queue.hpp  \
  asteria/src/llds/variable_hashset.hpp  \
  asteria/src/llds/reference_dictionary.hpp

pkginclude_syntaxdir = ${pkgincludedir}/syntax
pkginclude_syntax_HEADERS =  \
  asteria/src/syntax/source_location.hpp  \
  asteria/src/syntax/xprunit.hpp  \
  asteria/src/syntax/statement.hpp

pkginclude_runtimedir = ${pkgincludedir}/runtime
pkginclude_runtime_HEADERS =  \
  asteria/src/runtime/rcbase.hpp  \
  asteria/src/runtime/abstract_opaque.hpp  \
  asteria/src/runtime/abstract_function.hpp  \
  asteria/src/runtime/placeholder.hpp  \
  asteria/src/runtime/value.hpp  \
  asteria/src/runtime/reference_root.hpp  \
  asteria/src/runtime/reference_modifier.hpp  \
  asteria/src/runtime/reference.hpp  \
  asteria/src/runtime/variable.hpp  \
  asteria/src/runtime/variable_callback.hpp  \
  asteria/src/runtime/collector.hpp  \
  asteria/src/runtime/backtrace_frame.hpp  \
  asteria/src/runtime/exception.hpp  \
  asteria/src/runtime/abstract_context.hpp  \
  asteria/src/runtime/analytic_context.hpp  \
  asteria/src/runtime/executive_context.hpp  \
  asteria/src/runtime/global_context.hpp  \
  asteria/src/runtime/random_number_generator.hpp  \
  asteria/src/runtime/generational_collector.hpp  \
  asteria/src/runtime/variadic_arguer.hpp  \
  asteria/src/runtime/evaluation_stack.hpp  \
  asteria/src/runtime/instantiated_function.hpp  \
  asteria/src/runtime/air_node.hpp

pkginclude_compilerdir = ${pkgincludedir}/compiler
pkginclude_compiler_HEADERS =  \
  asteria/src/compiler/parser_error.hpp  \
  asteria/src/compiler/token.hpp  \
  asteria/src/compiler/token_stream.hpp  \
  asteria/src/compiler/infix_element.hpp  \
  asteria/src/compiler/parser.hpp  \
  asteria/src/compiler/simple_source_file.hpp

pkginclude_librarydir = ${pkgincludedir}/library
pkginclude_library_HEADERS =  \
  asteria/src/library/argument_reader.hpp  \
  asteria/src/library/simple_binding_wrapper.hpp  \
  asteria/src/library/bindings_version.hpp  \
  asteria/src/library/bindings_gc.hpp  \
  asteria/src/library/bindings_debug.hpp  \
  asteria/src/library/bindings_chrono.hpp  \
  asteria/src/library/bindings_string.hpp  \
  asteria/src/library/bindings_array.hpp  \
  asteria/src/library/bindings_numeric.hpp  \
  asteria/src/library/bindings_math.hpp  \
  asteria/src/library/bindings_filesystem.hpp  \
  asteria/src/library/bindings_checksum.hpp  \
  asteria/src/library/bindings_json.hpp

lib_libasteria_la_SOURCES =  \
  asteria/src/utilities.cpp  \
  asteria/src/llds/air_queue.cpp  \
  asteria/src/llds/variable_hashset.cpp  \
  asteria/src/llds/reference_dictionary.cpp  \
  asteria/src/syntax/source_location.cpp  \
  asteria/src/syntax/xprunit.cpp  \
  asteria/src/syntax/statement.cpp  \
  asteria/src/runtime/rcbase.cpp  \
  asteria/src/runtime/abstract_opaque.cpp  \
  asteria/src/runtime/abstract_function.cpp  \
  asteria/src/runtime/placeholder.cpp  \
  asteria/src/runtime/value.cpp  \
  asteria/src/runtime/reference_root.cpp  \
  asteria/src/runtime/reference_modifier.cpp  \
  asteria/src/runtime/reference.cpp  \
  asteria/src/runtime/variable.cpp  \
  asteria/src/runtime/variable_callback.cpp  \
  asteria/src/runtime/collector.cpp  \
  asteria/src/runtime/backtrace_frame.cpp  \
  asteria/src/runtime/exception.cpp  \
  asteria/src/runtime/abstract_context.cpp  \
  asteria/src/runtime/analytic_context.cpp  \
  asteria/src/runtime/executive_context.cpp  \
  asteria/src/runtime/global_context.cpp  \
  asteria/src/runtime/random_number_generator.cpp  \
  asteria/src/runtime/generational_collector.cpp  \
  asteria/src/runtime/variadic_arguer.cpp  \
  asteria/src/runtime/evaluation_stack.cpp  \
  asteria/src/runtime/instantiated_function.cpp  \
  asteria/src/runtime/air_node.cpp  \
  asteria/src/compiler/parser_error.cpp  \
  asteria/src/compiler/token.cpp  \
  asteria/src/compiler/token_stream.cpp  \
  asteria/src/compiler/infix_element.cpp  \
  asteria/src/compiler/parser.cpp  \
  asteria/src/compiler/simple_source_file.cpp  \
  asteria/src/library/argument_reader.cpp  \
  asteria/src/library/simple_binding_wrapper.cpp  \
  asteria/src/library/bindings_version.cpp  \
  asteria/src/library/bindings_gc.cpp  \
  asteria/src/library/bindings_debug.cpp  \
  asteria/src/library/bindings_chrono.cpp  \
  asteria/src/library/bindings_string.cpp  \
  asteria/src/library/bindings_array.cpp  \
  asteria/src/library/bindings_numeric.cpp  \
  asteria/src/library/bindings_math.cpp  \
  asteria/src/library/bindings_filesystem.cpp  \
  asteria/src/library/bindings_checksum.cpp  \
  asteria/src/library/bindings_json.cpp

lib_libasteria_la_CXXFLAGS =  \
  ${AM_CXXFLAGS} -include .pch.hpp

lib_libasteria_la_LIBADD =  \
  lib/librocket.la

lib_libasteria_la_LDFLAGS =  \
  -Wl,--no-undefined -no-undefined -version-info 0:0:0

bin_PROGRAMS =  \
  bin/asteria

bin_asteria_SOURCES =  \
  asteria/src/main.cpp

check_HEADERS =  \
  asteria/test/test_utilities.hpp

check_PROGRAMS =  \
  asteria/test/utilities.test  \
  asteria/test/value.test  \
  asteria/test/variable.test  \
  asteria/test/reference.test  \
  asteria/test/token_stream.test  \
  asteria/test/parser.test  \
  asteria/test/simple_source_file.test  \
  asteria/test/garbage_collection.test  \
  asteria/test/std_library.test  \
  asteria/test/varg.test  \
  asteria/test/operators.test  \
  asteria/test/proper_tail_call.test  \
  asteria/test/bindings_chrono.test  \
  asteria/test/bindings_string.test  \
  asteria/test/bindings_array.test  \
  asteria/test/bindings_numeric.test  \
  asteria/test/bindings_math.test  \
  asteria/test/bindings_filesystem.test  \
  asteria/test/bindings_checksum.test  \
  asteria/test/bindings_json.test

TESTS = ${check_PROGRAMS}

EXTRA_DIST =  \
  doc/operator-precedence.txt  \
  doc/standard-library.txt  \
  doc/syntax.txt
