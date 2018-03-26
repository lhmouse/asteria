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
class Initializer;
class Expression;

using Null      = std::nullptr_t;
using Boolean   = bool;
using Integer   = std::int64_t;
using Double    = double;
using String    = std::string;
using Opaque    = std::shared_ptr<void>;
using Array     = boost::container::deque<Asteria::Value_ptr<Variable>>;
using Object    = boost::container::flat_map<std::string, Asteria::Value_ptr<Variable>>;
using Function  = std::function<Asteria::Value_ptr<Variable> (Array &&)>;

using Statement_list  = boost::container::deque<Asteria::Value_ptr<Statement>>;
using Expression_list = boost::container::deque<Asteria::Value_ptr<Expression>>;
using Key_value_list  = boost::container::flat_map<std::string, Asteria::Value_ptr<Initializer>>;

}

extern template class boost::container::deque<Asteria::Value_ptr<Asteria::Variable>>;
extern template class boost::container::flat_map<std::string, Asteria::Value_ptr<Asteria::Variable>>;
extern template class std::function<Asteria::Value_ptr<Asteria::Variable> (Asteria::Array &&)>;

extern template class boost::container::deque<Asteria::Value_ptr<Asteria::Statement>>;
extern template class boost::container::deque<Asteria::Value_ptr<Asteria::Expression>>;
extern template class boost::container::flat_map<std::string, Asteria::Value_ptr<Asteria::Initializer>>;

#endif
