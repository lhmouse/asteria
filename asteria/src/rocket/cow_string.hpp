// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRING_HPP_
#define ROCKET_COW_STRING_HPP_

#include <string> // std::char_traits<>
#include <memory> // std::allocator<>, std::allocator_traits<>, std::addressof()
#include <istream> // std::streamsize, std::ios_base, std::basic_ios<>, std::basic_istream<>
#include <locale> // std::isspace()
#include <ostream> // std::basic_ostream<>
#include <atomic> // std::atomic<>
#include <type_traits> // so many...
#include <iterator> // std::iterator_traits<>, std::reverse_iterator<>, std::random_access_iterator_tag, std::distance()
#include <initializer_list> // std::initializer_list<>
#include <utility> // std::move(), std::forward(), std::declval()
#include <cstdint> // std::uint_fast32_t
#include <cstddef> // std::size_t, std::ptrdiff_t
#include "compatibility.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"

/* Differences from `std::basic_string`:
 * 1. All functions guarantee only basic exception safety rather than strong exception safety, hence are more efficient.
 * 2. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always return `const_reference`s.
 * 3. It is possible to create strings holding non-owning references of null-terminated character arrays allocated externally.
 * 4. The copy constructor and copy assignment operator will not throw exceptions.
 * 5. The constructor taking a sole const pointer is made `explicit`.
 * 6. The assignment operator taking a character and the one taking a const pointer are not provided.
 */

namespace rocket {

using ::std::char_traits;
using ::std::allocator;
using ::std::allocator_traits;
using ::std::streamsize;
using ::std::ios_base;
using ::std::basic_ios;
using ::std::basic_istream;
using ::std::basic_ostream;
using ::std::atomic;
using ::std::is_same;
using ::std::decay;
using ::std::is_array;
using ::std::is_trivial;
using ::std::enable_if;
using ::std::is_convertible;
using ::std::conditional;
using ::std::iterator_traits;
using ::std::reverse_iterator;
using ::std::initializer_list;
using ::std::uint_fast32_t;
using ::std::size_t;
using ::std::ptrdiff_t;

template<typename charT, typename traitsT = char_traits<charT>, typename allocatorT = allocator<charT>>
class basic_cow_string;

namespace details_cow_string {
	template<typename charT, typename traitsT>
	void handle_io_exception(basic_ios<charT, traitsT> &ios){
		// Set `ios_base::badbit` without causing `ios_base::failure` to be thrown.
		// XXX: Catch-then-ignore is **very** inefficient notwithstanding, it cannot be made more portable.
		try {
			ios.setstate(ios_base::badbit);
		} catch(ios_base::failure &){
			// Ignore this exception.
		}
		// Rethrow the **original** exception, if `ios_base::badbit` has been turned on in `os.exceptions()`.
		if(ios.exceptions() & ios_base::badbit){
			rethrow_current_exception();
		}
	}

	extern template void handle_io_exception(::std::ios  &ios);
	extern template void handle_io_exception(::std::wios &ios);

	template<typename charT, typename traitsT = char_traits<charT>, typename allocatorT = allocator<charT>>
	class storage_handle : private allocator_wrapper_base_for<allocatorT> {
	public:
		using value_type       = charT;
		using traits_type      = traitsT;
		using allocator_type   = allocatorT;

		using size_type        = typename allocator_traits<allocator_type>::size_type;
		using difference_type  = typename allocator_traits<allocator_type>::difference_type;
		using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
		using pointer          = typename allocator_traits<allocator_type>::pointer;

	private:
		struct storage {
			size_type n_blocks;
			typename allocator_traits<allocator_type>::template rebind_alloc<storage> alloc;
			atomic<difference_type> ref_count;
			value_type data[1]; // This makes room for the null terminator.

			storage(size_type xn_blocks, const allocator_type &xalloc) noexcept
				: n_blocks(xn_blocks), alloc(xalloc)
			{
				this->ref_count.store(1, ::std::memory_order_release);
			}
		};

		using allocator_base    = allocator_wrapper_base_for<allocatorT>;
		using storage_allocator = decltype(storage::alloc);

	private:
		// All blocks other than the first one provide storage for characters, while the first one provides storage for the null termiantor.
		static constexpr size_type do_get_capacity_of(size_type n_blocks) noexcept {
			return (n_blocks - 1) * sizeof(storage) / sizeof(value_type);
		}
		static constexpr size_type do_get_blocks_for(size_type n_chars) noexcept {
			return 1 + (n_chars * sizeof(value_type) + sizeof(storage) - 1) / sizeof(storage);
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
			const auto ptr = ((exchange))(this->m_ptr, ptr_new);
			if(ptr == nullptr){
				return;
			}
			// Decrement the reference count with acquire-release semantics to prevent races on `ptr->alloc`.
			const auto old = ptr->ref_count.fetch_sub(1, ::std::memory_order_acq_rel);
			ROCKET_ASSERT(old > 0);
			if(old > 1){
				return;
			}
			// If it has been decremented to zero, deallocate the block.
			const auto n_blocks = ptr->n_blocks;
			auto alloc = ::std::move(ptr->alloc);
			allocator_traits<storage_allocator>::destroy(alloc, ptr);
#ifdef ROCKET_DEBUG
			::std::memset(ptr, '~', sizeof(*ptr) * n_blocks);
#endif
			allocator_traits<storage_allocator>::deallocate(alloc, ptr, n_blocks);
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
			return do_get_capacity_of(ptr->n_blocks);
		}
		size_type max_size() const noexcept {
			auto alloc = storage_allocator(this->as_allocator());
			const auto max_n_blocks = allocator_traits<storage_allocator>::max_size(alloc);
			return do_get_capacity_of(max_n_blocks / 2 - 1);
		}
		size_type round_up_capacity(size_type cap) const {
			return do_get_capacity_of(do_get_blocks_for(this->check_size(0, cap)));
		}
		size_type check_size(size_type base, size_type add) const {
			const auto cap_max = this->max_size();
			ROCKET_ASSERT(base <= cap_max);
			if(cap_max - base < add){
				throw_length_error("storage_handle::check_size(): Increasing `%lld` by `%lld` would exceed the max length `%lld`.",
				                   static_cast<long long>(base), static_cast<long long>(add), static_cast<long long>(cap_max));
			}
			return base + add;
		}
		pointer data() const noexcept {
			const auto ptr = this->m_ptr;
			if(ptr == nullptr){
				return nullptr;
			}
			return ptr->data;
		}
		pointer reallocate(const_pointer src, size_type len, size_type cap){
			ROCKET_ASSERT(len <= cap);
			if(cap == 0){
				// Deallocate the block.
				this->do_reset(nullptr);
				return nullptr;
			}
			// Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
			const auto n_blocks = do_get_blocks_for(this->check_size(0, cap));
			auto alloc = storage_allocator(this->as_allocator());
			const auto ptr = static_cast<storage *>(allocator_traits<storage_allocator>::allocate(alloc, n_blocks));
#ifdef ROCKET_DEBUG
			::std::memset(ptr, '*', sizeof(*ptr) * n_blocks);
#endif
			allocator_traits<storage_allocator>::construct(alloc, ptr, n_blocks, this->as_allocator());
			// Copy characters into the new block, then terminate it with a null character.
			// Since `src` might point to somewhere in the old block, this has to be done before deallocating the old block.
			traits_type::copy(ptr->data, src, len);
			// Replace the current block.
			this->do_reset(ptr);
			return ptr->data;
		}
		void deallocate() noexcept {
			this->do_reset(nullptr);
		}

		void assign(const storage_handle &other) noexcept {
			const auto ptr = other.m_ptr;
			if(ptr){
				// Increment the reference count.
				const auto old = ptr->ref_count.fetch_add(1, ::std::memory_order_relaxed);
				ROCKET_ASSERT(old > 0);
			}
			this->do_reset(ptr);
		}
		void assign(storage_handle &&other) noexcept {
			const auto ptr = ((exchange))(other.m_ptr, nullptr);
			this->do_reset(ptr);
		}
		void swap(storage_handle &other) noexcept {
			::std::swap(this->m_ptr, other.m_ptr);
		}
	};

	extern template class storage_handle<char>;
	extern template class storage_handle<wchar_t>;
	extern template class storage_handle<char16_t>;
	extern template class storage_handle<char32_t>;

	template<typename stringT>
	class string_iterator_base {
		friend stringT;

	private:
		const stringT *m_str;

	protected:
		explicit constexpr string_iterator_base(const stringT *str) noexcept
			: m_str(str)
		{ }

