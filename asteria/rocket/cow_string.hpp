// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRING_HPP_
#define ROCKET_COW_STRING_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include "throw.hpp"
#include "char_traits.hpp"
#include "reference_counter.hpp"
#include "xallocator.hpp"

namespace rocket {

template<typename charT,
         typename traitsT = char_traits<charT>>
class basic_shallow_string;

template<typename charT,
         typename traitsT = char_traits<charT>,
         typename allocT = allocator<charT>>
class basic_cow_string;

#include "details/cow_string.ipp"

/* Differences from `std::basic_string`:
 * 1. All functions guarantee only basic exception safety rather than strong exception safety, hence
 *    are more efficient.
 * 2. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always
 *    return `const_reference`s.
 * 3. The copy constructor and copy assignment operator will not throw exceptions.
 * 4. The constructor taking a sole const pointer is made `explicit`.
 * 5. The assignment operator taking a character and the one taking a const pointer are not provided.
 * 6. It is possible to create strings holding non-owning references of null-terminated character
 *    arrays allocated externally.
 * 7. `data()` returns a null pointer if the string is empty.
 * 8. `erase()` and `substr()` cannot be called without arguments.
**/

template<typename charT, typename traitsT>
class basic_tinybuf;

template<typename charT, typename traitsT>
class basic_tinyfmt;

template<typename charT, typename traitsT>
class basic_shallow_string
  {
  private:
    const charT* m_ptr;
    size_t m_len;

  public:
    explicit constexpr
    basic_shallow_string(const charT* ptr) noexcept
      : m_ptr(ptr), m_len(traitsT::length(ptr))
      { }

    constexpr
    basic_shallow_string(const charT* ptr, size_t len) noexcept
      : m_ptr(ptr), m_len((ROCKET_ASSERT(traitsT::eq(ptr[len], charT())), len))
      { }

    template<typename allocT>
    explicit
    basic_shallow_string(const basic_cow_string<charT, traitsT, allocT>& str) noexcept
      : m_ptr(str.c_str()), m_len(str.length())
      { }

  public:
    constexpr const charT*
    c_str() const noexcept
      { return this->m_ptr;  }

    constexpr size_t
    length() const noexcept
      { return this->m_len;  }

    constexpr const charT*
    begin() const noexcept
      { return this->m_ptr;  }

    constexpr const charT*
    end() const noexcept
      { return this->m_ptr + this->m_len;  }

    constexpr const charT*
    data() const noexcept
      { return this->m_ptr;  }

    constexpr size_t
    size() const noexcept
      { return this->m_len;  }

    constexpr charT
    operator[](size_t pos) const noexcept
      { return ROCKET_ASSERT(pos <= this->m_len), this->m_ptr[pos];  }
  };

template
class basic_shallow_string<char>;

template
class basic_shallow_string<wchar_t>;

template
class basic_shallow_string<char16_t>;

template
class basic_shallow_string<char32_t>;

template<typename charT>
constexpr basic_shallow_string<charT, char_traits<charT>>
sref(const charT* ptr) noexcept
  { return basic_shallow_string<charT, char_traits<charT>>(ptr);  }

template<typename charT>
constexpr basic_shallow_string<charT, char_traits<charT>>
sref(const charT* ptr, ::std::size_t len) noexcept
  { return basic_shallow_string<charT, char_traits<charT>>(ptr, len);  }

template<typename charT, typename traitsT, typename allocT>
constexpr basic_shallow_string<charT, traitsT>
sref(const basic_cow_string<charT, traitsT, allocT>& str) noexcept
  { return basic_shallow_string<charT, traitsT>(str);  }

template<typename charT, typename traitsT>
inline basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, basic_shallow_string<charT, traitsT> sh)
  { return fmt.putn(sh.c_str(), sh.length());  }

