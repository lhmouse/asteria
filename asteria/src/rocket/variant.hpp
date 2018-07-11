// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#define ROCKET_VARIANT_HPP_

#include <type_traits> // so many...
#include <utility> // std::move(), std::forward(), std::declval(), std::swap()
#include <memory> // std::addressof()
#include <new> // placement new
#include <typeinfo>
#include "assert.hpp"
#include "throw.hpp"

namespace rocket {

using ::std::common_type;
using ::std::is_convertible;
using ::std::decay;
using ::std::enable_if;
using ::std::is_nothrow_constructible;
using ::std::is_nothrow_assignable;
using ::std::is_nothrow_copy_constructible;
using ::std::is_nothrow_move_constructible;
using ::std::is_nothrow_copy_assignable;
using ::std::is_nothrow_move_assignable;
using ::std::is_nothrow_destructible;
using ::std::integral_constant;
using ::std::conditional;
using ::std::false_type;
using ::std::true_type;
using ::std::type_info;

template<typename ...elementsT>
class variant;

namespace details_variant {
	template<unsigned indexT, typename ...typesT>
	struct type_getter
		// `type` is not defined.
	{ };
	template<typename firstT, typename ...remainingT>
	struct type_getter<0, firstT, remainingT...>
		: common_type<firstT>
	{ };
	template<unsigned indexT, typename firstT, typename ...remainingT>
	struct type_getter<indexT, firstT, remainingT...>
		: type_getter<indexT - 1, remainingT...>
	{ };

	template<unsigned indexT, typename expectT, typename ...typesT>
	struct type_finder
		// `value` is not defined.
	{ };
	template<unsigned indexT, typename expectT, typename firstT, typename ...remainingT>
	struct type_finder<indexT, expectT, firstT, remainingT...>
		: type_finder<indexT + 1, expectT, remainingT...>
	{ };
	template<unsigned indexT, typename expectT, typename ...remainingT>
	struct type_finder<indexT, expectT, expectT, remainingT...>
		: integral_constant<unsigned, indexT>
	{ };

	template<typename ...typesT>
	struct conjunction
		: true_type
	{ };
	template<typename firstT, typename ...remainingT>
	struct conjunction<firstT, remainingT...>
		: conditional<firstT::value, conjunction<remainingT...>, false_type>::type
	{ };

	template<typename expectT, typename ...typesT>
	struct has_type_recursive
		: false_type
	{ };
	template<typename expectT, typename firstT, typename ...remainingT>
	struct has_type_recursive<expectT, firstT, remainingT...>
		: has_type_recursive<expectT, remainingT...>
	{ };
	template<typename expectT, typename ...remainingT>
	struct has_type_recursive<expectT, expectT, remainingT...>
		: true_type
	{ };
	template<typename expectT, typename ...elementsT, typename ...remainingT>
	struct has_type_recursive<expectT, variant<elementsT...>, remainingT...>
		: conditional<has_type_recursive<expectT, elementsT...>::value, true_type, has_type_recursive<expectT, remainingT...>>::type
	{ };
	template<typename ...elementsT, typename ...remainingT>
	struct has_type_recursive<variant<elementsT...>, variant<elementsT...>, remainingT...>
		: true_type
	{ };

	template<unsigned indexT, typename expectT, typename ...typesT>
	struct recursive_type_finder
		// `value` is not defined.
	{ };
	template<unsigned indexT, typename expectT, typename firstT, typename ...remainingT>
	struct recursive_type_finder<indexT, expectT, firstT, remainingT...>
		: conditional<has_type_recursive<expectT, firstT>::value, integral_constant<unsigned, indexT>, recursive_type_finder<indexT + 1, expectT, remainingT...>>::type
	{ };

