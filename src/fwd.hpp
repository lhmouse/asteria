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
using Spref = const Sptr<ElementT> &;

// Aliases.
template<typename ElementT>
using Xptr_vector = boost::container::vector<Xptr<ElementT>>;
template<typename ElementT>
using Xptr_vector = boost::container::vector<Xptr<ElementT>>;
template<typename KeyT, typename ValueT>
using Xptr_map = boost::container::flat_map<KeyT, Xptr<ValueT>>;

// Struct definitions.
struct Scoped_variable {
	Xptr<Variable> variable;
	bool immutable;
};

struct Function_parameter {
	std::string identifier;
	Xptr<Initializer> default_initializer_opt;
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
using Function  = std::function<Reference (boost::container::vector<Reference> &&)>;
using Array     = boost::container::vector<Xptr<Variable>>;
using Object    = boost::container::flat_map<std::string, Xptr<Variable>>;
// If you want to add a new type, don't forget to update the enumerations in 'variable.hpp' and 'stored_value.hpp' accordingly.

}

#define ASTERIA_UNLESS_IS_BASE_OF(Base_, ParamT_)	\
	typename ::std::enable_if<	\
		!(::std::is_base_of<Base_, typename ::std::decay<ParamT_>::type>::value)	\
		>::type * = nullptr

#endif
