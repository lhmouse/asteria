// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FINAL_ALLOCATOR_WRAPPER_HPP_
#define ROCKET_FINAL_ALLOCATOR_WRAPPER_HPP_

#include <type_traits> // std::conditional<>
#include <utility> // std::move()

namespace rocket {

using ::std::conditional;

template<typename allocatorT>
class final_allocator_wrapper {
private:
	allocatorT m_alloc;

public:
	explicit final_allocator_wrapper(const allocatorT &alloc) noexcept
		: m_alloc(alloc)
	{ }
	explicit final_allocator_wrapper(allocatorT &&alloc) noexcept
		: m_alloc(::std::move(alloc))
	{ }

public:
	operator const allocatorT & () const noexcept {
		return this->m_alloc;
	}
	operator allocatorT & () noexcept {
		return this->m_alloc;
	}
};

template<typename allocatorT>
using allocator_wrapper_base_for =
#ifdef __cpp_lib_is_final
	typename conditional<true, final_allocator_wrapper<allocatorT>, allocatorT>::type
#else
	allocatorT
#endif
	;

}

#endif
