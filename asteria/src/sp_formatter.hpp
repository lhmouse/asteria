// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SP_FORMATTER_HPP_
#define ASTERIA_SP_FORMATTER_HPP_

#include "fwd.hpp"

namespace Asteria {

template<typename ElementT>
class Sp_formatter {
private:
	Sp<const ElementT> m_ptr;

public:
	Sp_formatter(const Sp<const ElementT> &ptr) noexcept
		: m_ptr(ptr)
	{ }

public:
	const Sp<const ElementT> & get() const noexcept {
		return m_ptr;
	}
};

template<typename ElementT>
Sp_formatter<typename std::remove_cv<ElementT>::type> sp_fmt(const Sp<ElementT> &ptr){
	return ptr;
}
template<typename ElementT>
Sp_formatter<typename std::remove_cv<ElementT>::type> sp_fmt(const Wp<ElementT> &ptr){
	return ptr.lock();
}
template<typename ElementT>
Sp_formatter<typename std::remove_cv<ElementT>::type> sp_fmt(const Vp<ElementT> &ptr){
	return ptr.share();
}

}

#endif