	template<size_t firstT, size_t ...remainingT>
	struct max_of
		: max_of<firstT, max_of<remainingT...>::value>
	{ };
	template<size_t firstT>
	struct max_of<firstT>
		: integral_constant<size_t, firstT>
	{ };
	template<size_t firstT, size_t secondT>
	struct max_of<firstT, secondT>
		: integral_constant<size_t, !(firstT < secondT) ? firstT : secondT>
	{ };

	template<typename ...elementsT>
	union storage_for {
		char bytes[max_of<sizeof(elementsT)...>::value];
		alignas(max_of<alignof(elementsT)...>::value) char align;
	};

	namespace details_is_nothrow_swappable {
		using ::std::swap;

		template<typename paramT>
		struct is_nothrow_swappable
			: integral_constant<bool, noexcept(swap(::std::declval<paramT &>(), ::std::declval<paramT &>()))>
		{ };
	}

	using details_is_nothrow_swappable::is_nothrow_swappable;

	template<typename typeT, typename ...paramsT>
	inline typeT * construct(typeT *ptr, paramsT &&...params){
		return ::new(static_cast<void *>(ptr)) typeT(::std::forward<paramsT>(params)...);
	}
	template<typename typeT>
	inline void destruct(typeT *ptr) noexcept {
		ptr->~typeT();
	}

	template<typename resultT, typename sourceT>
	inline const resultT * pun(const sourceT *ptr) noexcept {
		const auto vptr = static_cast<const void *>(ptr);
		return static_cast<const resultT *>(vptr);
	}
	template<typename resultT, typename sourceT>
	inline resultT * pun(sourceT *ptr) noexcept {
		const auto vptr = static_cast<void *>(ptr);
		return static_cast<resultT *>(vptr);
	}

	template<typename ...elementsT>
	struct visit_helper {
		template<typename storageT, typename visitorT, typename ...paramsT>
		void operator()(storageT * /*stor*/, unsigned /*expect*/, visitorT &&/*visitor*/, paramsT &&.../*params*/) const {
			ROCKET_ASSERT_MSG(false, "The type index provided was out of range.");
		}
	};
	template<typename firstT, typename ...remainingT>
	struct visit_helper<firstT, remainingT...> {
		template<typename storageT, typename visitorT, typename ...paramsT>
		void operator()(storageT *stor, unsigned expect, visitorT &&visitor, paramsT &&...params) const {
			if(expect == 0){
				::std::forward<visitorT>(visitor)(pun<firstT>(stor), ::std::forward<paramsT>(params)...);
			} else {
				visit_helper<remainingT...>()(stor, expect - 1, ::std::forward<visitorT>(visitor), ::std::forward<paramsT>(params)...);
			}
		}
	};

	struct visitor_copy_construct {
		template<typename elementT, typename sourceT>
		void operator()(elementT *ptr, const sourceT *src) const {
			construct<>(ptr, *pun<elementT>(src));
		}
	};
	struct visitor_move_construct {
		template<typename elementT, typename sourceT>
		void operator()(elementT *ptr, sourceT *src) const {
			construct<>(ptr, ::std::move(*pun<elementT>(src)));
		}
	};
	struct visitor_copy_assign {
		template<typename elementT, typename sourceT>
		void operator()(elementT *ptr, const sourceT *src) const {
			*ptr = *pun<elementT>(src);
		}
	};
	struct visitor_move_assign {
		template<typename elementT, typename sourceT>
		void operator()(elementT *ptr, sourceT *src) const {
			*ptr = ::std::move(*pun<elementT>(src));
		}
	};
	struct visitor_destruct {
		template<typename elementT>
		void operator()(elementT *ptr) const {
			destruct<>(ptr);
		}
	};
	struct visitor_get_type_info {
		template<typename elementT>
		void operator()(const elementT *ptr, const type_info **ti) const {
			*ti = ::std::addressof(typeid(*ptr));
		}
	};
	struct visitor_wrapper {
		template<typename nextT, typename elementT>
		void operator()(elementT *ptr, nextT &&next) const {
			::std::forward<nextT>(next)(*ptr);
		}
	};
	struct visitor_swap {
		template<typename elementT, typename sourceT>
		void operator()(elementT *ptr, sourceT *src) const {
			noadl::adl_swap(*ptr, *pun<elementT>(src));
		}
	};
}

template<typename ...elementsT>
class variant {
	static_assert(details_variant::conjunction<is_nothrow_destructible<elementsT>...>::value, "Destructors of candidate types are not allowed to throw exceptions.");

public:
	template<unsigned indexT>
	struct at {
		using type = typename details_variant::type_getter<indexT, elementsT...>::type;
	};
	template<typename elementT>
	struct index_of {
		enum : unsigned { value = details_variant::type_finder<0, elementT, elementsT...>::value };
	};

