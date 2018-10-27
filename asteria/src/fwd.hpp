// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <type_traits> // so many...
#include <utility> // std::move(), std::forward(), std::pair<>
#include <array> // std::array<>
#include <cstddef> // std::nullptr_t
#include <cstdint> // std::int64_t, std::uint64_t
#include "rocket/preprocessor_utilities.h"
#include "rocket/cow_string.hpp"
#include "rocket/cow_vector.hpp"
#include "rocket/cow_hashmap.hpp"
#include "rocket/static_vector.hpp"

namespace Asteria {

// Aliases
using Nullptr   = std::nullptr_t;
using Boolean   = bool;
using Sint8     = std::int8_t;
using Uint8     = std::uint8_t;
using Sint16    = std::int16_t;
using Uint16    = std::uint16_t;
using Sint32    = std::int32_t;
using Uint32    = std::uint32_t;
using Sint64    = std::int64_t;
using Uint64    = std::uint64_t;
using Sintmax   = std::intmax_t;
using Uintmax   = std::uintmax_t;
using Sintptr   = std::intptr_t;
using Uintptr   = std::uintptr_t;
using Size      = std::size_t;
using Diff      = std::ptrdiff_t;
using Float32   = float;
using Float64   = double;
using String    = rocket::cow_string;

template<typename ElementT, Size sizeT>
  using Array = std::array<ElementT, sizeT>;
template<typename ElementT>
  using Vector = rocket::cow_vector<ElementT>;
template<typename FirstT, typename SecondT>
  using Bivector = rocket::cow_vector<std::pair<FirstT, SecondT>>;
template<typename ElementT>
  using Dictionary = rocket::cow_hashmap<String, ElementT, String::hash, String::equal_to>;
template<typename ElementT, Size capacityT>
  using Svector = rocket::static_vector<ElementT, capacityT>;

// General Utilities
class Formatter;
class Runtime_error;
class Exception;

// Lexical Elements
class Xpnode;
class Expression;
class Statement;
class Block;
class Source_location;
class Function_header;

// Parser Objects
class Parser_error;
class Token;
class Token_stream;
class Parser;
class Single_source_file;

// Runtime Objects
class Value;
class Abstract_opaque;
class Shared_opaque_wrapper;
class Abstract_function;
class Shared_function_wrapper;
class Reference_root;
class Reference_modifier;
class Reference;
class Reference_stack;
class Variable;
class Variable_hashset;
class Abstract_variable_callback;
class Collector;
class Abstract_context;
class Analytic_context;
class Executive_context;
class Global_context;
class Global_collector;
class Variadic_arguer;
class Instantiated_function;

// Runtime Data Types
using D_null      = Nullptr;
using D_boolean   = Boolean;
using D_integer   = Sint64;
using D_real      = Float64;
using D_string    = String;
using D_opaque    = Shared_opaque_wrapper;
using D_function  = Shared_function_wrapper;
using D_array     = Vector<Value>;
using D_object    = Dictionary<Value>;

}

#endif
