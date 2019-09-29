// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef __FAST_MATH__
#  error Please turn off `-ffast-math`.
#endif

#include <utility>  // std::pair<>, rocket::move(), rocket::forward()
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
#include "../rocket/refcnt_object.hpp"
#include "../rocket/variant.hpp"
#include "../rocket/optional.hpp"
#include "../rocket/array.hpp"
#include "../rocket/ref_to.hpp"
#include "../rocket/tinyfmt_str.hpp"

// Macros
#define ASTERIA_SFINAE_CONSTRUCT(...)    ROCKET_ENABLE_IF(::std::is_constructible<ROCKET_CAR(__VA_ARGS__), ROCKET_CDR(__VA_ARGS__)>::value)
#define ASTERIA_SFINAE_ASSIGN(...)       ROCKET_ENABLE_IF(::rocket::is_lvalue_assignable<ROCKET_CAR(__VA_ARGS__), ROCKET_CDR(__VA_ARGS__)>::value)
#define ASTERIA_SFINAE_CONVERT(...)      ROCKET_ENABLE_IF(::std::is_convertible<ROCKET_CAR(__VA_ARGS__), ROCKET_CDR(__VA_ARGS__)>::value)

#define ASTERIA_VOID_T(...)              typename ::rocket::make_void<__VA_ARGS__>::type
#define ASTERIA_VOID_OF(...)             typename ::rocket::make_void<decltype(__VA_ARGS__)>::type

#define ASTERIA_AND_(x_, y_)             (bool(x_) && bool(y_))
#define ASTERIA_OR_(x_, y_)              (bool(x_) || bool(y_))
#define ASTERIA_COMMA_(x_, y_)           (void(x_) ,      (y_))

