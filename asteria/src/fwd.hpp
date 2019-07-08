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

#include <utility>  // Pair<>, rocket::move(), rocket::forward()
#include <cstddef>  // std::nullptr_t
#include <cstdint>  // std::uint8_t, std::int64_t
#include <climits>
#include "rocket/preprocessor_utilities.h"
#include "rocket/cow_string.hpp"
#include "rocket/cow_istringstream.hpp"
#include "rocket/cow_ostringstream.hpp"
#include "rocket/cow_vector.hpp"
#include "rocket/cow_hashmap.hpp"
#include "rocket/static_vector.hpp"
#include "rocket/prehashed_string.hpp"
#include "rocket/transparent_comparators.hpp"
#include "rocket/unique_handle.hpp"
#include "rocket/unique_ptr.hpp"
#include "rocket/refcnt_ptr.hpp"
#include "rocket/refcnt_object.hpp"
#include "rocket/variant.hpp"
#include "rocket/optional.hpp"

namespace Asteria {

// Utilities
class Formatter;
class Runtime_Error;
class Traceable_Exception;

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
class Reference_Stack;
class Reference_Dictionary;
class Evaluation_Stack;
class Variable;
class Variable_HashSet;
class Variable_Callback;
class Abstract_Variable_Callback;
class Collector;
class Abstract_Context;
class Analytic_Context;
class Executive_Context;
class Global_Context;
class Random_Number_Generator;
class Generational_Collector;
class Variadic_Arguer;
class Instantiated_Function;
class Backtrace_Frame;
class Air_Node;

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

// Type Aliases
using Cow_String = rocket::cow_string;
using PreHashed_String = rocket::prehashed_string;
using Cow_stringbuf = rocket::cow_stringbuf;
using Cow_isstream = rocket::cow_istringstream;
using Cow_osstream = rocket::cow_ostringstream;

// Template Aliases
template<typename E, typename D = std::default_delete<const E>> using Uptr = rocket::unique_ptr<E, D>;
template<typename E> using Rcptr = rocket::refcnt_ptr<E>;
template<typename E> using Rcobj = rocket::refcnt_object<E>;
template<typename E> using Cow_Vector = rocket::cow_vector<E>;
template<typename K, typename V, typename H> using Cow_HashMap = rocket::cow_hashmap<K, V, H, rocket::transparent_equal_to>;
template<typename E, std::size_t k> using Static_Vector = rocket::static_vector<E, k>;
template<typename... P> using Variant = rocket::variant<P...>;
template<typename T> using Opt = rocket::optional<T>;
template<typename F, typename S> using Pair = std::pair<F, S>;
template<typename E, std::size_t k> using Array = std::array<E, k>;

// Fundamental Types
using G_null      = std::nullptr_t;
using G_boolean   = bool;
using G_integer   = std::int64_t;
using G_real      = double;
using G_string    = Cow_String;
using G_opaque    = Rcobj<Abstract_Opaque>;
using G_function  = Rcobj<Abstract_Function>;
using G_array     = Cow_Vector<Value>;
using G_object    = Cow_HashMap<PreHashed_String, Value, PreHashed_String::hash>;

// Indices of Fundamental Types
enum Gtype : std::uint8_t
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
enum API_Version : std::uint32_t
  {
    api_version_none       = 0x00000000,  // no standard library
    api_version_0001_0000  = 0x00010000,  // version 1.0
    api_version_latest     = 0xFFFFFFFF,  // everything
  };

}  // namespace Asteria

#endif
