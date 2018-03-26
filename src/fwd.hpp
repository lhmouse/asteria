// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FWD_HPP_
#define ASTERIA_FWD_HPP_

#include <boost/container/deque.hpp>
#include <boost/container/flat_map.hpp>
#include <memory> // std::shared_ptr
#include <string>
#include <functional> // std::function

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

}

extern template class boost::container::deque<std::shared_ptr<Asteria::Variable>>;
extern template class boost::container::flat_map<std::string, std::shared_ptr<Asteria::Variable>>;
extern template class std::function<std::shared_ptr<Asteria::Variable> (boost::container::deque<std::shared_ptr<Asteria::Variable>> &&)>;

#endif
