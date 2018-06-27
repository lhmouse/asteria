// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <type_traits> // so many...
#include <utility> // std::move(), std::forward(), std::pair<>
#include <memory> // std::shared_ptr<>
#include <cstddef> // std::nullptr_t
#include <cstdint> // std::int64_t, std::uint64_t
#include <vector> // std::vector<>
#include <deque> // std::deque<>
#include <unordered_map> // std::unordered_map<>
#include "rocket/cow_string.hpp"
#include "rocket/value_ptr.hpp"

// Preprocessor utilities
#define ASTERIA_CAR(x_, ...)       x_
#define ASTERIA_CDR(x_, ...)       __VA_ARGS__
#define ASTERIA_QUOTE(...)         #__VA_ARGS__
#define ASTERIA_CAT2(x_, y_)       x_##y_
#define ASTERIA_CAT3(x_, y_, z_)   x_##y_##z_
#define ASTERIA_LAZY(f_, ...)      f_(__VA_ARGS__)

#define ASTERIA_ENABLE_IF_ACCEPTABLE_BY_VARIANT(C_, V_)	\
	typename ::std::enable_if<	\
		(::std::is_same<typename ::std::decay<C_>::type, V_>::value) || (V_::is_candidate<typename ::std::decay<C_>::type>::value)	\
		>::type * = nullptr

namespace Asteria {

// General utilities
class Insertable_streambuf;
class Insertable_ostream;
class Logger;

template<typename ElementT>
class Sp_formatter;

// Lexical elements (movable only)
class Initializer;
class Expression_node;
class Expression;
class Statement;
class Block;
class Parameter;
class Parser_result;
class Token;

// Runtime objects (movable only)
class Exception;
class Stored_value;
class Stored_reference;

// Runtime objects (neither copyable nor movable)
class Opaque_base;
class Function_base;
class Slim_function;
class Instantiated_function;
class Value;
class Reference;
class Variable;
class Scope;
class Recycler;

// Aliases.
template<typename ElementT>
using T_vector = std::vector<ElementT>;
template<typename ElementT>
using T_deque = std::deque<ElementT>;
template<typename ElementT>
using T_string_map = std::unordered_map<rocket::cow_string, ElementT, rocket::cow_string::hash, rocket::cow_string::equal_to>;
template<typename FirstT, typename SecondT>
using T_pair = std::pair<FirstT, SecondT>;

template<typename ElementT>
using Sp = std::shared_ptr<ElementT>;
template<typename ElementT>
using Spr = const std::shared_ptr<ElementT> &;
template<typename ElementT>
using Sp_vector = T_vector<Sp<ElementT>>;
template<typename ValueT>
using Sp_string_map = T_string_map<Sp<ValueT>>;

template<typename ElementT>
using Wp = std::weak_ptr<ElementT>;
template<typename ElementT>
using Wpr = const std::weak_ptr<ElementT> &;
template<typename ElementT>
using Wp_vector = T_vector<Wp<ElementT>>;
template<typename ValueT>
using Wp_string_map = T_string_map<Wp<ValueT>>;

template<typename ElementT>
using Vp = rocket::value_ptr<ElementT>;
//template<typename ElementT>
//using Vpr = const rocket::value_ptr<ElementT> &;
template<typename ElementT>
using Vp_vector = T_vector<Vp<ElementT>>;
template<typename ValueT>
using Vp_string_map = T_string_map<Vp<ValueT>>;

// Complementary data types used internally
using Nullptr             = std::nullptr_t;
using Boolean             = bool;
using Signed_integer      = std::int64_t;
using Unsigned_integer    = std::uint64_t;
using Double_precision    = double;
using Cow_string          = rocket::cow_string;
using Function_prototype  = void (Vp<Reference> &, Spr<Recycler>, Vp<Reference> &&, Vp_vector<Reference> &&);

// Data types exposed to users
using D_null      = Nullptr;
using D_boolean   = Boolean;
using D_integer   = Signed_integer;
using D_double    = Double_precision;
using D_string    = Cow_string;
using D_opaque    = Vp<Opaque_base>;
using D_function  = Sp<const Function_base>;
using D_array     = Vp_vector<Value>;
using D_object    = Vp_string_map<Value>;

}

#endif
