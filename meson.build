## Copyright (C) 2024-2025, LH_Mouse <lh_mouse@126.com>
##
## Permission to use, copy, modify, and/or distribute this
## software for any purpose with or without fee is hereby granted.
##
## THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
## WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
## THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
## CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
## LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
## NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
## CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

project('asteria',
        'cpp',
        version: '2.0.alpha',
        license: 'BSD-3-Clause',
        default_options: [ 'cpp_std=c++17', 'warning_level=2' ],
        meson_version: '>= 1.3.0')

#===========================================================
# List of files
#===========================================================
rocket_include = [
  'rocket/details/xcompiler_clang.i', 'rocket/details/xcompiler_gcc.i',
  'rocket/details/xcompiler_msvc.i', 'rocket/details/fwd.ipp',
  'rocket/details/xallocator.ipp', 'rocket/details/xstring.ipp',
  'rocket/details/variant.ipp', 'rocket/details/unique_handle.ipp',
  'rocket/details/cow_string.ipp', 'rocket/details/cow_vector.ipp',
  'rocket/details/cow_hashmap.ipp', 'rocket/details/prehashed_string.ipp',
  'rocket/details/unique_ptr.ipp', 'rocket/details/refcnt_ptr.ipp',
  'rocket/details/static_vector.ipp', 'rocket/details/array.ipp',
  'rocket/details/tinyfmt.ipp', 'rocket/details/linear_buffer.ipp',
  'rocket/xcompiler.h', 'rocket/fwd.hpp', 'rocket/xassert.hpp', 'rocket/xthrow.hpp',
  'rocket/xallocator.hpp', 'rocket/xstring.hpp', 'rocket/xhashtable.hpp',
  'rocket/xuchar.hpp', 'rocket/xascii.hpp', 'rocket/xmemory.hpp',
  'rocket/variant.hpp', 'rocket/unique_handle.hpp', 'rocket/unique_posix_file.hpp',
  'rocket/unique_posix_dir.hpp', 'rocket/unique_posix_fd.hpp',
  'rocket/reference_counter.hpp', 'rocket/cow_string.hpp', 'rocket/cow_vector.hpp',
  'rocket/cow_hashmap.hpp', 'rocket/unique_ptr.hpp', 'rocket/refcnt_ptr.hpp',
  'rocket/prehashed_string.hpp', 'rocket/static_vector.hpp', 'rocket/optional.hpp',
  'rocket/array.hpp', 'rocket/reference_wrapper.hpp', 'rocket/linear_buffer.hpp',
  'rocket/tinybuf_base.hpp', 'rocket/tinybuf.hpp', 'rocket/tinybuf_str.hpp',
  'rocket/tinybuf_file.hpp', 'rocket/tinybuf_ln.hpp', 'rocket/ascii_numput.hpp',
  'rocket/ascii_numget.hpp', 'rocket/tinyfmt.hpp', 'rocket/tinyfmt_str.hpp',
  'rocket/tinyfmt_file.hpp', 'rocket/tinyfmt_ln.hpp', 'rocket/atomic.hpp',
  'rocket/mutex.hpp', 'rocket/recursive_mutex.hpp', 'rocket/condition_variable.hpp',
  'rocket/once_flag.hpp', 'rocket/shared_function.hpp',
  'rocket/static_char_buffer.hpp' ]