	template<typename ...addT>
	struct prepend {
		using type = variant<addT..., elementsT...>;
	};
	template<typename ...addT>
	struct append {
		using type = variant<elementsT..., addT...>;
	};

	using storage = details_variant::storage_for<elementsT...>;

private:
	unsigned m_turnout : 1;
	unsigned m_index : 31;
	storage m_buffers[2];

private:
	const storage * do_get_front_buffer() const noexcept {
		const unsigned turnout = this->m_turnout;
		return this->m_buffers + turnout;
	}
	storage * do_get_front_buffer() noexcept {
		const unsigned turnout = this->m_turnout;
		return this->m_buffers + turnout;
	}
	const storage * do_get_back_buffer() const noexcept {
		const unsigned turnout = this->m_turnout;
		return this->m_buffers + (turnout ^ 1);
	}
	storage * do_get_back_buffer() noexcept {
		const unsigned turnout = this->m_turnout;
		return this->m_buffers + (turnout ^ 1);
	}

	void do_set_up_new_buffer(unsigned index_new) noexcept {
		const unsigned turnout_old = this->m_turnout;
		this->m_turnout = (turnout_old ^ 1) & 1;
		const unsigned index_old = this->m_index;
		this->m_index = index_new & 0x7FFFFFFF;
		// Destroy the old buffer and poison its contents.
		details_variant::visit_helper<elementsT...>()(this->m_buffers + turnout_old, index_old,
		                                              details_variant::visitor_destruct());
#ifdef ROCKET_DEBUG
		std::memset(this->m_buffers + turnout_old, '@', sizeof(storage));
#endif
	}

public:
	variant() noexcept(is_nothrow_constructible<typename details_variant::type_getter<0, elementsT...>::type>::value)
		: m_turnout(0)
	{
		// Value-initialize the first element.
		constexpr unsigned eindex = 0;
		using etype = typename details_variant::type_getter<eindex, elementsT...>::type;
		details_variant::construct(details_variant::pun<etype>(this->do_get_front_buffer()));
		this->m_index = 0;
	}
	template<typename elementT, typename enable_if<details_variant::has_type_recursive<typename decay<elementT>::type, elementsT...>::value>::type * = nullptr>
	variant(elementT &&elem)
		: m_turnout(0)
	{
		// This overload enables construction using the candidate of a nested variant.
		constexpr unsigned eindex = details_variant::recursive_type_finder<0, typename decay<elementT>::type, elementsT...>::value;
		using etype = typename details_variant::type_getter<eindex, elementsT...>::type;
		details_variant::construct(details_variant::pun<etype>(this->do_get_front_buffer()), ::std::forward<elementT>(elem));
		this->m_index = eindex;
	}
	variant(const variant &other) noexcept(details_variant::conjunction<is_nothrow_copy_constructible<elementsT>...>::value)
		: m_turnout(0)
	{
		// Copy-construct the active element from `other`.
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), other.m_index,
		                                              details_variant::visitor_copy_construct(), other.do_get_front_buffer());
		this->m_index = other.m_index;
	}
	variant(variant &&other) noexcept(details_variant::conjunction<is_nothrow_move_constructible<elementsT>...>::value)
		: m_turnout(0)
	{
		// Move-construct the active element from `other`.
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), other.m_index,
		                                              details_variant::visitor_move_construct(), other.do_get_front_buffer());
		this->m_index = other.m_index;
	}
	template<typename elementT, typename enable_if<details_variant::has_type_recursive<typename decay<elementT>::type, elementsT...>::value>::type * = nullptr>
	variant & operator=(elementT &&elem){
		// This overload, unlike `set()`, enables assignment using the candidate of a nested variant.
		constexpr unsigned eindex = details_variant::recursive_type_finder<0, typename decay<elementT>::type, elementsT...>::value;
		using etype = typename details_variant::type_getter<eindex, elementsT...>::type;
		if(this->m_index == eindex){
			// Assign the active element using perfect forwarding.
			*details_variant::pun<etype>(this->do_get_front_buffer()) = ::std::forward<elementT>(elem);
		} else {
			// Construct the active element using perfect forwarding, then destroy the old element.
			details_variant::construct(details_variant::pun<etype>(this->do_get_back_buffer()), ::std::forward<elementT>(elem));
			this->do_set_up_new_buffer(eindex);
		}
		return *this;
	}
	variant & operator=(const variant &other) noexcept(details_variant::conjunction<is_nothrow_copy_assignable<elementsT>..., is_nothrow_copy_constructible<elementsT>...>::value) {
		if(this->m_index == other.m_index){
			// Copy-assign the active element from `other`
			details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), other.m_index,
			                                              details_variant::visitor_copy_assign(), other.do_get_front_buffer());
		} else {
			// Copy-construct the active element from `other`, then destroy the old element.
			details_variant::visit_helper<elementsT...>()(this->do_get_back_buffer(), other.m_index,
			                                              details_variant::visitor_copy_construct(), other.do_get_front_buffer());
			this->do_set_up_new_buffer(other.m_index);
		}
		return *this;
	}
	variant & operator=(variant &&other) noexcept(details_variant::conjunction<is_nothrow_move_assignable<elementsT>..., is_nothrow_move_constructible<elementsT>...>::value) {
		if(this->m_index == other.m_index){
			// Move-assign the active element from `other`
			details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), other.m_index,
			                                              details_variant::visitor_move_assign(), other.do_get_front_buffer());
		} else {
			// Move-construct the active element from `other`, then destroy the old element.
			details_variant::visit_helper<elementsT...>()(this->do_get_back_buffer(), other.m_index,
			                                              details_variant::visitor_move_construct(), other.do_get_front_buffer());
			this->do_set_up_new_buffer(other.m_index);
		}
		return *this;
	}
	~variant(){
		// Destroy the active element and poison all contents.
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), this->m_index,
		                                              details_variant::visitor_destruct());
