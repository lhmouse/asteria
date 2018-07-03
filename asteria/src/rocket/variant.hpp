// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#define ROCKET_VARIANT_HPP_

#include <type_traits> // so many...
#include <utility> // std::move(), std::forward(), std::declval(), std::swap()
#include <memory> // std::addressof()
#include <new> // placement new
#include "assert.hpp"
#include "throw.hpp"

namespace rocket {

using ::std::is_convertible;
using ::std::decay;
using ::std::enable_if;
using ::std::is_const;
using ::std::remove_cv;
using ::std::is_nothrow_constructible;
using ::std::is_nothrow_assignable;
using ::std::is_nothrow_copy_constructible;
using ::std::is_nothrow_move_constructible;
using ::std::is_nothrow_copy_assignable;
using ::std::is_nothrow_move_assignable;
using ::std::is_nothrow_destructible;
using ::std::conditional;

template<typename ...elementsT>
class variant;

namespace details_variant {
	template<unsigned indexT, typename ...typesT>
	struct type_getter {
		// `type` is not defined.
	};
	template<typename firstT, typename ...remainingT>
	struct type_getter<0, firstT, remainingT...> {
		using type = firstT;
	};
	template<unsigned indexT, typename firstT, typename ...remainingT>
	struct type_getter<indexT, firstT, remainingT...> {
		using type = typename type_getter<indexT - 1, remainingT...>::type;
	};

	template<unsigned indexT, typename expectT, typename ...typesT>
	struct type_finder {
		// `value` is not defined.
	};
	template<unsigned indexT, typename expectT, typename firstT, typename ...remainingT>
	struct type_finder<indexT, expectT, firstT, remainingT...> {
		enum : unsigned { value = type_finder<indexT + 1, expectT, remainingT...>::value };
	};
	template<unsigned indexT, typename expectT, typename ...remainingT>
	struct type_finder<indexT, expectT, expectT, remainingT...> {
		enum : unsigned { value = indexT };
	};

	template<typename expectT, typename ...typesT>
	struct has_type_recursive {
		enum : bool { value = false };
	};
	template<typename expectT, typename firstT, typename ...remainingT>
	struct has_type_recursive<expectT, firstT, remainingT...> {
		enum : bool { value = has_type_recursive<expectT, remainingT...>::value };
	};
	template<typename expectT, typename ...remainingT>
	struct has_type_recursive<expectT, expectT, remainingT...> {
		enum : bool { value = true };
	};
	template<typename expectT, typename ...elementsT, typename ...remainingT>
	struct has_type_recursive<expectT, variant<elementsT...>, remainingT...> {
		enum : bool { value = has_type_recursive<expectT, elementsT...>::value || has_type_recursive<expectT, remainingT...>::value };
	};
	template<typename ...elementsT, typename ...remainingT>
	struct has_type_recursive<variant<elementsT...>, variant<elementsT...>, remainingT...> {
		enum : bool { value = true };
	};

	template<bool conditionT, unsigned trueT, typename falseT>
	struct conditional_index {
		enum : unsigned { value = trueT };
	};
	template<unsigned trueT, typename falseT>
	struct conditional_index<false, trueT, falseT> {
		enum : unsigned { value = falseT::value };
	};

	template<unsigned indexT, typename expectT, typename ...typesT>
	struct recursive_type_finder {
		// `value` is not defined.
	};
	template<unsigned indexT, typename expectT, typename firstT, typename ...remainingT>
	struct recursive_type_finder<indexT, expectT, firstT, remainingT...> {
		enum : unsigned { value = conditional_index<has_type_recursive<expectT, firstT>::value, indexT, recursive_type_finder<indexT + 1, expectT, remainingT...>>::value };
	};

	template<typename ...typesT>
	struct conjunction {
		enum : bool { value = true };
	};
	template<typename firstT, typename ...remainingT>
	struct conjunction<firstT, remainingT...> {
		enum : bool { value = firstT::value && conjunction<remainingT...>::value };
	};

	template<size_t firstT, size_t ...remainingT>
	struct max_of {
		enum : size_t { value = max_of<firstT, max_of<remainingT...>::value>::value };
	};
	template<size_t firstT>
	struct max_of<firstT> {
		enum : size_t { value = firstT };
	};
	template<size_t firstT, size_t secondT>
	struct max_of<firstT, secondT> {
		enum : size_t { value = (firstT >= secondT) ? firstT : secondT };
	};

	template<typename ...elementsT>
	struct storage_for {
		enum : size_t { align = max_of<alignof(elementsT)...>::value };
		enum : size_t { size = max_of<sizeof(elementsT)...>::value };
		alignas(+align) char bytes[+size];
	};