	protected:
		template<typename pointerT>
		pointerT do_assert_valid_pointer(pointerT ptr, bool to_dereference) const noexcept {
			const auto str = this->m_str;
			ROCKET_ASSERT_MSG(str, "This iterator has not been initialized.");
			const auto dist = static_cast<typename stringT::size_type>(ptr - str->data());
			ROCKET_ASSERT_MSG(dist <= str->size(), "This iterator has been invalidated.");
			if(to_dereference){
				ROCKET_ASSERT_MSG(dist < str->size(), "This iterator contains a past-the-end value and cannot be dereferenced.");
			}
			return ptr;
		}

	public:
		const stringT * parent() const noexcept {
			return m_str;
		}
	};

	// basic_cow_string::iterator
	template<typename stringT, typename charT>
	class string_iterator : public string_iterator_base<stringT> {
		friend stringT;
		friend string_iterator<stringT, const charT>;

	public:
		using value_type         = charT;
		using pointer            = value_type *;
		using reference          = value_type &;
		using difference_type    = ptrdiff_t;
		using iterator_category  = ::std::random_access_iterator_tag;

	private:
		pointer m_ptr;

	private:
		string_iterator(const stringT *str, pointer ptr) noexcept
			: string_iterator_base<stringT>(str)
			, m_ptr(this->do_assert_valid_pointer(ptr, false))
		{ }

	public:
		constexpr string_iterator() noexcept
			: string_iterator_base<stringT>(nullptr)
			, m_ptr(nullptr)
		{ }

	public:
		pointer tell() const noexcept {
			return this->do_assert_valid_pointer(this->m_ptr, false);
		}
		pointer tell_owned_by(const stringT *str) const noexcept {
			ROCKET_ASSERT(this->parent() == str);
			return this->tell();
		}
		string_iterator & seek(pointer ptr) noexcept {
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

	// basic_cow_string::const_iterator
	template<typename stringT, typename charT>
	class string_iterator<stringT, const charT> : public string_iterator_base<stringT> {
		friend stringT;
		friend string_iterator<stringT, charT>;

	public:
		using value_type         = charT;
		using pointer            = const value_type *;
		using reference          = const value_type &;
		using difference_type    = ptrdiff_t;
		using iterator_category  = ::std::random_access_iterator_tag;

	private:
		pointer m_ptr;

	private:
		string_iterator(const stringT *str, pointer ptr) noexcept
			: string_iterator_base<stringT>(str)
			, m_ptr(this->do_assert_valid_pointer(ptr, false))
		{ }

	public:
		constexpr string_iterator() noexcept
			: string_iterator_base<stringT>(nullptr)
			, m_ptr(nullptr)
		{ }
		string_iterator(const string_iterator<stringT, charT> &other) noexcept
			: string_iterator_base<stringT>(other)
			, m_ptr(other.m_ptr)
		{ }

	public:
		pointer tell() const noexcept {
			return this->do_assert_valid_pointer(this->m_ptr, false);
		}
		pointer tell_owned_by(const stringT *str) const noexcept {
			ROCKET_ASSERT(this->parent() == str);
			return this->tell();
		}
		string_iterator & seek(pointer ptr) noexcept {
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

	template<typename stringT, typename charT>
	inline string_iterator<stringT, charT> & operator++(string_iterator<stringT, charT> &rhs) noexcept {
		return rhs.seek(rhs.tell() + 1);
	}
	template<typename stringT, typename charT>
	inline string_iterator<stringT, charT> & operator--(string_iterator<stringT, charT> &rhs) noexcept {
		return rhs.seek(rhs.tell() - 1);
	}

	template<typename stringT, typename charT>
	inline string_iterator<stringT, charT> operator++(string_iterator<stringT, charT> &lhs, int) noexcept {
		auto res = lhs;
		lhs.seek(lhs.tell() + 1);
		return res;
	}
	template<typename stringT, typename charT>
	inline string_iterator<stringT, charT> operator--(string_iterator<stringT, charT> &lhs, int) noexcept {
		auto res = lhs;
		lhs.seek(lhs.tell() - 1);
		return res;
	}

	template<typename stringT, typename charT>
	inline string_iterator<stringT, charT> & operator+=(string_iterator<stringT, charT> &lhs, typename string_iterator<stringT, charT>::difference_type rhs) noexcept {
		return lhs.seek(lhs.tell() + rhs);
	}
	template<typename stringT, typename charT>
	inline string_iterator<stringT, charT> & operator-=(string_iterator<stringT, charT> &lhs, typename string_iterator<stringT, charT>::difference_type rhs) noexcept {
		return lhs.seek(lhs.tell() - rhs);
	}

	template<typename stringT, typename charT>
	inline string_iterator<stringT, charT> operator+(const string_iterator<stringT, charT> &lhs, typename string_iterator<stringT, charT>::difference_type rhs) noexcept {
		auto res = lhs;
		res.seek(res.tell() + rhs);
		return res;
	}
	template<typename stringT, typename charT>
	inline string_iterator<stringT, charT> operator-(const string_iterator<stringT, charT> &lhs, typename string_iterator<stringT, charT>::difference_type rhs) noexcept {
		auto res = lhs;
		res.seek(res.tell() - rhs);
		return res;
	}

	template<typename stringT, typename charT>
	inline string_iterator<stringT, charT> operator+(typename string_iterator<stringT, charT>::difference_type lhs, const string_iterator<stringT, charT> &rhs) noexcept {
		auto res = rhs;
		res.seek(res.tell() + lhs);
		return res;
	}
	template<typename stringT, typename xcharT, typename ycharT>
	inline typename string_iterator<stringT, xcharT>::difference_type operator-(const string_iterator<stringT, xcharT> &lhs, const string_iterator<stringT, ycharT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) - rhs.tell();
	}

	template<typename stringT, typename xcharT, typename ycharT>
	inline bool operator==(const string_iterator<stringT, xcharT> &lhs, const string_iterator<stringT, ycharT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) == rhs.tell();
	}
	template<typename stringT, typename xcharT, typename ycharT>
	inline bool operator!=(const string_iterator<stringT, xcharT> &lhs, const string_iterator<stringT, ycharT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) != rhs.tell();
	}
	template<typename stringT, typename xcharT, typename ycharT>
	inline bool operator<(const string_iterator<stringT, xcharT> &lhs, const string_iterator<stringT, ycharT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) < rhs.tell();
	}
	template<typename stringT, typename xcharT, typename ycharT>
	inline bool operator>(const string_iterator<stringT, xcharT> &lhs, const string_iterator<stringT, ycharT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) > rhs.tell();
	}
	template<typename stringT, typename xcharT, typename ycharT>
	inline bool operator<=(const string_iterator<stringT, xcharT> &lhs, const string_iterator<stringT, ycharT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) <= rhs.tell();
	}
	template<typename stringT, typename xcharT, typename ycharT>
	inline bool operator>=(const string_iterator<stringT, xcharT> &lhs, const string_iterator<stringT, ycharT> &rhs) noexcept {
		return lhs.tell_owned_by(rhs.parent()) >= rhs.tell();
	}

	// basic_cow_string::shallow
	template<typename charT, typename traitsT>
	class shallow;

	template<typename charT>
	class shallow_base {
	protected:
		static constexpr charT s_empty[1] = { charT() };
	};

	template<typename charT>
	constexpr charT shallow_base<charT>::s_empty[1];

	extern template class shallow_base<char>;
	extern template class shallow_base<wchar_t>;
	extern template class shallow_base<char16_t>;
	extern template class shallow_base<char32_t>;

	template<typename charT, typename traitsT>
	class shallow : public shallow_base<charT> {
	public:
		using char_type    = charT;
		using traits_type  = traitsT;
		using size_type    = size_t;

	private:
		const char_type *m_ptr;
		size_type m_len;

	public:
		shallow() noexcept
			: m_ptr(shallow_base<charT>::s_empty), m_len(0)
		{ }
		explicit shallow(const char_type *ptr) noexcept
			: m_ptr(ptr), m_len(traitsT::length(ptr))
		{ }
		template<typename allocatorT>
		explicit shallow(const basic_cow_string<charT, traitsT, allocatorT> &str) noexcept
			: m_ptr(str.data()), m_len(str.size())
		{ }

	public:
		const char_type * data() const noexcept {
			return this->m_ptr;
		}
		size_type size() const noexcept {
			return this->m_len;
		}
	};

	// Implement relational operators.
	template<typename charT, typename traitsT>
	struct comparator {
		using char_type    = charT;
		using traits_type  = traitsT;
		using size_type    = size_t;

		static int inequality(const char_type *s1, size_type n1, const char_type *s2, size_type n2) noexcept {
			if(n1 != n2){
				return 2;
			}
			if(s1 == s2){
				return 0;
			}
			const int res = traits_type::compare(s1, s2, ((min))(n1, n2));
			return res;
		}
		static int relation(const char_type *s1, size_type n1, const char_type *s2, size_type n2) noexcept {
			const int res = traits_type::compare(s1, s2, ((min))(n1, n2));
			if(res != 0){
				return res;
			}
			if(n1 < n2){
				return -1;
			}
			if(n1 > n2){
				return +1;
			}
			return 0;
		}
	};
}

