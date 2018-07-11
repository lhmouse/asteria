// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FINAL_ALLOCATOR_UTILITIES_HPP_
#define ROCKET_FINAL_ALLOCATOR_UTILITIES_HPP_

#include <type_traits> // std::conditional<>, std::false_type, std::true_type
#include <utility> // std::move(), std::swap()

namespace rocket {

using ::std::conditional;
using ::std::false_type;
using ::std::true_type;

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

struct allocator_copy_assign_from { };
struct allocator_move_assign_from { };
struct allocator_swap_with { };

template<typename tagT, typename allocatorT>
inline void manipulate_allocators(false_type, tagT, allocatorT &, const allocatorT &) noexcept {
	// nothing to do
}
template<typename allocatorT>
inline void manipulate_allocators(true_type, allocator_copy_assign_from, allocatorT &lhs, const allocatorT &rhs) noexcept {
	lhs = rhs;
}
template<typename allocatorT>
inline void manipulate_allocators(true_type, allocator_move_assign_from, allocatorT &lhs, allocatorT &&rhs) noexcept {
	lhs = ::std::move(rhs);
}
template<typename allocatorT>
inline void manipulate_allocators(true_type, allocator_swap_with, allocatorT &lhs, allocatorT &rhs) noexcept {
	noadl::adl_swap(lhs, rhs);
}

}

#endif
