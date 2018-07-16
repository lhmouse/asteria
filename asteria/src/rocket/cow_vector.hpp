// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_VECTOR_HPP_
#define ROCKET_COW_VECTOR_HPP_

#include <memory> // std::allocator<>, std::allocator_traits<>, std::addressof()
#include <atomic> // std::atomic<>
#include <type_traits> // so many...
#include <iterator> // std::iterator_traits<>, std::reverse_iterator<>, std::random_access_iterator_tag, std::distance()
#include <initializer_list> // std::initializer_list<>
#include <utility> // std::move(), std::forward(), std::declval()
#include <cstdint> // std::uint_fast32_t
#include <cstddef> // std::size_t, std::ptrdiff_t
#include <cstring> // std::memset()
#include "compatibility.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"

/* Differences from `std::vector`:
 * 1. All functions guarantee only basic exception safety rather than strong exception safety, hence are more efficient.
 * 2. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always return `const_reference`s.
 * 3. The copy constructor and copy assignment operator will not throw exceptions.
 * 4. The specialization for `bool` is not provided.
 */

namespace rocket {

using ::std::allocator;
using ::std::allocator_traits;
using ::std::atomic;
using ::std::is_same;
using ::std::decay;
using ::std::is_array;
using ::std::is_trivial;
using ::std::enable_if;
using ::std::is_convertible;
using ::std::is_nothrow_copy_constructible;
using ::std::is_nothrow_move_constructible;
using ::std::conditional;
using ::std::iterator_traits;
using ::std::reverse_iterator;
using ::std::initializer_list;
using ::std::size_t;
using ::std::ptrdiff_t;

template<typename valueT, typename allocatorT = allocator<valueT>>
class cow_vector;

namespace details_cow_vector {
	template<typename valueT, typename allocatorT>
	class storage_handle : private allocator_wrapper_base_for<allocatorT> {
	public:
		using value_type       = valueT;
		using allocator_type   = allocatorT;

		using size_type        = typename allocator_traits<allocator_type>::size_type;
		using difference_type  = typename allocator_traits<allocator_type>::difference_type;
		using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
		using pointer          = typename allocator_traits<allocator_type>::pointer;

	private:
		struct storage {
			typename allocator_traits<allocator_type>::template rebind_alloc<storage> st_alloc;
			size_type st_n_blocks;

			atomic<difference_type> ref_count;
			size_type n_values;
			ROCKET_EXTENSION(value_type data[0]);

			storage(const allocator_type &alloc, size_type n_blocks) noexcept
				: st_alloc(alloc), st_n_blocks(n_blocks)
			{
				this->n_values = 0;
				this->ref_count.store(1, ::std::memory_order_release);
			}
		};

		using allocator_base    = allocator_wrapper_base_for<allocatorT>;
		using storage_allocator = decltype(storage::st_alloc);

	private:
		static constexpr size_type do_reserve_blocks_for(size_type n_values) noexcept {
			return (n_values * sizeof(value_type) + sizeof(storage) - 1) / sizeof(storage) + 1;
		}
		static constexpr size_type do_get_capacity_of(size_type n_blocks) noexcept {
			return (n_blocks - 1) * sizeof(storage) / sizeof(value_type);
		}

	private:
		storage *m_ptr;

	public:
		explicit storage_handle(const allocator_type &alloc) noexcept
			: allocator_base(alloc)
			, m_ptr(nullptr)
		{ }
		explicit storage_handle(allocator_type &&alloc) noexcept
			: allocator_base(::std::move(alloc))
			, m_ptr(nullptr)
		{ }
		~storage_handle(){
			this->do_reset(nullptr);
		}

		storage_handle(const storage_handle &) = delete;
		storage_handle & operator=(const storage_handle &) = delete;

	private:
		void do_reset(storage *ptr_new) noexcept {
			const auto ptr = noadl::exchange(this->m_ptr, ptr_new);
			if(ptr == nullptr){
				return;
			}
			// Decrement the reference count with acquire-release semantics to prevent races on `ptr->st_alloc`.
			const auto old = ptr->ref_count.fetch_sub(1, ::std::memory_order_acq_rel);
			ROCKET_ASSERT(old > 0);
			if(old > 1){
				return;
			}
			// If it has been decremented to zero, destroy all values backwards.
			auto st_alloc = ::std::move(ptr->st_alloc);
			auto cur = ptr->n_values;
			while(cur != 0){
				ptr->n_values = --cur;
				allocator_traits<allocator_type>::destroy(this->as_allocator(), ptr->data + cur);
			}
			// Deallocate the block.
			const auto n_blocks = ptr->st_n_blocks;
			allocator_traits<storage_allocator>::destroy(st_alloc, ptr);
#ifdef ROCKET_DEBUG
			::std::memset(static_cast<void *>(ptr), '~', sizeof(*ptr) * n_blocks);
#endif
			allocator_traits<storage_allocator>::deallocate(st_alloc, ptr, n_blocks);
		}

	public:
		const allocator_type & as_allocator() const noexcept {
			return static_cast<const allocator_base &>(*this);
		}
		allocator_type & as_allocator() noexcept {
			return static_cast<allocator_base &>(*this);
		}

