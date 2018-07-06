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
#include <unordered_map> // std::unordered_map<>
#include "rocket/preprocessor_utilities.h"
#include "rocket/cow_string.hpp"
#include "rocket/value_ptr.hpp"

namespace Asteria {

// General utilities
class Insertable_streambuf;
class Insertable_ostream;
class Logger;

// Lexical elements
class Initializer;
class Expression_node;
class Statement;
class Parser_result;
class Token;

// Runtime objects
class Exception;
class Stored_reference;
class Opaque_base;
class Function_base;
class Slim_function;
class Instantiated_function;
class Value;
class Reference;
class Scope;

// TODO These will be removed.
template<typename ElementT>
using Sp = std::shared_ptr<ElementT>;
template<typename ElementT>
using Sp_cref = const std::shared_ptr<ElementT> &;

template<typename ElementT>
using Wp = std::weak_ptr<ElementT>;
template<typename ElementT>
using Wp_cref = const std::weak_ptr<ElementT> &;

template<typename ElementT>
using Vp = rocket::value_ptr<ElementT>;
template<typename ElementT>
using Vp_cref = typename std::conditional<std::is_const<ElementT>::value, const rocket::value_ptr<typename std::remove_const<ElementT>::type> &, rocket::value_ptr<ElementT> &>::type;

// Aliases
using Nullptr             = std::nullptr_t;
using Boolean             = bool;
using Signed_integer      = std::int64_t;
using Unsigned_integer    = std::uint64_t;
using Double_precision    = double;
using Cow_string          = rocket::cow_string;
using Cow_string_cref     = const Cow_string &;

template<typename ElementT>
using Vector = std::vector<ElementT>;
template<typename ElementT>
using Dictionary = std::unordered_map<Cow_string, ElementT, Cow_string::hash, Cow_string::equal_to>;

using Function_prototype  = void (Vp<Reference> &, Vp<Reference> &&, Vector<Vp<Reference>> &&);

// Data types exposed to users
using D_null      = Nullptr;
using D_boolean   = Boolean;
using D_integer   = Signed_integer;
using D_double    = Double_precision;
using D_string    = Cow_string;
using D_opaque    = std::shared_ptr<Opaque_base>;
using D_function  = std::shared_ptr<const Function_base>;
using D_array     = Vector<Vp<Value>>;
using D_object    = Dictionary<Vp<Value>>;

}

#endif
