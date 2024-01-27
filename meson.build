project('asteria', 'cpp', version: '1.0.alpha-0')

#===========================================================
# List of files
#===========================================================
rocket_include = [
  'rocket/details/xcompiler_clang.i',
  'rocket/details/xcompiler_gcc.i',
  'rocket/details/xcompiler_msvc.i',
  'rocket/details/fwd.ipp',
  'rocket/details/xallocator.ipp',
  'rocket/details/xstring.ipp',
  'rocket/details/variant.ipp',
  'rocket/details/unique_handle.ipp',
  'rocket/details/cow_string.ipp',
  'rocket/details/cow_vector.ipp',
  'rocket/details/cow_hashmap.ipp',
  'rocket/details/prehashed_string.ipp',
  'rocket/details/unique_ptr.ipp',
  'rocket/details/refcnt_ptr.ipp',
  'rocket/details/static_vector.ipp',
  'rocket/details/array.ipp',
  'rocket/details/tinyfmt.ipp',
  'rocket/details/linear_buffer.ipp',
  'rocket/xcompiler.h',
  'rocket/fwd.hpp',
  'rocket/xassert.hpp',
  'rocket/xthrow.hpp',
  'rocket/xallocator.hpp',
  'rocket/xstring.hpp',
  'rocket/xhashtable.hpp',
  'rocket/xuchar.hpp',
  'rocket/xascii.hpp',
  'rocket/xmemory.hpp',
  'rocket/variant.hpp',
  'rocket/unique_handle.hpp',
  'rocket/unique_posix_file.hpp',
  'rocket/unique_posix_dir.hpp',
  'rocket/unique_posix_fd.hpp',
  'rocket/reference_counter.hpp',
  'rocket/cow_string.hpp',
  'rocket/cow_vector.hpp',
  'rocket/cow_hashmap.hpp',
  'rocket/unique_ptr.hpp',
  'rocket/refcnt_ptr.hpp',
  'rocket/prehashed_string.hpp',
  'rocket/static_vector.hpp',
  'rocket/optional.hpp',
  'rocket/array.hpp',
  'rocket/reference_wrapper.hpp',
  'rocket/linear_buffer.hpp',
  'rocket/tinybuf_base.hpp',
  'rocket/tinybuf.hpp',
  'rocket/tinybuf_str.hpp',
  'rocket/tinybuf_file.hpp',
  'rocket/tinybuf_ln.hpp',
  'rocket/ascii_numput.hpp',
  'rocket/ascii_numget.hpp',
  'rocket/tinyfmt.hpp',
  'rocket/tinyfmt_str.hpp',
  'rocket/tinyfmt_file.hpp',
  'rocket/tinyfmt_ln.hpp',
  'rocket/atomic.hpp',
  'rocket/mutex.hpp',
  'rocket/recursive_mutex.hpp',
  'rocket/condition_variable.hpp',
  'rocket/once_flag.hpp',
  'rocket/bit_mask.hpp',
]

rocket_src = [
  'rocket/xassert.cpp',
  'rocket/xthrow.cpp',
  'rocket/xuchar.cpp',
  'rocket/xmemory.cpp',
  'rocket/cow_string.cpp',
  'rocket/linear_buffer.cpp',
  'rocket/tinybuf.cpp',
  'rocket/tinybuf_str.cpp',
  'rocket/tinybuf_file.cpp',
  'rocket/tinybuf_ln.cpp',
  'rocket/ascii_numput.cpp',
  'rocket/ascii_numget.cpp',
  'rocket/tinyfmt.cpp',
  'rocket/tinyfmt_str.cpp',
  'rocket/tinyfmt_file.cpp',
  'rocket/tinyfmt_ln.cpp',
  'rocket/prehashed_string.cpp',
]