asteria_include = [
  'asteria/details/value.ipp', 'asteria/details/variable_hashmap.ipp',
  'asteria/details/reference_dictionary.ipp',
  'asteria/fwd.hpp', 'asteria/utils.hpp', 'asteria/recursion_sentry.hpp',
  'asteria/value.hpp', 'asteria/source_location.hpp', 'asteria/simple_script.hpp',
  'asteria/llds/variable_hashmap.hpp', 'asteria/llds/reference_dictionary.hpp',
  'asteria/llds/reference_stack.hpp', 'asteria/llds/avm_rod.hpp',
  'asteria/runtime/enums.hpp', 'asteria/runtime/abstract_hooks.hpp',
  'asteria/runtime/reference.hpp', 'asteria/runtime/subscript.hpp',
  'asteria/runtime/variable.hpp', 'asteria/runtime/ptc_arguments.hpp',
  'asteria/runtime/runtime_error.hpp', 'asteria/runtime/abstract_context.hpp',
  'asteria/runtime/analytic_context.hpp', 'asteria/runtime/executive_context.hpp',
  'asteria/runtime/global_context.hpp', 'asteria/runtime/garbage_collector.hpp',
  'asteria/runtime/random_engine.hpp', 'asteria/runtime/module_loader.hpp',
  'asteria/runtime/variadic_arguer.hpp', 'asteria/runtime/instantiated_function.hpp',
  'asteria/runtime/air_node.hpp', 'asteria/runtime/air_optimizer.hpp',
  'asteria/runtime/argument_reader.hpp', 'asteria/runtime/binding_generator.hpp',
  'asteria/compiler/enums.hpp', 'asteria/compiler/compiler_error.hpp',
  'asteria/compiler/token.hpp', 'asteria/compiler/token_stream.hpp',
  'asteria/compiler/expression_unit.hpp', 'asteria/compiler/statement.hpp',
  'asteria/compiler/infix_element.hpp', 'asteria/compiler/statement_sequence.hpp',
  'asteria/library/version.hpp', 'asteria/library/gc.hpp', 'asteria/library/system.hpp',
  'asteria/library/debug.hpp', 'asteria/library/chrono.hpp', 'asteria/library/string.hpp',
  'asteria/library/array.hpp', 'asteria/library/numeric.hpp', 'asteria/library/math.hpp',
  'asteria/library/filesystem.hpp', 'asteria/library/checksum.hpp',
  'asteria/library/json.hpp', 'asteria/library/io.hpp', 'asteria/library/zlib.hpp',
  'asteria/library/ini.hpp', 'asteria/library/csv.hpp', 'asteria/library/rsa.hpp' ]

rocket_src = [
  'rocket/xassert.cpp', 'rocket/xthrow.cpp', 'rocket/xuchar.cpp', 'rocket/xmemory.cpp',
  'rocket/cow_string.cpp', 'rocket/linear_buffer.cpp', 'rocket/tinybuf.cpp',
  'rocket/tinybuf_str.cpp', 'rocket/tinybuf_file.cpp', 'rocket/tinybuf_ln.cpp',
  'rocket/ascii_numput.cpp', 'rocket/ascii_numget.cpp', 'rocket/tinyfmt.cpp',
  'rocket/tinyfmt_str.cpp', 'rocket/tinyfmt_file.cpp', 'rocket/tinyfmt_ln.cpp',
  'rocket/prehashed_string.cpp' ]

asteria_src = [
  'asteria/fwd.cpp', 'asteria/utils.cpp', 'asteria/recursion_sentry.cpp',
  'asteria/value.cpp', 'asteria/source_location.cpp', 'asteria/simple_script.cpp',
  'asteria/llds/variable_hashmap.cpp', 'asteria/llds/reference_dictionary.cpp',
  'asteria/llds/reference_stack.cpp', 'asteria/llds/avm_rod.cpp',
  'asteria/runtime/abstract_hooks.cpp', 'asteria/runtime/reference.cpp',
  'asteria/runtime/subscript.cpp', 'asteria/runtime/variable.cpp',
  'asteria/runtime/ptc_arguments.cpp', 'asteria/runtime/runtime_error.cpp',
  'asteria/runtime/abstract_context.cpp', 'asteria/runtime/analytic_context.cpp',
  'asteria/runtime/executive_context.cpp', 'asteria/runtime/global_context.cpp',
  'asteria/runtime/garbage_collector.cpp', 'asteria/runtime/random_engine.cpp',
  'asteria/runtime/module_loader.cpp', 'asteria/runtime/variadic_arguer.cpp',
  'asteria/runtime/instantiated_function.cpp', 'asteria/runtime/air_node.cpp',
  'asteria/runtime/air_optimizer.cpp', 'asteria/runtime/argument_reader.cpp',
  'asteria/runtime/binding_generator.cpp', 'asteria/compiler/compiler_error.cpp',
  'asteria/compiler/token.cpp', 'asteria/compiler/token_stream.cpp',
  'asteria/compiler/expression_unit.cpp', 'asteria/compiler/statement.cpp',
  'asteria/compiler/infix_element.cpp', 'asteria/compiler/statement_sequence.cpp',
  'asteria/library/version.cpp', 'asteria/library/gc.cpp', 'asteria/library/system.cpp',
  'asteria/library/debug.cpp', 'asteria/library/chrono.cpp', 'asteria/library/string.cpp',
  'asteria/library/array.cpp', 'asteria/library/numeric.cpp', 'asteria/library/math.cpp',
  'asteria/library/filesystem.cpp', 'asteria/library/checksum.cpp',
  'asteria/library/json.cpp', 'asteria/library/io.cpp', 'asteria/library/zlib.cpp',
  'asteria/library/ini.cpp', 'asteria/library/csv.cpp', 'asteria/library/rsa.cpp' ]

