// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <type_traits> // std::enable_if<>, std::decay<>, std::is_base_of<>
#include <utility> // std::move(), std::forward(), std::pair<>
#include <memory> // std::shared_ptr<>
#include <cstddef> // std::nullptr_t
#include <cstdint> // std::int64_t, std::uint64_t
#include <vector> // std::vector<>
#include <unordered_map> // std::unordered_map<>
#include "rocket/cow_string.hpp"
#include "rocket/value_ptr.hpp"

#define ASTERIA_ENABLE_IF(...)              typename ::std::enable_if<__VA_ARGS__>::type * = nullptr
#define ASTERIA_UNLESS_IS_BASE_OF(B_, T_)   ASTERIA_ENABLE_IF(::std::is_base_of<B_, typename ::std::decay<T_>::type>::value == false)

#define ASTERIA_CAR(x_, ...)    x_
#define ASTERIA_CDR(x_, ...)    __VA_ARGS__

namespace Asteria {

// General utilities.
class Insertable_streambuf;
class Insertable_ostream;
class Logger;

template<typename ElementT>
class Sptr_fmt;

// Lexical elements (movable only)
class Initializer;
class Expression_node;
class Expression;
class Statement;
class Block;
class Parameter;

// Runtime objects (movable only)
class Exception;
class Stored_value;
class Stored_reference;

// Runtime objects (neither copyable nor movable)
class Opaque_base;
class Function_base;
class Slim_function;
class Instantiated_function;
class Variable;
class Reference;
class Local_variable;
class Scope;
class Recycler;
class Executor;

// Aliases.
template<typename ElementT>
using T_vector = std::vector<ElementT>;
template<typename ElementT>
using T_string_map = std::unordered_map<rocket::cow_string, ElementT, rocket::cow_string::hash, rocket::cow_string::equal_to>;

template<typename ElementT>
using Sptr = std::shared_ptr<ElementT>;
template<typename ElementT>
using Spparam = const std::shared_ptr<ElementT> &;
template<typename ElementT>
using Sptr_vector = T_vector<Sptr<ElementT>>;
template<typename ValueT>
using Sptr_string_map = T_string_map<Sptr<ValueT>>;

template<typename ElementT>
using Wptr = std::weak_ptr<ElementT>;
template<typename ElementT>
using Wparg = const std::weak_ptr<ElementT> &;
template<typename ElementT>
using Wptr_vector = T_vector<Wptr<ElementT>>;
template<typename ValueT>
using Wptr_string_map = T_string_map<Wptr<ValueT>>;

template<typename ElementT>
using Xptr = rocket::value_ptr<ElementT>;
//template<typename ElementT>
//using Xparg = const rocket::value_ptr<ElementT> &;
template<typename ElementT>
using Xptr_vector = T_vector<Xptr<ElementT>>;
template<typename ValueT>
using Xptr_string_map = T_string_map<Xptr<ValueT>>;

// Data types used internally
using Signed_integer      = std::int64_t;
using Unsigned_integer    = std::uint64_t;
using Cow_string          = rocket::cow_string;
using Function_prototype  = void (Xptr<Reference> &, Spparam<Recycler>, Xptr<Reference> &&, Xptr_vector<Reference> &&);

// Data types exposed to users
using D_null      = std::nullptr_t;
using D_boolean   = bool;
using D_integer   = Signed_integer;
using D_double    = double;
using D_string    = Cow_string;
using D_opaque    = Xptr<Opaque_base>;
using D_function  = Sptr<const Function_base>;
using D_array     = Xptr_vector<Variable>;
using D_object    = Xptr_string_map<Variable>;

}

#endif
