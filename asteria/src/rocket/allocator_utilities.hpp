// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ALLOCATOR_UTILITIES_HPP_
#define ROCKET_ALLOCATOR_UTILITIES_HPP_

#include <type_traits> // std::conditional<>, std::false_type, std::true_type
#include <utility> // std::move(), std::swap()

namespace rocket {

using ::std::conditional;
using ::std::false_type;
using ::std::true_type;

namespace details_allocator_utilities {
	template<typename typeT>
	struct is_final
#ifdef __cpp_lib_is_final
		: ::std::is_final<typeT>
#else
		: false_type
#endif
	{ };

	template<typename allocatorT>
	class final_wrapper {
	private:
		allocatorT m_alloc;

	public:
		explicit final_wrapper(const allocatorT &alloc) noexcept
			: m_alloc(alloc)
		{ }
		explicit final_wrapper(allocatorT &&alloc) noexcept
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
}

template<typename allocatorT>
struct allocator_wrapper_base_for
	: conditional<details_allocator_utilities::is_final<allocatorT>::value, details_allocator_utilities::final_wrapper<allocatorT>, allocatorT>
{ };

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
