// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "fwd.hpp"

template class std::shared_ptr<void>;
template class boost::container::deque<std::shared_ptr<Asteria::Variable>>;
template class boost::container::flat_map<std::string, std::shared_ptr<Asteria::Variable>>;
template class std::function<std::shared_ptr<Asteria::Variable> (boost::container::deque<std::shared_ptr<Asteria::Variable>> &&)>;