		bool unique() const noexcept {
			const auto ptr = this->m_ptr;
			if(ptr == nullptr){
				return false;
			}
			return ptr->ref_count.load(::std::memory_order_relaxed) == 1;
		}
		size_type capacity() const noexcept {
			const auto ptr = this->m_ptr;
			if(ptr == nullptr){
				return 0;
			}
			return this->do_get_capacity_of(ptr->st_n_blocks);
		}
		size_type max_size() const noexcept {
			auto st_alloc = storage_allocator(this->as_allocator());
			const auto max_n_blocks = allocator_traits<storage_allocator>::max_size(st_alloc);
			return this->do_get_capacity_of(max_n_blocks / 2 - 1);
		}
		size_type check_size_add(size_type base, size_type add) const {
			const auto cap_max = this->max_size();
			ROCKET_ASSERT(base <= cap_max);
			if(cap_max - base < add){
				noadl::throw_length_error("storage_handle::check_size_add(): Increasing `%lld` by `%lld` would exceed the max length `%lld`.",
				                          static_cast<long long>(base), static_cast<long long>(add), static_cast<long long>(cap_max));
			}
			return base + add;
		}
		size_type round_up_capacity(size_type res_arg) const {
			const auto cap = this->check_size_add(0, res_arg);
			const auto n_blocks = this->do_reserve_blocks_for(cap);
			return this->do_get_capacity_of(n_blocks);
		}
		const_pointer data() const noexcept {
			const auto ptr = this->m_ptr;
			if(ptr == nullptr){
				return nullptr;
			}
			return ptr->data;
		}
		pointer mut_data() noexcept {
			ROCKET_ASSERT(this->unique());
			const auto ptr = this->m_ptr;
			ROCKET_ASSERT(ptr);
			return ptr->data;
		}
		size_type size() const noexcept {
			const auto ptr = this->m_ptr;
			if(ptr == nullptr){
				return 0;
			}
			return ptr->n_values;
		}
		pointer reallocate(size_type res_arg){
			const auto len = this->size();
			ROCKET_ASSERT(len <= res_arg);
			if(res_arg == 0){
				// Deallocate the block.
				this->do_reset(nullptr);
				return nullptr;
			}
			const auto cap = this->check_size_add(0, res_arg);
			// Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
			const auto n_blocks = this->do_reserve_blocks_for(cap);
			auto st_alloc = storage_allocator(this->as_allocator());
			const auto ptr = allocator_traits<storage_allocator>::allocate(st_alloc, n_blocks);
#ifdef ROCKET_DEBUG
			::std::memset(static_cast<void *>(ptr), '*', sizeof(*ptr) * n_blocks);
#endif
			allocator_traits<storage_allocator>::construct(st_alloc, ptr, this->as_allocator(), n_blocks);
			if(len != 0){
				auto cur = size_type(0);
				try {
					const auto src = this->m_ptr->data;
					// Move-constructs values into the new block.
					while(cur != len){
						allocator_traits<allocator_type>::construct(this->as_allocator(), ptr->data + cur, ::std::move(*(src + cur)));
						ptr->n_values = ++cur;
					}
				} catch(...){
					// If an exception is thrown, destroy all values that have been constructed so far.
					while(cur != 0){
						ptr->n_values = --cur;
						allocator_traits<allocator_type>::destroy(this->as_allocator(), ptr->data + cur);
					}
					// Deallocate the new block, then rethrow the exception.
					allocator_traits<storage_allocator>::destroy(st_alloc, ptr);
					allocator_traits<storage_allocator>::deallocate(st_alloc, ptr, n_blocks);
					throw;
				}
			}
			// Replace the current block.
			this->do_reset(ptr);
			return ptr->data;
		}
		void deallocate() noexcept {
			this->do_reset(nullptr);
		}

		void share_with(const storage_handle &other) noexcept {
			const auto ptr = other.m_ptr;
			if(ptr){
				// Increment the reference count.
				const auto old = ptr->ref_count.fetch_add(1, ::std::memory_order_relaxed);
				ROCKET_ASSERT(old > 0);
			}
			this->do_reset(ptr);
		}
		void share_with(storage_handle &&other) noexcept {
			const auto ptr = noadl::exchange(other.m_ptr, nullptr);
			this->do_reset(ptr);
		}
		void exchange_with(storage_handle &other) noexcept {
			::std::swap(this->m_ptr, other.m_ptr);
		}