asteria_include = [
  'asteria/details/value.ipp',
  'asteria/details/variable_hashmap.ipp',
  'asteria/details/reference_dictionary.ipp',
  'asteria/details/avm_rod.ipp',
  'asteria/precompiled.hpp',
  'asteria/fwd.hpp',
  'asteria/utils.hpp',
  'asteria/recursion_sentry.hpp',
  'asteria/value.hpp',
  'asteria/source_location.hpp',
  'asteria/simple_script.hpp',
  'asteria/llds/variable_hashmap.hpp',
  'asteria/llds/reference_dictionary.hpp',
  'asteria/llds/reference_stack.hpp',
  'asteria/llds/avm_rod.hpp',
  'asteria/runtime/enums.hpp',
  'asteria/runtime/abstract_hooks.hpp',
  'asteria/runtime/reference.hpp',
  'asteria/runtime/reference_modifier.hpp',
  'asteria/runtime/variable.hpp',
  'asteria/runtime/ptc_arguments.hpp',
  'asteria/runtime/runtime_error.hpp',
  'asteria/runtime/abstract_context.hpp',
  'asteria/runtime/analytic_context.hpp',
  'asteria/runtime/executive_context.hpp',
  'asteria/runtime/global_context.hpp',
  'asteria/runtime/garbage_collector.hpp',
  'asteria/runtime/random_engine.hpp',
  'asteria/runtime/module_loader.hpp',
  'asteria/runtime/variadic_arguer.hpp',
  'asteria/runtime/instantiated_function.hpp',
  'asteria/runtime/air_node.hpp',
  'asteria/runtime/air_optimizer.hpp',
  'asteria/runtime/argument_reader.hpp',
  'asteria/runtime/binding_generator.hpp',
  'asteria/compiler/enums.hpp',
  'asteria/compiler/compiler_error.hpp',
  'asteria/compiler/token.hpp',
  'asteria/compiler/token_stream.hpp',
  'asteria/compiler/expression_unit.hpp',
  'asteria/compiler/statement.hpp',
  'asteria/compiler/infix_element.hpp',
  'asteria/compiler/statement_sequence.hpp',
  'asteria/library/version.hpp',
  'asteria/library/gc.hpp',
  'asteria/library/system.hpp',
  'asteria/library/debug.hpp',
  'asteria/library/chrono.hpp',
  'asteria/library/string.hpp',
  'asteria/library/array.hpp',
  'asteria/library/numeric.hpp',
  'asteria/library/math.hpp',
  'asteria/library/filesystem.hpp',
  'asteria/library/checksum.hpp',
  'asteria/library/json.hpp',
  'asteria/library/io.hpp',
  'asteria/library/zlib.hpp',
  'asteria/library/ini.hpp',
  'asteria/library/csv.hpp',
]

asteria_src = [
  'asteria/fwd.cpp',
  'asteria/utils.cpp',
  'asteria/recursion_sentry.cpp',
  'asteria/value.cpp',
  'asteria/source_location.cpp',
  'asteria/simple_script.cpp',
  'asteria/llds/variable_hashmap.cpp',
  'asteria/llds/reference_dictionary.cpp',
  'asteria/llds/reference_stack.cpp',
  'asteria/llds/avm_rod.cpp',
  'asteria/runtime/abstract_hooks.cpp',
  'asteria/runtime/reference.cpp',
  'asteria/runtime/reference_modifier.cpp',
  'asteria/runtime/variable.cpp',
  'asteria/runtime/ptc_arguments.cpp',
  'asteria/runtime/runtime_error.cpp',
  'asteria/runtime/abstract_context.cpp',
  'asteria/runtime/analytic_context.cpp',
  'asteria/runtime/executive_context.cpp',
  'asteria/runtime/global_context.cpp',
  'asteria/runtime/garbage_collector.cpp',
  'asteria/runtime/random_engine.cpp',
  'asteria/runtime/module_loader.cpp',
  'asteria/runtime/variadic_arguer.cpp',
  'asteria/runtime/instantiated_function.cpp',
  'asteria/runtime/air_node.cpp',
  'asteria/runtime/air_optimizer.cpp',
  'asteria/runtime/argument_reader.cpp',
  'asteria/runtime/binding_generator.cpp',
  'asteria/compiler/compiler_error.cpp',
  'asteria/compiler/token.cpp',
  'asteria/compiler/token_stream.cpp',
  'asteria/compiler/expression_unit.cpp',
  'asteria/compiler/statement.cpp',
  'asteria/compiler/infix_element.cpp',
  'asteria/compiler/statement_sequence.cpp',
  'asteria/library/version.cpp',
  'asteria/library/gc.cpp',
  'asteria/library/system.cpp',
  'asteria/library/debug.cpp',
  'asteria/library/chrono.cpp',
  'asteria/library/string.cpp',
  'asteria/library/array.cpp',
  'asteria/library/numeric.cpp',
  'asteria/library/math.cpp',
  'asteria/library/filesystem.cpp',
  'asteria/library/checksum.cpp',
  'asteria/library/json.cpp',
  'asteria/library/io.cpp',
  'asteria/library/zlib.cpp',
  'asteria/library/ini.cpp',
  'asteria/library/csv.cpp',
]