template<typename charT, typename traitsT, typename allocatorT>
class basic_cow_string {
	static_assert(is_same<typename allocatorT::value_type, charT>::value, "`allocatorT::value_type` must name the same type as `charT`.");
	static_assert(is_array<charT>::value == false, "`charT` must not be an array type.");
	static_assert(is_trivial<charT>::value != false, "`charT` must be a trivial type.");

public:
	// types
	using value_type      = charT;
	using traits_type     = traitsT;
	using allocator_type  = allocatorT;

	using size_type        = typename allocator_traits<allocator_type>::size_type;
	using difference_type  = typename allocator_traits<allocator_type>::difference_type;
	using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
	using pointer          = typename allocator_traits<allocator_type>::pointer;
	using const_reference  = const value_type &;
	using reference        = value_type &;

	using const_iterator          = details_cow_string::string_iterator<basic_cow_string, const value_type>;
	using iterator                = details_cow_string::string_iterator<basic_cow_string, value_type>;
	using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
	using reverse_iterator        = ::std::reverse_iterator<iterator>;

	enum : size_type { npos = size_type(-1) };

	// helpers
	using shallow     = details_cow_string::shallow<value_type, traits_type>;
	using comparator  = details_cow_string::comparator<value_type, traits_type>;

	// associative container support
	struct equal_to;
	struct less;
	struct hash;

private:
	details_cow_string::storage_handle<charT, traitsT, allocatorT> m_sth;
	const_pointer m_ptr;
	size_type m_len;

public:
	// 24.3.2.2, construct/copy/destroy
	basic_cow_string() noexcept(noexcept(allocator_type()))
		: basic_cow_string(shallow(), allocator_type())
	{ }
	explicit basic_cow_string(const allocator_type &alloc) noexcept
		: basic_cow_string(shallow(), alloc)
	{ }
	basic_cow_string(shallow sh) noexcept(noexcept(allocator_type()))
		: basic_cow_string(sh, allocator_type())
	{ }
	basic_cow_string(shallow sh, const allocator_type &alloc) noexcept
		: m_sth(alloc), m_ptr(sh.data()), m_len(sh.size())
	{ }
	basic_cow_string(const basic_cow_string &other) noexcept
		: basic_cow_string(allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
	{
		this->assign(other);
	}
	basic_cow_string(const basic_cow_string &other, const allocator_type &alloc) noexcept
		: basic_cow_string(alloc)
	{
		this->assign(other);
	}
	basic_cow_string(basic_cow_string &&other) noexcept
		: basic_cow_string(::std::move(other.m_sth.as_allocator()))
	{
		this->assign(::std::move(other));
	}
	basic_cow_string(basic_cow_string &&other, const allocator_type &alloc) noexcept
		: basic_cow_string(alloc)
	{
		this->assign(::std::move(other));
	}
	basic_cow_string(const basic_cow_string &other, size_type pos, size_type n = npos, const allocator_type &alloc = allocator_type())
		: basic_cow_string(alloc)
	{
		this->assign(other, pos, n);
	}
	basic_cow_string(const_pointer s, size_type n, const allocator_type &alloc = allocator_type())
		: basic_cow_string(alloc)
	{
		this->assign(s, n);
	}
	explicit basic_cow_string(const_pointer s, const allocator_type &alloc = allocator_type())
		: basic_cow_string(alloc)
	{
		this->assign(s);
	}
	basic_cow_string(size_type n, value_type ch, const allocator_type &alloc = allocator_type())
		: basic_cow_string(alloc)
	{
		this->assign(n, ch);
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	basic_cow_string(inputT first, inputT last, const allocator_type &alloc = allocator_type())
		: basic_cow_string(alloc)
	{
		this->assign(::std::move(first), ::std::move(last));
	}
	basic_cow_string(initializer_list<value_type> init, const allocator_type &alloc = allocator_type())
		: basic_cow_string(alloc)
	{
		this->assign(init);
	}
	// Assignment operators.
	basic_cow_string & operator=(shallow sh) noexcept {
		return this->assign(sh);
	}
	basic_cow_string & operator=(const basic_cow_string &other) noexcept {
		if(this == &other){
			return *this;
		}
		((manipulate_allocators))(typename allocator_traits<allocator_type>::propagate_on_container_copy_assignment(),
		                          allocator_copy_assign_from(), this->m_sth.as_allocator(), other.m_sth.as_allocator());
		return this->assign(other);
	}
	basic_cow_string & operator=(basic_cow_string &&other) noexcept {
		((manipulate_allocators))(typename allocator_traits<allocator_type>::propagate_on_container_move_assignment(),
		                          allocator_move_assign_from(), this->m_sth.as_allocator(), ::std::move(other.m_sth.as_allocator()));
		return this->assign(::std::move(other));
	}
	basic_cow_string & operator=(initializer_list<value_type> init){
		return this->assign(init);
	}

private:
	// Reallocate the storage to `cap` characters, not including the null terminator. The first `len` characters are left intact and the rest are undefined.
	// The storage is owned by the current string exclusively after this function returns normally.
	pointer do_reallocate_no_set_length(size_type len, size_type cap){
		ROCKET_ASSERT(len <= cap);
		ROCKET_ASSERT(len <= this->m_len);
		ROCKET_ASSERT(cap != 0);
		const auto ptr = this->m_sth.reallocate(this->m_ptr, len, cap);
		ROCKET_ASSERT(this->m_sth.unique());
		this->m_ptr = ptr;
		return ptr + len;
	}
	// Reallocate more storage as needed, without shrinking.
	pointer do_auto_reallocate_no_set_length(size_type len, size_type cap_add){
		ROCKET_ASSERT(len <= this->m_len);
		auto cap = this->m_sth.check_size(len, cap_add);
		if((this->m_sth.unique() == false) || (this->m_sth.capacity() < cap)){
#ifndef ROCKET_DEBUG
			// Reserve more space for non-debug builds.
			cap |= len + len / 2;
			cap |= 64;
#endif
			this->do_reallocate_no_set_length(len, cap);
		}
		ROCKET_ASSERT(this->m_sth.capacity() >= cap);
		const auto ptr = const_cast<pointer>(this->m_ptr);
		return ptr + len;
	}
	// Add a null terminator at `ptr[len]` then set `len` there.
	void do_set_length(size_type len) noexcept {
		ROCKET_ASSERT(this->m_sth.unique());
		ROCKET_ASSERT(len <= this->m_sth.capacity());
		const auto ptr = const_cast<pointer>(this->m_ptr);
		ROCKET_ASSERT(ptr == this->m_sth.data());
		ROCKET_ASSERT(len <= this->m_sth.capacity());
		traits_type::assign(ptr + len, 1, value_type());
		this->m_len = len;
	}
	// Get a pointer to mutable storage.
	pointer do_ensure_unique(){
		if(this->m_sth.unique() == false){
			const auto len = this->m_len;
			this->do_reallocate_no_set_length(len, ((max))(len, size_type(1)));
			this->do_set_length(len);
		}
		ROCKET_ASSERT(this->m_sth.unique());
		const auto ptr = const_cast<pointer>(this->m_ptr);
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
			throw_out_of_range("basic_cow_string::do_clamp_substr(): The subscript `%lld` is out of range for a string of length `%lld`.",
			                   static_cast<long long>(tpos), static_cast<long long>(tlen));
		}
		return ((min))(tlen - tpos, n);
	}

	// This has to be generic to allow construction of a string from an array of integers... This is a nasty trick anyway.
	template<typename someT>
	bool do_check_overlap_generic(const someT &some) const noexcept {
		return static_cast<size_type>(reinterpret_cast<const value_type (&)[1]>(some) - this->data()) < this->size();
	}
	// `[first, last)` must not refer to anywhere inside the current object. Otherwise, the behavior is undefined.
	template<typename inputT>
	pointer do_append_nonempty_range_restrict(::std::input_iterator_tag, inputT first, inputT last){
		ROCKET_ASSERT(first != last);
		const auto len_old = this->size();
		size_type len_added = 0;
		pointer ptr;
		// Append characters one by one.
		auto it = first;
		do {
			ptr = this->do_auto_reallocate_no_set_length(len_old + len_added, 1) - len_added;
			this->do_set_length(len_old + len_added);
			traits_type::assign(ptr[len_added], *it);
			++len_added;
			this->do_set_length(len_old + len_added);
		} while(++it != last);
		// Return a pointer to the inserted area.
		return ptr;
	}
	template<typename inputT>
	pointer do_append_nonempty_range_restrict(::std::forward_iterator_tag, inputT first, inputT last){
		const auto range_dist = ::std::distance(first, last);
		static_assert(sizeof(range_dist) <= sizeof(size_type), "The `difference_type` of input iterators is larger than `size_type` of this string.");
		ROCKET_ASSERT(range_dist > 0);
		const auto len_old = this->size();
		size_type len_added = 0;
		// Reserve the space first.
		const auto ptr = this->do_auto_reallocate_no_set_length(len_old, static_cast<size_type>(range_dist));
		this->do_set_length(len_old);
		// Append characters one by one.
		auto it = first;
		do {
			ROCKET_ASSERT(len_added < static_cast<size_type>(range_dist));
			traits_type::assign(ptr[len_added], *it);
			++len_added;
			this->do_set_length(len_old + len_added);
		} while(++it != last);
		// Return a pointer to the inserted area.
		return ptr;
	}
	// Remove a substring. This function may throw `std::bad_alloc`.
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
		for(auto i = from; i < len - n + 1; ++i){
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
		for(auto i = ((min))(len - n, to); i + 1 > 0; --i){
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
	size_type size() const noexcept {
		return this->m_len;
	}
	size_type length() const noexcept {
		return this->size();
	}
	size_type max_size() const noexcept {
		return this->m_sth.max_size();
	}
	void resize(size_type n, value_type ch){
		const auto len_old = this->size();
		const auto len_add = n - ((min))(len_old, n);
		const auto wptr = this->do_auto_reallocate_no_set_length(len_old, len_add);
		traits_type::assign(wptr, len_add, ch);
		this->do_set_length(n);
	}
	void resize(size_type n){
		this->resize(n, value_type());
	}
	size_type capacity() const noexcept {
		return this->m_sth.capacity();
	}
	void reserve(size_type res_arg = 0){
		if(res_arg == 0){
			this->shrink_to_fit();
			return;
		}
		const auto len = this->size();
		const auto cap_new = this->m_sth.round_up_capacity(((max))(len, res_arg));
		// If the storage is shared with other strings, force rellocation to prevent copy-on-write upon modification.
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
	}
	void clear() noexcept {
		if(this->m_sth.unique()){
			// If the storage is owned exclusively by this string, truncate it and leave the buffer alone.
			this->do_set_length(0);
		} else {
			// Otherwise, detach it from `*this`.
			this->do_deallocate();
		}
		ROCKET_ASSERT(this->empty());
	}
	bool empty() const noexcept {
		return this->m_len == 0;
	}

	// 24.3.2.5, element access
	const_reference operator[](size_type pos) const noexcept {
		const auto len = this->size();
		// Reading from the character at `size()` is permitted.
		ROCKET_ASSERT(pos <= len);
		return this->data()[pos];
	}
	const_reference at(size_type pos) const {
		const auto len = this->size();
		if(pos >= len){
			throw_out_of_range("basic_cow_string::at(): The subscript `%lld` is not a writable position within a string of length `%lld`.",
			                   static_cast<long long>(pos), static_cast<long long>(len));
		}
		return this->data()[pos];
	}
	const_reference front() const noexcept {
		const auto len = this->size();
		ROCKET_ASSERT(len > 0);
		return this->data()[0];
	}
	const_reference back() const noexcept {
		const auto len = this->size();
		ROCKET_ASSERT(len > 0);
		return this->data()[len - 1];
	}
	// There is no `at()` overload that returns a non-const reference. This is the consequent overload which does that.
	// N.B. This is a non-standard extension.
	reference & mut(size_type pos){
		const auto len = this->size();
		if(pos >= len){
			throw_out_of_range("basic_cow_string::mut(): The subscript `%lld` is not a writable position within a string of length `%lld`.",
			                   static_cast<long long>(pos), static_cast<long long>(len));
		}
		return this->mut_data()[pos];
	}

	// 24.3.2.6, modifiers
	basic_cow_string & operator+=(const basic_cow_string & other){
		return this->append(other);
	}
	basic_cow_string & operator+=(shallow sh){
		return this->append(sh);
	}
	basic_cow_string & operator+=(const_pointer s){
		return this->append(s);
	}
	basic_cow_string & operator+=(value_type ch){
		return this->append(size_type(1), ch);
	}
	basic_cow_string & operator+=(initializer_list<value_type> init){
		return this->append(init);
	}

	basic_cow_string & append(shallow sh){
		return this->append(sh.data(), sh.size());
	}
	basic_cow_string & append(const basic_cow_string & other, size_type pos = 0, size_type n = npos){
		return this->append(other.data() + pos, other.do_clamp_substr(pos, n));
	}
	basic_cow_string & append(const_pointer s, size_type n){
		if(n == 0){
			return *this;
		}
		const auto len_old = this->size();
		if(this->do_check_overlap_generic(*s)){
			const auto tpos = s - this->data();
			const auto wptr = this->do_auto_reallocate_no_set_length(len_old, n);
			traits_type::move(wptr, this->data() + tpos, n);
		} else {
			const auto wptr = this->do_auto_reallocate_no_set_length(len_old, n);
			traits_type::copy(wptr, s, n);
		}
		this->do_set_length(len_old + n);
		return *this;
	}
	basic_cow_string & append(const_pointer s){
		return this->append(s, traits_type::length(s));
	}
	basic_cow_string & append(size_type n, value_type ch){
		if(n == 0){
			return *this;
		}
		const auto len_old = this->size();
		const auto wptr = this->do_auto_reallocate_no_set_length(len_old, n);
		traits_type::assign(wptr, n, ch);
		this->do_set_length(len_old + n);
		return *this;
	}
	basic_cow_string & append(initializer_list<value_type> init){
		return this->append(init.begin(), init.size());
	}
	// N.B. This is a non-standard extension.
	basic_cow_string & append(const_pointer first, const_pointer last){
		ROCKET_ASSERT(first <= last);
		return this->append(first, static_cast<size_type>(last - first));
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr, typename enable_if<is_convertible<inputT, const_pointer>::value == false>::type * = nullptr>
	basic_cow_string & append(inputT first, inputT last){
		if(first == last){
			return *this;
		}
		if(this->do_check_overlap_generic(*first)){
			auto other = basic_cow_string(shallow(*this), this->m_sth.as_allocator());
			using input_category = typename iterator_traits<inputT>::iterator_category;
			other.do_append_nonempty_range_restrict(input_category(), ::std::move(first), ::std::move(last));
			this->assign(::std::move(other));
		} else {
			using input_category = typename iterator_traits<inputT>::iterator_category;
			this->do_append_nonempty_range_restrict(input_category(), ::std::move(first), ::std::move(last));
		}
		return *this;
	}
	// The return type is a non-standard extension.
	basic_cow_string & push_back(value_type ch){
		return this->append(size_type(1), ch);
	}

	basic_cow_string & erase(size_type tpos = 0, size_type tn = npos){
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
	iterator erase(const_iterator ters){
		return this->erase(ters, const_iterator(this, ters.tell() + 1));
	}
	// N.B. This function may throw `std::bad_alloc()`.
	// The return type is a non-standard extension.
	basic_cow_string & pop_back(){
		ROCKET_ASSERT(this->size() > 0);
		return this->erase(this->size() - 1, 1);
	}

	basic_cow_string & assign(const basic_cow_string &other) noexcept {
		this->m_sth.assign(other.m_sth);
		this->m_ptr = other.m_ptr;
		this->m_len = other.m_len;
		return *this;
	}
	basic_cow_string & assign(basic_cow_string &&other) noexcept {
		this->m_sth.assign(::std::move(other.m_sth));
		this->m_ptr = ((exchange))(other.m_ptr, shallow().data());
		this->m_len = ((exchange))(other.m_len, size_type(0));
		return *this;
	}
	basic_cow_string & assign(shallow sh) noexcept {
		this->m_sth.deallocate();
		this->m_ptr = sh.data();
		this->m_len = sh.size();
		return *this;
	}
	basic_cow_string & assign(const basic_cow_string &other, size_type pos, size_type n = npos){
		if(this != &other){
			this->clear();
		}
		this->do_replace_no_bound_check(0, this->size(), other, pos, n);
		return *this;
	}
	basic_cow_string & assign(const_pointer s, size_type n){
		if((n != 0) && (this->do_check_overlap_generic(*s) == false)){
			this->clear();
		}
		this->do_replace_no_bound_check(0, this->size(), s, n);
		return *this;
	}
	basic_cow_string & assign(const_pointer s){
		if(this->do_check_overlap_generic(*s) == false){
			this->clear();
		}
		this->do_replace_no_bound_check(0, this->size(), s);
		return *this;
	}
	basic_cow_string & assign(size_type n, value_type ch){
		this->clear();
		this->do_replace_no_bound_check(0, this->size(), n, ch);
		return *this;
	}
	basic_cow_string & assign(initializer_list<value_type> init){
		this->clear();
		this->do_replace_no_bound_check(0, this->size(), init);
		return *this;
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	basic_cow_string & assign(inputT first, inputT last){
		if((first != last) && (this->do_check_overlap_generic(*first) == false)){
			this->clear();
		}
		this->do_replace_no_bound_check(0, this->size(), ::std::move(first), ::std::move(last));
		return *this;
	}

	basic_cow_string & insert(size_type tpos, shallow sh){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), sh);
		return *this;
	}
	basic_cow_string & insert(size_type tpos, const basic_cow_string & other, size_type pos = 0, size_type n = npos){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), other, pos, n);
		return *this;
	}
	basic_cow_string & insert(size_type tpos, const_pointer s, size_type n){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), s, n);
		return *this;
	}
	basic_cow_string & insert(size_type tpos, const_pointer s){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), s);
		return *this;
	}
	basic_cow_string & insert(size_type tpos, size_type n, value_type ch){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), n, ch);
		return *this;
	}
	// N.B. This is a non-standard extension.
	basic_cow_string & insert(size_type tpos, initializer_list<value_type> init){
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
	iterator insert(const_iterator tins, const basic_cow_string &other, size_type pos = 0, size_type n = npos){
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

	basic_cow_string & replace(size_type tpos, size_type tn, shallow sh){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn), sh);
		return *this;
	}
	basic_cow_string & replace(size_type tpos, size_type tn, const basic_cow_string &other, size_type pos = 0, size_type n = npos){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn), other, pos, n);
		return *this;
	}
	basic_cow_string & replace(size_type tpos, size_type tn, const_pointer s, size_type n){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn), s, n);
		return *this;
	}
	basic_cow_string & replace(size_type tpos, size_type tn, const_pointer s){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn), s);
		return *this;
	}
	basic_cow_string & replace(size_type tpos, size_type tn, size_type n, value_type ch){
		this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn), n, ch);
		return *this;
	}
	basic_cow_string & replace(const_iterator tfirst, const_iterator tlast, shallow sh){
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
		const auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
		this->do_replace_no_bound_check(tpos, tn, sh);
		return *this;
	}
	// N.B. The last two parameters are non-standard extensions.
	basic_cow_string & replace(const_iterator tfirst, const_iterator tlast, const basic_cow_string &other, size_type pos = 0, size_type n = npos){
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
		const auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
		this->do_replace_no_bound_check(tpos, tn, other, pos, n);
		return *this;
	}
	basic_cow_string & replace(const_iterator tfirst, const_iterator tlast, const_pointer s, size_type n){
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
		const auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
		this->do_replace_no_bound_check(tpos, tn, s, n);
		return *this;
	}
	basic_cow_string & replace(const_iterator tfirst, const_iterator tlast, const_pointer s){
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
		const auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
		this->do_replace_no_bound_check(tpos, tn, s);
		return *this;
	}
	basic_cow_string & replace(const_iterator tfirst, const_iterator tlast, size_type n, value_type ch){
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
		const auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
		this->do_replace_no_bound_check(tpos, tn, n, ch);
		return *this;
	}
	basic_cow_string & replace(const_iterator tfirst, const_iterator tlast, initializer_list<value_type> init){
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
		const auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
		this->do_replace_no_bound_check(tpos, tn, init);
		return *this;
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	basic_cow_string & replace(const_iterator tfirst, const_iterator tlast, inputT first, inputT last){
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
		const auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
		this->do_replace_no_bound_check(tpos, tn, ::std::move(first), ::std::move(last));
		return *this;
	}
	// N.B. This is a non-standard extension.
	basic_cow_string & replace(const_iterator tfirst, const_iterator tlast, value_type ch){
		return this->replace(tfirst, tlast, size_type(1), ch);
	}

	// N.B. The last parameter is a non-standard extension.
	size_type copy(pointer s, size_type n, size_type tpos = 0, size_type tn = npos) const {
		const auto rlen = ((min))(this->do_clamp_substr(tpos, tn), n);
		traits_type::copy(s, this->data() + tpos, rlen);
		return rlen;
	}

	void swap(basic_cow_string &other) noexcept {
		((manipulate_allocators))(typename allocator_traits<allocator_type>::propagate_on_container_swap(),
		                          allocator_swap_with(), this->m_sth.as_allocator(), other.m_sth.as_allocator());
		this->m_sth.swap(other.m_sth);
		::std::swap(this->m_ptr, other.m_ptr);
		::std::swap(this->m_len, other.m_len);
	}

	// 24.3.2.7, string operations
	const_pointer data() const noexcept {
		return this->m_ptr;
	}
	// Get a pointer to mutable data. This function may throw `std::bad_alloc()`.
	// N.B. This is a non-standard extension.
	pointer mut_data(){
		return this->do_ensure_unique();
	}
	const_pointer c_str() const noexcept {
		return this->data();
	}
	// N.B. The return type differs from `std::basic_string`.
	const allocator_type & get_allocator() const noexcept {
		return this->m_sth.as_allocator();
	}
	allocator_type & get_allocator() noexcept {
		return this->m_sth.as_allocator();
	}

	// N.B. This is a non-standard extension.
	shallow as_shallow() const noexcept {
		return shallow(*this);
	}

	size_type find(shallow sh, size_type from = 0) const noexcept {
		return this->find(sh.data(), from, sh.size());
	}
	size_type find(const basic_cow_string &other, size_type from = 0) const noexcept {
		return this->find(other.data(), from, other.size());
	}
	// N.B. This is a non-standard extension.
	size_type find(const basic_cow_string &other, size_type from, size_type pos, size_type n = npos) const {
		return this->find(other.data() + pos, from, other.do_clamp_substr(pos, n));
	}
	size_type find(const_pointer s, size_type from, size_type n) const noexcept {
		return this->do_find_forwards_if(from, n, [&](const_pointer ts){ return traits_type::compare(ts, s, n) == 0; });
	}
	size_type find(const_pointer s, size_type from = 0) const noexcept {
		return this->find(s, from, traits_type::length(s));
	}
	size_type find(value_type ch, size_type from = 0) const noexcept {
		// return this->do_find_forwards_if(from, 1, [&](const_pointer ts){ return traits_type::eq(*ts, ch) != false; });
		if(from >= this->size()){
			return npos;
		}
		const auto ptr = traits_type::find(this->data() + from, this->size() - from, ch);
		if(ptr == nullptr){
			return npos;
		}
		auto res = static_cast<size_type>(ptr - this->data());
		ROCKET_ASSERT(res != npos);
		return res;
	}

	size_type rfind(shallow sh, size_type to = npos) const noexcept {
		return this->rfind(sh.data(), to, sh.size());
	}
	// N.B. This is a non-standard extension.
	size_type rfind(const basic_cow_string &other, size_type to = npos) const noexcept {
		return this->rfind(other.data(), to, other.size());
	}
	size_type rfind(const basic_cow_string &other, size_type to, size_type pos, size_type n = npos) const {
		return this->rfind(other.data() + pos, to, other.do_clamp_substr(pos, n));
	}
	size_type rfind(const_pointer s, size_type to, size_type n) const noexcept {
		return this->do_find_backwards_if(to, n, [&](const_pointer ts){ return traits_type::compare(ts, s, n) == 0; });
	}
	size_type rfind(const_pointer s, size_type to = npos) const noexcept {
		return this->rfind(s, to, traits_type::length(s));
	}
	size_type rfind(value_type ch, size_type to = npos) const noexcept {
		// return this->do_find_backwards_if(to, 1, [&](const_pointer ts){ return traits_type::eq(*ts, ch) != false; });
		if(this->size() == 0){
			return npos;
		}
		const auto find_end = ((min))(this->size() - 1, to) + 1;
		auto res = size_type(npos);
		for(;;){
			const auto ptr = traits_type::find(this->data() + (res + 1), find_end - (res + 1), ch);
			if(ptr == nullptr){
				break;
			}
			res = static_cast<size_type>(ptr - this->data());
			ROCKET_ASSERT(res != npos);
			if(res + 1 >= find_end){
				break;
			}
		}
		return res;
	}

	size_type find_first_of(shallow sh, size_type from = 0) const noexcept {
		return this->find_first_of(sh.data(), from, sh.size());
	}
	// N.B. This is a non-standard extension.
	size_type find_first_of(const basic_cow_string &other, size_type from = 0) const noexcept {
		return this->find_first_of(other.data(), from, other.size());
	}
	size_type find_first_of(const basic_cow_string &other, size_type from, size_type pos, size_type n = npos) const {
		return this->find_first_of(other.data() + pos, from, other.do_clamp_substr(pos, n));
	}
	size_type find_first_of(const_pointer s, size_type from, size_type n) const noexcept {
		return this->do_find_forwards_if(from, 1, [&](const_pointer ts){ return traits_type::find(s, n, *ts) != nullptr; });
	}
	size_type find_first_of(const_pointer s, size_type from = 0) const noexcept {
		return this->find_first_of(s, from, traits_type::length(s));
	}
	size_type find_first_of(value_type ch, size_type from = 0) const noexcept {
		return this->find(ch, from);
	}

	size_type find_last_of(shallow sh, size_type to = npos) const noexcept {
		return this->find_last_of(sh.data(), to, sh.size());
	}
	// N.B. This is a non-standard extension.
	size_type find_last_of(const basic_cow_string &other, size_type to = npos) const noexcept {
		return this->find_last_of(other.data(), to, other.size());
	}
	size_type find_last_of(const basic_cow_string &other, size_type to, size_type pos, size_type n = npos) const {
		return this->find_last_of(other.data() + pos, to, other.do_clamp_substr(pos, n));
	}
	size_type find_last_of(const_pointer s, size_type to, size_type n) const noexcept {
		return this->do_find_backwards_if(to, 1, [&](const_pointer ts){ return traits_type::find(s, n, *ts) != nullptr; });
	}
	size_type find_last_of(const_pointer s, size_type to = npos) const noexcept {
		return this->find_last_of(s, to, traits_type::length(s));
	}
	size_type find_last_of(value_type ch, size_type to = npos) const noexcept {
		return this->rfind(ch, to);
	}

	size_type find_first_not_of(shallow sh, size_type from = 0) const noexcept {
		return this->find_first_not_of(sh.data(), from, sh.size());
	}
	// N.B. This is a non-standard extension.
	size_type find_first_not_of(const basic_cow_string &other, size_type from = 0) const noexcept {
		return this->find_first_not_of(other.data(), from, other.size());
	}
	size_type find_first_not_of(const basic_cow_string &other, size_type from, size_type pos, size_type n = npos) const {
		return this->find_first_not_of(other.data() + pos, from, other.do_clamp_substr(pos, n));
	}
	size_type find_first_not_of(const_pointer s, size_type from, size_type n) const noexcept {
		return this->do_find_forwards_if(from, 1, [&](const_pointer ts){ return traits_type::find(s, n, *ts) == nullptr; });
	}
	size_type find_first_not_of(const_pointer s, size_type from = 0) const noexcept {
		return this->find_first_not_of(s, from, traits_type::length(s));
	}
	size_type find_first_not_of(value_type ch, size_type from = 0) const noexcept {
		return this->do_find_forwards_if(from, 1, [&](const_pointer ts){ return traits_type::eq(*ts, ch) == false; });
	}

	size_type find_last_not_of(shallow sh, size_type to = npos) const noexcept {
		return this->find_last_not_of(sh.data(), to, sh.size());
	}
	// N.B. This is a non-standard extension.
	size_type find_last_not_of(const basic_cow_string &other, size_type to = npos) const noexcept {
		return this->find_last_not_of(other.data(), to, other.size());
	}
	size_type find_last_not_of(const basic_cow_string &other, size_type to, size_type pos, size_type n = npos) const {
		return this->find_last_not_of(other.data() + pos, to, other.do_clamp_substr(pos, n));
	}
	size_type find_last_not_of(const_pointer s, size_type to, size_type n) const noexcept {
		return this->do_find_backwards_if(to, 1, [&](const_pointer ts){ return traits_type::find(s, n, *ts) == nullptr; });
	}
	size_type find_last_not_of(const_pointer s, size_type to = npos) const noexcept {
		return this->find_last_not_of(s, to, traits_type::length(s));
	}
	size_type find_last_not_of(value_type ch, size_type to = npos) const noexcept {
		return this->do_find_backwards_if(to, 1, [&](const_pointer ts){ return traits_type::eq(*ts, ch) == false; });
	}

	// N.B. This is a non-standard extension.
	template<typename predT>
	size_type find_first_if(predT pred, size_type from = 0) const {
		return this->do_find_forwards_if(from, 1, [&](const char *p){ return pred(*p); });
	}
	// N.B. This is a non-standard extension.
	template<typename predT>
	size_type find_last_if(predT pred, size_type to = npos) const {
		return this->do_find_backwards_if(to, 1, [&](const char *p){ return pred(*p); });
	}

	basic_cow_string substr(size_type pos = 0, size_type n = npos) const {
		if((pos == 0) && (n >= this->size())){
			// Utilize reference counting.
			return basic_cow_string(*this, this->m_sth.as_allocator());
		}
		return basic_cow_string(*this, pos, n, this->m_sth.as_allocator());
	}

	int compare(shallow sh) const noexcept {
		return this->compare(sh.data(), sh.size());
	}
	int compare(const basic_cow_string &other) const noexcept {
		return this->compare(other.data(), other.size());
	}
	// N.B. This is a non-standard extension.
	int compare(const basic_cow_string &other, size_type pos, size_type n = npos) const {
		return this->compare(other.data() + pos, other.do_clamp_substr(pos, n));
	}
	// N.B. This is a non-standard extension.
	int compare(const_pointer s, size_type n) const noexcept {
		return comparator::relation(this->data(), this->size(), s, n);
	}
	int compare(const_pointer s) const noexcept {
		return this->compare(s, traits_type::length(s));
	}
	int compare(size_type tpos, size_type tn, shallow sh) const {
		return this->compare(tpos, tn, sh.data(), sh.size());
	}
	// N.B. The last two parameters are non-standard extensions.
	int compare(size_type tpos, size_type tn, const basic_cow_string &other, size_type pos = 0, size_type n = npos) const {
		return this->compare(tpos, tn, other.data() + pos, other.do_clamp_substr(pos, n));
	}
	int compare(size_type tpos, size_type tn, const_pointer s, size_type n) const {
		return comparator::relation(this->data() + tpos, this->do_clamp_substr(tpos, tn), s, n);
	}
	int compare(size_type tpos, size_type tn, const_pointer s) const {
		return this->compare(tpos, tn, s, traits_type::length(s));
	}
	// N.B. This is a non-standard extension.
	int compare(size_type tpos, shallow sh) const {
		return this->compare(tpos, npos, sh);
	}
	// N.B. This is a non-standard extension.
	int compare(size_type tpos, const basic_cow_string &other, size_type pos = 0, size_type n = npos) const {
		return this->compare(tpos, npos, other, pos, n);
	}
	// N.B. This is a non-standard extension.
	int compare(size_type tpos, const_pointer s, size_type n) const {
		return this->compare(tpos, npos, s, n);
	}
	// N.B. This is a non-standard extension.
	int compare(size_type tpos, const_pointer s) const {
		return this->compare(tpos, npos, s);
	}

	// N.B. These are extensions but might be standardized in C++20.
	bool starts_with(shallow sh) const noexcept {
		return this->starts_with(sh.data(), sh.size());
	}
	bool starts_with(const basic_cow_string &other) const noexcept {
		return this->starts_with(other.data(), other.size());
	}
	// N.B. This is a non-standard extension.
	bool starts_with(const basic_cow_string &other, size_type pos, size_type n = npos) const {
		return this->starts_with(other.data() + pos, other.do_clamp_substr(pos, n));
	}
	bool starts_with(const_pointer s, size_type n) const noexcept {
		return (n <= this->size()) && (traits_type::compare(this->data(), s, n) == 0);
	}
	bool starts_with(const_pointer s) const noexcept {
		return this->starts_with(s, traits_type::length(s));
	}
	bool starts_with(value_type ch) const noexcept {
		return (1 <= this->size()) && traits_type::eq(this->front(), ch);
	}

	bool ends_with(shallow sh) const noexcept {
		return this->ends_with(sh.data(), sh.size());
	}
	bool ends_with(const basic_cow_string &other) const noexcept {
		return this->ends_with(other.data(), other.size());
	}
	// N.B. This is a non-standard extension.
	bool ends_with(const basic_cow_string &other, size_type pos, size_type n = npos) const {
		return this->ends_with(other.data() + pos, other.do_clamp_substr(pos, n));
	}
	bool ends_with(const_pointer s, size_type n) const noexcept {
		return (n <= this->size()) && (traits_type::compare(this->data() + this->size() - n, s, n) == 0);
	}
	bool ends_with(const_pointer s) const noexcept {
		return this->ends_with(s, traits_type::length(s));
	}
	bool ends_with(value_type ch) const noexcept {
		return (1 <= this->size()) && traits_type::eq(this->back(), ch);
	}
};

