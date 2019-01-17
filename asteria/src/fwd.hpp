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
#include "rocket/prehashed_string.hpp"
#include "rocket/refcounted_object.hpp"

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
class Refcounted_Base;
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
using D_opaque    = rocket::refcounted_object<Abstract_Opaque>;
using D_function  = rocket::refcounted_object<Abstract_Function>;
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

}

#endif