repl_src = [
  'repl/fwd.hpp',
  'repl/globals.cpp',
  'repl/single.cpp',
  'repl/interact.cpp',
  'repl/commands.cpp',
  'repl/main.cpp',
  'repl/editline.cpp',
]

test_src = [
  'test/xstring.cpp',
  'test/ascii_numget.cpp',
  'test/ascii_numget_float.cpp',
  'test/ascii_numget_double.cpp',
  'test/ascii_numput_float.cpp',
  'test/ascii_numput_double.cpp',
  'test/utils.cpp',
  'test/value.cpp',
  'test/variable.cpp',
  'test/reference.cpp',
  'test/token_stream.cpp',
  'test/statement_sequence.cpp',
  'test/simple_script.cpp',
  'test/gc.cpp',
  'test/gc2.cpp',
  'test/gc_loop.cpp',
  'test/varg.cpp',
  'test/vcall.cpp',
  'test/operators_o0.cpp',
  'test/operators_o1.cpp',
  'test/operators_o2.cpp',
  'test/proper_tail_call.cpp',
  'test/stack_overflow.cpp',
  'test/structured_binding.cpp',
  'test/global_identifier.cpp',
  'test/variadic_function_call.cpp',
  'test/defer.cpp',
  'test/defer_ptc.cpp',
  'test/trailing_commas.cpp',
  'test/system.cpp',
  'test/chrono.cpp',
  'test/string.cpp',
  'test/array.cpp',
  'test/numeric.cpp',
  'test/math.cpp',
  'test/filesystem.cpp',
  'test/checksum.cpp',
  'test/json.cpp',
  'test/import.cpp',
  'test/bypassed_variable.cpp',
  'test/github_71.cpp',
  'test/github_78.cpp',
  'test/github_84.cpp',
  'test/github_85.cpp',
  'test/github_98.cpp',
  'test/github_101.cpp',
  'test/ref.cpp',
  'test/github_105.cpp',
  'test/github_108.cpp',
  'test/github_113.cpp',
  'test/github_116.cpp',
  'test/zlib.cpp',
  'test/c_stack_overflow.cpp',
  'test/var_mod.cpp',
  'test/ini.cpp',
  'test/csv.cpp',
  'test/binding_variable.cpp',
  'test/ptc_hooks_throw.cpp',
  'test/ptc_hooks_return.cpp',
  'test/switch_defer.cpp',
  'test/for_each.cpp',
  'test/github_102.cpp',
  'test/github_308.cpp',
]

#===========================================================
# Global configuration
#===========================================================
cxx = meson.get_compiler('cpp')
cxx_is_i386 = cxx.compiles('int foo = __i386__;')
cxx_is_amd64 = cxx.compiles('int foo = __amd64__;')
cxx_asm_intel = cxx.compiles('int foo(int a) { return a + 1; }', args: [ '-masm=intel' ])
cxx_has_uchar = cxx.links('extern "C" int mbrtoc16(); int main() { return mbrtoc16 != 0; }')

if cxx_asm_intel
  add_project_arguments('-masm=intel', language: [ 'c', 'cpp' ])
endif

if cxx_is_i386
  add_project_arguments('-msse2', '-mfpmath=sse', language: [ 'c', 'cpp' ])
endif

if get_option('enable-avx') and (cxx_is_i386 or cxx_is_amd64)
  add_project_arguments('-mavx', language: [ 'c', 'cpp' ])
