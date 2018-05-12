// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SPTR_FMT_HPP_
#define ASTERIA_SPTR_FMT_HPP_

#include "fwd.hpp"

namespace Asteria {

template<typename ElementT>
class Sptr_fmt {
private:
	Sptr<const ElementT> m_ptr;

public:
	Sptr_fmt(const Sptr<const ElementT> &ptr) noexcept
		: m_ptr(ptr)
	{ }

public:
	const Sptr<const ElementT> & get() const noexcept {
		return m_ptr;
	}
};

template<typename ElementT>
Sptr_fmt<typename std::remove_cv<ElementT>::type> sptr_fmt(const Sptr<ElementT> &ptr){
	return ptr;
}
template<typename ElementT>
Sptr_fmt<typename std::remove_cv<ElementT>::type> sptr_fmt(const Wptr<ElementT> &ptr){
	return ptr.lock();
}
template<typename ElementT>
Sptr_fmt<typename std::remove_cv<ElementT>::type> sptr_fmt(const Xptr<ElementT> &ptr){
	return ptr.share();
}

}

#endif