		template<typename ...paramsT>
		pointer emplace_back_n(size_type n, paramsT &&...params){
			if(n == 0){
				return nullptr;
			}
			const auto ptr = this->m_ptr;
			ROCKET_ASSERT(ptr);
			ROCKET_ASSERT(this->unique());
			auto cur = ptr->n_values;
			ROCKET_ASSERT(n <= this->capacity() - cur);
			const auto end = cur + n;
			while(cur != end){
				allocator_traits<allocator_type>::construct(this->as_allocator(), ptr->data + cur, ::std::forward<paramsT>(params)...);
				ptr->n_values = ++cur;
			}
			return ptr->data + cur - n;
		}
		pointer pop_back_n(size_type n) noexcept {
			if(n == 0){
				return nullptr;
			}
			const auto ptr = this->m_ptr;
			ROCKET_ASSERT(ptr);
			ROCKET_ASSERT(this->unique());
			auto cur = ptr->n_values;
			ROCKET_ASSERT(n <= cur);
			const auto end = cur - n;
			while(cur != end){
				ptr->n_values = --cur;
				allocator_traits<allocator_type>::destroy(this->as_allocator(), ptr->data + cur);
			}
			return ptr->data + cur;
		}
		void rotate_left(size_type after, size_type offset){
			auto bot = after;
			auto brk = after + offset;
			//   |<- isl ->|<- isr ->|
			//   bot       brk       end
			// > 0 1 2 3 4 5 6 7 8 9 -
			auto isl = brk - bot;
			if(isl == 0){
				return;
			}
			const auto ptr = this->m_ptr;
			ROCKET_ASSERT(ptr);
			ROCKET_ASSERT(this->unique());
			const auto end = ptr->n_values;
			ROCKET_ASSERT(after <= end);
			ROCKET_ASSERT(offset <= end - after);
			auto isr = end - brk;
			if(isr == 0){
				return;
			}
			auto stp = brk;
		loop:
			if(isl < isr){
				// Before:  bot   brk           end
				//        > 0 1 2 3 4 5 6 7 8 9 -
				// After:         bot   brk     end
				//        > 3 4 5 0 1 2 6 7 8 9 -
				do {
					noadl::adl_swap(ptr->data[bot++], ptr->data[brk++]);
				} while(bot != stp);
				stp = brk;
				// `isr` will have been decreased by `isl`, which will not result in zero.
				isr = end - brk;
				// `isl` is unchanged.
				goto loop;
			}
			if(isl > isr){
				// Before:  bot           brk   end
				//        > 0 1 2 3 4 5 6 7 8 9 -
				// After:       bot       brk   end
				//        > 7 8 9 3 4 5 6 0 1 2 -
				do {
					noadl::adl_swap(ptr->data[bot++], ptr->data[brk++]);
				} while(brk != end);
				brk = stp;
				// `isl` will have been decreased by `isr`, which will not result in zero.
				isl = brk - bot;
				// `isr` is unchanged.
				goto loop;
			}
			// Before:  bot       brk       end
			//        > 0 1 2 3 4 5 6 7 8 9 -
			// After:             bot       brk
			//        > 3 4 5 0 1 2 6 7 8 9 -
			do {
				noadl::adl_swap(ptr->data[bot++], ptr->data[brk++]);
			} while(bot != stp);
		}
	};

	template<typename vectorT, typename valueT>
	class vector_iterator {
		friend vectorT;
		friend vector_iterator<vectorT, const valueT>;

	public:
		using value_type         = valueT;
		using pointer            = value_type *;
		using reference          = value_type &;
		using difference_type    = ptrdiff_t;
		using iterator_category  = ::std::random_access_iterator_tag;

	private:
		const vectorT *m_str;
		valueT *m_ptr;

	private:
		constexpr vector_iterator(const vectorT *str, valueT *ptr) noexcept
			: m_str(str), m_ptr(ptr)
		{ }

	public:
		constexpr vector_iterator() noexcept
			: vector_iterator(nullptr, nullptr)
		{ }
		template<typename yvalueT, typename enable_if<is_convertible<yvalueT *, valueT *>::value>::type * = nullptr>
		constexpr vector_iterator(const vector_iterator<vectorT, yvalueT> &other) noexcept
			: vector_iterator(other.m_str, other.m_ptr)
		{ }

	private:
		template<typename pointerT>
		pointerT do_assert_valid_pointer(pointerT ptr, bool to_dereference) const noexcept {
			const auto str = this->m_str;
			ROCKET_ASSERT_MSG(str, "This iterator has not been initialized.");
			const auto dist = static_cast<typename vectorT::size_type>(ptr - str->data());
			ROCKET_ASSERT_MSG(dist <= str->size(), "This iterator has been invalidated.");
			if(to_dereference){
				ROCKET_ASSERT_MSG(dist < str->size(), "This iterator contains a past-the-end value and cannot be dereferenced.");
			}
			return ptr;
		}

	public:
		const vectorT * parent() const noexcept {
			return this->m_str;
		}

		pointer tell() const noexcept {
			return this->do_assert_valid_pointer(this->m_ptr, false);
		}
		pointer tell_owned_by(const vectorT *str) const noexcept {
			ROCKET_ASSERT(this->m_str == str);
			return this->tell();
		}
		vector_iterator & seek(pointer ptr) noexcept {
			this->m_ptr = this->do_assert_valid_pointer(ptr, false);
			return *this;
		}

		reference operator*() const noexcept {
			return *(this->do_assert_valid_pointer(this->m_ptr, true));
		}
		reference operator[](difference_type off) const noexcept {
			return *(this->do_assert_valid_pointer(this->m_ptr + off, true));
		}
	};

	template<typename vectorT, typename valueT>
	inline vector_iterator<vectorT, valueT> & operator++(vector_iterator<vectorT, valueT> &rhs) noexcept {
		return rhs.seek(rhs.tell() + 1);
	}
	template<typename vectorT, typename valueT>
	inline vector_iterator<vectorT, valueT> & operator--(vector_iterator<vectorT, valueT> &rhs) noexcept {
		return rhs.seek(rhs.tell() - 1);
	}

	template<typename vectorT, typename valueT>
	inline vector_iterator<vectorT, valueT> operator++(vector_iterator<vectorT, valueT> &lhs, int) noexcept {
		auto res = lhs;
		lhs.seek(lhs.tell() + 1);
		return res;
	}
	template<typename vectorT, typename valueT>
	inline vector_iterator<vectorT, valueT> operator--(vector_iterator<vectorT, valueT> &lhs, int) noexcept {
		auto res = lhs;
		lhs.seek(lhs.tell() - 1);
		return res;
	}

