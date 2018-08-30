// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <type_traits> // so many...
#include <utility> // std::move(), std::forward(), std::pair<>
#include <memory> // std::shared_ptr<>, std::weak_ptr<>, std::make_shared()
#include <cstddef> // std::nullptr_t
#include <cstdint> // std::int64_t, std::uint64_t
#include "rocket/preprocessor_utilities.h"
#include "rocket/cow_string.hpp"
#include "rocket/cow_vector.hpp"
#include "rocket/cow_hashmap.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

// Aliases
using Null      = std::nullptr_t;
using Boolean   = bool;
using Signed    = std::int64_t;
using Unsigned  = std::uint64_t;
using Double    = double;
using String    = rocket::cow_string;

template<typename ElementT>
  using Vector = rocket::cow_vector<ElementT>;
template<typename FirstT, typename SecondT>
  using Bivector = rocket::cow_vector<std::pair<FirstT, SecondT>>;
template<typename ElementT>
  using Dictionary = rocket::cow_hashmap<String, ElementT, String::hash, String::equal_to>;

template<typename ElementT>
  using Sbase = rocket::refcounted_base<ElementT>;
template<typename ElementT>
  using Sptr = rocket::refcounted_ptr<ElementT>;

// General utilities
class Formatter;
class Exception;
class Runtime_error;
class Backtracer;

// Lexical elements
class Parser_result;
class Token;
class Xpnode;
class Expression;
class Statement;
class Block;

// Runtime objects
class Value;
class Abstract_opaque;
class Abstract_function;
class Reference_root;
class Reference_modifier;
class Reference;
class Variable;
class Abstract_context;
class Analytic_context;
class Executive_context;
class Variadic_arguer;
class Instantiated_function;

// Runtime data types exposed to users
using D_null      = Null;
using D_boolean   = Boolean;
using D_integer   = Signed;
using D_double    = Double;
using D_string    = String;
using D_opaque    = Sptr<Abstract_opaque>;
using D_function  = Sptr<const Abstract_function>;
using D_array     = Vector<Value>;
using D_object    = Dictionary<Value>;

}

#endif
