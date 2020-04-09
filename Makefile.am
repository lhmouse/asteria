ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -pipe -pthread -Wall -Wextra  \
  -D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE -D_WIN32_WINNT=0x0600  \
  -f{inline-limit=2400,strict-{aliasing,overflow},merge-all-constants}  \
  -fno-{math-errno,trapping-math,devirtualize-speculatively}  \
  -Werror={{,sign-}conversion,write-strings,return-type,double-promotion}  \
  -W{invalid-pch,switch-enum,suggest-attribute=noreturn,undef,shadow,missing-field-initializers}  \
  -Wunused-{function,label,local-typedefs,{,but-set-}{variable,parameter}}
AM_CXXFLAGS = -std=c++1z  \
  -Wzero-as-null-pointer-constant -Wno-redundant-move  \
  -Werror={non-virtual-dtor,missing-declarations}
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

CLEANFILES =  \
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
  asteria/rocket/utilities.hpp  \
  asteria/rocket/throw.hpp  \
  asteria/rocket/allocator_utilities.hpp  \
  asteria/rocket/hash_table_utilities.hpp  \
  asteria/rocket/variant.hpp  \
  asteria/rocket/fill_iterator.hpp  \
  asteria/rocket/unique_handle.hpp  \
  asteria/rocket/unique_posix_file.hpp  \
  asteria/rocket/unique_posix_dir.hpp  \
  asteria/rocket/unique_posix_fd.hpp  \
  asteria/rocket/reference_counter.hpp  \
  asteria/rocket/char_traits.hpp  \
  asteria/rocket/cow_string.hpp  \
  asteria/rocket/cow_vector.hpp  \
  asteria/rocket/cow_hashmap.hpp  \
  asteria/rocket/unique_ptr.hpp  \
  asteria/rocket/refcnt_ptr.hpp  \
  asteria/rocket/prehashed_string.hpp  \
  asteria/rocket/static_vector.hpp  \
  asteria/rocket/optional.hpp  \
  asteria/rocket/array.hpp  \
  asteria/rocket/reference_wrapper.hpp  \
  asteria/rocket/linear_buffer.hpp  \
  asteria/rocket/tinybuf.hpp  \
  asteria/rocket/tinybuf_str.hpp  \
  asteria/rocket/tinybuf_file.hpp  \
  asteria/rocket/ascii_numput.hpp  \
  asteria/rocket/ascii_numget.hpp  \
  asteria/rocket/tinyfmt.hpp  \
  asteria/rocket/tinyfmt_str.hpp  \
  asteria/rocket/tinyfmt_file.hpp  \
  asteria/rocket/format.hpp  \
  asteria/rocket/atomic_flag.hpp

pkginclude_rocket_platformdir = ${pkgincludedir}/rocket/platform
pkginclude_rocket_platform_HEADERS =  \
  asteria/rocket/platform/compiler_clang.h  \
  asteria/rocket/platform/compiler_gcc.h  \
  asteria/rocket/platform/compiler_msvc.h

pkginclude_rocket_detailsdir = ${pkgincludedir}/rocket/details
pkginclude_rocket_details_HEADERS =  \
  asteria/rocket/details/utilities.tcc  \
  asteria/rocket/details/allocator_utilities.tcc  \
  asteria/rocket/details/variant.tcc  \
  asteria/rocket/details/unique_handle.tcc  \
  asteria/rocket/details/char_traits.tcc  \
  asteria/rocket/details/cow_string.tcc  \
  asteria/rocket/details/cow_vector.tcc  \
  asteria/rocket/details/cow_hashmap.tcc  \
  asteria/rocket/details/prehashed_string.tcc  \
  asteria/rocket/details/unique_ptr.tcc  \
  asteria/rocket/details/refcnt_ptr.tcc  \
  asteria/rocket/details/static_vector.tcc  \
  asteria/rocket/details/array.tcc  \
  asteria/rocket/details/linear_buffer.tcc

lib_librocket_la_SOURCES =  \
  asteria/rocket/assert.cpp  \
  asteria/rocket/throw.cpp  \
  asteria/rocket/cow_string.cpp  \
  asteria/rocket/linear_buffer.cpp  \
  asteria/rocket/tinybuf.cpp  \
  asteria/rocket/tinybuf_str.cpp  \
  asteria/rocket/tinybuf_file.cpp  \
  asteria/rocket/ascii_numput.cpp  \
  asteria/rocket/ascii_numget.cpp  \
  asteria/rocket/tinyfmt.cpp  \
  asteria/rocket/tinyfmt_str.cpp  \
  asteria/rocket/tinyfmt_file.cpp  \
  asteria/rocket/format.cpp