	template<typename vectorT, typename valueT>
	inline vector_iterator<vectorT, valueT> & operator+=(vector_iterator<vectorT, valueT> &lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept {
		return lhs.seek(lhs.tell() + rhs);
	}
	template<typename vectorT, typename valueT>
	inline vector_iterator<vectorT, valueT> & operator-=(vector_iterator<vectorT, valueT> &lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept {
		return lhs.seek(lhs.tell() - rhs);
	}

	template<typename vectorT, typename valueT>
	inline vector_iterator<vectorT, valueT> operator+(const vector_iterator<vectorT, valueT> &lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept {
		auto res = lhs;
		res.seek(res.tell() + rhs);
		return res;
	}
	template<typename vectorT, typename valueT>
	inline vector_iterator<vectorT, valueT> operator-(const vector_iterator<vectorT, valueT> &lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept {
		auto res = lhs;
		res.seek(res.tell() - rhs);
		return res;
	}

	template<typename vectorT, typename valueT>
	inline vector_iterator<vectorT, valueT> operator+(typename vector_iterator<vectorT, valueT>::difference_type lhs, const vector_iterator<vectorT, valueT> &rhs) noexcept {
		auto res = rhs;
		res.seek(res.tell() + lhs);
		return res;
	}
	template<typename vectorT, typename xvalueT, typename yvalueT>
	inline typename vector_iterator<vectorT, xvalueT>::difference_type operator-(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) - rhs.tell();
	}

	template<typename vectorT, typename xvalueT, typename yvalueT>
	inline bool operator==(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) == rhs.tell();
	}
	template<typename vectorT, typename xvalueT, typename yvalueT>
	inline bool operator!=(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) != rhs.tell();
	}
	template<typename vectorT, typename xvalueT, typename yvalueT>
	inline bool operator<(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) < rhs.tell();
	}
	template<typename vectorT, typename xvalueT, typename yvalueT>
	inline bool operator>(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) > rhs.tell();
	}
	template<typename vectorT, typename xvalueT, typename yvalueT>
	inline bool operator<=(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) <= rhs.tell();
	}
	template<typename vectorT, typename xvalueT, typename yvalueT>
	inline bool operator>=(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) >= rhs.tell();
	}
}