template<typename charT, typename traitsT, typename allocatorT>
struct basic_cow_string<charT, traitsT, allocatorT>::equal_to {
	using result_type = bool;
	using first_argument_type = basic_cow_string;
	using second_argument_type = basic_cow_string;

	result_type operator()(const first_argument_type &lhs, const second_argument_type &rhs) const noexcept {
		if(lhs.data() == rhs.data()){
			return true;
		}
		if(lhs.size() != rhs.size()){
			return false;
		}
		return lhs.compare(rhs) == 0;
	}
};

template<typename charT, typename traitsT, typename allocatorT>
struct basic_cow_string<charT, traitsT, allocatorT>::less {
	using result_type = bool;
	using first_argument_type = basic_cow_string;
	using second_argument_type = basic_cow_string;

	result_type operator()(const first_argument_type &lhs, const second_argument_type &rhs) const noexcept {
		return lhs.compare(rhs) < 0;
	}
};

template<typename charT, typename traitsT, typename allocatorT>
struct basic_cow_string<charT, traitsT, allocatorT>::hash {
	using result_type = size_t;
	using argument_type = basic_cow_string;

	result_type operator()(const argument_type &str) const noexcept {
		// This implements the 32-bit FNV-1a hash algorithm.
		uint_fast32_t reg = 2166136261u;
		for(auto p = str.begin(); p != str.end(); ++p){
			reg ^= static_cast<uint_fast32_t>(traits_type::to_int_type(*p));
			reg *= 16777619u;
		}
		return static_cast<result_type>(reg);
	}
};