endif

if get_option('enable-debug-checks')
  add_project_arguments('-D_GLIBCXX_DEBUG', '-D_LIBCPP_DEBUG', language: [ 'c', 'cpp' ])
endif

if cxx_has_uchar
  add_project_arguments('-DHAVE_UCHAR_H', language: [ 'c', 'cpp' ])
endif

if get_option('enable-sanitizer') == 'address'
  add_project_arguments('-fsanitize=address,undefined', language: [ 'c', 'cpp' ])
endif

if get_option('enable-sanitizer') == 'thread'
  add_project_arguments('-fsanitize=thread', language: [ 'c', 'cpp' ])
endif

add_project_arguments(
    '-Wall', '-Wextra', '-Winvalid-pch',
    '-D_FILE_OFFSET_BITS=64', '-D_POSIX_C_SOURCE=200809', '-D_DEFAULT_SOURCE',
    '-D_DARWIN_C_SOURCE', '-U_FORTIFY_SOURCE', '-fno-stack-protector',
    '-fstrict-aliasing', '-fstrict-overflow', '-fmerge-all-constants',
    '-frounding-math', '-fno-math-errno', '-fno-trapping-math','-fno-ident',
    '-fno-devirtualize-speculatively','-fno-semantic-interposition',
    '-Werror=conversion', '-Werror=sign-compare', '-Werror=sign-conversion',
    '-Werror=write-strings', '-Werror=return-type', '-Werror=double-promotion',
    '-Wmissing-declarations', '-Wmissing-field-initializers', '-Wshadow',
    '-Wsuggest-attribute=noreturn', '-Wswitch-enum', '-Wunused-function',
    '-Wunused-label', '-Wunused-local-typedefs', '-Wunused-but-set-variable',
    '-Wunused-but-set-parameter',
    language: [ 'c', 'cpp' ])

add_project_arguments(
    '-std=c++17', '-fno-elide-constructors', '-fvisibility-inlines-hidden',
    '-fnothrow-opt', '-fno-enforce-eh-specs', '-Wzero-as-null-pointer-constant',
    '-Wno-redundant-move', '-Werror=non-virtual-dtor',
    language: [ 'cpp' ])

#===========================================================
# Rules for headers
#===========================================================
install_headers(rocket_include, subdir: 'rocket')
install_headers(asteria_include, subdir: 'asteria')

conf = configuration_data()
conf.set('abi_major', meson.project_version().split('.')[0])
conf.set('abi_minor', meson.project_version().split('.')[1])
conf.set_quoted('abi_string', meson.project_version())
configure_file(input: 'asteria/version.h.in', configuration: conf, output: 'version.h')
install_headers(meson.current_build_dir() + '/version.h', subdir: 'asteria')

#===========================================================
# Rules for 'lib/libasteria.so'
#===========================================================
dep_zlib = dependency('zlib', version: '>= 1.2.9')
dep_pcre2 = dependency('libpcre2-8')
dep_openssl = dependency('openssl')
dep_iconv = cxx.find_library('libiconv', required: false)

lib_asteria = shared_library('asteria',
      sources: [ rocket_src, asteria_src ],
      soversion: conf.get('abi_major') + '.' + conf.get('abi_minor') + '.0',
      dependencies: [ dep_zlib, dep_pcre2, dep_openssl, dep_iconv ],
      cpp_pch: 'asteria/precompiled.hpp',
      install: true)

#===========================================================
# Rules for 'bin/asteria'
#===========================================================
if get_option('enable-repl')
  # This is actually required, but we pass `required: false`, as we want
  # to compose a more friendly message later.
  dep_editline = dependency('libedit', required: false)
  if not dep_editline.found()
    error('libedit is required to build the REPL. If you do not need the')
  endif

  executable('asteria',
             sources: repl_src,
             dependencies: [ dep_editline ],
             cpp_pch: 'asteria/precompiled.hpp',
             link_with: lib_asteria,
             install: true)
endif

#===========================================================
# Rules for tests
#===========================================================
foreach src: test_src
  test = executable(src.underscorify(), src, link_with: lib_asteria)
  test(src.underscorify(), test)
endforeach
