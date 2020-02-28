// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef __FAST_MATH__
#  error Please turn off `-ffast-math`.
#endif

#include <initializer_list>  // ::std::initializer_list<>
#include <utility>  // ::std::pair<>, ::std::move(), ::std::forward(), ::std::integer_sequence<>
#include <cstddef>  // nullptr_t
#include <cstdint>  // uint8_t, int64_t
#include <climits>
#include "../rocket/preprocessor_utilities.h"
#include "../rocket/cow_string.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/cow_hashmap.hpp"
#include "../rocket/static_vector.hpp"
#include "../rocket/prehashed_string.hpp"
#include "../rocket/unique_handle.hpp"
#include "../rocket/unique_ptr.hpp"
#include "../rocket/refcnt_ptr.hpp"
#include "../rocket/variant.hpp"
#include "../rocket/optional.hpp"
#include "../rocket/array.hpp"
#include "../rocket/reference_wrapper.hpp"
#include "../rocket/tinyfmt.hpp"

namespace Asteria {

// Macros
#define ASTERIA_SFINAE_CONSTRUCT(T, ...)    ROCKET_ENABLE_IF(::std::is_constructible<T, __VA_ARGS__>::value)
#define ASTERIA_SFINAE_ASSIGN(T, ...)       ROCKET_ENABLE_IF(::rocket::is_lvalue_assignable<T, __VA_ARGS__>::value)
#define ASTERIA_SFINAE_CONVERT(T, ...)      ROCKET_ENABLE_IF(::std::is_convertible<T, __VA_ARGS__>::value)

#define ASTERIA_VARIANT_CONSTRUCTOR(C, V, T, t)   template<typename T, ASTERIA_SFINAE_CONSTRUCT(V, T&&)> C(T&& t)
#define ASTERIA_VARIANT_ASSIGNMENT(C, V, T, t)    template<typename T, ASTERIA_SFINAE_ASSIGN(V, T&&)> C& operator=(T&& t)

#define ASTERIA_VOID_T(...)        typename ::rocket::make_void<__VA_ARGS__>::type
#define ASTERIA_VOID_OF(...)       typename ::rocket::make_void<decltype(__VA_ARGS__)>::type

// `using`-directives
using ::std::initializer_list;
using ::std::integer_sequence;
using ::std::index_sequence;
using ::std::nullptr_t;
using ::std::max_align_t;
using ::std::int8_t;
using ::std::uint8_t;
using ::std::int16_t;
using ::std::uint16_t;
using ::std::int32_t;
using ::std::uint32_t;
using ::std::int64_t;
using ::std::uint64_t;
using ::std::intptr_t;
using ::std::uintptr_t;
using ::std::intmax_t;
using ::std::uintmax_t;
using ::std::ptrdiff_t;
using ::std::size_t;
using ::std::exception;

using ::rocket::cbegin;
using ::rocket::cend;
using ::rocket::begin;
using ::rocket::end;
using ::rocket::xswap;
using ::rocket::swap;
using ::rocket::nullopt;

// Low-level Data Structure
class Variable_HashSet;
class Reference_Dictionary;
class AVMC_Queue;

// Runtime
class Rcbase;
class Source_Location;
class Recursion_Sentry;
class Value;
class Abstract_Opaque;
class Abstract_Function;
class Abstract_Hooks;
class Runtime_Error;
class Reference_Root;
class Reference_Modifier;
class Reference;
class Evaluation_Stack;
class Variable;
class Variable_Callback;
class Tail_Call_Arguments_Fwd;
class Tail_Call_Arguments;
class Collector;
class Abstract_Context;
class Analytic_Context;
class Executive_Context;
class Global_Context;
class Random_Number_Generator;
class Generational_Collector;
class Variadic_Arguer;
class Instantiated_Function;
class AIR_Node;
class Backtrace_Frame;
class Simple_Script;
enum AIR_Status : uint8_t;
enum PTC_Aware : uint8_t;

// Compiler
class Parser_Error;
class Token;
class Token_Stream;
class Expression_Unit;
class Statement;
class Infix_Element;
class Statement_Sequence;
enum Punctuator : uint8_t;
enum Keyword : uint8_t;
enum Jump_Target : uint8_t;
enum Precedence : uint8_t;
enum Xop : uint8_t;

// Library
class Argument_Reader;
class Simple_Binding_Wrapper;

template<typename E, typename D = ::std::default_delete<const E>> using uptr = ::rocket::unique_ptr<E, D>;
template<typename E> using rcptr = ::rocket::refcnt_ptr<E>;
template<typename E> using cow_vector = ::rocket::cow_vector<E>;
template<typename K, typename V, typename H> using cow_hashmap = ::rocket::cow_hashmap<K, V, H, ::std::equal_to<void>>;
template<typename E, size_t k> using sso_vector = ::rocket::static_vector<E, k>;
template<typename... P> using variant = ::rocket::variant<P...>;
template<typename T> using opt = ::rocket::optional<T>;
template<typename F, typename S> using pair = ::std::pair<F, S>;
template<typename F, typename S> using cow_bivector = ::rocket::cow_vector<::std::pair<F, S>>;
template<typename E, size_t... k> using array = ::rocket::array<E, k...>;
template<typename E> using ref_to = ::rocket::reference_wrapper<E>;

// Aliases
using cow_string = ::rocket::cow_string;
using cow_wstring = ::rocket::cow_wstring;
using cow_u16string = ::rocket::cow_u16string;
using cow_u32string = ::rocket::cow_u32string;
using phsh_string = ::rocket::prehashed_string;
using tinybuf = ::rocket::tinybuf;
using tinyfmt = ::rocket::tinyfmt;

// Fundamental types
using G_null      = nullptr_t;
using G_boolean   = bool;
using G_integer   = int64_t;
using G_real      = double;
using G_string    = cow_string;
using G_opaque    = rcptr<Abstract_Opaque>;
using G_function  = rcptr<Abstract_Function>;
using G_array     = cow_vector<Value>;
using G_object    = cow_hashmap<phsh_string, Value, phsh_string::hash>;

// Stack frame types
enum Frame_Type : uint8_t
  {
    frame_type_native  = 0,  // The frame could not be determined.
    frame_type_throw   = 1,  // An exception was thrown here.
    frame_type_catch   = 2,  // An exception was caught here.
    frame_type_call    = 3,  // An exception propagated across a function call.
    frame_type_func    = 4,  // An exception propagated across a function boundary.
    frame_type_defer   = 5,  // A new exception was thrown here.
  };

ROCKET_PURE_FUNCTION extern const char* describe_frame_type(Frame_Type type) noexcept;

// Garbage collection generations
enum GC_Generation : uint8_t
  {
    gc_generation_newest  = 0,
    gc_generation_middle  = 1,
    gc_generation_oldest  = 2,
  };

// Indices of fundamental types
enum Gtype : uint8_t
  {
    gtype_null      = 0,
    gtype_boolean   = 1,
    gtype_integer   = 2,
    gtype_real      = 3,
    gtype_string    = 4,
    gtype_opaque    = 5,
    gtype_function  = 6,
    gtype_array     = 7,
    gtype_object    = 8,
  };

ROCKET_PURE_FUNCTION extern const char* describe_gtype(Gtype gtype) noexcept;

// Value comparison results
enum Compare : uint8_t
  {
    compare_unordered  = 0,  // The LHS operand is unordered with the RHS operand.
    compare_less       = 1,  // The LHS operand is less than the RHS operand.
    compare_equal      = 2,  // The LHS operand is equal to the RHS operand.
    compare_greater    = 3,  // The LHS operand is greater than the RHS operand.
  };

// Parser status codes
enum Parser_Status : uint32_t
  {
    parser_status_success                                    =    0,
    parser_status_utf8_sequence_invalid                      = 1001,
    parser_status_utf8_sequence_incomplete                   = 1002,
    parser_status_utf_code_point_invalid                     = 1003,
    parser_status_null_character_disallowed                  = 1004,
    parser_status_token_character_unrecognized               = 2001,
    parser_status_string_literal_unclosed                    = 2002,
    parser_status_escape_sequence_unknown                    = 2003,
    parser_status_escape_sequence_incomplete                 = 2004,
    parser_status_escape_sequence_invalid_hex                = 2005,
    parser_status_escape_utf_code_point_invalid              = 2006,
    parser_status_numeric_literal_invalid                    = 2007,
    parser_status_integer_literal_overflow                   = 2008,
    parser_status_integer_literal_inexact                    = 2009,
    parser_status_real_literal_overflow                      = 2010,
    parser_status_real_literal_underflow                     = 2011,
    parser_status_numeric_literal_suffix_invalid             = 2012,
    parser_status_block_comment_unclosed                     = 2014,
    parser_status_digit_separator_following_nondigit         = 2015,
    parser_status_identifier_expected                        = 3002,
    parser_status_semicolon_expected                         = 3003,
    parser_status_string_literal_expected                    = 3004,
    parser_status_statement_expected                         = 3005,
    parser_status_equals_sign_expected                       = 3006,
    parser_status_expression_expected                        = 3007,
    parser_status_open_brace_expected                        = 3008,
    parser_status_closed_brace_or_statement_expected         = 3009,
    parser_status_open_parenthesis_expected                  = 3010,
    parser_status_parameter_or_ellipsis_expected             = 3011,
    parser_status_closed_parenthesis_expected                = 3012,
    parser_status_colon_expected                             = 3013,
    parser_status_closed_brace_or_switch_clause_expected     = 3014,
    parser_status_keyword_while_expected                     = 3015,
    parser_status_keyword_catch_expected                     = 3016,
    parser_status_comma_expected                             = 3017,
    parser_status_for_statement_initializer_expected         = 3018,
    parser_status_semicolon_or_expression_expected           = 3019,
    parser_status_closed_brace_expected                      = 3020,
    parser_status_too_many_elements                          = 3021,
    parser_status_closed_parenthesis_or_argument_expected    = 3022,
    parser_status_closed_bracket_expected                    = 3023,
    parser_status_open_brace_or_equal_initializer_expected   = 3024,
    parser_status_equals_sign_or_colon_expected              = 3025,
    parser_status_closed_bracket_or_comma_expected           = 3026,
    parser_status_closed_brace_or_comma_expected             = 3027,
    parser_status_closed_bracket_or_expression_expected      = 3028,
    parser_status_closed_brace_or_json5_key_expected         = 3029,
  };

ROCKET_PURE_FUNCTION extern const char* describe_parser_status(Parser_Status status) noexcept;

// API versioning of the standard library
enum API_Version : uint32_t
  {
    api_version_none       = 0x00000000,  // no standard library
    api_version_0001_0000  = 0x00010000,  // version 1.0
    api_version_sentinel /* auto inc */,  // [subtract one to get the maximum version number]
    api_version_latest     = 0xFFFFFFFF,  // everything
  };

// Options for source parsing and code generation
template<uint32_t... paramsT> struct Compiler_Options_template;
template<uint32_t fragmentT> struct Compiler_Options_fragment;

template<uint32_t versionT>
    struct Compiler_Options_template<versionT> : Compiler_Options_template<versionT, versionT>
  {
  };
template<uint32_t versionT, uint32_t fragmentT>
    struct Compiler_Options_template<versionT, fragmentT> : Compiler_Options_template<versionT, fragmentT - 1>,
                                                            Compiler_Options_fragment<fragmentT>
  {
    // All members from `Compiler_Options_fragment<fragmentT>` have been brought in.
    // This struct does not define anything by itself.
  };
template<uint32_t versionT> struct Compiler_Options_template<versionT, 0>
  {
    // This member exists in all derived structs.
    // We would like these structs to be trivial, hence virtual bases are not an option.
    uint8_t version = versionT;
  };

template<> struct Compiler_Options_fragment<1>
  {
    // Make single quotes behave similiar to double quotes. [useful when parsing JSON5 text]
    bool escapable_single_quotes : 1;
    // Parse keywords as identifiers. [useful when parsing JSON text]
    bool keywords_as_identifiers : 1;
    // Parse integer literals as real literals. [useful when parsing JSON text]
    bool integers_as_reals : 1;
    // Do not implement proper tail calls. [more commonly known as tail call optimization]
    bool no_proper_tail_calls : 1;
    // Suppress all optimization techniques.
    bool no_optimization : 1;
    // Do not generate code for single-step traps other than on function calls.
    bool no_plain_single_step_traps : 1;

    // Note: Please keep this struct as compact as possible.
  };

template<> struct Compiler_Options_fragment<2>
  {
    // Note: Please keep this struct as compact as possible.
  };

// These are aliases for historical versions.
using Compiler_Options_v1 = Compiler_Options_template<1>;
using Compiler_Options_v2 = Compiler_Options_template<2>;

// This is always an alias for the latest version.
using Compiler_Options = Compiler_Options_v2;

// This value is initialized statically and never destroyed.
extern const unsigned char null_value_storage[];
static const Value& null_value = reinterpret_cast<const Value&>(null_value_storage);

// Aliases for bindings (shorter but less readable)
using Global = Global_Context;
using Null = G_null;

using Bval = G_boolean;
using Ival = G_integer;
using Rval = G_real;
using Sval = G_string;
using Pval = G_opaque;
using Fval = G_function;
using Aval = G_array;
using Oval = G_object;

using Bopt = opt<G_boolean>;
using Iopt = opt<G_integer>;
using Ropt = opt<G_real>;
using Sopt = opt<G_string>;
using Popt = opt<G_opaque>;
using Fopt = opt<G_function>;
using Aopt = opt<G_array>;
using Oopt = opt<G_object>;

}  // namespace Asteria

#endif