#ifdef ROCKET_DEBUG
		this->m_index = 0x6EEFDEAD;
		std::memset(m_buffers, '#', sizeof(m_buffers));
#endif
	}

public:
	unsigned index() const noexcept {
		ROCKET_ASSERT(this->m_index < sizeof...(elementsT));
		return this->m_index;
	}
	const type_info & type() const noexcept {
		const type_info *ti = nullptr;
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), this->m_index,
		                                              details_variant::visitor_get_type_info(), &ti);
		ROCKET_ASSERT(ti);
		return *ti;
	}
	template<typename elementT>
	const elementT * get() const noexcept {
		constexpr unsigned eindex = details_variant::type_finder<0, elementT, elementsT...>::value;
		if(this->m_index != eindex){
			return nullptr;
		}
		return details_variant::pun<elementT>(this->do_get_front_buffer());
	}
	template<typename elementT>
	elementT * get() noexcept {
		constexpr unsigned eindex = details_variant::type_finder<0, elementT, elementsT...>::value;
		if(this->m_index != eindex){
			return nullptr;
		}
		return details_variant::pun<elementT>(this->do_get_front_buffer());
	}
	template<typename elementT>
	const elementT & as() const {
		constexpr unsigned eindex = details_variant::type_finder<0, elementT, elementsT...>::value;
		const auto ptr = this->get<elementT>();
		if(!ptr){
			noadl::throw_invalid_argument("variant::get(): The index requested is `%d` (`%s`), but the index currently active is `%d` (`%s`).",
			                              static_cast<int>(eindex), typeid(elementT).name(), static_cast<int>(this->index()), this->type().name());
		}
		return *ptr;
	}
	template<typename elementT>
	elementT & as(){
		constexpr unsigned eindex = details_variant::type_finder<0, elementT, elementsT...>::value;
		const auto ptr = this->get<elementT>();
		if(!ptr){
			noadl::throw_invalid_argument("variant::get(): The index requested is `%d` (`%s`), but the index currently active is `%d` (`%s`).",
			                              static_cast<int>(eindex), typeid(elementT).name(), static_cast<int>(this->index()), this->type().name());
		}
		return *ptr;
	}
	template<typename elementT>
	void set(elementT &&elem) noexcept(details_variant::conjunction<is_nothrow_move_assignable<elementsT>..., is_nothrow_move_constructible<elementsT>...>::value) {
		// This overload, unlike `operator=()`, does not accept the candidate of a nested variant.
		constexpr unsigned eindex = details_variant::type_finder<0, typename decay<elementT>::type, elementsT...>::value;
		using etype = typename details_variant::type_getter<eindex, elementsT...>::type;
		if(this->m_index == eindex){
			// Assign the active element using perfect forwarding.
			*details_variant::pun<etype>(this->do_get_front_buffer()) = ::std::forward<elementT>(elem);
		} else {
			// Construct the active element using perfect forwarding, then destroy the old element.
			details_variant::construct(details_variant::pun<etype>(this->do_get_back_buffer()), ::std::forward<elementT>(elem));
			this->do_set_up_new_buffer(eindex);
		}
	}

	template<typename visitorT>
	void visit(visitorT &&visitor) const {
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), this->m_index,
		                                              details_variant::visitor_wrapper(), ::std::forward<visitorT>(visitor));
	}
	template<typename visitorT>
	void visit(visitorT &&visitor){
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), this->m_index,
		                                              details_variant::visitor_wrapper(), ::std::forward<visitorT>(visitor));
	}

	void swap(variant &other) noexcept(details_variant::conjunction<details_variant::is_nothrow_swappable<elementsT>..., is_nothrow_move_constructible<elementsT>...>::value) {
		if(this->m_index == other.m_index){
			// Swap the active elements.
			details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), other.m_index,
			                                              details_variant::visitor_swap(), other.do_get_front_buffer());
		} else {
			// Move-construct the active element in `*this` from `other`.
			details_variant::visit_helper<elementsT...>()(this->do_get_back_buffer(), other.m_index,
			                                              details_variant::visitor_move_construct(), other.do_get_front_buffer());
			try {
				// Move-construct the active element in `other` from `*this`.
				details_variant::visit_helper<elementsT...>()(other.do_get_back_buffer(), this->m_index,
				                                              details_variant::visitor_move_construct(), this->do_get_front_buffer());
			} catch(...){
				// In case of an exception, the second object will not have been constructed.
				// Destroy the first object that has just been constructed, then rethrow the exception.
				details_variant::visit_helper<elementsT...>()(this->do_get_back_buffer(), other.m_index,
				                                              details_variant::visitor_destruct());
				throw;
			}
			// Destroy both elements.
			const unsigned this_index = this->m_index;
			this->do_set_up_new_buffer(other.m_index);
			other.do_set_up_new_buffer(this_index);
		}
	}
};

template<typename ...elementsT>
void swap(variant<elementsT...> &lhs, variant<elementsT...> &other) noexcept(noexcept(lhs.swap(other))) {
	lhs.swap(other);
}

}

#endif