extern template class basic_cow_string<char>;
extern template class basic_cow_string<wchar_t>;
extern template class basic_cow_string<char16_t>;
extern template class basic_cow_string<char32_t>;

using cow_string     = basic_cow_string<char>;
using cow_wstring    = basic_cow_string<wchar_t>;
using cow_u16string  = basic_cow_string<char16_t>;
using cow_u32string  = basic_cow_string<char32_t>;

template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(const basic_cow_string<charT, traitsT, allocatorT> &lhs, typename basic_cow_string<charT, traitsT, allocatorT>::shallow rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	res.reserve(lhs.size() + rhs.size());
	res.append(lhs.data(), lhs.size());
	res.append(rhs.data(), rhs.size());
	return res;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const charT *rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	const auto rhs_len = traitsT::length(rhs);
	res.reserve(lhs.size() + rhs_len);
	res.append(lhs.data(), lhs.size());
	res.append(rhs, rhs_len);
	return res;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(const basic_cow_string<charT, traitsT, allocatorT> &lhs, charT rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	res.reserve(lhs.size() + 1);
	res.append(lhs.data(), lhs.size());
	res.append(typename string_type::size_type(1), rhs);
	return res;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(basic_cow_string<charT, traitsT, allocatorT> &&lhs, typename basic_cow_string<charT, traitsT, allocatorT>::shallow rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	res.assign(::std::move(lhs));
	res.append(rhs.data(), rhs.size());
	return res;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(basic_cow_string<charT, traitsT, allocatorT> &&lhs, const charT *rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	const auto rhs_len = traitsT::length(rhs);
	res.assign(::std::move(lhs));
	res.append(rhs, rhs_len);
	return res;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(basic_cow_string<charT, traitsT, allocatorT> &&lhs, charT rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	res.assign(::std::move(lhs));
	res.append(typename string_type::size_type(1), rhs);
	return res;
}

template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(typename basic_cow_string<charT, traitsT, allocatorT>::shallow lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	res.reserve(lhs.size() + rhs.size());
	res.append(lhs.data(), lhs.size());
	res.append(rhs.data(), rhs.size());
	return res;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(const charT *lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	const auto lhs_len = traitsT::length(lhs);
	res.reserve(lhs_len + rhs.size());
	res.append(lhs, lhs_len);
	res.append(rhs.data(), rhs.size());
	return res;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(charT lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	res.reserve(1 + rhs.size());
	res.append(typename string_type::size_type(1), lhs);
	res.append(rhs.data(), rhs.size());
	return res;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(typename basic_cow_string<charT, traitsT, allocatorT>::shallow lhs, basic_cow_string<charT, traitsT, allocatorT> &&rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	res.assign(::std::move(rhs));
	res.insert(typename string_type::size_type(0), lhs.data(), lhs.size());
	return res;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(const charT *lhs, basic_cow_string<charT, traitsT, allocatorT> &&rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	const auto lhs_len = traitsT::length(lhs);
	res.assign(::std::move(rhs));
	res.insert(typename string_type::size_type(0), lhs, lhs_len);
	return res;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(charT lhs, basic_cow_string<charT, traitsT, allocatorT> &&rhs){
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	string_type res;
	res.assign(::std::move(rhs));
	res.insert(typename string_type::size_type(0), typename string_type::size_type(1), lhs);
	return res;
}

template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs){
	return ((operator+))(lhs, typename basic_cow_string<charT, traitsT, allocatorT>::shallow(rhs));
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_cow_string<charT, traitsT, allocatorT> operator+(basic_cow_string<charT, traitsT, allocatorT> &&lhs, basic_cow_string<charT, traitsT, allocatorT> &&rhs){
	return (lhs.size() + rhs.size() <= lhs.capacity()) ? ((operator+))(::std::move(lhs), rhs) : ((operator+))(lhs, ::std::move(rhs));
}

template<typename charT, typename traitsT, typename allocatorT>
inline bool operator==(const basic_cow_string<charT, traitsT, allocatorT> &lhs, typename basic_cow_string<charT, traitsT, allocatorT>::shallow rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::inequality(lhs.data(), lhs.size(), rhs.data(), rhs.size()) == 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator!=(const basic_cow_string<charT, traitsT, allocatorT> &lhs, typename basic_cow_string<charT, traitsT, allocatorT>::shallow rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::inequality(lhs.data(), lhs.size(), rhs.data(), rhs.size()) != 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator<(const basic_cow_string<charT, traitsT, allocatorT> &lhs, typename basic_cow_string<charT, traitsT, allocatorT>::shallow rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) < 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator>(const basic_cow_string<charT, traitsT, allocatorT> &lhs, typename basic_cow_string<charT, traitsT, allocatorT>::shallow rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) > 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator<=(const basic_cow_string<charT, traitsT, allocatorT> &lhs, typename basic_cow_string<charT, traitsT, allocatorT>::shallow rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) <= 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator>=(const basic_cow_string<charT, traitsT, allocatorT> &lhs, typename basic_cow_string<charT, traitsT, allocatorT>::shallow rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) >= 0;
}

template<typename charT, typename traitsT, typename allocatorT>
inline bool operator==(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const charT *rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::inequality(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) == 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator!=(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const charT *rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::inequality(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) != 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator<(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const charT *rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) < 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator>(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const charT *rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) > 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator<=(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const charT *rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) <= 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator>=(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const charT *rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) >= 0;
}

template<typename charT, typename traitsT, typename allocatorT>
inline bool operator==(typename basic_cow_string<charT, traitsT, allocatorT>::shallow lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::inequality(lhs.data(), lhs.size(), rhs.data(), rhs.size()) == 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator!=(typename basic_cow_string<charT, traitsT, allocatorT>::shallow lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::inequality(lhs.data(), lhs.size(), rhs.data(), rhs.size()) != 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator<(typename basic_cow_string<charT, traitsT, allocatorT>::shallow lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) < 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator>(typename basic_cow_string<charT, traitsT, allocatorT>::shallow lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) > 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator<=(typename basic_cow_string<charT, traitsT, allocatorT>::shallow lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) <= 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator>=(typename basic_cow_string<charT, traitsT, allocatorT>::shallow lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) >= 0;
}

template<typename charT, typename traitsT, typename allocatorT>
inline bool operator==(const charT *lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::inequality(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) == 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator!=(const charT *lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::inequality(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) != 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator<(const charT *lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) < 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator>(const charT *lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) > 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator<=(const charT *lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) <= 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator>=(const charT *lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) >= 0;
}

template<typename charT, typename traitsT, typename allocatorT>
inline bool operator==(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::inequality(lhs.data(), lhs.size(), rhs.data(), rhs.size()) == 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator!=(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::inequality(lhs.data(), lhs.size(), rhs.data(), rhs.size()) != 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator<(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) < 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator>(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) > 0;
}
template<typename charT, typename traitsT, typename allocatorT>
inline bool operator<=(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) <= 0;
}

template<typename charT, typename traitsT, typename allocatorT>
inline bool operator>=(const basic_cow_string<charT, traitsT, allocatorT> &lhs, const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	using string_type = basic_cow_string<charT, traitsT, allocatorT>;
	return string_type::comparator::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) >= 0;
}

template<typename charT, typename traitsT, typename allocatorT>
inline void swap(basic_cow_string<charT, traitsT, allocatorT> &lhs, basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	lhs.swap(rhs);
}

template<typename charT, typename traitsT, typename allocatorT>
inline typename basic_cow_string<charT, traitsT, allocatorT>::const_iterator begin(const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	return rhs.begin();
}
template<typename charT, typename traitsT, typename allocatorT>
inline typename basic_cow_string<charT, traitsT, allocatorT>::const_iterator end(const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	return rhs.end();
}
template<typename charT, typename traitsT, typename allocatorT>
inline typename basic_cow_string<charT, traitsT, allocatorT>::const_reverse_iterator rbegin(const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	return rhs.rbegin();
}
template<typename charT, typename traitsT, typename allocatorT>
inline typename basic_cow_string<charT, traitsT, allocatorT>::const_reverse_iterator rend(const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	return rhs.rend();
}

template<typename charT, typename traitsT, typename allocatorT>
inline typename basic_cow_string<charT, traitsT, allocatorT>::const_iterator cbegin(const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	return rhs.cbegin();
}
template<typename charT, typename traitsT, typename allocatorT>
inline typename basic_cow_string<charT, traitsT, allocatorT>::const_iterator cend(const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	return rhs.cend();
}
template<typename charT, typename traitsT, typename allocatorT>
inline typename basic_cow_string<charT, traitsT, allocatorT>::const_reverse_iterator crbegin(const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	return rhs.crbegin();
}
template<typename charT, typename traitsT, typename allocatorT>
inline typename basic_cow_string<charT, traitsT, allocatorT>::const_reverse_iterator crend(const basic_cow_string<charT, traitsT, allocatorT> &rhs) noexcept {
	return rhs.crend();
}

template<typename charT, typename traitsT, typename allocatorT>
basic_istream<charT, traitsT> & operator>>(basic_istream<charT, traitsT> &is, basic_cow_string<charT, traitsT, allocatorT> &str){
	// Initiate this FormattedInputFunction.
	const typename basic_istream<charT, traitsT>::sentry sentry(is, false);
	if(!sentry){
		return is;
	}
	try {
		using traits_type = typename basic_istream<charT, traitsT>::traits_type;
		// Clear the contents of `str`.
		str.erase();
		// Determine the maximum number of characters to extract.
		const auto width = is.width();
		auto len_max = width;
		if(len_max <= 0){
			len_max = static_cast<streamsize>(str.max_size());
			ROCKET_ASSERT(len_max > 0);
		}
		// This locale object is used by `std::isspace()`.
		const auto loc = is.getloc();
		// Extract characters and append them to `str`.
		auto ich = is.rdbuf()->sgetc();
		for(;;){
			if(traits_type::eq_int_type(ich, traits_type::eof())){
				is.setstate(ios_base::eofbit);
				break;
			}
			if(static_cast<streamsize>(str.size()) >= len_max){
				break;
			}
			const auto ch = traits_type::to_char_type(ich);
			if(::std::isspace<charT>(ch, loc)){
				break;
			}
			str.push_back(ch);
			ich = is.rdbuf()->snextc();
		}
		// If this function extracts no characters, set `std::ios_base::failbit`.
		if(str.empty()){
			is.setstate(ios_base::failbit);
		}
		is.width(0);
	} catch(...){
		details_cow_string::handle_io_exception(is);
	}
	return is;
}

extern template ::std::istream  & operator>>(::std::istream  &is, cow_string  &str);
extern template ::std::wistream & operator>>(::std::wistream &is, cow_wstring &str);

template<typename charT, typename traitsT, typename allocatorT>
basic_ostream<charT, traitsT> & operator<<(basic_ostream<charT, traitsT> &os, const basic_cow_string<charT, traitsT, allocatorT> &str){
	// Initiate this FormattedOutputFunction.
	const typename basic_ostream<charT, traitsT>::sentry sentry(os);
	if(!sentry){
		return os;
	}
	try {
		using traits_type = typename basic_ostream<charT, traitsT>::traits_type;
		// Determine the minimum number of characters to insert.
		const auto width = os.width();
		const auto len = static_cast<streamsize>(str.size());
		ROCKET_ASSERT(len >= 0);
		auto len_rem = ((max))(width, len);
		// Insert characters into `os`, which are from `str` if `offset` is within `[0, len)` and are copied from `os.fill()` otherwise.
		auto offset = len - len_rem;
		if((os.flags() & ios_base::adjustfield) == ios_base::left){
			offset = 0;
		}
		for(;;){
			if(len_rem <= 0){
				break;
			}
			streamsize written;
			if((0 <= offset) && (offset < len)){
				written = os.rdbuf()->sputn(str.data() + offset, len - offset);
				if(written == 0){
					os.setstate(ios_base::failbit);
					break;
				}
			} else {
				if(traits_type::eq_int_type(os.rdbuf()->sputc(os.fill()), traits_type::eof())){
					os.setstate(ios_base::failbit);
					break;
				}
				written = 1;
			}
			len_rem -= written;
			offset += written;
		}
		os.width(0);
	} catch(...){
		details_cow_string::handle_io_exception(os);
	}
	return os;
}

extern template ::std::ostream  & operator<<(::std::ostream  &os, const cow_string  &str);
extern template ::std::wostream & operator<<(::std::wostream &os, const cow_wstring &str);

template<typename charT, typename traitsT, typename allocatorT>
basic_istream<charT, traitsT> & getline(basic_istream<charT, traitsT> &is, basic_cow_string<charT, traitsT, allocatorT> &str, charT delim){
	// Initiate this UnformattedInputFunction.
	const typename basic_istream<charT, traitsT>::sentry sentry(is, true);
	if(!sentry){
		return is;
	}
	try {
		using traits_type = typename basic_istream<charT, traitsT>::traits_type;
		// Clear the contents of `str`.
		str.erase();
		// Extract characters and append them to `str`.
		auto ich = is.rdbuf()->sgetc();
		bool eol = false;
		for(;;){
			if(traits_type::eq_int_type(ich, traits_type::eof())){
				is.setstate(ios_base::eofbit);
				break;
			}
			const auto ch = traits_type::to_char_type(ich);
			if(traits_type::eq(ch, delim)){
				// Discard the delimiter.
				ich = is.rdbuf()->snextc();
				eol = true;
				break;
			}
			if(str.size() >= str.max_size()){
				is.setstate(ios_base::failbit);
				break;
			}
			str.push_back(ch);
			ich = is.rdbuf()->snextc();
		}
		// If this function extracts no characters, set `std::ios_base::failbit`.
		if(!eol && str.empty()){
			is.setstate(ios_base::failbit);
		}
	} catch(...){
		details_cow_string::handle_io_exception(is);
	}
	return is;
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_istream<charT, traitsT> & getline(basic_istream<charT, traitsT> &&is, basic_cow_string<charT, traitsT, allocatorT> &str, charT delim){
	return ((getline))(is, str, delim);
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_istream<charT, traitsT> & getline(basic_istream<charT, traitsT> &is, basic_cow_string<charT, traitsT, allocatorT> &str){
	return ((getline))(is, str, is.widen('\n'));
}
template<typename charT, typename traitsT, typename allocatorT>
inline basic_istream<charT, traitsT> & getline(basic_istream<charT, traitsT> &&is, basic_cow_string<charT, traitsT, allocatorT> &str){
	return ((getline))(is, str);
}

extern template ::std::istream  & getline(::std::istream  &is, cow_string  &str, char    delim);
extern template ::std::wistream & getline(::std::wistream &is, cow_wstring &str, wchar_t delim);
extern template ::std::istream  & getline(::std::istream  &is, cow_string  &str);
extern template ::std::wistream & getline(::std::wistream &is, cow_wstring &str);

}

#endif