	template<typename typeT, typename ...paramsT>
	inline typeT * construct(typeT *ptr, paramsT &&...params){
		return ::new(static_cast<void *>(ptr)) typeT(::std::forward<paramsT>(params)...);
	}
	template<typename typeT>
	inline void destruct(typeT *ptr) noexcept {
		ptr->~typeT();
	}

	template<typename resultT, typename sourceT>
	inline resultT punning_cast(sourceT *ptr) noexcept {
		return static_cast<resultT>(static_cast<typename conditional<is_const<sourceT>::value, const void, void>::type *>(ptr));
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
				::std::forward<visitorT>(visitor)(punning_cast<firstT *>(stor), ::std::forward<paramsT>(params)...);
			} else {
				visit_helper<remainingT...>()(stor, expect - 1, ::std::forward<visitorT>(visitor), ::std::forward<paramsT>(params)...);
			}
		}
	};

	struct visitor_copy_construct {
		template<typename elementT, typename sourceT>
		void operator()(elementT *ptr, const sourceT *src) const {
			construct<>(ptr, *punning_cast<const elementT *>(src));
		}
	};
	struct visitor_move_construct {
		template<typename elementT, typename sourceT>
		void operator()(elementT *ptr, sourceT *src) const {
			construct<>(ptr, ::std::move(*punning_cast<elementT *>(src)));
		}
	};
	struct visitor_copy_assign {
		template<typename elementT, typename sourceT>
		void operator()(elementT *ptr, const sourceT *src) const {
			*ptr = *punning_cast<const elementT *>(src);
		}
	};
	struct visitor_move_assign {
		template<typename elementT, typename sourceT>
		void operator()(elementT *ptr, sourceT *src) const {
			*ptr = ::std::move(*punning_cast<elementT *>(src));
		}
	};
	struct visitor_destruct {
		template<typename elementT>
		void operator()(elementT *ptr) const {
			destruct<>(ptr);
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
			using ::std::swap;
			swap(*ptr, *punning_cast<elementT *>(src));
		}
	};

	template<typename paramT>
	struct is_nothrow_forward_constructible {
		enum : bool { value = is_nothrow_constructible<typename decay<paramT>::type, paramT &&>::value };
	};
	template<typename paramT>
	struct is_nothrow_forward_assignable {
		enum : bool { value = is_nothrow_assignable<typename decay<paramT>::type, paramT &&>::value };
	};

	template<typename paramT>
	constexpr bool check_nothrow_swappable() noexcept {
		using ::std::swap;
		return noexcept(swap(::std::declval<paramT &>(), ::std::declval<paramT &>()));
	}

	template<typename paramT>
	struct is_nothrow_swappable {
		enum : bool { value = check_nothrow_swappable<paramT>() };
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
	template<typename elementT>
	struct is_candidate {
		enum : bool { value = details_variant::has_type_recursive<typename decay<elementT>::type, elementsT...>::value };
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
		details_variant::visit_helper<elementsT...>()(this->m_buffers + turnout_old, index_old, details_variant::visitor_destruct());
#ifdef ROCKET_DEBUG
		std::memset(this->m_buffers + turnout_old, '@', sizeof(storage));
#endif
	}

public:
	variant() noexcept(is_nothrow_constructible<typename details_variant::type_getter<0, elementsT...>::type>::value)
		: m_turnout(0)
	{
		constexpr unsigned eindex = 0;
		using etype = typename details_variant::type_getter<eindex, elementsT...>::type;
		details_variant::construct(details_variant::punning_cast<etype *>(this->do_get_front_buffer()));
		this->m_index = 0;
	}
	template<typename elementT, typename enable_if<is_candidate<elementT>::value>::type * = nullptr>
	variant(elementT &&elem) noexcept(details_variant::is_nothrow_forward_constructible<elementT>::value)
		: m_turnout(0)
	{
		constexpr unsigned eindex = details_variant::recursive_type_finder<0, typename decay<elementT>::type, elementsT...>::value;
		using etype = typename details_variant::type_getter<eindex, elementsT...>::type;
		details_variant::construct(details_variant::punning_cast<etype *>(this->do_get_front_buffer()), ::std::forward<elementT>(elem));
		this->m_index = eindex;
	}
	variant(const variant &other) noexcept(details_variant::conjunction<is_nothrow_copy_constructible<elementsT>...>::value)
		: m_turnout(0)
	{
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), other.m_index, details_variant::visitor_copy_construct(), other.do_get_front_buffer());
		this->m_index = other.m_index;
	}
	variant(variant &&other) noexcept(details_variant::conjunction<is_nothrow_move_constructible<elementsT>...>::value)
		: m_turnout(0)
	{
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), other.m_index, details_variant::visitor_move_construct(), other.do_get_front_buffer());
		this->m_index = other.m_index;
	}
	variant & operator=(const variant &other) noexcept(details_variant::conjunction<is_nothrow_copy_assignable<elementsT>..., is_nothrow_copy_constructible<elementsT>...>::value) {
		if(this->m_index == other.m_index){
			details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), this->m_index, details_variant::visitor_copy_assign(), other.do_get_front_buffer());
		} else {
			details_variant::visit_helper<elementsT...>()(this->do_get_back_buffer(), other.m_index, details_variant::visitor_copy_construct(), other.do_get_front_buffer());
			this->do_set_up_new_buffer(other.m_index);
		}
		return *this;
	}
	variant & operator=(variant &&other) noexcept(details_variant::conjunction<is_nothrow_move_assignable<elementsT>..., is_nothrow_move_constructible<elementsT>...>::value) {
		if(this->m_index == other.m_index){
			details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), this->m_index, details_variant::visitor_move_assign(), other.do_get_front_buffer());
		} else {
			details_variant::visit_helper<elementsT...>()(this->do_get_back_buffer(), other.m_index, details_variant::visitor_move_construct(), other.do_get_front_buffer());
			this->do_set_up_new_buffer(other.m_index);
		}
		return *this;
	}
	~variant(){
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), this->m_index, details_variant::visitor_destruct());
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
	template<typename elementT>
	const elementT * try_get() const noexcept {
		constexpr unsigned eindex = details_variant::type_finder<0, elementT, elementsT...>::value;
		if(this->m_index != eindex){
			return nullptr;
		}
		return details_variant::punning_cast<const elementT *>(this->do_get_front_buffer());
	}
	template<typename elementT>
	elementT * try_get() noexcept {
		constexpr unsigned eindex = details_variant::type_finder<0, elementT, elementsT...>::value;
		if(this->m_index != eindex){
			return nullptr;
		}
		return details_variant::punning_cast<elementT *>(this->do_get_front_buffer());
	}
	template<typename elementT>
	const elementT & get() const {
		constexpr unsigned eindex = details_variant::type_finder<0, elementT, elementsT...>::value;
		const auto ptr = this->try_get<elementT>();
		if(!ptr){
			throw_invalid_argument("variant::get(): The index of the type requested is `%d`, but the index of the type currently active is `%d`.",
			                       static_cast<int>(eindex), static_cast<int>(this->index()));
		}
		return *ptr;
	}
	template<typename elementT>
	elementT & get(){
		constexpr unsigned eindex = details_variant::type_finder<0, elementT, elementsT...>::value;
		const auto ptr = this->try_get<elementT>();
		if(!ptr){
			throw_invalid_argument("variant::get(): The index of the type requested is `%d`, but the index of the type currently active is `%d`.",
			                       static_cast<int>(eindex), static_cast<int>(this->index()));
		}
		return *ptr;
	}
	template<typename elementT>
	void set(elementT &&elem) noexcept(details_variant::conjunction<is_nothrow_move_assignable<elementsT>..., is_nothrow_move_constructible<elementsT>...>::value) {
		constexpr unsigned eindex = details_variant::type_finder<0, typename decay<elementT>::type, elementsT...>::value;
		using etype = typename details_variant::type_getter<eindex, elementsT...>::type;
		if(this->m_index == eindex){
			*details_variant::punning_cast<etype *>(this->do_get_front_buffer()) = ::std::forward<elementT>(elem);
		} else {
			details_variant::construct(details_variant::punning_cast<etype *>(this->do_get_back_buffer()), ::std::forward<elementT>(elem));
			this->do_set_up_new_buffer(eindex);
		}
	}

	template<typename visitorT>
	void visit(visitorT &&visitor) const {
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), this->m_index, details_variant::visitor_wrapper(), ::std::forward<visitorT>(visitor));
	}
	template<typename visitorT>
	void visit(visitorT &&visitor){
		details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), this->m_index, details_variant::visitor_wrapper(), ::std::forward<visitorT>(visitor));
	}

	void swap(variant &other) noexcept(details_variant::conjunction<details_variant::is_nothrow_swappable<elementsT>..., is_nothrow_move_constructible<elementsT>...>::value) {
		if(this->m_index == other.m_index){
			details_variant::visit_helper<elementsT...>()(this->do_get_front_buffer(), this->m_index, details_variant::visitor_swap(), other.do_get_front_buffer());
		} else {
			details_variant::visit_helper<elementsT...>()(this->do_get_back_buffer(), other.m_index, details_variant::visitor_move_construct(), other.do_get_front_buffer());
			try {
				details_variant::visit_helper<elementsT...>()(other.do_get_back_buffer(), this->m_index, details_variant::visitor_move_construct(), this->do_get_front_buffer());
			} catch(...){
				details_variant::visit_helper<elementsT...>()(this->do_get_back_buffer(), other.m_index, details_variant::visitor_destruct());
				rethrow_current_exception();
			}
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