template<typename valueT, typename allocatorT>
class cow_vector {
	static_assert(is_same<typename allocatorT::value_type, valueT>::value, "`allocatorT::value_type` must denote the same type as `valueT`.");
	static_assert(is_array<valueT>::value == false, "`valueT` must not be an array type.");
/*
public:
	// types
	using value_type      = valueT;
	using allocator_type  = allocatorT;

	using size_type        = typename allocator_traits<allocator_type>::size_type;
	using difference_type  = typename allocator_traits<allocator_type>::difference_type;
	using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
	using pointer          = typename allocator_traits<allocator_type>::pointer;
	using const_reference  = const value_type &;
	using reference        = value_type &;

	using const_iterator          = details_cow_vector::vector_iterator<cow_vector, const value_type>;
	using iterator                = details_cow_vector::vector_iterator<cow_vector, value_type>;
	using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
	using reverse_iterator        = ::std::reverse_iterator<iterator>;

private:
	details_cow_vector::storage_handle<valueT, allocatorT> m_sth;
	const_pointer m_ptr;
	size_type m_len;

public:
	// 26.3.11.2, construct/copy/destroy
	cow_vector() noexcept(noexcept(allocator_type()))
		: cow_vector(allocator_type())
	{ }
	explicit cow_vector(const allocator_type &alloc) noexcept
		: m_sth(alloc), m_ptr(nullptr), m_len(0)
	{ }
	cow_vector(const cow_vector &other) noexcept
		: cow_vector(allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
	{
		this->assign(other);
	}
	cow_vector(const cow_vector &other, const allocator_type &alloc) noexcept
		: cow_vector(alloc)
	{
		this->assign(other);
	}
	cow_vector(cow_vector &&other) noexcept
		: cow_vector(::std::move(other.m_sth.as_allocator()))
	{
		this->assign(::std::move(other));
	}
	cow_vector(cow_vector &&other, const allocator_type &alloc) noexcept
		: cow_vector(alloc)
	{
		this->assign(::std::move(other));
	}
	cow_vector(size_type n, const allocator_type &alloc = allocator_type())
		: cow_vector(alloc)
	{
		this->assign(n);
	}
	cow_vector(size_type n, const value_type &value, const allocator_type &alloc = allocator_type())
		: cow_vector(alloc)
	{
		this->assign(n, value);
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	cow_vector(inputT first, inputT last, const allocator_type &alloc = allocator_type())
		: cow_vector(alloc)
	{
		this->assign(::std::move(first), ::std::move(last));
	}
	cow_vector(initializer_list<value_type> init, const allocator_type &alloc = allocator_type())
		: cow_vector(alloc)
	{
		this->assign(init);
	}
	cow_vector & operator=(const cow_vector &other) noexcept {
		if(this == &other){
			return *this;
		}
		noadl::manipulate_allocators(typename allocator_traits<allocator_type>::propagate_on_container_copy_assignment(),
		                             allocator_copy_assign_from(), this->m_sth.as_allocator(), other.m_sth.as_allocator());
		return this->assign(other);
	}
	cow_vector & operator=(cow_vector &&other) noexcept {
		noadl::manipulate_allocators(typename allocator_traits<allocator_type>::propagate_on_container_move_assignment(),
		                             allocator_move_assign_from(), this->m_sth.as_allocator(), ::std::move(other.m_sth.as_allocator()));
		return this->assign(::std::move(other));
	}
	cow_vector & operator=(initializer_list<value_type> init){
		return this->assign(init);
	}

private:
	// Reallocate the storage to `cap` valueacters, not including the null terminator. The first `len` valueacters are left intact and the rest are undefined.
	// The storage is owned by the current vector exclusively after this function returns normally.
	pointer do_reallocate_no_set_length(size_type len, size_type res_arg){
		ROCKET_ASSERT(len <= res_arg);
		ROCKET_ASSERT(len <= this->m_len);
		ROCKET_ASSERT(res_arg != 0);
		const auto ptr = this->m_sth.reallocate(len, res_arg);
		ROCKET_ASSERT(this->m_sth.unique());
		this->m_ptr = ptr;
		return ptr + len;
	}
	// Reallocate more storage as needed, without shrinking.
	pointer do_reallocate_more_no_set_length(size_type cap_add){
		const auto len = this->m_len;
		auto cap = this->m_sth.check_size_add(len, cap_add);
		if((this->m_sth.unique() == false) || (this->m_sth.capacity() < cap)){
#ifndef ROCKET_DEBUG
			// Reserve more space for non-debug builds.
			cap = noadl::max(cap, len + len / 2 + 31);
#endif
			this->do_reallocate_no_set_length(len, cap);
		}
		ROCKET_ASSERT(this->m_sth.capacity() >= cap);
		const auto ptr = this->m_sth.mut_data();
		ROCKET_ASSERT(ptr == this->m_ptr);
		return ptr + len;
	}
	// Add a null terminator at `ptr[len]` then set `len` there.
	void do_set_length(size_type len) noexcept {
		ROCKET_ASSERT(this->m_sth.unique());
		ROCKET_ASSERT(len <= this->m_sth.capacity());
		const auto ptr = this->m_sth.mut_data();
		ROCKET_ASSERT(ptr == this->m_ptr);
		traits_type::assign(ptr[len], value_type());
		this->m_len = len;
	}
	// Get a pointer to mutable storage.
	pointer do_ensure_unique(){
		if(this->m_sth.unique() == false){
			const auto len = this->m_len;
			this->do_reallocate_no_set_length(len, noadl::max(len, size_type(1)));
			this->do_set_length(len);
		}
		ROCKET_ASSERT(this->m_sth.unique());
		const auto ptr = this->m_sth.mut_data();
		ROCKET_ASSERT(ptr == this->m_ptr);
		return ptr;
	}
	// Deallocate any dynamic storage.
	void do_deallocate() noexcept {
		this->m_sth.deallocate();
		this->m_ptr = shallow().data();
		this->m_len = 0;
	}

	// This function works the same way as `substr()`.
	// Ensure `tpos` is in `[0, size()]` and return `min(n, size() - tpos)`.
	size_type do_clamp_substr(size_type tpos, size_type n) const {
		const auto tlen = this->size();
		if(tpos > tlen){
			noadl::throw_out_of_range("cow_vector::do_clamp_substr(): The subscript `%lld` is out of range for a vector of length `%lld`.",
			                          static_cast<long long>(tpos), static_cast<long long>(tlen));
		}
		return noadl::min(tlen - tpos, n);
	}

	// This has to be generic to allow construction of a vector from an array of integers... This is a nasty trick anyway.
	template<typename someT>
	bool do_check_overlap_generic(const someT &some) const noexcept {
		return static_cast<size_type>(reinterpret_cast<const value_type (&)[1]>(some) - this->data()) < this->size();
	}

	// Remove a subvector. This function may throw `std::bad_alloc`.
	pointer do_erase_no_bound_check(size_type tpos, size_type tn){
		const auto len_old = this->size();
		ROCKET_ASSERT(tpos <= len_old);
		ROCKET_ASSERT(tn <= len_old - tpos);
		const auto ptr = this->do_ensure_unique();
		traits_type::move(ptr + tpos, ptr + tpos + tn, len_old - tpos - tn);
		this->do_set_length(len_old - tn);
		return ptr + tpos;
	}
	// This function template implements `assign()`, `insert()` and `replace()` functions.
	template<typename ...paramsT>
	pointer do_replace_no_bound_check(size_type tpos, size_type tn, paramsT &&...params){
		const auto len_old = this->size();
		ROCKET_ASSERT(tpos <= len_old);
		ROCKET_ASSERT(tn <= len_old - tpos);
		this->append(::std::forward<paramsT>(params)...);
		this->append(this->data() + tpos + tn, len_old - tpos - tn);
		return this->do_erase_no_bound_check(tpos, len_old - tpos);
	}

	// These are generic implementations for `{find,rfind,find_{first,last}{,_not}_of}()` functions.
	template<typename predT>
	size_type do_find_forwards_if(size_type from, size_type n, predT pred) const {
		const auto len = this->size();
		if(len < n){
			return npos;
		}
		for(auto i = from; i != len - n + 1; ++i){
			if(pred(this->data() + i)){
				ROCKET_ASSERT(i < len);
				ROCKET_ASSERT(i != npos);
				return i;
			}
		}
		return npos;
	}
	template<typename predT>
	size_type do_find_backwards_if(size_type to, size_type n, predT pred) const {
		const auto len = this->size();
		if(len < n){
			return npos;
		}
		for(auto i = noadl::min(len - n, to); i + 1 != 0; --i){
			if(pred(this->data() + i)){
				ROCKET_ASSERT(i < len);
				ROCKET_ASSERT(i != npos);
				return i;
			}
		}
		return npos;
	}

public:
	// 24.3.2.3, iterators
	const_iterator begin() const noexcept {
		return const_iterator(this, this->data());
	}
	const_iterator end() const noexcept {
		return const_iterator(this, this->data() + this->size());
	}
	const_reverse_iterator rbegin() const noexcept {
		return const_reverse_iterator(this->end());
	}
	const_reverse_iterator rend() const noexcept {
		return const_reverse_iterator(this->begin());
	}

	const_iterator cbegin() const noexcept {
		return this->begin();
	}
	const_iterator cend() const noexcept {
		return this->end();
	}
	const_reverse_iterator crbegin() const noexcept {
		return this->rbegin();
	}
	const_reverse_iterator crend() const noexcept {
		return this->rend();
	}

	// N.B. This function may throw `std::bad_alloc()`.
	// N.B. This is a non-standard extension.
	iterator mut_begin(){
		return iterator(this, this->mut_data());
	}
	// N.B. This function may throw `std::bad_alloc()`.
	// N.B. This is a non-standard extension.
	iterator mut_end(){
		return iterator(this, this->mut_data() + this->size());
	}
	// N.B. This function may throw `std::bad_alloc()`.
	// N.B. This is a non-standard extension.
	reverse_iterator mut_rbegin(){
		return reverse_iterator(this->mut_end());
	}
	// N.B. This function may throw `std::bad_alloc()`.
	// N.B. This is a non-standard extension.
	reverse_iterator mut_rend(){
		return reverse_iterator(this->mut_begin());
	}

	// 24.3.2.4, capacity
	bool empty() const noexcept {
		return this->m_len == 0;
	}
	size_type size() const noexcept {
		return this->m_len;
	}
	size_type length() const noexcept {
		return this->size();
	}
	size_type max_size() const noexcept {
		return this->m_sth.max_size();
	}
	void resize(size_type n, value_type ch = value_type()){
		const auto len_old = this->size();
		if(len_old == n){
			return;
		}
		if(len_old < n){
			this->append(n - len_old, ch);
		} else {
			this->do_ensure_unique();
			this->do_set_length(n);
		}
		ROCKET_ASSERT(this->size() == n);
	}
	size_type capacity() const noexcept {
		return this->m_sth.capacity();
	}
	void reserve(size_type res_arg){
		const auto len = this->size();
		const auto cap_new = this->m_sth.round_up_capacity(noadl::max(len, res_arg));
		// If the storage is shared with other vectors, force rellocation to prevent copy-on-write upon modification.
		if((this->m_sth.unique() != false) && (this->m_sth.capacity() >= cap_new)){
			return;
		}
		this->do_reallocate_no_set_length(len, cap_new);
		this->do_set_length(len);
		ROCKET_ASSERT(this->capacity() >= res_arg);
	}
	void shrink_to_fit(){
		const auto len = this->size();
		const auto cap_min = this->m_sth.round_up_capacity(len);
		// Don't increase memory usage.
		if((this->m_sth.unique() == false) || (this->m_sth.capacity() <= cap_min)){
			return;
		}
		if(len != 0){
			this->do_reallocate_no_set_length(len, len);
			this->do_set_length(len);
		} else {
			this->do_deallocate();
		}
		ROCKET_ASSERT(this->capacity() <= cap_min);
	}
	void clear() noexcept {
		if(this->m_sth.unique()){
			// If the storage is owned exclusively by this vector, truncate it and leave the buffer alone.
			this->do_set_length(0);
		} else {
			// Otherwise, detach it from `*this`.
			this->do_deallocate();
		}
		ROCKET_ASSERT(this->empty());
	}

	// 24.3.2.5, element access
	const_reference operator[](size_type pos) const noexcept {
		const auto len = this->size();
		// Reading from the valueacter at `size()` is permitted.
		ROCKET_ASSERT(pos <= len);
		return this->data()[pos];
	}
	const_reference at(size_type pos) const {
		const auto len = this->size();
		if(pos >= len){
			noadl::throw_out_of_range("cow_vector::at(): The subscript `%lld` is not a writable position within a vector of length `%lld`.",
			                          static_cast<long long>(pos), static_cast<long long>(len));
		}
		return this->data()[pos];
	}
	const_reference front() const noexcept {
		return this->operator[](0);
	}
	const_reference back() const noexcept {
		return this->operator[](this->size() - 1);
	}
	// There is no `at()` overload that returns a non-const reference. This is the consequent overload which does that.
	// N.B. This is a non-standard extension.
	reference mut(size_type pos){
		const auto len = this->size();
		if(pos >= len){
			noadl::throw_out_of_range("cow_vector::mut(): The subscript `%lld` is not a writable position within a vector of length `%lld`.",
			                          static_cast<long long>(pos), static_cast<long long>(len));
		}
		return this->mut_data()[pos];
	}
	// N.B. This is a non-standard extension.
	reference mut_front() noexcept {
		return this->mut(0);
	}
	// N.B. This is a non-standard extension.
	reference mut_back() noexcept {
		return this->mut(this->size() - 1);
	}

	// 24.3.2.6, modifiers
	cow_vector & operator+=(const cow_vector & other){
		return this->append(other);
	}
	cow_vector & operator+=(shallow sh){
		return this->append(sh);
	}
	cow_vector & operator+=(const_pointer s){
		return this->append(s);
	}
	cow_vector & operator+=(value_type ch){
		return this->append(size_type(1), ch);
	}
	cow_vector & operator+=(initializer_list<value_type> init){
		return this->append(init);
	}

	cow_vector & append(shallow sh){
		return this->append(sh.data(), sh.size());
	}
	cow_vector & append(const cow_vector & other, size_type pos = 0, size_type n = npos){
		return this->append(other.data() + pos, other.do_clamp_substr(pos, n));
	}
	cow_vector & append(const_pointer s, size_type n){
		if(n == 0){
			return *this;
		}
		const auto len_old = this->size();
		if(this->do_check_overlap_generic(*s)){
			const auto tpos = s - this->data();
			const auto wptr = this->do_reallocate_more_no_set_length(n);
			traits_type::move(wptr, this->data() + tpos, n);
		} else {
			const auto wptr = this->do_reallocate_more_no_set_length(n);
			traits_type::copy(wptr, s, n);
		}
		this->do_set_length(len_old + n);
		return *this;
	}
	cow_vector & append(const_pointer s){
		return this->append(s, traits_type::length(s));
	}
	cow_vector & append(size_type n, value_type ch){
		if(n == 0){
			return *this;
		}
		const auto len_old = this->size();
		const auto wptr = this->do_reallocate_more_no_set_length(n);
		traits_type::assign(wptr, n, ch);
		this->do_set_length(len_old + n);
		return *this;
	}
	cow_vector & append(initializer_list<value_type> init){
		return this->append(init.begin(), init.size());
	}
	// N.B. This is a non-standard extension.
	cow_vector & append(const_pointer first, const_pointer last){
		ROCKET_ASSERT(first <= last);
		return this->append(first, static_cast<size_type>(last - first));
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr, typename enable_if<is_convertible<inputT, const_pointer>::value == false>::type * = nullptr>
	cow_vector & append(inputT first, inputT last){
		if(first == last){
			return *this;
		}
		if(this->do_check_overlap_generic(*first)){
			auto other = cow_vector(shallow(*this), this->m_sth.as_allocator());
			// Append the range into the temporary vector.
			auto it = std::move(first);
			do {
				other.push_back(*it);
			} while(++it != last);
			// Then move it into `*this`.
			this->assign(::std::move(other));
		} else {
			// It should be safe to append to `*this` directly.
			auto it = std::move(first);
			do {
				this->push_back(*it);
			} while(++it != last);
		}
		return *this;
	}
	// The return type is a non-standard extension.
	cow_vector & push_back(value_type ch){
		const auto len_old = this->size();
		const auto wptr = this->do_reallocate_more_no_set_length(1);
		traits_type::assign(*wptr, ch);
		this->do_set_length(len_old + 1);
		return *this;
	}

	cow_vector & erase(size_type tpos = 0, size_type tn = npos){
		this->do_erase_no_bound_check(tpos, this->do_clamp_substr(tpos, tn));
		return *this;
	}
	// N.B. This function may throw `std::bad_alloc()`.
	iterator erase(const_iterator tfirst, const_iterator tlast){
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
		const auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
		return iterator(this, this->do_erase_no_bound_check(tpos, tn));
	}
	// N.B. This function may throw `std::bad_alloc()`.
	iterator erase(const_iterator trm){
		return this->erase(trm, const_iterator(this, trm.tell() + 1));
	}
	// N.B. This function may throw `std::bad_alloc()`.
	// The return type is a non-standard extension.
	cow_vector & pop_back(){
		ROCKET_ASSERT(this->empty() == false);
		this->do_ensure_unique();
		this->do_set_length(this->size() - 1);
		return *this;
	}

	cow_vector & assign(const cow_vector &other) noexcept {
		this->m_sth.share_with(other.m_sth);
		this->m_ptr = other.m_ptr;
		this->m_len = other.m_len;
		return *this;
	}
	cow_vector & assign(cow_vector &&other) noexcept {
		this->m_sth.share_with(::std::move(other.m_sth));
		this->m_ptr = noadl::exchange(other.m_ptr, shallow().data());
		this->m_len = noadl::exchange(other.m_len, size_type(0));
		return *this;
	}
	cow_vector & assign(shallow sh) noexcept {
		this->m_sth.deallocate();
		this->m_ptr = sh.data();
		this->m_len = sh.size();
		return *this;
	}
	cow_vector & assign(const cow_vector &other, size_type pos, size_type n = npos){
		if(this != &other){
			this->clear();
		}
		if(this->empty()){
			this->append(other, pos, n);
		} else {
			this->do_replace_no_bound_check(0, this->size(), other, pos, n);
		}
		return *this;
	}
	cow_vector & assign(const_pointer s, size_type n){
		if((n != 0) && (this->do_check_overlap_generic(*s) == false)){
			this->clear();
		}
		if(this->empty()){
			this->append(s, n);
		} else {
			this->do_replace_no_bound_check(0, this->size(), s, n);
		}
		return *this;
	}
	cow_vector & assign(const_pointer s){
		if(this->do_check_overlap_generic(*s) == false){
			this->clear();
		}
		if(this->empty()){
			this->append(s);
		} else {
			this->do_replace_no_bound_check(0, this->size(), s);
		}
		return *this;
	}
	cow_vector & assign(size_type n, value_type ch){
		this->clear();
		this->append(n, ch);
		return *this;
	}
	cow_vector & assign(initializer_list<value_type> init){
		this->clear();
		this->append(init);
		return *this;
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	cow_vector & assign(inputT first, inputT last){
		if((first != last) && (this->do_check_overlap_generic(*first) == false)){
			this->clear();
		}
		if(this->empty()){
			this->append(::std::move(first), ::std::move(last));
		} else {
			this->do_replace_no_bound_check(0, this->size(), ::std::move(first), ::std::move(last));
		}
		return *this;
	}

	cow_vector & insert(size_type tpos, shallow sh){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), sh);
		return *this;
	}
	cow_vector & insert(size_type tpos, const cow_vector & other, size_type pos = 0, size_type n = npos){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), other, pos, n);
		return *this;
	}
	cow_vector & insert(size_type tpos, const_pointer s, size_type n){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), s, n);
		return *this;
	}
	cow_vector & insert(size_type tpos, const_pointer s){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), s);
		return *this;
	}
	cow_vector & insert(size_type tpos, size_type n, value_type ch){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), n, ch);
		return *this;
	}
	// N.B. This is a non-standard extension.
	cow_vector & insert(size_type tpos, initializer_list<value_type> init){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), init);
		return *this;
	}
	// N.B. This is a non-standard extension.
	iterator insert(const_iterator tins, shallow sh){
		const auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
		const auto wptr = this->do_replace_no_bound_check(tpos, 0, sh);
		return iterator(this, wptr);
	}
	// N.B. This is a non-standard extension.
	iterator insert(const_iterator tins, const cow_vector &other, size_type pos = 0, size_type n = npos){
		const auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
		const auto wptr = this->do_replace_no_bound_check(tpos, 0, other, pos, n);
		return iterator(this, wptr);
	}
	// N.B. This is a non-standard extension.
	iterator insert(const_iterator tins, const_pointer s, size_type n){
		const auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
		const auto wptr = this->do_replace_no_bound_check(tpos, 0, s, n);
		return iterator(this, wptr);
	}
	// N.B. This is a non-standard extension.
	iterator insert(const_iterator tins, const_pointer s){
		const auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
		const auto wptr = this->do_replace_no_bound_check(tpos, 0, s);
		return iterator(this, wptr);
	}
	iterator insert(const_iterator tins, size_type n, value_type ch){
		const auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
		const auto wptr = this->do_replace_no_bound_check(tpos, 0, n, ch);
		return iterator(this, wptr);
	}
	iterator insert(const_iterator tins, initializer_list<value_type> init){
		const auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
		const auto wptr = this->do_replace_no_bound_check(tpos, 0, init);
		return iterator(this, wptr);
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	iterator insert(const_iterator tins, inputT first, inputT last){
		const auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
		const auto wptr = this->do_replace_no_bound_check(tpos, 0, ::std::move(first), ::std::move(last));
		return iterator(this, wptr);
	}
	iterator insert(const_iterator tins, value_type ch){
		return this->insert(tins, size_type(1), ch);
	}

	void swap(cow_vector &other) noexcept {
		noadl::manipulate_allocators(typename allocator_traits<allocator_type>::propagate_on_container_swap(),
		                             allocator_swap_with(), this->m_sth.as_allocator(), other.m_sth.as_allocator());
		this->m_sth.exchange_with(other.m_sth);
		::std::swap(this->m_ptr, other.m_ptr);
		::std::swap(this->m_len, other.m_len);
	}

	// 24.3.2.7, vector operations
	const_pointer data() const noexcept {
		return this->m_ptr;
	}
	// Get a pointer to mutable data. This function may throw `std::bad_alloc()`.
	// N.B. This is a non-standard extension.
	pointer mut_data(){
		return this->do_ensure_unique();
	}
	// N.B. The return type differs from `std::vector`.
	const allocator_type & get_allocator() const noexcept {
		return this->m_sth.as_allocator();
	}
	allocator_type & get_allocator() noexcept {
		return this->m_sth.as_allocator();
	}
*/};

