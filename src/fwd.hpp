// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <boost/container/vector.hpp>
#include <boost/container/flat_map.hpp>
#include <string> // std::string
#include <functional> // std::function
#include <type_traits> // std::enable_if, std::decay, std::is_base_of
#include <utility> // std::move, std::pair
#include <memory> // std::shared_ptr
#include <cstddef> // std::nullptr_t
#include <cstdint> // std::int64_t
#include "value_ptr.hpp"

namespace Asteria {

// General utilities.
class Insertable_streambuf;
class Insertable_ostream;
class Logger;

// Lexical elements (movable only)
class Initializer;
class Expression;
class Statement;

// Runtime objects (copyable)
class Variable;
class Exception;

// Runtime objects (movable only)
class Nullable_value;
class Reference;

// Runtime objects (not movable)
class Scope;
class Recycler;

// General utilities.
struct Deleted_copy {
	constexpr Deleted_copy() = default;
	Deleted_copy(Deleted_copy &&) = default;
	Deleted_copy &operator=(Deleted_copy &&) = default;
};
struct Deleted_move {
	constexpr Deleted_move() = default;
	Deleted_move(Deleted_move &&) = delete;
	Deleted_move &operator=(Deleted_move &&) = delete;
};

// Aliases.
template<typename ElementT>
using Shared_ptr = std::shared_ptr<ElementT>;

template<typename ElementT>
using Value_ptr_vector = boost::container::vector<Value_ptr<ElementT>>;
template<typename ElementT>
using Value_ptr_vector = boost::container::vector<Value_ptr<ElementT>>;
template<typename KeyT, typename ValueT>
using Value_ptr_map = boost::container::flat_map<KeyT, Value_ptr<ValueT>>;

// Struct definitions.
struct Named_variable {
	Value_ptr<Variable> variable;
};

struct Magic_handle {
	std::string magic;
	std::shared_ptr<void> handle;
};

// Runtime types.
using Null      = std::nullptr_t;
using Boolean   = bool;
using Integer   = std::int64_t;
using Double    = double;
using String    = std::string;
using Opaque    = Magic_handle;
using Array     = Value_ptr_vector<Variable>;
using Object    = Value_ptr_map<std::string, Variable>;
using Function  = std::function<Shared_ptr<Variable> (boost::container::vector<Shared_ptr<Variable>> &&)>;
// If you want to add a new type, don't forget to update the enumerations in 'variable.hpp' and 'nullable_value.hpp' accordingly.

}

#define ASTERIA_UNLESS_IS_BASE_OF(Base_, ParamT_)	\
	typename ::std::enable_if<	\
		!(::std::is_base_of<Base_, typename ::std::decay<ParamT_>::type>::value)	\
		>::type * = nullptr

#endif