lib_librocket_la_CXXFLAGS =  \
  ${AM_CXXFLAGS} -include .pch.hpp

pkginclude_HEADERS =  \
  asteria/src/precompiled.hpp  \
  asteria/src/fwd.hpp  \
  asteria/src/utilities.hpp  \
  asteria/src/recursion_sentry.hpp  \
  asteria/src/value.hpp  \
  asteria/src/source_location.hpp  \
  asteria/src/simple_script.hpp

pkginclude_lldsdir = ${pkgincludedir}/llds
pkginclude_llds_HEADERS =  \
  asteria/src/llds/variable_hashset.hpp  \
  asteria/src/llds/reference_dictionary.hpp  \
  asteria/src/llds/avmc_queue.hpp

pkginclude_runtimedir = ${pkgincludedir}/runtime
pkginclude_runtime_HEADERS =  \
  asteria/src/runtime/enums.hpp  \
  asteria/src/runtime/abstract_hooks.hpp  \
  asteria/src/runtime/reference_root.hpp  \
  asteria/src/runtime/reference_modifier.hpp  \
  asteria/src/runtime/reference.hpp  \
  asteria/src/runtime/variable.hpp  \
  asteria/src/runtime/variable_callback.hpp  \
  asteria/src/runtime/ptc_arguments.hpp  \
  asteria/src/runtime/collector.hpp  \
  asteria/src/runtime/backtrace_frame.hpp  \
  asteria/src/runtime/runtime_error.hpp  \
  asteria/src/runtime/abstract_context.hpp  \
  asteria/src/runtime/analytic_context.hpp  \
  asteria/src/runtime/executive_context.hpp  \
  asteria/src/runtime/global_context.hpp  \
  asteria/src/runtime/genius_collector.hpp  \
  asteria/src/runtime/random_engine.hpp  \
  asteria/src/runtime/loader_lock.hpp  \
  asteria/src/runtime/variadic_arguer.hpp  \
  asteria/src/runtime/evaluation_stack.hpp  \
  asteria/src/runtime/instantiated_function.hpp  \
  asteria/src/runtime/air_node.hpp  \
  asteria/src/runtime/argument_reader.hpp

pkginclude_compilerdir = ${pkgincludedir}/compiler
pkginclude_compiler_HEADERS =  \
  asteria/src/compiler/enums.hpp  \
  asteria/src/compiler/parser_error.hpp  \
  asteria/src/compiler/token.hpp  \
  asteria/src/compiler/token_stream.hpp  \
  asteria/src/compiler/expression_unit.hpp  \
  asteria/src/compiler/statement.hpp  \
  asteria/src/compiler/infix_element.hpp  \
  asteria/src/compiler/statement_sequence.hpp

pkginclude_librarydir = ${pkgincludedir}/library
pkginclude_library_HEADERS =  \
  asteria/src/library/version.hpp  \
  asteria/src/library/system.hpp  \
  asteria/src/library/debug.hpp  \
  asteria/src/library/chrono.hpp  \
  asteria/src/library/string.hpp  \
  asteria/src/library/array.hpp  \
  asteria/src/library/numeric.hpp  \
  asteria/src/library/math.hpp  \
  asteria/src/library/filesystem.hpp  \
  asteria/src/library/checksum.hpp  \
  asteria/src/library/json.hpp  \
  asteria/src/library/io.hpp