repl_src = [
  'repl/fwd.hpp', 'repl/globals.cpp', 'repl/single.cpp', 'repl/interact.cpp',
  'repl/commands.cpp', 'repl/main.cpp', 'repl/libedit.cpp' ]

test_src = [
  'test/xstring.cpp', 'test/ascii_numget.cpp', 'test/ascii_numget_float.cpp',
  'test/ascii_numget_double.cpp', 'test/ascii_numput_float.cpp',
  'test/ascii_numput_double.cpp', 'test/utils.cpp', 'test/value.cpp', 'test/variable.cpp',
  'test/reference.cpp', 'test/token_stream.cpp', 'test/statement_sequence.cpp',
  'test/simple_script.cpp', 'test/gc.cpp', 'test/gc2.cpp', 'test/gc_loop.cpp',
  'test/varg.cpp', 'test/vcall.cpp', 'test/operators_o0.cpp', 'test/operators_o1.cpp',
  'test/operators_o2.cpp', 'test/proper_tail_call.cpp', 'test/stack_overflow.cpp',
  'test/structured_binding.cpp', 'test/global_identifier.cpp', 'test/rsa.cpp',
  'test/variadic_function_call.cpp', 'test/defer.cpp', 'test/defer_ptc.cpp',
  'test/trailing_commas.cpp', 'test/system.cpp', 'test/chrono.cpp', 'test/string.cpp',
  'test/array.cpp', 'test/numeric.cpp', 'test/math.cpp', 'test/filesystem.cpp',
  'test/checksum.cpp', 'test/json.cpp', 'test/import.cpp', 'test/bypassed_variable.cpp',
  'test/github_71.cpp', 'test/github_78.cpp', 'test/github_84.cpp', 'test/github_85.cpp',
  'test/github_98.cpp', 'test/github_101.cpp', 'test/ref.cpp', 'test/github_105.cpp',
  'test/github_108.cpp', 'test/github_113.cpp', 'test/github_116.cpp', 'test/zlib.cpp',
  'test/c_stack_overflow.cpp', 'test/var_mod.cpp', 'test/ini.cpp', 'test/csv.cpp',
  'test/binding_variable.cpp', 'test/ptc_hooks_throw.cpp', 'test/ptc_hooks_return.cpp',
  'test/switch_defer.cpp', 'test/for_each.cpp', 'test/github_102.cpp',
  'test/github_308.cpp', 'test/github_312.cpp', 'test/github_321.cpp' ]

#===========================================================
# Global configuration
#===========================================================
ver = configuration_data()
ver.set('abi_major', meson.project_version().split('.')[0])
ver.set('abi_minor', meson.project_version().split('.')[1])
ver.set_quoted('abi_string', meson.project_version())

cxx = meson.get_compiler('cpp')
cxx_is_i386 = cxx.has_define('__i386__')
cxx_is_amd64 = cxx.has_define('__amd64__')