template<typename charT, typename traitsT, typename allocT>
class basic_cow_string
  {
    static_assert(!is_array<charT>::value, "invalid character type");
    static_assert(is_trivial<charT>::value, "characters must be trivial");
    static_assert(is_same<typename allocT::value_type, charT>::value, "inappropriate allocator type");

  public:
    // types
    using value_type      = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocT;

    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using difference_type  = typename allocator_traits<allocator_type>::difference_type;
    using const_reference  = const value_type&;
    using reference        = value_type&;

    using const_iterator      = details_cow_string::string_iterator<basic_cow_string, const value_type>;
    using iterator            = details_cow_string::string_iterator<basic_cow_string, value_type>;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

    using shallow_type = basic_shallow_string<charT, traitsT>;
    struct hash;

    static constexpr size_type npos = size_type(-1);

  private:
    using storage_handle = details_cow_string::storage_handle<allocator_type, traits_type>;

  private:
    storage_handle m_sth;
    const value_type* m_ptr;
    size_type m_len;

  public:
    // 24.3.2.2, construct/copy/destroy
    constexpr
    basic_cow_string(shallow_type sh, const allocator_type& alloc = allocator_type()) noexcept
      : m_sth(alloc),
        m_ptr(sh.c_str()), m_len(sh.length())
      { }

    explicit constexpr
    basic_cow_string(const allocator_type& alloc) noexcept
      : m_sth(alloc),
        m_ptr(storage_handle::null_char), m_len()
      { }

    basic_cow_string(const basic_cow_string& other) noexcept
      : m_sth(allocator_traits<allocator_type>::select_on_container_copy_construction(
                                                    other.m_sth.as_allocator())),
        m_ptr(other.m_ptr), m_len(other.m_len)
      { this->m_sth.share_with(other.m_sth);  }

    basic_cow_string(const basic_cow_string& other, const allocator_type& alloc) noexcept
      : m_sth(alloc),
        m_ptr(other.m_ptr), m_len(other.m_len)
      { this->m_sth.share_with(other.m_sth);  }

    basic_cow_string(basic_cow_string&& other) noexcept
      : m_sth(::std::move(other.m_sth.as_allocator())),
        m_ptr(::std::exchange(other.m_ptr, storage_handle::null_char)),
        m_len(::std::exchange(other.m_len, size_type()))
      { this->m_sth.exchange_with(other.m_sth);  }

    basic_cow_string(basic_cow_string&& other, const allocator_type& alloc) noexcept
      : m_sth(alloc),
        m_ptr(::std::exchange(other.m_ptr, storage_handle::null_char)),
        m_len(::std::exchange(other.m_len, size_type()))
      { this->m_sth.exchange_with(other.m_sth);  }

    constexpr
    basic_cow_string() noexcept(is_nothrow_constructible<allocator_type>::value)
      : basic_cow_string(allocator_type())
      { }

    basic_cow_string(const basic_cow_string& other, size_type pos, size_type n = npos,
                     const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(other, pos, n);  }

    basic_cow_string(const value_type* s, size_type n, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(s, n);  }

    explicit
    basic_cow_string(const value_type* s, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(s);  }

    basic_cow_string(size_type n, value_type ch, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(n, ch);  }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string(inputT first, inputT last, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(::std::move(first), ::std::move(last));  }

    basic_cow_string(initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(init.begin(), init.end());  }

    basic_cow_string&
    operator=(const basic_cow_string& other) noexcept
      { noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.share_with(other.m_sth);
        m_ptr = other.m_ptr;
        m_len = other.m_len;
        return *this;  }

    basic_cow_string&
    operator=(basic_cow_string&& other) noexcept
      { noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        m_ptr = ::std::exchange(other.m_ptr, storage_handle::null_char);
        m_len = ::std::exchange(other.m_len, size_type());
        return *this;  }

    basic_cow_string&
    operator=(shallow_type sh) noexcept
      { this->m_ptr = sh.c_str();
        this->m_len = sh.length();
        return *this;  }

    basic_cow_string&
    operator=(initializer_list<value_type> init)
      { return this->assign(init.begin(), init.end());  }

    basic_cow_string&
    swap(basic_cow_string& other) noexcept
      { noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        noadl::xswap(this->m_ptr, other.m_ptr);
        noadl::xswap(this->m_len, other.m_len);
        return *this;  }

  private:
    basic_cow_string&
    do_deallocate() noexcept
      {
        this->m_sth.deallocate();
        this->m_ptr = storage_handle::null_char;
        this->m_len = 0;
        return *this;
      }

    [[noreturn]] ROCKET_NEVER_INLINE void
    do_throw_subscript_out_of_range(size_type pos, const char* rel) const
      {
        noadl::sprintf_and_throw<out_of_range>(
              "cow_string: subscript out of range (`%llu` %s `%llu`)",
              static_cast<unsigned long long>(pos), rel,
              static_cast<unsigned long long>(this->size()));
      }

    // This function works the same way as `substr()`.
    // Ensure `tpos` is in `[0, size()]` and return `min(tn, size() - tpos)`.
    size_type
    do_clamp_substr(size_type tpos, size_type tn) const
      {
        size_type len = this->size();
        if(tpos > len)
          this->do_throw_subscript_out_of_range(tpos, ">");
        return noadl::min(tn, len - tpos);
      }

    // This function is used to implement `replace()` after the replacement has been appended.
    // `tpos` and `tlen` are arguments to `replace()`. `kpos` is the old length prior to `append()`.
    value_type*
    do_swizzle_unchecked(size_type tpos, size_type tlen, size_type kpos)
      {
        // Swap the intervals [`tpos+tlen`,`kpos`) and [`kpos`,`len`).
        auto ptr = this->mut_data();
        size_type len = this->size();
        noadl::rotate(ptr, tpos + tlen, kpos, len);

        // Erase the interval [`tpos`,`tpos+tlen`).
        // Note the null terminator has to be copied as well.
        if(tlen != 0) {
          traits_type::move(ptr + tpos, ptr + tpos + tlen, len + 1 - tpos - tlen);
          this->m_len -= tlen;
        }
        return ptr + tpos;
      }

    // This function is used to implement `replace()` if the replacement cannot alias `*this`.
    // `tpos` and `tlen` are arguments to `replace()`. `n` is the length to reserve.
    value_type*
    do_reserve_divide_unchecked(size_type tpos, size_type tlen, size_type n)
      {
        if(tlen == n)
          return this->mut_data() + tpos;

        // If there is enough space, push [`tpos+tlen`,`size`] by `n-tlen` characters.
        // Note the null terminator has to be copied as well.
        auto ptr = this->m_sth.mut_data_opt();
        size_type cap = this->capacity();
        size_type len = this->size();
        size_type slen = len - tpos - tlen;
        if(ROCKET_EXPECT(ptr && (n <= cap - len + tlen))) {
          traits_type::move(ptr + tpos + n, ptr + tpos + tlen, slen + 1);
          this->m_ptr = ptr;  // note the storage might be unowned
          this->m_len += n - tlen;
          return ptr + tpos;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        ptr = sth.reallocate_more(this->data(), tpos, (slen + n) | n | cap / 2);

        // Copy [`tpos+tlen`,`size`] into the new storage.
        // Note the null terminator has to be copied as well.
        traits_type::copy(ptr + tpos + n, this->data() + tpos + tlen, slen + 1);

        // Set the new storage up.
        this->m_sth.exchange_with(sth);
        this->m_ptr = ptr;
        this->m_len += n - tlen;
        return ptr + tpos;
      }

    // These are generic implementations for `{{,r}find,find_{first,last}{,_not}_of}()` functions.
    template<typename predT>
    size_type
    do_find_forwards_if(size_type from, size_type n, predT&& pred) const
      {
        // If the string is too short, no match could exist.
        if(this->size() < n)
          return npos;

        // If the search starts at a position where no enough characters may follow, fail.
        size_type rlen = this->size() - n;
        if(from > rlen)
          return npos;

        // Search the interval [from,rlen] in normal order.
        size_type cur = from;
        while(!pred(this->data() + cur))
          if(cur++ == rlen)
            return npos;  // not found

        // A character has been found so don't return `npos`.
        ROCKET_ASSERT(cur != npos);
        return cur;
      }

    template<typename predT>
    size_type
    do_find_backwards_if(size_type to, size_type n, predT&& pred) const
      {
        // If the string is too short, no match could exist.
        if(this->size() < n)
          return npos;

        // Unlike `do_find_forwards_if()`, there is always a range to search.
        size_type rlen = this->size() - n;
        if(rlen > to)
          rlen = to;

        // Search the interval [0,rlen] in reverse order.
        size_type cur = rlen;
        while(!pred(this->data() + cur))
          if(cur-- == 0)
            return npos;  // not found;

        // A character has been found so don't return `npos`.
        ROCKET_ASSERT(cur != npos);
        return cur;
      }

  public:
    // 24.3.2.3, iterators
    const_iterator
    begin() const noexcept
      { return const_iterator(this->data(), 0, this->size());  }

    const_iterator
    end() const noexcept
      { return const_iterator(this->data(), this->size(), this->size());  }

    const_reverse_iterator
    rbegin() const noexcept
      { return const_reverse_iterator(this->end());  }

    const_reverse_iterator
    rend() const noexcept
      { return const_reverse_iterator(this->begin());  }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator
    mut_begin()
      { return iterator(this->mut_data(), 0, this->size());  }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator
    mut_end()
      { return iterator(this->mut_data(), this->size(), this->size());  }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    reverse_iterator
    mut_rbegin()
      { return reverse_iterator(this->mut_end());  }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    reverse_iterator
    mut_rend()
      { return reverse_iterator(this->mut_begin());  }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    ::std::move_iterator<iterator>
    move_begin()
      { return ::std::move_iterator<iterator>(this->mut_begin());  }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    ::std::move_iterator<iterator>
    move_end()
      { return ::std::move_iterator<iterator>(this->mut_end());  }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    ::std::move_iterator<reverse_iterator>
    move_rbegin()
      { return ::std::move_iterator<reverse_iterator>(this->mut_rbegin());  }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    ::std::move_iterator<reverse_iterator>
    move_rend()
      { return ::std::move_iterator<reverse_iterator>(this->mut_rend());  }

    // 24.3.2.4, capacity
    constexpr bool
    empty() const noexcept
      { return this->m_len == 0;  }

    constexpr size_type
    size() const noexcept
      { return this->m_len;  }

    constexpr size_type
    length() const noexcept
      { return this->m_len;  }

    // N.B. This is a non-standard extension.
    constexpr difference_type
    ssize() const noexcept
      { return static_cast<difference_type>(this->size());  }

    constexpr size_type
    max_size() const noexcept
      { return this->m_sth.max_size();  }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    resize(size_type n, value_type ch = value_type())
      {
        if(this->size() < n)
          return this->append(n - this->size(), ch);
        else
          return this->pop_back(this->size() - n);
      }

    constexpr size_type
    capacity() const noexcept
      { return this->m_sth.capacity();  }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    reserve(size_type res_arg)
      {
        // Note zero is a special request to reduce capacity.
        if(res_arg == 0)
          return this->shrink_to_fit();

        // Calculate the minimum capacity to reserve. This must include all existent characters.
        // Don't reallocate if the storage is unique and there is enough room.
        size_type rcap = this->m_sth.round_up_capacity(noadl::max(this->size(), res_arg));
        if(this->unique() && (this->capacity() >= rcap))
          return *this;

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        auto ptr = sth.reallocate_more(this->data(), this->size(), rcap - this->size());

        // Set the new storage up. The length is left intact.
        this->m_sth.exchange_with(sth);
        this->m_ptr = ptr;
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    shrink_to_fit()
      {
        // If the string is empty, deallocate any dynamic storage. The length is left intact.
        if(this->empty())
          return this->do_deallocate();

        // Calculate the minimum capacity to reserve. This must include all existent characters.
        // Don't reallocate if the storage is shared or tight.
        size_type rcap = this->m_sth.round_up_capacity(this->size());
        if(!this->unique() || (this->capacity() <= rcap))
          return *this;

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        auto ptr = sth.reallocate_more(this->data(), this->size(), 0);

        // Set the new storage up. The length is left intact.
        this->m_sth.exchange_with(sth);
        this->m_ptr = ptr;
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    clear() noexcept
      {
        // If storage is shared, detach it.
        if(!this->m_sth.unique())
          return this->do_deallocate();

        this->m_ptr = storage_handle::null_char;
        this->m_len = 0;
        return *this;
      }

    // N.B. This is a non-standard extension.
    bool
    unique() const noexcept
      { return this->m_sth.unique();  }

    // N.B. This is a non-standard extension.
    long
    use_count() const noexcept
      { return this->m_sth.use_count();  }

    // 24.3.2.5, element access
    const_reference
    at(size_type pos) const
      {
        if(pos >= this->size())
          this->do_throw_subscript_out_of_range(pos, ">=");
        return this->data()[pos];
      }

    // N.B. This is a non-standard extension.
    template<typename subscriptT,
    ROCKET_ENABLE_IF(is_integral<subscriptT>::value && (sizeof(subscriptT) <= sizeof(size_type)))>
    const_reference
    at(subscriptT pos) const
      { return this->at(static_cast<size_type>(pos));  }

    const_reference
    operator[](size_type pos) const noexcept
      {
        // Note reading from the character at `size()` is permitted.
        ROCKET_ASSERT(pos <= this->size());
        return this->c_str()[pos];
      }

    // N.B. This is a non-standard extension.
    template<typename subscriptT,
    ROCKET_ENABLE_IF(is_integral<subscriptT>::value && (sizeof(subscriptT) <= sizeof(size_type)))>
    const_reference
    operator[](subscriptT pos) const noexcept
      { return this->operator[](static_cast<size_type>(pos));  }

    const_reference
    front() const noexcept
      {
        ROCKET_ASSERT(!this->empty());
        return this->data()[0];
      }

    const_reference
    back() const noexcept
      {
        ROCKET_ASSERT(!this->empty());
        return this->data()[this->size() - 1];
      }

    // There is no `at()` overload that returns a non-const reference.
    // This is the consequent overload which does that.
    // N.B. This is a non-standard extension.
    reference
    mut(size_type pos)
      {
        if(pos >= this->size())
          this->do_throw_subscript_out_of_range(pos, ">=");
        return this->mut_data()[pos];
      }

    // N.B. This is a non-standard extension.
    template<typename subscriptT,
    ROCKET_ENABLE_IF(is_integral<subscriptT>::value && (sizeof(subscriptT) <= sizeof(size_type)))>
    reference
    mut(subscriptT pos)
      { return this->mut(static_cast<size_type>(pos));  }

    // N.B. This is a non-standard extension.
    reference
    mut_front()
      {
        ROCKET_ASSERT(!this->empty());
        return this->mut_data()[0];
      }

    // N.B. This is a non-standard extension.
    reference
    mut_back()
      {
        ROCKET_ASSERT(!this->empty());
        return this->mut_data()[this->size() - 1];
      }

    // 24.3.2.6, modifiers
    basic_cow_string&
    operator+=(shallow_type sh)
      { return this->append(sh);  }

    basic_cow_string&
    operator+=(const basic_cow_string& other)
      { return this->append(other);  }

    basic_cow_string&
    operator+=(const value_type* s)
      { return this->append(s);  }

    basic_cow_string&
    operator+=(value_type ch)
      { return this->push_back(ch);  }

    basic_cow_string&
    operator+=(initializer_list<value_type> init)
      { return this->append(init);  }

    // N.B. This is a non-standard extension.
    basic_cow_string&
    operator<<(shallow_type sh)
      { return this->append(sh);  }

    basic_cow_string&
    operator<<(const basic_cow_string& other)
      { return this->append(other);  }

    // N.B. This is a non-standard extension.
    basic_cow_string&
    operator<<(const value_type* s)
      { return this->append(s);  }

    // N.B. This is a non-standard extension.
    basic_cow_string&
    operator<<(value_type ch)
      { return this->push_back(ch);  }

    // N.B. This is a non-standard extension.
    basic_cow_string&
    operator<<(initializer_list<value_type> init)
      { return this->append(init);  }

    basic_cow_string&
    append(shallow_type sh)
      { return this->append(sh.c_str(), sh.length());  }

    basic_cow_string&
    append(const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      { return this->append(other.data() + pos, other.do_clamp_substr(pos, n));  }

    basic_cow_string&
    append(const value_type* s, size_type n)
      {
        if(n == 0)
          return *this;

        // If the storage is unique and there is enough space, append the string in place.
        auto ptr = this->m_sth.mut_data_opt();
        size_type cap = this->capacity();
        size_type len = this->size();
        if(ROCKET_EXPECT(ptr && (n <= cap - len))) {
          traits_type::copy(ptr + len, s, n);
          traits_type::assign(*(ptr + len + n), value_type());
          this->m_ptr = ptr;  // note the storage might be unowned
          this->m_len += n;
          return *this;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        ptr = sth.reallocate_more(this->data(), len, n | cap / 2);

        // Copy the string.
        traits_type::copy(ptr + len, s, n);
        traits_type::assign(*(ptr + len + n), value_type());

        // Set the new storage up and increase the length.
        this->m_sth.exchange_with(sth);
        this->m_ptr = ptr;
        this->m_len += n;
        return *this;
      }

    basic_cow_string&
    append(size_type n, value_type ch)
      {
        if(n == 0)
          return *this;

        // If the storage is unique and there is enough space, append the string in place.
        auto ptr = this->m_sth.mut_data_opt();
        size_type cap = this->capacity();
        size_type len = this->size();
        if(ROCKET_EXPECT(ptr && (n <= cap - len))) {
          traits_type::assign(ptr + len, n, ch);
          traits_type::assign(*(ptr + len + n), value_type());
          this->m_ptr = ptr;  // note the storage might be unowned
          this->m_len += n;
          return *this;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        ptr = sth.reallocate_more(this->data(), len, n | cap / 2);

        // Copy the string.
        traits_type::assign(ptr + len, n, ch);
        traits_type::assign(*(ptr + len + n), value_type());

        // Set the new storage up and increase the length.
        this->m_sth.exchange_with(sth);
        this->m_ptr = ptr;
        this->m_len += n;
        return *this;
      }

    basic_cow_string&
    append(const value_type* s)
      { return this->append(s, traits_type::length(s));  }

    basic_cow_string&
    append(initializer_list<value_type> init)
      { return this->append(init.begin(), init.size());  }

    // N.B. This is a non-standard extension.
    basic_cow_string&
    append(const value_type* first, const value_type* last)
      {
        ROCKET_ASSERT(first <= last);
        return this->append(first, static_cast<size_t>(last - first));
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value),
    ROCKET_DISABLE_IF(is_convertible<inputT, const value_type*>::value)>
    basic_cow_string&
    append(inputT first, inputT last)
      {
        if(first == last)
          return *this;

        size_t dist = noadl::estimate_distance(first, last);
        size_type n = static_cast<size_type>(dist);

        // If the storage is unique and there is enough space, append the string in place.
        auto ptr = this->m_sth.mut_data_opt();
        size_type cap = this->capacity();
        size_type len = this->size();
        if(ROCKET_EXPECT(dist && (dist == n) && ptr && (n <= cap - len))) {
          n = 0;
          for(auto it = ::std::move(first);  it != last;  ++it)
            traits_type::assign(*(ptr + len + n++), *it);
          traits_type::assign(*(ptr + len + n), value_type());
          this->m_ptr = ptr;  // note the storage might be unowned
          this->m_len += n;
          return *this;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        if(ROCKET_EXPECT(dist && (dist == n))) {
          // The length is known.
          ptr = sth.reallocate_more(this->data(), len, n | cap / 2);

          // Copy the string into the new storage.
          n = 0;
          for(auto it = ::std::move(first);  it != last;  ++it)
            traits_type::assign(*(ptr + len + n++), *it);
        }
        else {
          // The length is not known.
          ptr = sth.reallocate_more(this->data(), len, 31 | cap / 2);
          cap = sth.capacity();

          // Reallocate the storage if necessary.
          n = 0;
          for(auto it = ::std::move(first);  it != last;  ++it) {
            if(ROCKET_UNEXPECT(len + n >= cap)) {
              ptr = sth.reallocate_more(ptr, len + n, cap / 2);
              cap = sth.capacity();
            }
            traits_type::assign(*(ptr + len + n++), *it);
          }
        }
        traits_type::assign(*(ptr + len + n), value_type());

        // Set the new storage up and increase the length.
        this->m_sth.exchange_with(sth);
        this->m_ptr = ptr;
        this->m_len += n;
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    push_back(value_type ch)
      {
        this->append(size_type(1), ch);
        return *this;
      }

    // N.B. There is no default argument for `tpos`.
    basic_cow_string&
    erase(size_type tpos, size_type tn = npos)
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);

        this->do_reserve_divide_unchecked(tpos, tlen, 0);
        return *this;
      }

    // N.B. This function may throw `std::bad_alloc`.
    iterator
    erase(const_iterator first, const_iterator last)
      {
        ROCKET_ASSERT_MSG(first <= last, "invalid range");
        size_type tpos = static_cast<size_type>(first - this->begin());
        size_type tlen = static_cast<size_type>(last - first);

        auto ptr = this->do_reserve_divide_unchecked(tpos, tlen, 0);
        return iterator(ptr - tpos, tpos, this->size());
      }

    // N.B. This function may throw `std::bad_alloc`.
    iterator
    erase(const_iterator pos)
      {
        size_type tpos = static_cast<size_type>(pos - this->begin());

        auto ptr = this->do_reserve_divide_unchecked(tpos, 1, 0);
        return iterator(ptr - tpos, tpos, this->size());
      }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. The return type and parameter are non-standard extensions.
    basic_cow_string&
    pop_back(size_type n = 1)
      {
        ROCKET_ASSERT_MSG(n <= this->size(), "no enough characters to pop");
        auto ptr = this->mut_data();
        size_type len = this->size();
        traits_type::assign(*(ptr + len - n), value_type());
        this->m_len -= n;
        return *this;
      }

    basic_cow_string&
    assign(const basic_cow_string& other, size_type pos, size_type n = npos)
      {
        // Note `other` may be `*this`.
        size_type kpos = this->size();
        this->append(other, pos, n);
        this->do_swizzle_unchecked(0, kpos, kpos);
        return *this;
      }

    basic_cow_string&
    assign(const value_type* s, size_type n)
      {
        // Note `s` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(s, n);
        this->do_swizzle_unchecked(0, kpos, kpos);
        return *this;
      }

    basic_cow_string&
    assign(const value_type* s)
      {
        // Note `s` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(s);
        this->do_swizzle_unchecked(0, kpos, kpos);
        return *this;
      }

    basic_cow_string&
    assign(size_type n, value_type ch)
      {
        this->clear();
        this->append(n, ch);
        return *this;
      }

    basic_cow_string&
    assign(initializer_list<value_type> init)
      {
        this->clear();
        this->append(init);
        return *this;
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string&
    assign(inputT first, inputT last)
      {
        // Note `first` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(::std::move(first), ::std::move(last));
        this->do_swizzle_unchecked(0, kpos, kpos);
        return *this;
      }

    basic_cow_string&
    insert(size_type tpos, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        this->do_clamp_substr(tpos, 0);  // just check

        // Note `other` may be `*this`.
        size_type kpos = this->size();
        this->append(other, pos, n);
        this->do_swizzle_unchecked(tpos, 0, kpos);
        return *this;
      }

    basic_cow_string&
    insert(size_type tpos, const value_type* s, size_type n)
      {
        this->do_clamp_substr(tpos, 0);  // just check

        // Note `s` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(s, n);
        this->do_swizzle_unchecked(tpos, 0, kpos);
        return *this;
      }

    basic_cow_string&
    insert(size_type tpos, const value_type* s)
      {
        this->do_clamp_substr(tpos, 0);  // just check

        // Note `s` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(s);
        this->do_swizzle_unchecked(tpos, 0, kpos);
        return *this;
      }

    basic_cow_string&
    insert(size_type tpos, size_type n, value_type ch)
      {
        this->do_clamp_substr(tpos, 0);  // just check

        // Note no aliasing is possible.
        auto ptr = this->do_reserve_divide_unchecked(tpos, 0, n);
        traits_type::assign(ptr, n, ch);
        return *this;
      }

    // N.B. This is a non-standard extension.
    basic_cow_string&
    insert(size_type tpos, initializer_list<value_type> init)
      {
        this->do_clamp_substr(tpos, 0);  // just check

        // Note no aliasing is possible.
        auto ptr = this->do_reserve_divide_unchecked(tpos, 0, init.size());
        traits_type::copy(ptr, init.begin(), init.size());
        return *this;
      }

    // N.B. This is a non-standard extension.
    iterator
    insert(const_iterator tins, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // Note `other` may be `*this`.
        size_type kpos = this->size();
        this->append(other, pos, n);
        auto ptr = this->do_swizzle_unchecked(tpos, 0, kpos);
        return iterator(ptr - tpos, tpos, this->size());
      }

    // N.B. This is a non-standard extension.
    iterator
    insert(const_iterator tins, const value_type* s, size_type n)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // Note `s` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(s, n);
        auto ptr = this->do_swizzle_unchecked(tpos, 0, kpos);
        return iterator(ptr - tpos, tpos, this->size());
      }

    // N.B. This is a non-standard extension.
    iterator
    insert(const_iterator tins, const value_type* s)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // Note `s` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(s);
        auto ptr = this->do_swizzle_unchecked(tpos, 0, kpos);
        return iterator(ptr - tpos, tpos, this->size());
      }

    iterator
    insert(const_iterator tins, size_type n, value_type ch)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // Note no aliasing is possible.
        auto ptr = this->do_reserve_divide_unchecked(tpos, 0, n);
        traits_type::assign(ptr, n, ch);
        return iterator(ptr - tpos, tpos, this->size());
      }

    iterator
    insert(const_iterator tins, initializer_list<value_type> init)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // Note no aliasing is possible.
        auto ptr = this->do_reserve_divide_unchecked(tpos, 0, init.size());
        traits_type::copy(ptr, init.begin(), init.size());
        return iterator(ptr - tpos, tpos, this->size());
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    iterator
    insert(const_iterator tins, inputT first, inputT last)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // Note `first` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(::std::move(first), ::std::move(last));
        auto ptr = this->do_swizzle_unchecked(tpos, 0, kpos);
        return iterator(ptr - tpos, tpos, this->size());
      }

    iterator
    insert(const_iterator tins, value_type ch)
      { return this->insert(tins, size_type(1), ch);  }

    basic_cow_string&
    replace(size_type tpos, size_type tn, const basic_cow_string& other,
            size_type pos = 0, size_type n = npos)
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);

        // Note `other` may be `*this`.
        size_type kpos = this->size();
        this->append(other, pos, n);
        this->do_swizzle_unchecked(tpos, tlen, kpos);
        return *this;
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, const value_type* s, size_type n)
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);

        // Note `s` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(s, n);
        this->do_swizzle_unchecked(tpos, tlen, kpos);
        return *this;
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, const value_type* s)
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);

        // Note `s` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(s);
        this->do_swizzle_unchecked(tpos, tlen, kpos);
        return *this;
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, size_type n, value_type ch)
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);

        // Note no aliasing is possible.
        auto ptr = this->do_reserve_divide_unchecked(tpos, tlen, n);
        traits_type::assign(ptr, n, ch);
        return *this;
      }

    // N.B. The last two parameters are non-standard extensions.
    basic_cow_string&
    replace(const_iterator first, const_iterator last, const basic_cow_string& other,
            size_type pos = 0, size_type n = npos)
      {
        ROCKET_ASSERT_MSG(first <= last, "invalid range");
        size_type tpos = static_cast<size_type>(first - this->begin());
        size_type tlen = static_cast<size_type>(last - first);

        // Note `other` may be `*this`.
        size_type kpos = this->size();
        this->append(other, pos, n);
        this->do_swizzle_unchecked(tpos, tlen, kpos);
        return *this;
      }

    basic_cow_string&
    replace(const_iterator first, const_iterator last, const value_type* s, size_type n)
      {
        ROCKET_ASSERT_MSG(first <= last, "invalid range");
        size_type tpos = static_cast<size_type>(first - this->begin());
        size_type tlen = static_cast<size_type>(last - first);

        // Note `s` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(s, n);
        this->do_swizzle_unchecked(tpos, tlen, kpos);
        return *this;
      }

    basic_cow_string&
    replace(const_iterator first, const_iterator last, const value_type* s)
      {
        ROCKET_ASSERT_MSG(first <= last, "invalid range");
        size_type tpos = static_cast<size_type>(first - this->begin());
        size_type tlen = static_cast<size_type>(last - first);

        // Note `s` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(s);
        this->do_swizzle_unchecked(tpos, tlen, kpos);
        return *this;
      }

    basic_cow_string&
    replace(const_iterator first, const_iterator last, size_type n, value_type ch)
      {
        ROCKET_ASSERT_MSG(first <= last, "invalid range");
        size_type tpos = static_cast<size_type>(first - this->begin());
        size_type tlen = static_cast<size_type>(last - first);

        // Note no aliasing is possible.
        auto ptr = this->do_reserve_divide_unchecked(tpos, tlen, n);
        traits_type::assign(ptr, n, ch);
        return *this;
      }

    basic_cow_string&
    replace(const_iterator first, const_iterator last, initializer_list<value_type> init)
      {
        ROCKET_ASSERT_MSG(first <= last, "invalid range");
        size_type tpos = static_cast<size_type>(first - this->begin());
        size_type tlen = static_cast<size_type>(last - first);

        // Note no aliasing is possible.
        auto ptr = this->do_reserve_divide_unchecked(tpos, tlen, init.size());
        traits_type::copy(ptr, init.begin(), init.size());
        return *this;
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string&
    replace(const_iterator first, const_iterator last, inputT ofirst, inputT olast)
      {
        ROCKET_ASSERT_MSG(first <= last, "invalid range");
        size_type tpos = static_cast<size_type>(first - this->begin());
        size_type tlen = static_cast<size_type>(last - first);

        // Note `first` may overlap with `this->data()`.
        size_type kpos = this->size();
        this->append(::std::move(ofirst), ::std::move(olast));
        this->do_swizzle_unchecked(tpos, tlen, kpos);
        return *this;
      }

    // N.B. This is a non-standard extension.
    basic_cow_string&
    replace(const_iterator first, const_iterator last, value_type ch)
      {
        ROCKET_ASSERT_MSG(first <= last, "invalid range");
        size_type tpos = static_cast<size_type>(first - this->begin());
        size_type tlen = static_cast<size_type>(last - first);

        // Note no aliasing is possible.
        auto ptr = this->do_reserve_divide_unchecked(tpos, tlen, 1);
        traits_type::assign(*ptr, ch);
        return *this;
      }

    // N.B. This is a non-standard extension.
    size_type
    copy(size_type tpos, value_type* s, size_type tn) const
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);

        traits_type::copy(s, this->data() + tpos, tlen);
        return tlen;
      }

    size_type
    copy(value_type* s, size_type tn) const
      { return this->copy(0, s, tn);  }

    // 24.3.2.7, string operations
    constexpr const value_type*
    data() const noexcept
      { return this->m_ptr;  }

    constexpr const value_type*
    c_str() const noexcept
      { return this->m_ptr;  }

    // N.B. This is a non-standard extension.
    const value_type*
    safe_c_str() const
      {
        size_type clen = traits_type::length(this->m_ptr);
        if(clen != this->m_len) {
          noadl::sprintf_and_throw<domain_error>(
                "cow_string: embedded null character detected (at `%llu`)",
                static_cast<unsigned long long>(clen));
        }
        return this->m_ptr;
      }

    // Get a pointer to mutable data. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    ROCKET_ALWAYS_INLINE value_type*
    mut_data()
      {
        auto ptr = this->m_sth.mut_data_opt();
        if(ROCKET_EXPECT(ptr))
          return ptr;

        // If the string is empty, return a pointer to constant storage.
        if(this->empty())
          return const_cast<value_type*>(this->data());

        // Reallocate the storage. The length is left intact.
        ptr = this->m_sth.reallocate_more(this->data(), this->size(), 0);
        this->m_ptr = ptr;
        return ptr;
      }

    // N.B. The return type differs from `std::basic_string`.
    constexpr const allocator_type&
    get_allocator() const noexcept
      { return this->m_sth.as_allocator();  }

    allocator_type&
    get_allocator() noexcept
      { return this->m_sth.as_allocator();  }

    // N.B. This is a non-standard extension.
    size_type
    find(size_type from, const basic_cow_string& other) const noexcept
      { return this->find(from, other.data(), other.size());  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find(const basic_cow_string& other) const noexcept
      { return this->find(size_type(0), other);  }

    // N.B. This is a non-standard extension.
    size_type
    find(size_type from, const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->find(from, other.data() + pos, other.do_clamp_substr(pos, n));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find(const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->find(size_type(0), other, pos, n);  }

    // N.B. This is a non-standard extension.
    size_type
    find(size_type from, const value_type* s, size_type n) const noexcept
      { return this->do_find_forwards_if(from, n,
                   [&](const value_type* ts) { return traits_type::compare(ts, s, n) == 0;  });  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find(const value_type* s, size_type n) const noexcept
      { return this->find(size_type(0), s, n);  }

    // N.B. This is a non-standard extension.
    size_type
    find(size_type from, const value_type* s) const noexcept
      { return this->find(from, s, traits_type::length(s));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find(const value_type* s) const noexcept
      { return this->find(size_type(0), s);  }

    // N.B. This is a non-standard extension.
    size_type
    find(size_type from, value_type ch) const noexcept
      {
        // This can be optimized.
        if(from >= this->size())
          return npos;

        auto ptr = traits_type::find(this->data() + from, this->size() - from, ch);
        if(!ptr)
          return npos;

        auto tpos = static_cast<size_type>(ptr - this->data());
        ROCKET_ASSERT(tpos < npos);
        return tpos;
      }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find(value_type ch) const noexcept
      { return this->find(size_type(0), ch);  }

    // N.B. This is a non-standard extension.
    size_type
    rfind(size_type to, const basic_cow_string& other) const noexcept
      { return this->rfind(to, other.data(), other.size());  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    rfind(const basic_cow_string& other) const noexcept
      { return this->rfind(size_type(-1), other);  }

    // N.B. This is a non-standard extension.
    size_type
    rfind(size_type to, const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->rfind(to, other.data() + pos, other.do_clamp_substr(pos, n));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    rfind(const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->rfind(size_type(-1), other, pos, n);  }

    // N.B. This is a non-standard extension.
    size_type
    rfind(size_type to, const value_type* s, size_type n) const noexcept
      { return this->do_find_backwards_if(to, n,
                   [&](const value_type* ts) { return traits_type::compare(ts, s, n) == 0;  });  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    rfind(const value_type* s, size_type n) const noexcept
      { return this->rfind(size_type(-1), s, n);  }

    // N.B. This is a non-standard extension.
    size_type
    rfind(size_type to, const value_type* s) const noexcept
      { return this->rfind(to, s, traits_type::length(s));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    rfind(const value_type* s) const noexcept
      { return this->rfind(size_type(-1), s);  }

    // N.B. This is a non-standard extension.
    size_type
    rfind(size_type to, value_type ch) const noexcept
      { return this->do_find_backwards_if(to, 1,
                   [&](const value_type* ts) { return traits_type::eq(*ts, ch);  });  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    rfind(value_type ch) const noexcept
      { return this->rfind(size_type(-1), ch);  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_of(size_type from, const basic_cow_string& other) const noexcept
      { return this->find_first_of(from, other.data(), other.size());  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_first_of(const basic_cow_string& other) const noexcept
      { return this->find_first_of(size_type(0), other);  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_of(size_type from, const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->find_first_of(from, other.data() + pos, other.do_clamp_substr(pos, n));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_first_of(const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->find_first_of(size_type(0), other, pos, n);  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_of(size_type from, const value_type* s, size_type n) const noexcept
      { return this->do_find_forwards_if(from, 1,
                   [&](const value_type* ts) { return traits_type::find(s, n, *ts);  });  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_first_of(const value_type* s, size_type n) const noexcept
      { return this->find_first_of(size_type(0), s, n);  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_of(size_type from, const value_type* s) const noexcept
      { return this->find_first_of(from, s, traits_type::length(s));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_first_of(const value_type* s) const noexcept
      { return this->find_first_of(size_type(0), s);  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_of(size_type from, value_type ch) const noexcept
      { return this->find(from, ch);  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_first_of(value_type ch) const noexcept
      { return this->find_first_of(size_type(0), ch);  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_of(size_type to, const basic_cow_string& other) const noexcept
      { return this->find_last_of(to, other.data(), other.size());  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_last_of(const basic_cow_string& other) const noexcept
      { return this->find_last_of(size_type(-1), other);  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_of(size_type to, const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->find_last_of(to, other.data() + pos, other.do_clamp_substr(pos, n));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_last_of(const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->find_last_of(size_type(-1), other, pos, n);  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_of(size_type to, const value_type* s, size_type n) const noexcept
      { return this->do_find_backwards_if(to, 1,
                   [&](const value_type* ts) { return traits_type::find(s, n, *ts);  });  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_last_of(const value_type* s, size_type n) const noexcept
      { return this->find_last_of(size_type(-1), s, n);  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_of(size_type to, const value_type* s) const noexcept
      { return this->find_last_of(to, s, traits_type::length(s));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_last_of(const value_type* s) const noexcept
      { return this->find_last_of(size_type(-1), s);  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_of(size_type to, value_type ch) const noexcept
      { return this->rfind(to, ch);  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_last_of(value_type ch) const noexcept
      { return this->find_last_of(size_type(-1), ch);  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_not_of(size_type from, const basic_cow_string& other) const noexcept
      { return this->find_first_not_of(from, other.data(), other.size());  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_first_not_of(const basic_cow_string& other) const noexcept
      { return this->find_first_not_of(size_type(0), other);  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_not_of(size_type from, const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->find_first_not_of(from, other.data() + pos, other.do_clamp_substr(pos, n));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_first_not_of(const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->find_first_not_of(size_type(0), other, pos, n);  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_not_of(size_type from, const value_type* s, size_type n) const noexcept
      { return this->do_find_forwards_if(from, 1,
                   [&](const value_type* ts) { return !traits_type::find(s, n, *ts);  });  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_first_not_of(const value_type* s, size_type n) const noexcept
      { return this->find_first_not_of(size_type(0), s, n);  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_not_of(size_type from, const value_type* s) const noexcept
      { return this->find_first_not_of(from, s, traits_type::length(s));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_first_not_of(const value_type* s) const noexcept
      { return this->find_first_not_of(size_type(0), s);  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_not_of(size_type from, value_type ch) const noexcept
      { return this->do_find_forwards_if(from, 1,
                   [&](const value_type* ts) { return !traits_type::eq(*ts, ch);  });  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_first_not_of(value_type ch) const noexcept
      { return this->find_first_not_of(size_type(0), ch);  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_not_of(size_type to, const basic_cow_string& other) const noexcept
      { return this->find_last_not_of(to, other.data(), other.size());  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_last_not_of(const basic_cow_string& other) const noexcept
      { return this->find_last_not_of(size_type(-1), other);  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_not_of(size_type to, const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->find_last_not_of(to, other.data() + pos, other.do_clamp_substr(pos, n));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_last_not_of(const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->find_last_not_of(size_type(-1), other, pos, n);  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_not_of(size_type to, const value_type* s, size_type n) const noexcept
      { return this->do_find_backwards_if(to, 1,
                   [&](const value_type* ts) { return !traits_type::find(s, n, *ts);  });  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_last_not_of(const value_type* s, size_type n) const noexcept
      { return this->find_last_not_of(size_type(-1), s, n);  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_not_of(size_type to, const value_type* s) const noexcept
      { return this->find_last_not_of(to, s, traits_type::length(s));  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_last_not_of(const value_type* s) const noexcept
      { return this->find_last_not_of(size_type(-1), s);  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_not_of(size_type to, value_type ch) const noexcept
      { return this->do_find_backwards_if(to, 1,
                   [&](const value_type* ts) { return !traits_type::eq(*ts, ch);  });  }

    // N.B. The signature differs from `std::basic_string`.
    size_type
    find_last_not_of(value_type ch) const noexcept
      { return this->find_last_not_of(size_type(-1), ch);  }

    // N.B. This is a non-standard extension.
    template<typename predT>
    size_type
    find_first_if(size_type from, predT&& pred) const
      { return this->do_find_forwards_if(from, 1, [&](const char* p) { return pred(*p);  });  }

    // N.B. This is a non-standard extension.
    template<typename predT>
    size_type
    find_first_if(predT&& pred) const
      { return this->find_first_if(size_type(0), ::std::forward<predT>(pred));  }

    // N.B. This is a non-standard extension.
    template<typename predT>
    size_type
    find_last_if(size_type to, predT&& pred) const
      { return this->do_find_backwards_if(to, 1, [&](const char* p) { return pred(*p);  });  }

    // N.B. This is a non-standard extension.
    template<typename predT>
    size_type
    find_last_if(predT&& pred) const
      { return this->find_last_if(size_type(-1), ::std::forward<predT>(pred));  }

    // N.B. There is no default argument for `tpos`.
    basic_cow_string
    substr(size_type tpos, size_type tn = npos) const
      {
        if((tpos == 0) && (tn >= this->size()))
          return basic_cow_string(*this, this->m_sth.as_allocator());
        else
          return basic_cow_string(*this, tpos, tn, this->m_sth.as_allocator());
      }

    int
    compare(const basic_cow_string& other) const noexcept
      { return this->compare(other.data(), other.size());  }

    // N.B. This is a non-standard extension.
    int
    compare(const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->compare(other.data() + pos, other.do_clamp_substr(pos, n));  }

    // N.B. This is a non-standard extension.
    int
    compare(const value_type* s, size_type n) const noexcept
      { return details_cow_string::comparator<charT, traitsT>::relation(
                   this->data(), this->size(), s, n);  }

    int
    compare(const value_type* s) const noexcept
      { return this->compare(s, traits_type::length(s));  }

    // N.B. The last two parameters are non-standard extensions.
    int
    compare(size_type tpos, size_type tn, const basic_cow_string& other,
            size_type pos = 0, size_type n = npos) const
      { return this->compare(tpos, tn, other.data() + pos, other.do_clamp_substr(pos, n));  }

    int
    compare(size_type tpos, size_type tn, const value_type* s, size_type n) const
      { return details_cow_string::comparator<charT, traitsT>::relation(
                   this->data() + tpos, this->do_clamp_substr(tpos, tn), s, n);  }

    int
    compare(size_type tpos, size_type tn, const value_type* s) const
      { return this->compare(tpos, tn, s, traits_type::length(s));  }

    // N.B. This is a non-standard extension.
    int
    compare(size_type tpos, const basic_cow_string& other, size_type pos = 0, size_type n = npos) const
      { return this->compare(tpos, npos, other, pos, n);  }

    // N.B. This is a non-standard extension.
    int
    compare(size_type tpos, const value_type* s, size_type n) const
      { return this->compare(tpos, npos, s, n);  }

    // N.B. This is a non-standard extension.
    int
    compare(size_type tpos, const value_type* s) const
      { return this->compare(tpos, npos, s);  }

    // N.B. These are extensions but might be standardized in C++20.
    bool
    starts_with(const basic_cow_string& other) const noexcept
      { return this->starts_with(other.data(), other.size());  }

    // N.B. This is a non-standard extension.
    bool
    starts_with(const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->starts_with(other.data() + pos, other.do_clamp_substr(pos, n));  }

    bool
    starts_with(const value_type* s, size_type n) const noexcept
      { return (n <= this->size()) && (traits_type::compare(this->data(), s, n) == 0);  }

    bool
    starts_with(const value_type* s) const noexcept
      { return this->starts_with(s, traits_type::length(s));  }

    bool
    starts_with(value_type ch) const noexcept
      { return (1 <= this->size()) && traits_type::eq(this->front(), ch);  }

    bool
    ends_with(const basic_cow_string& other) const noexcept
      { return this->ends_with(other.data(), other.size());  }

    // N.B. This is a non-standard extension.
    bool
    ends_with(const basic_cow_string& other, size_type pos, size_type n = npos) const
      { return this->ends_with(other.data() + pos, other.do_clamp_substr(pos, n));  }

    bool
    ends_with(const value_type* s, size_type n) const noexcept
      { return (n <= this->size()) &&
               (traits_type::compare(this->data() + this->size() - n, s, n) == 0);  }

    bool
    ends_with(const value_type* s) const noexcept
      { return this->ends_with(s, traits_type::length(s));  }

    bool
    ends_with(value_type ch) const noexcept
      { return (1 <= this->size()) && traits_type::eq(this->back(), ch);  }
  };

#if __cpp_inline_variables + 0 < 201606  // < c++17
template<typename charT, typename traitsT, typename allocT>
const typename basic_cow_string<charT, traitsT, allocT>::size_type
  basic_cow_string<charT, traitsT, allocT>::npos;
#endif

template<typename charT, typename traitsT, typename allocT>
struct basic_cow_string<charT, traitsT, allocT>::hash
  {
    using result_type    = size_t;
    using argument_type  = basic_cow_string;

    constexpr result_type
    operator()(const argument_type& str) const noexcept
      {
        details_cow_string::basic_hasher<charT, traitsT> hf;
        hf.append(str.data(), str.size());
        return hf.finish();
      }

    constexpr result_type
    operator()(const charT* s) const noexcept
      {
        details_cow_string::basic_hasher<charT, traitsT> hf;
        hf.append(s);
        return hf.finish();
      }
  };

extern template
class basic_cow_string<char>;

extern template
class basic_cow_string<wchar_t>;

extern template
class basic_cow_string<char16_t>;

extern template
class basic_cow_string<char32_t>;

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(const basic_cow_string<charT, traitsT, allocT>& lhs,
          const basic_cow_string<charT, traitsT, allocT>& rhs)
  {
    auto res = lhs;
    res.append(rhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(basic_cow_string<charT, traitsT, allocT>&& lhs,
          basic_cow_string<charT, traitsT, allocT>&& rhs)
  {
    auto ntotal = lhs.size() + rhs.size();
    if(ROCKET_EXPECT((ntotal <= lhs.capacity()) || (ntotal > rhs.capacity())))
      return ::std::move(lhs.append(rhs));
    else
      return ::std::move(rhs.insert(0, lhs));
  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(basic_cow_string<charT, traitsT, allocT>&& lhs,
          const basic_cow_string<charT, traitsT, allocT>& rhs)
  { return ::std::move(lhs.append(rhs));  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(const basic_cow_string<charT, traitsT, allocT>& lhs,
          basic_cow_string<charT, traitsT, allocT>&& rhs)
  { return ::std::move(rhs.insert(0, lhs));  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs)
  {
    auto res = lhs;
    res.append(rhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(const basic_cow_string<charT, traitsT, allocT>& lhs, charT rhs)
  {
    auto res = lhs;
    res.push_back(rhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
  {
    auto res = rhs;
    res.insert(0, lhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(charT lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
  {
    auto res = rhs;
    res.insert(0, 1, lhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(basic_cow_string<charT, traitsT, allocT>&& lhs, const charT* rhs)
  { return ::std::move(lhs.append(rhs));  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(basic_cow_string<charT, traitsT, allocT>&& lhs, charT rhs)
  { return ::std::move(lhs.append(1, rhs));  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(const charT* lhs, basic_cow_string<charT, traitsT, allocT>&& rhs)
  { return ::std::move(rhs.insert(0, lhs));  }

template<typename charT, typename traitsT, typename allocT>
inline basic_cow_string<charT, traitsT, allocT>
operator+(charT lhs, basic_cow_string<charT, traitsT, allocT>&& rhs)
  { return ::std::move(rhs.insert(0, 1, lhs));  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator==(const basic_cow_string<charT, traitsT, allocT>& lhs,
           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) == 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator!=(const basic_cow_string<charT, traitsT, allocT>& lhs,
           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) != 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator<(const basic_cow_string<charT, traitsT, allocT>& lhs,
          const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) < 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator>(const basic_cow_string<charT, traitsT, allocT>& lhs,
          const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) > 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator<=(const basic_cow_string<charT, traitsT, allocT>& lhs,
           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) <= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator>=(const basic_cow_string<charT, traitsT, allocT>& lhs,
           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) >= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator==(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) == 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator!=(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) != 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator<(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) < 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator>(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) > 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator<=(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) <= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator>=(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) >= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator==(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) == 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator!=(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) != 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator<(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) < 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator>(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) > 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator<=(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) <= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator>=(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) >= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator==(const basic_cow_string<charT, traitsT, allocT>& lhs,
           basic_shallow_string<charT, traitsT> rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) == 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator!=(const basic_cow_string<charT, traitsT, allocT>& lhs,
           basic_shallow_string<charT, traitsT> rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) != 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator<(const basic_cow_string<charT, traitsT, allocT>& lhs,
          basic_shallow_string<charT, traitsT> rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) < 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator>(const basic_cow_string<charT, traitsT, allocT>& lhs,
          basic_shallow_string<charT, traitsT> rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) > 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator<=(const basic_cow_string<charT, traitsT, allocT>& lhs,
           basic_shallow_string<charT, traitsT> rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) <= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator>=(const basic_cow_string<charT, traitsT, allocT>& lhs,
           basic_shallow_string<charT, traitsT> rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) >= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator==(basic_shallow_string<charT, traitsT> lhs,
           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) == 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator!=(basic_shallow_string<charT, traitsT> lhs,
           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) != 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator<(basic_shallow_string<charT, traitsT> lhs,
          const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) < 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator>(basic_shallow_string<charT, traitsT> lhs,
          const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) > 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator<=(basic_shallow_string<charT, traitsT> lhs,
           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) <= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline bool
operator>=(basic_shallow_string<charT, traitsT> lhs,
           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) >= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline void
swap(basic_cow_string<charT, traitsT, allocT>& lhs,
     basic_cow_string<charT, traitsT, allocT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

template<typename charT, typename traitsT, typename allocT>
inline basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt,
           const basic_cow_string<charT, traitsT, allocT>& str)
  { return fmt.putn(str.data(), str.size());  }

template<typename charT, typename traitsT, typename allocT>
inline bool
getline(basic_cow_string<charT, traitsT, allocT>& str, basic_tinybuf<charT, traitsT>& buf)
  {
    str.clear();
    typename traitsT::int_type ch;

    for(;;)
      if(traitsT::is_eof(ch = buf.getc()))
        return !str.empty();  // end of stream
      else if(traitsT::eq(traitsT::to_char_type(ch), '\n'))
        return true;  // end of line
      else
        str += traitsT::to_char_type(ch);  // plain character
  }

using shallow_string     = basic_shallow_string<char>;
using shallow_wstring    = basic_shallow_string<wchar_t>;
using shallow_u16string  = basic_shallow_string<char16_t>;
using shallow_u32string  = basic_shallow_string<char32_t>;

using cow_string     = basic_cow_string<char>;
using cow_wstring    = basic_cow_string<wchar_t>;
using cow_u16string  = basic_cow_string<char16_t>;
using cow_u32string  = basic_cow_string<char32_t>;

}  // namespace rocket

#endif
