// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <utility>  // std::pair<>, std::move(), std::forward()
#include <cstddef>  // std::nullptr_t
#include <cstdint>  // std::uint8_t, std::int64_t
#include "rocket/cow_string.hpp"
#include "rocket/cow_vector.hpp"
#include "rocket/cow_hashmap.hpp"
#include "rocket/static_vector.hpp"
#include "rocket/prehashed_string.hpp"
#include "rocket/refcnt_object.hpp"
#include "rocket/refcnt_ptr.hpp"
#include "rocket/unique_ptr.hpp"

namespace Asteria {

// Utilities
class Formatter;
class Runtime_Error;
class Exception;

// Syntax
class Source_Location;
class Xpnode;
class Expression;
class Statement;
class Block;

// Runtime
class RefCnt_Base;
class Value;
class Abstract_Opaque;
class Abstract_Function;
class Reference_Root;
class Reference_Modifier;
class Reference;
class Reference_Stack;
class Reference_Dictionary;
class Variable;
class Variable_Hashset;
class Abstract_Variable_Callback;
class Collector;
class Abstract_Context;
class Analytic_Context;
class Executive_Context;
class Function_Analytic_Context;
class Function_Executive_Context;
class Global_Context;
class Generational_Collector;
class Variadic_Arguer;
class Instantiated_Function;

// Compiler
class Parser_Error;
class Token;
class Token_Stream;
class Parser;
class Single_Source_File;

// Library
class Argument_Sentry;

// Fundamental Types
using D_null      = std::nullptr_t;
using D_boolean   = bool;
using D_integer   = std::int64_t;
using D_real      = double;
using D_string    = rocket::cow_string;
using D_opaque    = rocket::refcnt_object<Abstract_Opaque>;
using D_function  = rocket::refcnt_object<Abstract_Function>;
using D_array     = rocket::cow_vector<Value>;
using D_object    = rocket::cow_hashmap<rocket::prehashed_string, Value,
                                        rocket::prehashed_string::hash,
                                        rocket::prehashed_string::equal_to>;

// Indices of Fundamental Types
enum Value_Type : std::uint8_t
  {
    type_null      = 0,
    type_boolean   = 1,
    type_integer   = 2,
    type_real      = 3,
    type_string    = 4,
    type_opaque    = 5,
    type_function  = 6,
    type_array     = 7,
    type_object    = 8,
  };

// Aliases
template<typename ElementT>
  using Unique_Ptr = rocket::unique_ptr<ElementT>;
template<typename ElementT>
  using RefCnt_Ptr = rocket::refcnt_ptr<ElementT>;
template<typename ElementT>
  using RefCnt_Object = rocket::refcnt_object<ElementT>;

template<typename ElementT>
  using Cow_Vector = rocket::cow_vector<ElementT>;
template<typename KeyT, typename ValueT>
  using Cow_HashMap = rocket::cow_hashmap<KeyT, ValueT>;
template<typename ElementT, std::size_t capacityT>
  using Static_Vector = rocket::static_vector<ElementT, capacityT>;

using Cow_String = rocket::cow_string;
using PreHashed_String = rocket::prehashed_string;

}

#endif
