// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <string> // std::string
#include <functional> // std::function
#include <type_traits> // std::enable_if, std::decay, std::is_base_of
#include <utility> // std::move, std::forward, std::pair
#include <memory> // std::shared_ptr
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
class Variable;
class Reference;
class Stored_value;

// Runtime objects (neither copyable nor movable)
class Scope;
class Recycler;

// Aliases.
template<typename ElementT>
using Sptr = std::shared_ptr<ElementT>;
template<typename ElementT>
using Spref = const std::shared_ptr<ElementT> &;

template<typename ElementT>
using Xptr_vector = boost::container::vector<Xptr<ElementT>>;
template<typename ElementT>
using Xptr_vector = boost::container::vector<Xptr<ElementT>>;
template<typename KeyT, typename ValueT>
using Xptr_map = boost::container::flat_map<KeyT, Xptr<ValueT>>;

// Runtime types.
struct Uuid_handle {
	std::array<unsigned char, 16> uuid;
	std::shared_ptr<void> handle;
};

using Argument_generator_prototype = Xptr<Reference> (Spref<Recycler> recycler);
using Binding_function_prototype   = Xptr<Reference> (Spref<Recycler> recycler, boost::container::vector<Xptr<Reference>> &&arguments);

struct Binding_function {
	boost::container::vector<std::function<Argument_generator_prototype>> default_argument_generators_opt;
	std::function<Binding_function_prototype> function;
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

// Miscellaneous struct definitions.
struct Scoped_variable {
	Xptr<Variable> variable_opt;
	bool immutable;
};

}

#define ASTERIA_UNLESS_IS_BASE_OF(Base_, ParamT_)     typename ::std::enable_if<!(::std::is_base_of<Base_, typename ::std::decay<ParamT_>::type>::value)>::type * = nullptr

#endif
