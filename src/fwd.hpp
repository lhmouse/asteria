// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <string> // std::string
#include <functional> // std::function
#include <type_traits> // std::enable_if<>, std::decay<>, std::is_base_of<>
#include <utility> // std::move(), std::forward(), std::pair<>
#include <memory> // std::shared_ptr<>
#include <cstddef> // std::nullptr_t
#include <cstdint> // std::int64_t
#include <boost/container/vector.hpp>
#include <boost/container/flat_map.hpp>
#include "xptr.hpp"

namespace Asteria {

// General utilities.
class Insertable_streambuf;
class Insertable_ostream;
class Logger;

// Lexical elements (movable only)
class Initializer;
class Expression;
class Statement;

// Runtime objects (copyable and movable)
class Exception;

// Runtime objects (movable only)
class Stored_value;
class Stored_reference;

// Runtime objects (neither copyable nor movable)
class Variable;
class Reference;
class Scope;
class Recycler;

// Aliases.
template<typename ElementT>
using Sptr = std::shared_ptr<ElementT>;
template<typename ElementT>
using Spref = const std::shared_ptr<ElementT> &;

template<typename ElementT>
using Wptr = std::weak_ptr<ElementT>;
template<typename ElementT>
using Wpref = const std::weak_ptr<ElementT> &;

template<typename ElementT>
using Sptr_vector = boost::container::vector<Sptr<ElementT>>;
template<typename KeyT, typename ValueT>
using Sptr_map = boost::container::flat_map<KeyT, Sptr<ValueT>>;

template<typename ElementT>
using Wptr_vector = boost::container::vector<Wptr<ElementT>>;
template<typename KeyT, typename ValueT>
using Wptr_map = boost::container::flat_map<KeyT, Wptr<ValueT>>;

template<typename ElementT>
using Xptr_vector = boost::container::vector<Xptr<ElementT>>;
template<typename KeyT, typename ValueT>
using Xptr_map = boost::container::flat_map<KeyT, Xptr<ValueT>>;

// Runtime objects.
struct Local_variable {
	Xptr<Variable> variable_opt;
	bool immutable;
};

enum Comparison_result : unsigned {
	comparison_result_unordered  = 1,
	comparison_result_less       = 2,
	comparison_result_equal      = 3,
	comparison_result_greater    = 4,
};

// Runtime types.
struct Uuid_handle {
	std::array<unsigned char, 16> uuid;
	std::shared_ptr<void> handle;
};

struct Binding_function {
	Sptr_vector<const Variable> default_arguments_opt;
	std::function<Xptr<Reference> (Spref<Recycler>, Xptr<Reference> /*this_opt*/, Xptr_vector<Reference> &&/*arguments*/)> function;
};

using D_null      = std::nullptr_t;
using D_boolean   = bool;
using D_integer   = std::int64_t;
using D_double    = double;
using D_string    = std::string;
using D_opaque    = Uuid_handle;
using D_function  = Binding_function;
using D_array     = Xptr_vector<Variable>;
using D_object    = Xptr_map<std::string, Variable>;

}

#define ASTERIA_UNLESS_IS_BASE_OF(Base_, ParamT_)     typename ::std::enable_if<!(::std::is_base_of<Base_, typename ::std::decay<ParamT_>::type>::value)>::type * = nullptr

#endif
