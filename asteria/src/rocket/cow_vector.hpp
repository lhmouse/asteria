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
 * 5. Comparison operators are not provided.
 * 6. The value type may be incomplete. It need be neither copy-assignable nor move-assignable, but must be swappable.
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
using ::std::is_copy_constructible;
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
	struct storage_header {
		static_assert(is_array<valueT>::value == false, "`valueT` must not be an array type.");

		allocatorT alloc;
		size_t n_blocks;
		atomic<ptrdiff_t> ref_count;
		size_t n_elems;
		ROCKET_EXTENSION(valueT data[0]);

		storage_header(allocatorT &&xalloc, size_t xblocks) noexcept
			: alloc(::std::move(xalloc)), n_blocks(xblocks)
		{
			this->n_elems = 0;
			this->ref_count.store(1, ::std::memory_order_release);
		}
		storage_header(const storage_header &) = delete;

		template<typename ...paramsT>
		void do_push_unsafe(paramsT &&...params){
			allocator_traits<allocatorT>::construct(this->alloc, this->data + this->n_elems, ::std::forward<paramsT>(params)...);
			++(this->n_elems);
		}
		void do_pop_unsafe() noexcept {
			--(this->n_elems);
			allocator_traits<allocatorT>::destroy(this->alloc, this->data + this->n_elems);
		}
	};

	template<bool copyableT>
	struct copy_or_throw_helper {
		template<typename valueT, typename allocatorT>
		static void do_copy(storage_header<valueT, allocatorT> * /*ptr*/, const valueT & /*value*/){
			noadl::throw_domain_error("copy_or_throw_helper::do_copy(): The `value_type` of this `cow_vector` is not copy-constructible.");
		}
	};
	template<>
	struct copy_or_throw_helper<true> {
		template<typename valueT, typename allocatorT>
		static void do_copy(storage_header<valueT, allocatorT> *ptr, const valueT &value){
			ptr->do_push_unsafe(value);
		}
	};

	template<typename valueT, typename allocatorT>
	class storage_handle : private allocator_wrapper_base_for<allocatorT>::type {
	public:
		using value_type       = valueT;
		using allocator_type   = allocatorT;

		using size_type        = typename allocator_traits<allocator_type>::size_type;
		using difference_type  = typename allocator_traits<allocator_type>::difference_type;
		using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
		using pointer          = typename allocator_traits<allocator_type>::pointer;

	private:
		using allocator_base    = typename allocator_wrapper_base_for<allocatorT>::type;
		using storage           = storage_header<value_type, allocator_type>;
		using storage_allocator = typename allocator_traits<allocator_type>::template rebind_alloc<storage>;

	private:
		static constexpr size_type do_reserve_blocks_for(size_type n_elems) noexcept {
			return (n_elems * sizeof(value_type) + sizeof(storage) - 1) / sizeof(storage) + 1;
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
			// Decrement the reference count with acquire-release semantics to prevent races on `ptr->alloc`.
			const auto old = ptr->ref_count.fetch_sub(1, ::std::memory_order_acq_rel);
			ROCKET_ASSERT(old > 0);
			if(old > 1){
				return;
			}
			// If it has been decremented to zero, destroy all elements backwards.
			while(ptr->n_elems != 0){
				ptr->do_pop_unsafe();
			}
			// Deallocate the block.
			auto st_alloc = storage_allocator(::std::move(ptr->alloc));
			const auto n_blocks = ptr->n_blocks;
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
			return this->do_get_capacity_of(ptr->n_blocks);
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
				noadl::throw_length_error("storage_handle::check_size_add(): Increasing `%lld` by `%lld` would exceed the max size `%lld`.",
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
			const auto ptr = this->m_ptr;
			if(ptr == nullptr){
				return nullptr;
			}
			ROCKET_ASSERT(this->unique());
			return ptr->data;
		}
		size_type size() const noexcept {
			const auto ptr = this->m_ptr;
			if(ptr == nullptr){
				return 0;
			}
			return ptr->n_elems;
		}
		pointer reallocate(size_type len, size_type res_arg){
			ROCKET_ASSERT(len <= res_arg);
			if(res_arg == 0){
				// Deallocate the block.
				this->do_reset(nullptr);
				return nullptr;
			}
			const auto cap = this->check_size_add(0, res_arg);
			// Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
			const auto n_blocks = this->do_reserve_blocks_for(cap);
			auto alloc = this->as_allocator();
			auto st_alloc = storage_allocator(alloc);
			const auto ptr = allocator_traits<storage_allocator>::allocate(st_alloc, n_blocks);
#ifdef ROCKET_DEBUG
			::std::memset(static_cast<void *>(ptr), '*', sizeof(*ptr) * n_blocks);
#endif
			allocator_traits<storage_allocator>::construct(st_alloc, ptr, ::std::move(alloc), n_blocks);
			if(len != 0){
				const auto ptr_old = this->m_ptr;
				ROCKET_ASSERT(ptr_old);
				ROCKET_ASSERT(len <= ptr_old->n_elems);
				const bool should_copy = is_trivial<value_type>::value || (ptr_old->ref_count.load(::std::memory_order_relaxed) != 1);
				try {
					if(should_copy){
						// Copy-construct elements into the new block.
						while(ptr->n_elems != len){
							copy_or_throw_helper<is_copy_constructible<value_type>::value>::do_copy(ptr, ptr_old->data[ptr->n_elems]);
						}
					} else {
						// Move-construct elements into the new block.
						while(ptr->n_elems != len){
							ptr->do_push_unsafe(::std::move(ptr_old->data[ptr->n_elems]));
						}
					}
				} catch(...){
					// If an exception is thrown, destroy all elements that have been constructed so far.
					while(ptr->n_elems != 0){
						ptr->do_pop_unsafe();
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
			ROCKET_ASSERT(n <= this->capacity() - ptr->n_elems);
			for(size_type i = 0; i < n; ++i){
				ptr->do_push_unsafe(::std::forward<paramsT>(params)...);
			}
			return ptr->data + ptr->n_elems - n;
		}
		pointer pop_back_n(size_type n) noexcept {
			if(n == 0){
				return nullptr;
			}
			const auto ptr = this->m_ptr;
			ROCKET_ASSERT(ptr);
			ROCKET_ASSERT(this->unique());
			ROCKET_ASSERT(n <= ptr->n_elems);
			for(size_type i = 0; i < n; ++i){
				ptr->do_pop_unsafe();
			}
			return ptr->data + ptr->n_elems;
		}
		void rotate(size_type after, size_type seek){
			ROCKET_ASSERT(after <= seek);
			auto bot = after;
			auto brk = seek;
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
			const auto end = ptr->n_elems;
			ROCKET_ASSERT(seek <= end);
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
		const vectorT *m_vec;
		valueT *m_ptr;

	private:
		constexpr vector_iterator(const vectorT *vec, valueT *ptr) noexcept
			: m_vec(vec), m_ptr(ptr)
		{ }

	public:
		constexpr vector_iterator() noexcept
			: vector_iterator(nullptr, nullptr)
		{ }
		template<typename yvalueT, typename enable_if<is_convertible<yvalueT *, valueT *>::value>::type * = nullptr>
		constexpr vector_iterator(const vector_iterator<vectorT, yvalueT> &other) noexcept
			: vector_iterator(other.m_vec, other.m_ptr)
		{ }

	private:
		template<typename pointerT>
		pointerT do_assert_valid_pointer(pointerT ptr, bool to_dereference) const noexcept {
			const auto vec = this->m_vec;
			ROCKET_ASSERT_MSG(vec, "This iterator has not been initialized.");
			const auto dist = static_cast<typename vectorT::size_type>(ptr - vec->data());
			ROCKET_ASSERT_MSG(dist <= vec->size(), "This iterator has been invalidated.");
			if(to_dereference){
				ROCKET_ASSERT_MSG(dist < vec->size(), "This iterator contains a past-the-end value and cannot be dereferenced.");
			}
			return ptr;
		}

	public:
		const vectorT * parent() const noexcept {
			return this->m_vec;
		}

		pointer tell() const noexcept {
			return this->do_assert_valid_pointer(this->m_ptr, false);
		}
		pointer tell_owned_by(const vectorT *vec) const noexcept {
			ROCKET_ASSERT(this->m_vec == vec);
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

public:
	// 26.3.11.2, construct/copy/destroy
	cow_vector() noexcept(noexcept(allocator_type()))
		: cow_vector(allocator_type())
	{ }
	explicit cow_vector(const allocator_type &alloc) noexcept
		: m_sth(alloc)
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
	// Reallocate the storage to `cap` elements.
	// The storage is owned by the current string exclusively after this function returns normally.
	void do_reallocate(size_type len, size_type res_arg){
		ROCKET_ASSERT(len <= res_arg);
		const auto ptr = this->m_sth.reallocate(len, res_arg);
		if(ptr == nullptr){
			// The storage has been deallocated.
			return;
		}
		ROCKET_ASSERT(this->m_sth.unique());
	}
	// Reallocate more storage as needed, without shrinking.
	void do_reallocate_more(size_type cap_add){
		const auto len = this->m_sth.size();
		auto cap = this->m_sth.check_size_add(len, cap_add);
		if((this->m_sth.unique() == false) || (this->m_sth.capacity() < cap)){
#ifndef ROCKET_DEBUG
			// Reserve more space for non-debug builds.
			cap = noadl::max(cap, len + len / 2 + 7);
#endif
			this->do_reallocate(len, cap);
		}
		ROCKET_ASSERT(this->m_sth.capacity() >= cap);
	}
	// Deallocate any dynamic storage.
	void do_deallocate() noexcept {
		this->m_sth.deallocate();
	}

	template<typename ...paramsT>
	iterator do_insert(const_iterator tins, paramsT &&...params){
		const auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
		const auto len_old = this->size();
		this->append(::std::forward<paramsT>(params)...);
		this->m_sth.rotate(tpos, len_old);
		return iterator(this, this->mut_data() + tpos);
	}

public:
	// iterators
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

	// 26.3.11.3, capacity
	bool empty() const noexcept {
		return this->m_sth.size() == 0;
	}
	size_type size() const noexcept {
		return this->m_sth.size();
	}
	size_type max_size() const noexcept {
		return this->m_sth.max_size();
	}
	// N.B. The parameter pack is a non-standard extension.
	template<typename ...paramsT>
	void resize(size_type n, const paramsT &...params){
		const auto len_old = this->size();
		if(len_old == n){
			return;
		}
		if(len_old < n){
			this->append(n - len_old, params...);
		} else {
			this->pop_back(len_old - n);
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
		this->do_reallocate(len, cap_new);
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
			this->do_reallocate(len, len);
		} else {
			this->do_deallocate();
		}
		ROCKET_ASSERT(this->capacity() <= cap_min);
	}
	void clear() noexcept {
		if(this->m_sth.unique()){
			// If the storage is owned exclusively by this vector, truncate it and leave the buffer alone.
			this->m_sth.pop_back_n(this->size());
		} else {
			// Otherwise, detach it from `*this`.
			this->do_deallocate();
		}
		ROCKET_ASSERT(this->empty());
	}
	// N.B. This is a non-standard extension.
	bool unique() const noexcept {
		return this->m_sth.unique();
	}

	// element access
	const_reference operator[](size_type pos) const noexcept {
		const auto len = this->size();
		ROCKET_ASSERT(pos < len);
		return this->data()[pos];
	}
	const_reference at(size_type pos) const {
		const auto len = this->size();
		if(pos >= len){
			noadl::throw_out_of_range("cow_vector::at(): The subscript `%lld` is not a writable position within a vector of size `%lld`.",
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
			noadl::throw_out_of_range("cow_vector::mut(): The subscript `%lld` is not a writable position within a vector of size `%lld`.",
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

	// N.B. This is a non-standard extension.
	template<typename ...paramsT>
	cow_vector & append(size_type n, const paramsT &...params){
		this->do_reallocate_more(n);
		this->m_sth.emplace_back_n(n, params...);
		return *this;
	}
	// N.B. This is a non-standard extension.
	cow_vector & append(initializer_list<value_type> init){
		return this->append(init.begin(), init.end());
	}
	// N.B. This is a non-standard extension.
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	cow_vector & append(inputT first, inputT last){
		this->do_reallocate_more(noadl::estimate_distance(first, last));
		for(auto it = ::std::move(first); it != last; ++it){
			this->emplace_back(*it);
		}
		return *this;
	}
	// 26.3.11.5, modifiers
	template<typename ...paramsT>
	reference emplace_back(paramsT &&...params){
		this->do_reallocate_more(1);
		const auto wptr = this->m_sth.emplace_back_n(1, ::std::forward<paramsT>(params)...);
		return *wptr;
	}
	// N.B. The return type is a non-standard extension.
	reference push_back(const value_type &value){
		return this->emplace_back(value);
	}
	// N.B. The return type is a non-standard extension.
	reference push_back(value_type &&value){
		return this->emplace_back(::std::move(value));
	}

	template<typename ...paramsT>
	iterator emplace(const_iterator tins, paramsT &&...params){
		const auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
		const auto len_old = this->size();
		this->emplace_back(::std::forward<paramsT>(params)...);
		this->m_sth.rotate(tpos, len_old);
		return iterator(this, this->mut_data() + tpos);
	}
	iterator insert(const_iterator tins, const value_type &value){
		return this->emplace(tins, value);
	}
	iterator insert(const_iterator tins, value_type &&value){
		return this->emplace(tins, ::std::move(value));
	}
	// N.B. The parameter pack is a non-standard extension.
	template<typename ...paramsT>
	iterator insert(const_iterator tins, size_type n, const paramsT &...params){
		return this->do_insert(tins, n, params...);
	}
	iterator insert(const_iterator tins, initializer_list<value_type> init){
		return this->do_insert(tins, init);
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	iterator insert(const_iterator tins, inputT first, inputT last){
		return this->do_insert(tins, ::std::move(first), ::std::move(last));
	}

	// N.B. This function may throw `std::bad_alloc()`.
	iterator erase(const_iterator tfirst, const_iterator tlast){
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
		const auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
		const auto len_old = this->size();
		ROCKET_ASSERT(tpos <= len_old);
		ROCKET_ASSERT(tn <= len_old - tpos);
		pointer ptr;
		if(tpos + tn == len_old){
			if(this->m_sth.unique() == false){
				this->do_reallocate(tpos, len_old);
			} else {
				this->m_sth.pop_back_n(tn);
			}
			ptr = this->m_sth.mut_data();
		} else {
			if(this->m_sth.unique() == false){
				this->do_reallocate(len_old, len_old);
			}
			ptr = this->m_sth.mut_data();
			this->m_sth.rotate(tpos, tpos + tn);
			this->m_sth.pop_back_n(tn);
		}
		return iterator(this, ptr + tpos);
	}
	// N.B. This function may throw `std::bad_alloc()`.
	iterator erase(const_iterator trm){
		return this->erase(trm, const_iterator(this, trm.tell() + 1));
	}
	// N.B. This function may throw `std::bad_alloc()`.
	// N.B. The return type and parameter are non-standard extensions.
	cow_vector & pop_back(size_type n = 1){
		if(n == 0){
			return *this;
		}
		const auto len_old = this->size();
		ROCKET_ASSERT(n <= len_old);
		if(this->m_sth.unique() == false){
			this->do_reallocate(len_old - n, len_old);
		} else {
			this->m_sth.pop_back_n(n);
		}
		return *this;
	}

	// N.B. The return type is a non-standard extension.
	cow_vector & assign(const cow_vector &other) noexcept {
		this->m_sth.share_with(other.m_sth);
		return *this;
	}
	// N.B. The return type is a non-standard extension.
	cow_vector & assign(cow_vector &&other) noexcept {
		this->m_sth.share_with(::std::move(other.m_sth));
		return *this;
	}
	// N.B. The parameter pack is a non-standard extension.
	// N.B. The return type is a non-standard extension.
	template<typename ...paramsT>
	cow_vector & assign(size_type n, const paramsT &...params){
		this->clear();
		this->insert(this->begin(), n, params...);
		return *this;
	}
	// N.B. The return type is a non-standard extension.
	cow_vector & assign(initializer_list<value_type> init){
		this->clear();
		this->insert(this->begin(), init);
		return *this;
	}
	// N.B. The return type is a non-standard extension.
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	cow_vector & assign(inputT first, inputT last){
		this->clear();
		this->insert(this->begin(), ::std::move(first), ::std::move(last));
		return *this;
	}

	void swap(cow_vector &other) noexcept {
		noadl::manipulate_allocators(typename allocator_traits<allocator_type>::propagate_on_container_swap(),
		                             allocator_swap_with(), this->m_sth.as_allocator(), other.m_sth.as_allocator());
		this->m_sth.exchange_with(other.m_sth);
	}

	// 26.3.11.4, data access
	const_pointer data() const noexcept {
		return this->m_sth.data();
	}
	// Get a pointer to mutable data. This function may throw `std::bad_alloc()`.
	// N.B. This is a non-standard extension.
	pointer mut_data(){
		const auto len = this->size();
		if(len == 0){
			return nullptr;
		}
		if(this->m_sth.unique() == false){
			this->do_reallocate(len, len);
		}
		return this->m_sth.mut_data();
	}
	// N.B. The return type differs from `std::vector`.
	const allocator_type & get_allocator() const noexcept {
		return this->m_sth.as_allocator();
	}
	allocator_type & get_allocator() noexcept {
		return this->m_sth.as_allocator();
	}
};

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