lib_libasteria_la_SOURCES =  \
  asteria/src/fwd.cpp  \
  asteria/src/utilities.cpp  \
  asteria/src/recursion_sentry.cpp  \
  asteria/src/value.cpp  \
  asteria/src/source_location.cpp  \
  asteria/src/simple_script.cpp  \
  asteria/src/llds/variable_hashset.cpp  \
  asteria/src/llds/reference_dictionary.cpp  \
  asteria/src/llds/avmc_queue.cpp  \
  asteria/src/runtime/enums.cpp  \
  asteria/src/runtime/abstract_hooks.cpp  \
  asteria/src/runtime/reference_root.cpp  \
  asteria/src/runtime/reference_modifier.cpp  \
  asteria/src/runtime/reference.cpp  \
  asteria/src/runtime/variable.cpp  \
  asteria/src/runtime/variable_callback.cpp  \
  asteria/src/runtime/ptc_arguments.cpp  \
  asteria/src/runtime/collector.cpp  \
  asteria/src/runtime/backtrace_frame.cpp  \
  asteria/src/runtime/runtime_error.cpp  \
  asteria/src/runtime/abstract_context.cpp  \
  asteria/src/runtime/analytic_context.cpp  \
  asteria/src/runtime/executive_context.cpp  \
  asteria/src/runtime/global_context.cpp  \
  asteria/src/runtime/genius_collector.cpp  \
  asteria/src/runtime/random_engine.cpp  \
  asteria/src/runtime/loader_lock.cpp  \
  asteria/src/runtime/variadic_arguer.cpp  \
  asteria/src/runtime/evaluation_stack.cpp  \
  asteria/src/runtime/instantiated_function.cpp  \
  asteria/src/runtime/air_node.cpp  \
  asteria/src/runtime/argument_reader.cpp  \
  asteria/src/compiler/enums.cpp  \
  asteria/src/compiler/parser_error.cpp  \
  asteria/src/compiler/token.cpp  \
  asteria/src/compiler/token_stream.cpp  \
  asteria/src/compiler/expression_unit.cpp  \
  asteria/src/compiler/statement.cpp  \
  asteria/src/compiler/infix_element.cpp  \
  asteria/src/compiler/statement_sequence.cpp  \
  asteria/src/library/version.cpp  \
  asteria/src/library/system.cpp  \
  asteria/src/library/debug.cpp  \
  asteria/src/library/chrono.cpp  \
  asteria/src/library/string.cpp  \
  asteria/src/library/array.cpp  \
  asteria/src/library/numeric.cpp  \
  asteria/src/library/math.cpp  \
  asteria/src/library/filesystem.cpp  \
  asteria/src/library/checksum.cpp  \
  asteria/src/library/json.cpp  \
  asteria/src/library/io.cpp

lib_libasteria_la_CXXFLAGS =  \
  ${AM_CXXFLAGS} -include .pch.hpp

lib_libasteria_la_LIBADD =  \
  lib/librocket.la

lib_libasteria_la_LDFLAGS =  \
  -Wl,--no-undefined -no-undefined -version-info 0:0:0

bin_PROGRAMS =  \
  bin/asteria

bin_asteria_SOURCES =  \
  asteria/src/repl.cpp

check_HEADERS =  \
  asteria/test/test_utilities.hpp

check_PROGRAMS =  \
  asteria/test/utilities.test  \
  asteria/test/value.test  \
  asteria/test/variable.test  \
  asteria/test/reference.test  \
  asteria/test/token_stream.test  \
  asteria/test/statement_sequence.test  \
  asteria/test/simple_script.test  \
  asteria/test/gc.test  \
  asteria/test/varg.test  \
  asteria/test/operators.test  \
  asteria/test/proper_tail_call.test  \
  asteria/test/stack_overflow.test  \
  asteria/test/structured_binding.test  \
  asteria/test/global_identifier.test  \
  asteria/test/variadic_function_call.test  \
  asteria/test/defer.test  \
  asteria/test/defer_ptc.test  \
  asteria/test/trailing_commas.test  \
  asteria/test/system.test  \
  asteria/test/chrono.test  \
  asteria/test/string.test  \
  asteria/test/array.test  \
  asteria/test/numeric.test  \
  asteria/test/math.test  \
  asteria/test/filesystem.test  \
  asteria/test/checksum.test  \
  asteria/test/json.test  \
  asteria/test/import.test  \
  asteria/test/github_65.test  \
  asteria/test/github_71.test  \
  asteria/test/github_78.test  \
  asteria/test/github_84.test  \
  asteria/test/github_85.test  \
  asteria/test/github_98.test

TESTS = ${check_PROGRAMS}

EXTRA_DIST =  \
  asteria/doc/operator-precedence.txt  \
  asteria/doc/standard-library.txt  \
  asteria/doc/syntax.txt  \
  asteria/doc/examples.txt