template<typename valueT, typename allocatorT>
inline void swap(cow_vector<valueT, allocatorT> &lhs, cow_vector<valueT, allocatorT> &rhs) noexcept {
	lhs.swap(rhs);
}

template<typename valueT, typename allocatorT>
inline typename cow_vector<valueT, allocatorT>::const_iterator begin(const cow_vector<valueT, allocatorT> &rhs) noexcept {
	return rhs.begin();
}
template<typename valueT, typename allocatorT>
inline typename cow_vector<valueT, allocatorT>::const_iterator end(const cow_vector<valueT, allocatorT> &rhs) noexcept {
	return rhs.end();
}
template<typename valueT, typename allocatorT>
inline typename cow_vector<valueT, allocatorT>::const_reverse_iterator rbegin(const cow_vector<valueT, allocatorT> &rhs) noexcept {
	return rhs.rbegin();
}
template<typename valueT, typename allocatorT>
inline typename cow_vector<valueT, allocatorT>::const_reverse_iterator rend(const cow_vector<valueT, allocatorT> &rhs) noexcept {
	return rhs.rend();
}

template<typename valueT, typename allocatorT>
inline typename cow_vector<valueT, allocatorT>::const_iterator cbegin(const cow_vector<valueT, allocatorT> &rhs) noexcept {
	return rhs.cbegin();
}
template<typename valueT, typename allocatorT>
inline typename cow_vector<valueT, allocatorT>::const_iterator cend(const cow_vector<valueT, allocatorT> &rhs) noexcept {
	return rhs.cend();
}
template<typename valueT, typename allocatorT>
inline typename cow_vector<valueT, allocatorT>::const_reverse_iterator crbegin(const cow_vector<valueT, allocatorT> &rhs) noexcept {
	return rhs.crbegin();
}
template<typename valueT, typename allocatorT>
inline typename cow_vector<valueT, allocatorT>::const_reverse_iterator crend(const cow_vector<valueT, allocatorT> &rhs) noexcept {
	return rhs.crend();
}

}

#endif
