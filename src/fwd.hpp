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

// Runtime objects (movable only)
class Variable;
class Reference;
class Exception;
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
struct Opaque_struct {
	std::array<unsigned char, 16> uuid;
	std::intptr_t context;
	std::shared_ptr<void> handle;
};

struct Function_struct {
	boost::container::vector<std::function<Reference (Spref<Recycler>)>> default_argument_list_opt;
	std::function<Reference (Spref<Recycler>, boost::container::vector<Reference> &&)> payload;
};

using Null      = std::nullptr_t;
using Boolean   = bool;
using Integer   = std::int64_t;
using Double    = double;
using String    = std::string;
using Opaque    = Opaque_struct;
using Function  = Function_struct;
using Array     = Xptr_vector<Variable>;
using Object    = Xptr_map<std::string, Variable>;

// Miscellaneous struct definitions.
struct Scoped_variable {
	Xptr<Variable> variable;
	bool immutable;
};

}

#define ASTERIA_UNLESS_IS_BASE_OF(Base_, ParamT_)     typename ::std::enable_if<!(::std::is_base_of<Base_, typename ::std::decay<ParamT_>::type>::value)>::type * = nullptr

#endif