namespace Asteria {

// Utilities
class Formatter;
class Runtime_Error;

// Low-level Data Structure
class Variable_HashSet;
class Reference_Dictionary;
class AVMC_Queue;

// Runtime
class Rcbase;
class Source_Location;
class Value;
class Abstract_Opaque;
class Abstract_Function;
class Abstract_Hooks;
class Placeholder;
class Reference_Root;
class Reference_Modifier;
class Reference;
class Evaluation_Stack;
class Variable;
class Variable_Callback;
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
class Exception;
class Simple_Script;

// Compiler
class Parser_Error;
class Token;
class Token_Stream;
class Xprunit;
class Statement;
class Infix_Element;
class Statement_Sequence;

// Library
class Argument_Reader;
class Simple_Binding_Wrapper;

// Aliases
using std::nullptr_t;
using std::max_align_t;
using std::int8_t;
using std::uint8_t;
using std::int16_t;
using std::uint16_t;
using std::int32_t;
using std::uint32_t;
using std::int64_t;
using std::uint64_t;
using std::intptr_t;
using std::uintptr_t;
using std::intmax_t;
using std::uintmax_t;
using std::ptrdiff_t;
using std::size_t;

using cow_string = rocket::cow_string;
using cow_wstring = rocket::cow_wstring;
using cow_u16string = rocket::cow_u16string;
using cow_u32string = rocket::cow_u32string;
using phsh_string = rocket::prehashed_string;
using cow_stringbuf = rocket::cow_stringbuf;
using tinyfmt_str = rocket::tinyfmt_str;
using tinyfmt = rocket::tinyfmt;

template<typename E, typename D = std::default_delete<const E>> using uptr = rocket::unique_ptr<E, D>;
template<typename E> using rcptr = rocket::refcnt_ptr<E>;
template<typename E> using rcobj = rocket::refcnt_object<E>;
template<typename E> using cow_vector = rocket::cow_vector<E>;
template<typename K, typename V, typename H> using cow_hashmap = rocket::cow_hashmap<K, V, H, std::equal_to<void>>;
template<typename E, size_t k> using sso_vector = rocket::static_vector<E, k>;
template<typename... P> using variant = rocket::variant<P...>;
template<typename T> using opt = rocket::optional<T>;
template<typename F, typename S> using pair = std::pair<F, S>;
template<typename F, typename S> using cow_bivector = rocket::cow_vector<std::pair<F, S>>;
template<typename E, size_t... k> using array = rocket::array<E, k...>;
template<typename E> using ref_to = rocket::ref_to<E>;

// Fundamental types
using G_null      = nullptr_t;
using G_boolean   = bool;
using G_integer   = int64_t;
using G_real      = double;
using G_string    = cow_string;
using G_opaque    = rcobj<Abstract_Opaque>;
using G_function  = rcobj<Abstract_Function>;
using G_array     = cow_vector<Value>;
using G_object    = cow_hashmap<phsh_string, Value, phsh_string::hash>;

// IR status codes
enum AIR_Status : uint8_t
  {
    air_status_next             = 0,
    air_status_return           = 1,
    air_status_break_unspec     = 2,
    air_status_break_switch     = 3,
    air_status_break_while      = 4,
    air_status_break_for        = 5,
    air_status_continue_unspec  = 6,
    air_status_continue_while   = 7,
    air_status_continue_for     = 8,
  };

// Tail call optimization (TCO) awareness
enum TCO_Aware : uint8_t
  {
    tco_aware_none     = 0,  // Tail call optimization is not allowed.
    tco_aware_by_ref   = 1,  // The tail call is forwarded by reference.
    tco_aware_by_val   = 2,  // The tail call is forwarded by value.
    tco_aware_nullify  = 3,  // The tail call is forwarded but its result is discarded.
  };

// Dead code elimination (DCE) results
enum DCE_Result : uint8_t
  {
    dce_none   = 0,  // The current node should be left intact.
    dce_empty  = 1,  // The current node is empty and can be removed.
    dce_prune  = 2,  // All nodes following the current one are unreachable and can be removed.
  };

// Stack frame types
enum Frame_Type : uint8_t
  {
    frame_type_native  = 0,  // The frame could not be determined.
    frame_type_throw   = 1,  // An exception was thrown here.
    frame_type_catch   = 2,  // An exception was caught here.
    frame_type_func    = 3,  // An exception propagated across a function boundary.
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

// Punctuators
enum Punctuator : uint8_t
  {
    punctuator_add         =  0,  // +
    punctuator_add_eq      =  1,  // +=
    punctuator_sub         =  2,  // -
    punctuator_sub_eq      =  3,  // -=
    punctuator_mul         =  4,  // *
    punctuator_mul_eq      =  5,  // *=
    punctuator_div         =  6,  // /
    punctuator_div_eq      =  7,  // /=
    punctuator_mod         =  8,  // %
    punctuator_mod_eq      =  9,  // %=
    punctuator_inc         = 10,  // ++
    punctuator_dec         = 11,  // --
    punctuator_sll         = 12,  // <<<
    punctuator_sll_eq      = 13,  // <<<=
    punctuator_srl         = 14,  // >>>
    punctuator_srl_eq      = 15,  // >>>=
    punctuator_sla         = 16,  // <<
    punctuator_sla_eq      = 17,  // <<=
    punctuator_sra         = 18,  // >>
    punctuator_sra_eq      = 19,  // >>=
    punctuator_andb        = 20,  // &
    punctuator_andb_eq     = 21,  // &=
    punctuator_andl        = 22,  // &&
    punctuator_andl_eq     = 23,  // &&=
    punctuator_orb         = 24,  // |
    punctuator_orb_eq      = 25,  // |=
    punctuator_orl         = 26,  // ||
    punctuator_orl_eq      = 27,  // ||=
    punctuator_xorb        = 28,  // ^
    punctuator_xorb_eq     = 29,  // ^=
    punctuator_notb        = 30,  // ~
    punctuator_notl        = 31,  // !
    punctuator_cmp_eq      = 32,  // ==
    punctuator_cmp_ne      = 33,  // !=
    punctuator_cmp_lt      = 34,  // <
    punctuator_cmp_gt      = 35,  // >
    punctuator_cmp_lte     = 36,  // <=
    punctuator_cmp_gte     = 37,  // >=
    punctuator_dot         = 38,  // .
    punctuator_quest       = 39,  // ?
    punctuator_quest_eq    = 40,  // ?=
    punctuator_assign      = 41,  // =
    punctuator_parenth_op  = 42,  // (
    punctuator_parenth_cl  = 43,  // )
    punctuator_bracket_op  = 44,  // [
    punctuator_bracket_cl  = 45,  // ]
    punctuator_brace_op    = 46,  // {
    punctuator_brace_cl    = 47,  // }
    punctuator_comma       = 48,  // ,
    punctuator_colon       = 49,  // :
    punctuator_semicol     = 50,  // ;
    punctuator_spaceship   = 51,  // <=>
    punctuator_coales      = 52,  // ??
    punctuator_coales_eq   = 53,  // ??=
    punctuator_ellipsis    = 54,  // ...
    punctuator_end_a       = 55,  // [$]
  };

ROCKET_PURE_FUNCTION extern const char* stringify_punctuator(Punctuator punct) noexcept;

// Keywords
enum Keyword : uint8_t
  {
    keyword_var       =  0,
    keyword_const     =  1,
    keyword_func      =  2,
    keyword_if        =  3,
    keyword_else      =  4,
    keyword_switch    =  5,
    keyword_case      =  6,
    keyword_default   =  7,
    keyword_do        =  8,
    keyword_while     =  9,
    keyword_for       = 10,
    keyword_each      = 11,
    keyword_try       = 12,
    keyword_catch     = 13,
    keyword_defer     = 14,
    keyword_break     = 15,
    keyword_continue  = 16,
    keyword_throw     = 17,
    keyword_return    = 18,
    keyword_null      = 19,
    keyword_true      = 20,
    keyword_false     = 21,
    keyword_nan       = 22,
    keyword_infinity  = 23,
    keyword_this      = 24,
    keyword_unset     = 25,
    keyword_lengthof  = 26,
    keyword_typeof    = 27,
    keyword_and       = 28,
    keyword_or        = 29,
    keyword_not       = 30,
    keyword_assert    = 31,
    keyword_sqrt      = 32,  // __sqrt
    keyword_isnan     = 33,  // __isnan
    keyword_isinf     = 34,  // __isinf
    keyword_abs       = 35,  // __abs
    keyword_signb     = 36,  // __signb
    keyword_round     = 37,  // __round
    keyword_floor     = 38,  // __floor
    keyword_ceil      = 39,  // __ceil
    keyword_trunc     = 40,  // __trunc
    keyword_iround    = 41,  // __iround
    keyword_ifloor    = 42,  // __ifloor
    keyword_iceil     = 43,  // __iceil
    keyword_itrunc    = 44,  // __itrunc
    keyword_fma       = 45,  // __fma
  };

ROCKET_PURE_FUNCTION extern const char* stringify_keyword(Keyword kwrd) noexcept;

// Target of jump statements
enum Jump_Target : uint8_t
  {
    jump_target_unspec  = 0,
    jump_target_switch  = 1,
    jump_target_while   = 2,
    jump_target_for     = 3,
  };

// Infix operator precedences
enum Precedence : uint8_t
  {
    precedence_multiplicative  =  1,
    precedence_additive        =  2,
    precedence_shift           =  3,
    precedence_relational      =  4,
    precedence_equality        =  5,
    precedence_bitwise_and     =  6,
    precedence_bitwise_xor     =  7,
    precedence_bitwise_or      =  8,
    precedence_logical_and     =  9,
    precedence_logical_or      = 10,
    precedence_coalescence     = 11,
    precedence_assignment      = 12,
    precedence_lowest          = 99,
  };

// Operators
enum Xop : uint8_t
  {
    xop_inc_post  =  0,  // ++ (postfix)
    xop_dec_post  =  1,  // -- (postfix)
    xop_subscr    =  2,  // []
    xop_pos       =  3,  // +
    xop_neg       =  4,  // -
    xop_notb      =  5,  // ~
    xop_notl      =  6,  // !
    xop_inc_pre   =  7,  // ++ (prefix)
    xop_dec_pre   =  8,  // -- (prefix)
    xop_unset     =  9,  // unset
    xop_lengthof  = 10,  // lengthof
    xop_typeof    = 11,  // typeof
    xop_sqrt      = 12,  // __sqrt
    xop_isnan     = 13,  // __isnan
    xop_isinf     = 14,  // __isinf
    xop_abs       = 15,  // __abs
    xop_signb     = 16,  // __signb
    xop_round     = 17,  // __round
    xop_floor     = 18,  // __floor
    xop_ceil      = 19,  // __ceil
    xop_trunc     = 20,  // __trunc
    xop_iround    = 21,  // __iround
    xop_ifloor    = 22,  // __ifloor
    xop_iceil     = 23,  // __iceil
    xop_itrunc    = 24,  // __itrunc
    xop_cmp_eq    = 25,  // ==
    xop_cmp_ne    = 26,  // !=
    xop_cmp_lt    = 27,  // <
    xop_cmp_gt    = 28,  // >
    xop_cmp_lte   = 29,  // <=
    xop_cmp_gte   = 30,  // >=
    xop_cmp_3way  = 31,  // <=>
    xop_add       = 32,  // +
    xop_sub       = 33,  // -
    xop_mul       = 34,  // *
    xop_div       = 35,  // /
    xop_mod       = 36,  // %
    xop_sll       = 37,  // <<<
    xop_srl       = 38,  // >>>
    xop_sla       = 39,  // <<
    xop_sra       = 40,  // >>
    xop_andb      = 41,  // &
    xop_orb       = 42,  // |
    xop_xorb      = 43,  // ^
    xop_assign    = 44,  // =
    xop_fma_3     = 45,  // __fma()
    xop_end_a     = 46,  // [$]
  };

// Parser status codes
enum Parser_Status : uint32_t
  {
    parser_status_success                                    =    0,
    parser_status_ifstream_open_failure                      = 1001,
    parser_status_utf8_sequence_invalid                      = 1002,
    parser_status_utf8_sequence_incomplete                   = 1003,
    parser_status_utf_code_point_invalid                     = 1004,
    parser_status_null_character_disallowed                  = 1005,
    parser_status_token_character_unrecognized               = 2001,
    parser_status_string_literal_unclosed                    = 2002,
    parser_status_escape_sequence_unknown                    = 2003,
    parser_status_escape_sequence_incomplete                 = 2004,
    parser_status_escape_sequence_invalid_hex                = 2005,
    parser_status_escape_utf_code_point_invalid              = 2006,
    parser_status_numeric_literal_incomplete                 = 2007,
    parser_status_numeric_literal_suffix_disallowed          = 2008,
    parser_status_numeric_literal_exponent_overflow          = 2009,
    parser_status_integer_literal_overflow                   = 2010,
    parser_status_integer_literal_exponent_negative          = 2011,
    parser_status_real_literal_overflow                      = 2012,
    parser_status_real_literal_underflow                     = 2013,
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
    parser_status_too_many_array_elements                    = 3021,
    parser_status_closed_parenthesis_or_argument_expected    = 3022,
    parser_status_closed_bracket_expected                    = 3023,
    parser_status_open_brace_or_equal_initializer_expected   = 3024,
    parser_status_equals_sign_or_colon_expected              = 3025,
  };

ROCKET_PURE_FUNCTION extern const char* describe_parser_status(Parser_Status status) noexcept;

// API versioning of the standard library
enum API_Version : uint32_t
  {
    api_version_none       = 0x00000000,  // no standard library
    api_version_0001_0000  = 0x00010000,  // version 1.0
    api_version_sentinel /* auto inc */,  // [subtract one from this value to get the maximum version number]
    api_version_latest     = 0xFFFFFFFF,  // everything
  };

// Options for source parsing and code generation
template<uint32_t... paramsT> struct Compiler_Options_template;
template<uint32_t fragmentT> struct Compiler_Options_fragment;

template<uint32_t versionT> struct Compiler_Options_template<versionT> : Compiler_Options_template<versionT, versionT>
  {
  };
template<uint32_t versionT, uint32_t fragmentT> struct Compiler_Options_template<versionT, fragmentT> : Compiler_Options_template<versionT, fragmentT - 1>,
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
    // Do not remove unreachable or insignificant code.
    bool no_dead_code_elimination : 1;

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

}  // namespace Asteria

#endif
