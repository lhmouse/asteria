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
#include "../rocket/cow_istringstream.hpp"
#include "../rocket/cow_ostringstream.hpp"
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

// Macros
#define ASTERIA_SFINAE_CONSTRUCT(...)    ROCKET_ENABLE_IF(::std::is_constructible<ROCKET_CAR(__VA_ARGS__), ROCKET_CDR(__VA_ARGS__)>::value)
#define ASTERIA_SFINAE_ASSIGN(...)       ROCKET_ENABLE_IF(::std::is_assignable<ROCKET_CAR(__VA_ARGS__), ROCKET_CDR(__VA_ARGS__)>::value)

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

// Syntax
class Source_Location;
class Xprunit;
class Statement;

// Runtime
class Rcbase;
class Value;
class Abstract_Opaque;
class Abstract_Function;
class Placeholder;
class Reference_Root;
class Reference_Modifier;
class Reference;
class Evaluation_Stack;
class Variable;
class Variable_Callback;
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

// Compiler
class Parser_Error;
class Token;
class Token_Stream;
class Infix_Element;
class Parser;
class Simple_Source_File;

// Library
class Argument_Reader;
class Simple_Binding_Wrapper;

// Aliases
using std::nullptr_t;
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
using cow_isstream = rocket::cow_istringstream;
using cow_osstream = rocket::cow_ostringstream;

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
template<typename E, size_t ...k> using array = rocket::array<E, k...>;

// Fundamental Types
using G_null      = nullptr_t;
using G_boolean   = bool;
using G_integer   = int64_t;
using G_real      = double;
using G_string    = cow_string;
using G_opaque    = rcobj<Abstract_Opaque>;
using G_function  = rcobj<Abstract_Function>;
using G_array     = cow_vector<Value>;
using G_object    = cow_hashmap<phsh_string, Value, phsh_string::hash>;

// Indices of Fundamental Types
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

// API versioning of the standard library
enum API_Version : uint32_t
  {
    api_version_none       = 0x00000000,  // no standard library
    api_version_0001_0000  = 0x00010000,  // version 1.0
    api_version_latest     = 0xFFFFFFFF,  // everything
  };

// Options for source parsing and code generation
struct Compiler_Options
  {
    // These fields have a default value of `false`.
    union {
      struct {
        // Make single quotes behave similiar to double quotes. [useful when parsing JSON5 text]
        bool escapable_single_quote_strings : 1;
        // Parse keywords as identifiers. [useful when parsing JSON text]
        bool keywords_as_identifiers : 1;
        // Parse integer literals as real literals. [useful when parsing JSON text]
        bool integers_as_reals : 1;
      };
      uint8_t bitfields_cant_have_initializers_0 = 0x00;
    };
    // These fields have a default value of `true`.
    union {
      struct {
        // Enable proper tail calls. [commonly known as tail call optimization]
        bool proper_tail_calls : 1;
      };
      uint8_t bitfields_cant_have_initializers_1 = 0xFF;
    };
  };

}  // namespace Asteria

#endif
