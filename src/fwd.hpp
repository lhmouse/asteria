// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <boost/container/deque.hpp>
#include <boost/container/flat_map.hpp>
#include <iosfwd> // std::ostream
#include <string> // std::string
#include <functional> // std::function
#include <cstddef> // std::nullptr_t
#include <cstdint> // std::int64_t
#include "value_ptr.hpp"

namespace Asteria {

template<typename ...TypesT>
struct Type_tuple;

class Insertable_streambuf;
class Insertable_ostream;
class Logger;

class Variable;

class Statement;
class Expression;
class Initializer;

template<typename ElementT>
using Value_ptr_deque = boost::container::deque<Value_ptr<ElementT>>;
template<typename KeyT, typename ValueT>
using Value_ptr_map = boost::container::flat_map<KeyT, Value_ptr<ValueT>>;

using Null      = std::nullptr_t;
using Boolean   = bool;
using Integer   = std::int64_t;
using Double    = double;
using String    = std::string;
using Opaque    = std::shared_ptr<void>;
using Array     = Value_ptr_deque<Variable>;
using Object    = Value_ptr_map<std::string, Variable>;
using Function  = std::function<Asteria::Value_ptr<Variable> (Array &&)>;

}

extern template class boost::container::deque<Asteria::Value_ptr<Asteria::Variable>>;
extern template class boost::container::flat_map<std::string, Asteria::Value_ptr<Asteria::Variable>>;
extern template class std::function<Asteria::Value_ptr<Asteria::Variable> (Asteria::Array &&)>;

#endif
