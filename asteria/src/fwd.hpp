// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <type_traits> // so many...
#include <utility> // std::move(), std::forward(), std::pair<>
#include <memory> // std::shared_ptr<>, std::weak_ptr<>
#include <cstddef> // std::nullptr_t
#include <cstdint> // std::int64_t, std::uint64_t
#include "rocket/preprocessor_utilities.h"
#include "rocket/cow_string.hpp"
#include "rocket/cow_vector.hpp"
#include "rocket/cow_hashmap.hpp"

namespace Asteria
{

// Aliases
using Nullptr           = std::nullptr_t;
using Boolean           = bool;
using Signed_integer    = std::int64_t;
using Unsigned_integer  = std::uint64_t;
using Double_precision  = double;
using Cow_string        = rocket::cow_string;

template<typename ElementT>
  using Vector = rocket::cow_vector<ElementT>;
template<typename ElementT>
  using Dictionary = rocket::cow_hashmap<Cow_string, ElementT, Cow_string::hash, Cow_string::equal_to>;
template<typename FirstT, typename SecondT>
  using Pair = std::pair<FirstT, SecondT>;

template<typename ElementT>
  using Uptr = std::unique_ptr<ElementT>;

template<typename ElementT>
  using Sptr = std::shared_ptr<ElementT>;
template<typename ElementT>
  using Wptr = std::weak_ptr<ElementT>;

template<typename ElementT>
  using Spcr = const Sptr<ElementT> &;
template<typename ElementT>
  using Wpcr = const Wptr<ElementT> &;

// General utilities
class Logger;

// Lexical elements
class Parser_result;
class Token;

// Runtime objects
class Value;
class Opaque_base;
class Function_base;
class Reference_root;
class Reference_designator;
class Variable;
using Reference = Pair<Reference_root, Vector<Reference_designator>>;

// Runtime data types exposed to users
using D_null      = Nullptr;
using D_boolean   = Boolean;
using D_integer   = Signed_integer;
using D_double    = Double_precision;
using D_string    = Cow_string;
using D_opaque    = Sptr<Opaque_base>;
using D_function  = Sptr<const Function_base>;
using D_array     = Vector<Value>;
using D_object    = Dictionary<Value>;

}

#endif
