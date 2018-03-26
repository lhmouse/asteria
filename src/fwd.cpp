// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "fwd.hpp"

template class boost::container::deque<Asteria::Value_ptr<Asteria::Variable>>;
template class boost::container::flat_map<std::string, Asteria::Value_ptr<Asteria::Variable>>;
template class std::function<Asteria::Value_ptr<Asteria::Variable> (Asteria::Array &&)>;

template class boost::container::deque<Asteria::Value_ptr<Asteria::Statement>>;
template class boost::container::deque<Asteria::Value_ptr<Asteria::Expression>>;
template class boost::container::flat_map<std::string, Asteria::Value_ptr<Asteria::Initializer>>;
