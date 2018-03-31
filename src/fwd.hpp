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

// Lexical elements (copyable and movable)
class Initializer;
class Expression;
class Statement;

// Runtime objects (copyable and movable)
class Variable;
class Stored_value;
class Reference;

// Runtime objects (movable only)
class Exception;

// Runtime objects (neither copyable nor movable)
class Scope;
class Recycler;

// Aliases.
template<typename ElementT>
using Value_ptr_vector = boost::container::vector<Value_ptr<ElementT>>;
template<typename ElementT>
using Value_ptr_vector = boost::container::vector<Value_ptr<ElementT>>;
template<typename KeyT, typename ValueT>
using Value_ptr_map = boost::container::flat_map<KeyT, Value_ptr<ValueT>>;

// Struct definitions.
struct Scoped_variable {
	Value_ptr<Variable> variable;
	bool immutable;
};

struct Function_parameter {
	std::string identifier;
	Value_ptr<Initializer> default_initializer_opt;
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
using Array     = boost::container::vector<Value_ptr<Variable>>;
using Object    = boost::container::flat_map<std::string, Value_ptr<Variable>>;
// If you want to add a new type, don't forget to update the enumerations in 'variable.hpp' and 'stored_value.hpp' accordingly.

}

#define ASTERIA_UNLESS_IS_BASE_OF(Base_, ParamT_)	\
	typename ::std::enable_if<	\
		!(::std::is_base_of<Base_, typename ::std::decay<ParamT_>::type>::value)	\
		>::type * = nullptr

// Instantiated in 'src/reference.cpp'.
extern template class std::function<Asteria::Reference (boost::container::vector<Asteria::Reference> &&)>;
// Instantiated in 'src/variable.cpp'.
extern template class boost::container::vector<Asteria::Value_ptr<Asteria::Variable>>;
extern template class boost::container::flat_map<std::string, Asteria::Value_ptr<Asteria::Variable>>;

#endif
