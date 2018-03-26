// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <boost/container/deque.hpp>
#include <boost/container/flat_map.hpp>
#include <memory> // std::shared_ptr
#include <string>
#include <functional> // std::function
#include <cstddef> // std::nullptr_t
#include <cstdint> // std::int64_t

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
using Array     = boost::container::deque<std::shared_ptr<Variable>>;
using Object    = boost::container::flat_map<std::string, std::shared_ptr<Variable>>;
using Function  = std::function<std::shared_ptr<Variable> (boost::container::deque<std::shared_ptr<Variable>> &&)>;

}

extern template class std::shared_ptr<void>;
extern template class boost::container::deque<std::shared_ptr<Asteria::Variable>>;
extern template class boost::container::flat_map<std::string, std::shared_ptr<Asteria::Variable>>;
extern template class std::function<std::shared_ptr<Asteria::Variable> (boost::container::deque<std::shared_ptr<Asteria::Variable>> &&)>;

#endif