add_project_arguments(
    '-Winvalid-pch', '-D_FILE_OFFSET_BITS=64', '-D_POSIX_C_SOURCE=200809',
    '-D_DEFAULT_SOURCE', '-D_DARWIN_C_SOURCE',
    cxx.get_supported_arguments(
      '-fno-elide-constructors', '-fvisibility-inlines-hidden', '-fstrict-aliasing',
      '-fno-stack-protector', '-fstrict-overflow', '-fmerge-all-constants',
      '-frounding-math', '-fno-math-errno', '-fno-ident', '-fno-trapping-math',
      '-fno-devirtualize-speculatively', '-fno-semantic-interposition', '-fno-plt',
      '-fno-omit-frame-pointer', '-momit-leaf-frame-pointer', '-masm=intel',
      '-mno-outline-atomics', '-Wno-option-ignored',
      '-Werror=conversion', '-Werror=sign-compare', '-Werror=sign-conversion',
      '-Werror=write-strings', '-Werror=return-type', '-Werror=double-promotion',
      '-Werror=non-virtual-dtor', '-Wzero-as-null-pointer-constant',
      '-Wmissing-declarations', '-Wmissing-field-initializers', '-Wshadow',
      '-Wsuggest-attribute=noreturn', '-Wswitch-enum', '-Wunused-function',
      '-Wunused-label', '-Wunused-local-typedefs', '-Wunused-but-set-variable',
      '-Wunused-but-set-parameter', '-Wno-redundant-move', '-Wno-missing-braces',
      '-Wno-unqualified-std-cast-call', '-Wno-string-plus-int',
      '-Wno-bitwise-instead-of-logical'),
    language: 'cpp')

if cxx_is_i386
  add_project_arguments('-msse2', '-mfpmath=sse', language: 'cpp')
endif

if get_option('enable-avx2') and (cxx_is_i386 or cxx_is_amd64)
  add_project_arguments('-mavx2', language: 'cpp')
endif

if not cxx.has_header_symbol('uchar.h', 'mbrtoc16')
  add_project_arguments('-DASTERIA_NO_UCHAR', language: 'cpp')
endif

if get_option('enable-debug-checks')
  add_project_arguments('-D_GLIBCXX_DEBUG', '-D_LIBCPP_DEBUG', language: 'cpp')
endif

#===========================================================
# Dependencies
#===========================================================
dep_zlib = dependency('zlib', version: '>= 1.2.9')
dep_pcre2 = dependency('libpcre2-8')
dep_openssl = dependency('openssl')
dep_iconv = dependency('iconv')
dep_libedit = disabler()

if get_option('enable-repl')
  dep_libedit = dependency('libedit', 'wineditline',
        not_found_message: 'libedit is required to build the REPL. If you do not '
                           + 'need it, use `meson setup -Denable-repl=false`.')
endif

#===========================================================
# Rules for headers
#===========================================================
configure_file(input: 'asteria/version.h.in',
               configuration: ver,
               output: 'version.h',
               install: false)

install_headers(rocket_include, preserve_path: true)
install_headers(asteria_include, preserve_path: true)
install_headers(meson.current_build_dir() / 'version.h', subdir: 'asteria')

#===========================================================
# Rules for 'lib/libasteria.so'
#===========================================================
lib_asteria = library('asteria',
      cpp_pch: 'asteria/xprecompiled.hpp',
      sources: [ rocket_src, asteria_src ],
      dependencies: [ dep_zlib, dep_pcre2, dep_openssl, dep_iconv ],
      soversion: ver.get('abi_major'),
      version: '.'.join([ ver.get('abi_major'), ver.get('abi_minor'), '0' ]),
      install: true)

import('pkgconfig').generate(lib_asteria,
      name: 'asteria',
      url: 'https://github.com/lhmouse/asteria',
      description: 'The Asteria Programming Language is a heavyweight, slow and '
                   + 'incomplete replacement for Lua.')

#===========================================================
# Rules for 'bin/asteria'
#===========================================================
executable('asteria',
      cpp_pch: 'asteria/xprecompiled.hpp',
      sources: [ repl_src ],
      dependencies: [ dep_libedit ],
      link_with: lib_asteria,
      install: true)

#===========================================================
# Rules for tests
#===========================================================
foreach src : test_src
  test_exe = executable(src.underscorify(), src,
      link_with: lib_asteria,
      build_by_default: false,
      install: false)

  test('..' / src, test_exe)
endforeach
