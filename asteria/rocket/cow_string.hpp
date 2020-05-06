// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRING_HPP_
#define ROCKET_COW_STRING_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "char_traits.hpp"
#include "allocator_utilities.hpp"
#include "reference_counter.hpp"

namespace rocket {

template<typename charT, typename traitsT>
class basic_tinyfmt;

template<typename charT, typename traitsT = char_traits<charT>>
class basic_shallow_string;

template<typename charT, typename traitsT = char_traits<charT>, typename allocT = allocator<charT>>
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
class basic_shallow_string
  {
  private:
    const charT* m_ptr;
    size_t m_len;

  public:
    explicit constexpr
    basic_shallow_string(const charT* ptr)
    noexcept
      : m_ptr(ptr), m_len(traitsT::length(ptr))
      { }

    constexpr
    basic_shallow_string(const charT* ptr, size_t len)
    noexcept
      : m_ptr(ptr), m_len((ROCKET_ASSERT(traitsT::eq(ptr[len], charT())), len))
      { }

    template<typename allocT>
    explicit
    basic_shallow_string(const basic_cow_string<charT, traitsT, allocT>& str)
    noexcept
      : m_ptr(str.c_str()), m_len(str.length())
      { }

  public:
    constexpr
    const charT*
    c_str()
    const noexcept
      { return this->m_ptr;  }

    constexpr
    size_t
    length()
    const noexcept
      { return this->m_len;  }
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
constexpr
basic_shallow_string<charT, char_traits<charT>>
sref(const charT* ptr)
noexcept
  { return basic_shallow_string<charT, char_traits<charT>>(ptr);  }

template<typename charT>
constexpr
basic_shallow_string<charT, char_traits<charT>>
sref(const charT* ptr, ::std::size_t len)
noexcept
  { return basic_shallow_string<charT, char_traits<charT>>(ptr, len);  }

template<typename charT, typename traitsT, typename allocT>
constexpr
basic_shallow_string<charT, traitsT>
sref(const basic_cow_string<charT, traitsT, allocT>& str)
noexcept
  { return basic_shallow_string<charT, traitsT>(str);  }

template<typename charT, typename traitsT>
inline
basic_tinyfmt<charT, traitsT>&
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

    using const_iterator          = details_cow_string::string_iterator<basic_cow_string, const value_type>;
    using iterator                = details_cow_string::string_iterator<basic_cow_string, value_type>;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

    using shallow_type   = basic_shallow_string<charT, traitsT>;

    static constexpr size_type npos = size_type(-1);
    static const value_type null_char[1];

    // hash support
    struct hash;

  private:
    details_cow_string::storage_handle<allocator_type, traits_type> m_sth;
    const value_type* m_ptr = null_char;
    size_type m_len = 0;

  public:
    // 24.3.2.2, construct/copy/destroy
    constexpr
    basic_cow_string(shallow_type sh, const allocator_type& alloc = allocator_type())
    noexcept
      : m_sth(alloc), m_ptr(sh.c_str()), m_len(sh.length())
      { }

    explicit constexpr
    basic_cow_string(const allocator_type& alloc)
    noexcept
      : m_sth(alloc)
      { }

    basic_cow_string(const basic_cow_string& other)
    noexcept
      : m_sth(allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
      { this->assign(other);  }

    basic_cow_string(const basic_cow_string& other, const allocator_type& alloc)
    noexcept
      : m_sth(alloc)
      { this->assign(other);  }

    basic_cow_string(basic_cow_string&& other)
    noexcept
      : m_sth(::std::move(other.m_sth.as_allocator()))
      { this->assign(::std::move(other));  }

    basic_cow_string(basic_cow_string&& other, const allocator_type& alloc)
    noexcept
      : m_sth(alloc)
      { this->assign(::std::move(other));  }

    constexpr
    basic_cow_string(nullopt_t = nullopt_t())
    noexcept(is_nothrow_constructible<allocator_type>::value)
      : basic_cow_string(allocator_type())
      { }

    basic_cow_string(const basic_cow_string& other, size_type pos, size_type n = npos,
                     const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->assign(other, pos, n);  }

    basic_cow_string(const value_type* s, size_type n, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->assign(s, n);  }

    explicit
    basic_cow_string(const value_type* s, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->assign(s);  }

    basic_cow_string(size_type n, value_type ch, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->assign(n, ch);  }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string(inputT first, inputT last, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->assign(::std::move(first), ::std::move(last));  }

    basic_cow_string(initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->assign(init);  }

    basic_cow_string&
    operator=(nullopt_t)
    noexcept
      {
        this->clear();
        return *this;
      }

    basic_cow_string&
    operator=(shallow_type sh)
    noexcept
      {
        this->assign(sh);
        return *this;
      }

    basic_cow_string&
    operator=(const basic_cow_string& other)
    noexcept
      {
        noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->assign(other);
        return *this;
      }

    basic_cow_string&
    operator=(basic_cow_string&& other)
    noexcept
      {
        noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->assign(::std::move(other));
        return *this;
      }

    basic_cow_string&
    operator=(initializer_list<value_type> init)
      {
        this->assign(init);
        return *this;
      }

  private:
    // Reallocate the storage to `res_arg` characters, not including the null terminator.
    value_type*
    do_reallocate(size_type len_one, size_type off_two, size_type len_two, size_type res_arg)
      {
        ROCKET_ASSERT(len_one <= off_two);
        ROCKET_ASSERT(off_two <= this->m_len);
        ROCKET_ASSERT(len_two <= this->m_len - off_two);
        auto ptr = this->m_sth.reallocate(this->m_ptr, len_one, off_two, len_two, res_arg);
        ROCKET_ASSERT(!ptr || this->m_sth.unique());
        this->m_ptr = ptr ? ptr : null_char;
        this->m_len = len_one + len_two;
        return ptr;
      }

    // Add a null terminator at `ptr[len]` then set `len` there.
    void
    do_set_length(size_type len)
    noexcept
      {
        ROCKET_ASSERT(len <= this->m_sth.capacity());
        auto ptr = this->m_sth.mut_data_unchecked();
        if(ptr) {
          ROCKET_ASSERT(ptr == this->m_ptr);
          traits_type::assign(ptr[len], value_type());
        }
        this->m_len = len;
      }

    // Clear contents. Deallocate the storage if it is shared at all.
    void
    do_clear()
    noexcept
      {
        if(!this->unique()) {
          this->m_ptr = null_char;
          this->m_len = 0;
          this->m_sth.deallocate();
        }
        else
          this->do_set_length(0);
      }

    // Reallocate more storage as needed, without shrinking.
    void
    do_reserve_more(size_type cap_add)
      {
        auto len = this->size();
        auto cap = this->m_sth.check_size_add(len, cap_add);
        if(!this->unique() || ROCKET_UNEXPECT(this->capacity() < cap)) {
#ifndef ROCKET_DEBUG
          // Reserve more space for non-debug builds.
          cap |= len / 2 + 31;
#endif
          this->do_reallocate(0, 0, len, cap | 1);
        }
        ROCKET_ASSERT(this->capacity() >= cap);
      }

    [[noreturn]] ROCKET_NOINLINE
    void
    do_throw_subscript_out_of_range(size_type pos)
    const
      {
        noadl::sprintf_and_throw<out_of_range>("cow_string: subscript out of range (`%llu` > `%llu`)",
                                               static_cast<unsigned long long>(pos),
                                               static_cast<unsigned long long>(this->size()));
      }

    // This function works the same way as `substr()`.
    // Ensure `tpos` is in `[0, size()]` and return `min(tn, size() - tpos)`.
    size_type
    do_clamp_substr(size_type tpos, size_type tn)
    const
      {
        auto tlen = this->size();
        if(tpos > tlen)
          this->do_throw_subscript_out_of_range(tpos);
        return noadl::min(tlen - tpos, tn);
      }

    template<typename... paramsT>
    value_type*
    do_replace_no_bound_check(size_type tpos, size_type tn, paramsT&&... params)
      {
        auto len_old = this->size();
        ROCKET_ASSERT(tpos <= len_old);
        details_cow_string::tagged_append(this, ::std::forward<paramsT>(params)...);
        auto len_add = this->size() - len_old;
        auto len_sfx = len_old - (tpos + tn);
        this->do_reserve_more(len_sfx);
        auto ptr = this->m_sth.mut_data_unchecked();
        traits_type::copy(ptr + len_old + len_add, ptr + tpos + tn, len_sfx);
        traits_type::move(ptr + tpos, ptr + len_old, len_add + len_sfx);
        this->do_set_length(len_old + len_add - tn);
        return ptr + tpos;
      }

    value_type*
    do_erase_no_bound_check(size_type tpos, size_type tn)
      {
        auto len_old = this->size();
        ROCKET_ASSERT(tpos <= len_old);
        ROCKET_ASSERT(tn <= len_old - tpos);
        if(!this->unique()) {
          auto ptr = this->do_reallocate(tpos, tpos + tn, len_old - (tpos + tn), len_old);
          return ptr + tpos;
        }
        auto ptr = this->m_sth.mut_data_unchecked();
        traits_type::move(ptr + tpos, ptr + tpos + tn, len_old - (tpos + tn));
        this->do_set_length(len_old - tn);
        return ptr + tpos;
      }

    // These are generic implementations for `{{,r}find,find_{first,last}{,_not}_of}()` functions.
    template<typename predT>
    size_type
    do_xfind_if(size_type first, size_type last, difference_type step, predT pred)
    const
      {
        auto cur = first;
        for(;;) {
          auto ptr = this->data() + cur;
          if(pred(ptr))
            return ROCKET_ASSERT(cur != npos), cur;
          if(cur == last)
            return npos;
          cur += static_cast<size_type>(step);
        }
      }

    template<typename predT>
    size_type
    do_find_forwards_if(size_type from, size_type n, predT pred)
    const
      {
        auto len = this->size();
        if(len < n)
          return npos;

        auto rlen = len - n;
        if(from > rlen)
          return npos;

        return this->do_xfind_if(from, rlen, +1, ::std::move(pred));
      }

    template<typename predT>
    size_type
    do_find_backwards_if(size_type to, size_type n, predT pred)
    const
      {
        auto len = this->size();
        if(len < n)
          return npos;

        auto rlen = len - n;
        if(to > rlen)
          return this->do_xfind_if(rlen, 0, -1, ::std::move(pred));

        return this->do_xfind_if(to, 0, -1, ::std::move(pred));
      }

  public:
    // 24.3.2.3, iterators
    const_iterator
    begin()
    const noexcept
      { return const_iterator(this, this->data());  }

    const_iterator
    end()
    const noexcept
      { return const_iterator(this, this->data() + this->size());  }

    const_reverse_iterator
    rbegin()
    const noexcept
      { return const_reverse_iterator(this->end());  }

    const_reverse_iterator
    rend()
    const noexcept
      { return const_reverse_iterator(this->begin());  }

    const_iterator
    cbegin()
    const noexcept
      { return this->begin();  }

    const_iterator
    cend()
    const noexcept
      { return this->end();  }

    const_reverse_iterator
    crbegin()
    const noexcept
      { return this->rbegin();  }

    const_reverse_iterator
    crend()
    const noexcept
      { return this->rend();  }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator
    mut_begin()
      { return iterator(this, this->mut_data());  }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator
    mut_end()
      { return iterator(this, this->mut_data() + this->size());  }

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

    // 24.3.2.4, capacity
    constexpr
    bool
    empty()
    const noexcept
      { return this->m_len == 0;  }

    constexpr
    size_type
    size()
    const noexcept
      { return this->m_len;  }

    constexpr
    size_type
    length()
    const noexcept
      { return this->m_len;  }

    // N.B. This is a non-standard extension.
    constexpr
    difference_type
    ssize()
    const noexcept
      { return static_cast<difference_type>(this->size());  }

    constexpr
    size_type
    max_size()
    const noexcept
      { return this->m_sth.max_size();  }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    resize(size_type n, value_type ch = value_type())
      {
        auto len_old = this->size();
        if(len_old < n)
          this->append(n - len_old, ch);
        else if(len_old > n)
          this->pop_back(len_old - n);
        ROCKET_ASSERT(this->size() == n);
        return *this;
      }

    constexpr
    size_type
    capacity()
    const noexcept
      { return this->m_sth.capacity();  }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    reserve(size_type res_arg)
      {
        auto len = this->size();
        auto cap_new = this->m_sth.round_up_capacity(noadl::max(len, res_arg));
        // If the storage is shared with other strings, force rellocation to prevent copy-on-write
        // upon modification.
        if(this->unique() && (this->capacity() >= cap_new))
          return *this;

        this->do_reallocate(0, 0, len, cap_new);
        ROCKET_ASSERT(this->capacity() >= res_arg);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    shrink_to_fit()
      {
        auto len = this->size();
        auto cap_min = this->m_sth.round_up_capacity(len);
        // Don't increase memory usage.
        if(!this->unique() || (this->capacity() <= cap_min))
          return *this;

        this->do_reallocate(0, 0, len, len);
        ROCKET_ASSERT(this->capacity() <= cap_min);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    clear()
    noexcept
      {
        if(this->empty())
          return *this;

        this->do_clear();
        return *this;
      }

    // N.B. This is a non-standard extension.
    bool
    unique()
    const noexcept
      { return this->m_sth.unique();  }

    // N.B. This is a non-standard extension.
    long
    use_count()
    const noexcept
      { return this->m_sth.use_count();  }

    // 24.3.2.5, element access
    const_reference
    at(size_type pos)
    const
      {
        auto len = this->size();
        if(pos >= len)
          this->do_throw_subscript_out_of_range(pos);
        return this->data()[pos];
      }

    const_reference
    operator[](size_type pos)
    const noexcept
      {
        auto len = this->size();
        // Reading from the character at `size()` is permitted.
        ROCKET_ASSERT(pos <= len);
        return this->c_str()[pos];
      }

    const_reference
    front()
    const noexcept
      {
        auto len = this->size();
        ROCKET_ASSERT(len > 0);
        return this->data()[0];
      }

    const_reference
    back()
    const noexcept
      {
        auto len = this->size();
        ROCKET_ASSERT(len > 0);
        return this->data()[len - 1];
      }

    // There is no `at()` overload that returns a non-const reference.
    // This is the consequent overload which does that.
    // N.B. This is a non-standard extension.
    reference
    mut(size_type pos)
      {
        auto len = this->size();
        if(pos >= len)
          this->do_throw_subscript_out_of_range(pos);
        return this->mut_data()[pos];
      }

    // N.B. This is a non-standard extension.
    reference
    mut_front()
      {
        auto len = this->size();
        ROCKET_ASSERT(len > 0);
        return this->mut_data()[0];
      }

    // N.B. This is a non-standard extension.
    reference
    mut_back()
      {
        auto len = this->size();
        ROCKET_ASSERT(len > 0);
        return this->mut_data()[len - 1];
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

        auto len_old = this->size();
        // Check for overlapped strings before `do_reserve_more()`.
        auto srpos = static_cast<uintptr_t>(s - this->data());
        this->do_reserve_more(n);
        auto ptr = this->m_sth.mut_data_unchecked();
        if(srpos < len_old) {
          traits_type::move(ptr + len_old, ptr + srpos, n);
          this->do_set_length(len_old + n);
          return *this;
        }
        traits_type::copy(ptr + len_old, s, n);
        this->do_set_length(len_old + n);
        return *this;
      }

    basic_cow_string&
    append(const value_type* s)
      {
        return this->append(s, traits_type::length(s));
      }

    basic_cow_string&
    append(size_type n, value_type ch)
      {
        if(n == 0)
          return *this;

        auto len_old = this->size();
        this->do_reserve_more(n);
        auto ptr = this->m_sth.mut_data_unchecked();
        traits_type::assign(ptr + len_old, n, ch);
        this->do_set_length(len_old + n);
        return *this;
      }

    basic_cow_string&
    append(initializer_list<value_type> init)
      {
        return this->append(init.begin(), init.size());
      }

    // N.B. This is a non-standard extension.
    basic_cow_string&
    append(const value_type* first, const value_type* last)
      {
        ROCKET_ASSERT(first <= last);
        return this->append(first, static_cast<size_type>(last - first));
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value && !is_convertible<inputT, const value_type*>::value)>
    basic_cow_string&
    append(inputT first, inputT last)
      {
        if(first == last)
          return *this;

        basic_cow_string other(this->m_sth.as_allocator());
        other.reserve(this->size() + noadl::estimate_distance(first, last));
        other.append(this->data(), this->size());
        noadl::ranged_do_while(::std::move(first), ::std::move(last),
                               [&](const inputT& it) { other.push_back(*it);  });
        this->assign(::std::move(other));
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    push_back(value_type ch)
      {
        auto len_old = this->size();
        this->do_reserve_more(1);
        auto ptr = this->m_sth.mut_data_unchecked();
        traits_type::assign(ptr[len_old], ch);
        this->do_set_length(len_old + 1);
        return *this;
      }

    // N.B. There is no default argument for `tpos`.
    basic_cow_string&
    erase(size_type tpos, size_type tn = npos)
      {
        this->do_erase_no_bound_check(tpos, this->do_clamp_substr(tpos, tn));
        return *this;
      }

    // N.B. This function may throw `std::bad_alloc`.
    iterator
    erase(const_iterator tfirst, const_iterator tlast)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        auto ptr = this->do_erase_no_bound_check(tpos, tn);
        return iterator(this, ptr);
      }

    // N.B. This function may throw `std::bad_alloc`.
    iterator
    erase(const_iterator tfirst)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto ptr = this->do_erase_no_bound_check(tpos, 1);
        return iterator(this, ptr);
      }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. The return type and parameter are non-standard extensions.
    basic_cow_string&
    pop_back(size_type n = 1)
      {
        if(n == 0)
          return *this;

        auto len_old = this->size();
        ROCKET_ASSERT(n <= len_old);
        if(!this->unique()) {
          this->do_reallocate(0, 0, len_old - n, len_old);
          return *this;
        }
        this->do_set_length(len_old - n);
        return *this;
      }

    basic_cow_string&
    assign(shallow_type sh)
    noexcept
      {
        this->m_sth.deallocate();
        this->m_ptr = sh.c_str();
        this->m_len = sh.length();
        return *this;
      }

    basic_cow_string&
    assign(const basic_cow_string& other)
    noexcept
      {
        this->m_sth.share_with(other.m_sth);
        this->m_ptr = other.m_ptr;
        this->m_len = other.m_len;
        return *this;
      }

    basic_cow_string&
    assign(basic_cow_string&& other)
    noexcept
      {
        this->m_sth.share_with(::std::move(other.m_sth));
        this->m_ptr = ::std::exchange(other.m_ptr, null_char);
        this->m_len = ::std::exchange(other.m_len, size_type(0));
        return *this;
      }

    basic_cow_string&
    assign(const basic_cow_string& other, size_type pos, size_type n = npos)
      {
        this->do_replace_no_bound_check(0, this->size(), details_cow_string::append, other, pos, n);
        return *this;
      }

    basic_cow_string&
    assign(const value_type* s, size_type n)
      {
        this->do_replace_no_bound_check(0, this->size(), details_cow_string::append, s, n);
        return *this;
      }

    basic_cow_string&
    assign(const value_type* s)
      {
        this->do_replace_no_bound_check(0, this->size(), details_cow_string::append, s);
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
        this->do_replace_no_bound_check(0, this->size(), details_cow_string::append,
                                        ::std::move(first), ::std::move(last));
        return *this;
      }

    basic_cow_string&
    insert(size_type tpos, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        this->do_clamp_substr(tpos, 0);  // just check
        this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, other, pos, n);
        return *this;
      }

    basic_cow_string&
    insert(size_type tpos, const value_type* s, size_type n)
      {
        this->do_clamp_substr(tpos, 0);  // just check
        this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, s, n);
        return *this;
      }

    basic_cow_string&
    insert(size_type tpos, const value_type* s)
      {
        this->do_clamp_substr(tpos, 0);  // just check
        this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, s);
        return *this;
      }

    basic_cow_string&
    insert(size_type tpos, size_type n, value_type ch)
      {
        this->do_clamp_substr(tpos, 0);  // just check
        this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, n, ch);
        return *this;
      }

    // N.B. This is a non-standard extension.
    basic_cow_string&
    insert(size_type tpos, initializer_list<value_type> init)
      {
        this->do_clamp_substr(tpos, 0);  // just check
        this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, init);
        return *this;
      }

    // N.B. This is a non-standard extension.
    iterator
    insert(const_iterator tins, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, other, pos, n);
        return iterator(this, ptr);
      }

    // N.B. This is a non-standard extension.
    iterator
    insert(const_iterator tins, const value_type* s, size_type n)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, s, n);
        return iterator(this, ptr);
      }

    // N.B. This is a non-standard extension.
    iterator
    insert(const_iterator tins, const value_type* s)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, s);
        return iterator(this, ptr);
      }

    iterator
    insert(const_iterator tins, size_type n, value_type ch)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, n, ch);
        return iterator(this, ptr);
      }

    iterator
    insert(const_iterator tins, initializer_list<value_type> init)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, init);
        return iterator(this, ptr);
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    iterator
    insert(const_iterator tins, inputT first, inputT last)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append,
                                                   ::std::move(first), ::std::move(last));
        return iterator(this, ptr);
      }

    iterator
    insert(const_iterator tins, value_type ch)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::push_back, ch);
        return iterator(this, ptr);
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn),
                                        details_cow_string::append, other, pos, n);
        return *this;
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, const value_type* s, size_type n)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn),
                                        details_cow_string::append, s, n);
        return *this;
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, const value_type* s)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn),
                                        details_cow_string::append, s);
        return *this;
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, size_type n, value_type ch)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn),
                                        details_cow_string::append, n, ch);
        return *this;
      }

    // N.B. The last two parameters are non-standard extensions.
    basic_cow_string&
    replace(const_iterator tfirst, const_iterator tlast, const basic_cow_string& other,
            size_type pos = 0, size_type n = npos)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, other, pos, n);
        return *this;
      }

    basic_cow_string&
    replace(const_iterator tfirst, const_iterator tlast, const value_type* s, size_type n)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, s, n);
        return *this;
      }

    basic_cow_string&
    replace(const_iterator tfirst, const_iterator tlast, const value_type* s)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, s);
        return *this;
      }

    basic_cow_string&
    replace(const_iterator tfirst, const_iterator tlast, size_type n, value_type ch)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, n, ch);
        return *this;
      }

    basic_cow_string&
    replace(const_iterator tfirst, const_iterator tlast, initializer_list<value_type> init)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, init);
        return *this;
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string&
    replace(const_iterator tfirst, const_iterator tlast, inputT first, inputT last)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append,
                                        ::std::move(first), ::std::move(last));
        return *this;
      }

    // N.B. This is a non-standard extension.
    basic_cow_string&
    replace(const_iterator tfirst, const_iterator tlast, value_type ch)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::push_back, ch);
        return *this;
      }

    size_type
    copy(value_type* s, size_type tn, size_type tpos = 0)
    const
      {
        auto rlen = this->do_clamp_substr(tpos, tn);
        traits_type::copy(s, this->data() + tpos, rlen);
        return rlen;
      }

    basic_cow_string&
    swap(basic_cow_string& other)
    noexcept
      {
        noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        noadl::xswap(this->m_ptr, other.m_ptr);
        noadl::xswap(this->m_len, other.m_len);
        return *this;
      }

    // 24.3.2.7, string operations
    const value_type*
    data()
    const noexcept
      { return this->m_ptr;  }

    const value_type*
    c_str()
    const noexcept
      { return this->m_ptr;  }

    // N.B. This is a non-standard extension.
    const value_type*
    safe_c_str()
    const
      {
        auto clen = traits_type::length(this->m_ptr);
        if(clen != this->m_len)
          noadl::sprintf_and_throw<domain_error>("cow_string: embedded null character detected (at `%llu`)",
                                                 static_cast<unsigned long long>(clen));
        return this->m_ptr;
      }

    // Get a pointer to mutable data. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    value_type*
    mut_data()
      {
        if(ROCKET_UNEXPECT(!this->empty() && !this->unique())) {
          return this->do_reallocate(0, 0, this->size(), this->size() | 1);
        }
        return this->m_sth.mut_data_unchecked();
      }

    // N.B. The return type differs from `std::basic_string`.
    constexpr
    const allocator_type&
    get_allocator()
    const noexcept
      { return this->m_sth.as_allocator();  }

    allocator_type&
    get_allocator()
    noexcept
      { return this->m_sth.as_allocator();  }

    size_type
    find(const basic_cow_string& other, size_type from = 0)
    const noexcept
      { return this->find(other.data(), from, other.size());  }

    // N.B. This is a non-standard extension.
    size_type
    find(const basic_cow_string& other, size_type from, size_type pos, size_type n = npos)
    const
      { return this->find(other.data() + pos, from, other.do_clamp_substr(pos, n));  }

    size_type
    find(const value_type* s, size_type from, size_type n)
    const noexcept
      { return this->do_find_forwards_if(from, n,
                   [&](const value_type* ts) { return traits_type::compare(ts, s, n) == 0;  });  }

    size_type
    find(const value_type* s, size_type from = 0)
    const noexcept
      { return this->find(s, from, traits_type::length(s));  }

    size_type
    find(value_type ch, size_type from = 0)
    const noexcept
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

    size_type
    rfind(const basic_cow_string& other, size_type to = npos)
    const noexcept
      { return this->rfind(other.data(), to, other.size());  }

    // N.B. This is a non-standard extension.
    size_type
    rfind(const basic_cow_string& other, size_type to, size_type pos, size_type n = npos)
    const
      { return this->rfind(other.data() + pos, to, other.do_clamp_substr(pos, n));  }

    size_type
    rfind(const value_type* s, size_type to, size_type n)
    const noexcept
      { return this->do_find_backwards_if(to, n,
                   [&](const value_type* ts) { return traits_type::compare(ts, s, n) == 0;  });  }

    size_type
    rfind(const value_type* s, size_type to = npos)
    const noexcept
      { return this->rfind(s, to, traits_type::length(s));  }

    size_type
    rfind(value_type ch, size_type to = npos)
    const noexcept
      { return this->do_find_backwards_if(to, 1,
                   [&](const value_type* ts) { return traits_type::eq(*ts, ch);  });  }

    size_type
    find_first_of(const basic_cow_string& other, size_type from = 0)
    const noexcept
      { return this->find_first_of(other.data(), from, other.size());  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_of(const basic_cow_string& other, size_type from, size_type pos, size_type n = npos)
    const
      { return this->find_first_of(other.data() + pos, from, other.do_clamp_substr(pos, n));  }

    size_type
    find_first_of(const value_type* s, size_type from, size_type n)
    const noexcept
      { return this->do_find_forwards_if(from, 1,
                   [&](const value_type* ts) { return traits_type::find(s, n, *ts) != nullptr;  });  }

    size_type
    find_first_of(const value_type* s, size_type from = 0)
    const noexcept
      { return this->find_first_of(s, from, traits_type::length(s));  }

    size_type
    find_first_of(value_type ch, size_type from = 0)
    const noexcept
      { return this->find(ch, from);  }

    size_type
    find_last_of(const basic_cow_string& other, size_type to = npos)
    const noexcept
      { return this->find_last_of(other.data(), to, other.size());  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_of(const basic_cow_string& other, size_type to, size_type pos, size_type n = npos)
    const
      { return this->find_last_of(other.data() + pos, to, other.do_clamp_substr(pos, n));  }

    size_type
    find_last_of(const value_type* s, size_type to, size_type n)
    const noexcept
      { return this->do_find_backwards_if(to, 1,
                   [&](const value_type* ts) { return traits_type::find(s, n, *ts) != nullptr;  });  }

    size_type
    find_last_of(const value_type* s, size_type to = npos)
    const noexcept
      { return this->find_last_of(s, to, traits_type::length(s));  }

    size_type
    find_last_of(value_type ch, size_type to = npos)
    const noexcept
      { return this->rfind(ch, to);  }

    size_type
    find_first_not_of(const basic_cow_string& other, size_type from = 0)
    const noexcept
      { return this->find_first_not_of(other.data(), from, other.size());  }

    // N.B. This is a non-standard extension.
    size_type
    find_first_not_of(const basic_cow_string& other, size_type from, size_type pos, size_type n = npos)
    const
      { return this->find_first_not_of(other.data() + pos, from, other.do_clamp_substr(pos, n));  }

    size_type
    find_first_not_of(const value_type* s, size_type from, size_type n)
    const noexcept
      { return this->do_find_forwards_if(from, 1,
                   [&](const value_type* ts) { return traits_type::find(s, n, *ts) == nullptr;  });  }

    size_type
    find_first_not_of(const value_type* s, size_type from = 0)
    const noexcept
      { return this->find_first_not_of(s, from, traits_type::length(s));  }

    size_type
    find_first_not_of(value_type ch, size_type from = 0)
    const noexcept
      { return this->do_find_forwards_if(from, 1,
                   [&](const value_type* ts) { return !traits_type::eq(*ts, ch);  });  }

    size_type
    find_last_not_of(const basic_cow_string& other, size_type to = npos)
    const noexcept
      { return this->find_last_not_of(other.data(), to, other.size());  }

    // N.B. This is a non-standard extension.
    size_type
    find_last_not_of(const basic_cow_string& other, size_type to, size_type pos, size_type n = npos)
    const
      { return this->find_last_not_of(other.data() + pos, to, other.do_clamp_substr(pos, n));  }

    size_type
    find_last_not_of(const value_type* s, size_type to, size_type n)
    const noexcept
      { return this->do_find_backwards_if(to, 1,
                   [&](const value_type* ts) { return traits_type::find(s, n, *ts) == nullptr;  });  }

    size_type
    find_last_not_of(const value_type* s, size_type to = npos)
    const noexcept
      { return this->find_last_not_of(s, to, traits_type::length(s));  }

    size_type
    find_last_not_of(value_type ch, size_type to = npos)
    const noexcept
      { return this->do_find_backwards_if(to, 1,
                   [&](const value_type* ts) { return !traits_type::eq(*ts, ch);  });  }

    // N.B. This is a non-standard extension.
    template<typename predT>
    size_type
    find_first_if(predT pred, size_type from = 0)
    const
      { return this->do_find_forwards_if(from, 1, [&](const char* p) { return pred(*p);  });  }

    // N.B. This is a non-standard extension.
    template<typename predT>
    size_type
    find_last_if(predT pred, size_type to = npos)
    const
      { return this->do_find_backwards_if(to, 1, [&](const char* p) { return pred(*p);  });  }

    // N.B. There is no default argument for `tpos`.
    basic_cow_string
    substr(size_type tpos, size_type tn = npos)
    const
      {
        if((tpos == 0) && (tn >= this->size()))
          // Utilize reference counting.
          return basic_cow_string(*this, this->m_sth.as_allocator());
        else
          return basic_cow_string(*this, tpos, tn, this->m_sth.as_allocator());
      }

    int
    compare(const basic_cow_string& other)
    const noexcept
      { return this->compare(other.data(), other.size());  }

    // N.B. This is a non-standard extension.
    int
    compare(const basic_cow_string& other, size_type pos, size_type n = npos)
    const
      { return this->compare(other.data() + pos, other.do_clamp_substr(pos, n));  }

    // N.B. This is a non-standard extension.
    int
    compare(const value_type* s, size_type n)
    const noexcept
      { return details_cow_string::comparator<charT, traitsT>::relation(
                   this->data(), this->size(), s, n);  }

    int
    compare(const value_type* s)
    const noexcept
      { return this->compare(s, traits_type::length(s));  }

    // N.B. The last two parameters are non-standard extensions.
    int
    compare(size_type tpos, size_type tn, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
    const
      { return this->compare(tpos, tn, other.data() + pos, other.do_clamp_substr(pos, n));  }

    int
    compare(size_type tpos, size_type tn, const value_type* s, size_type n)
    const
      { return details_cow_string::comparator<charT, traitsT>::relation(
                   this->data() + tpos, this->do_clamp_substr(tpos, tn), s, n);  }

    int
    compare(size_type tpos, size_type tn, const value_type* s)
    const
      { return this->compare(tpos, tn, s, traits_type::length(s));  }

    // N.B. This is a non-standard extension.
    int
    compare(size_type tpos, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
    const
      { return this->compare(tpos, npos, other, pos, n);  }

    // N.B. This is a non-standard extension.
    int
    compare(size_type tpos, const value_type* s, size_type n)
    const
      { return this->compare(tpos, npos, s, n);  }

    // N.B. This is a non-standard extension.
    int
    compare(size_type tpos, const value_type* s)
    const
      { return this->compare(tpos, npos, s);  }

    // N.B. These are extensions but might be standardized in C++20.
    bool
    starts_with(const basic_cow_string& other)
    const noexcept
      { return this->starts_with(other.data(), other.size());  }

    // N.B. This is a non-standard extension.
    bool
    starts_with(const basic_cow_string& other, size_type pos, size_type n = npos)
    const
      { return this->starts_with(other.data() + pos, other.do_clamp_substr(pos, n));  }

    bool
    starts_with(const value_type* s, size_type n)
    const noexcept
      { return (n <= this->size()) && (traits_type::compare(this->data(), s, n) == 0);  }

    bool
    starts_with(const value_type* s)
    const noexcept
      { return this->starts_with(s, traits_type::length(s));  }

    bool
    starts_with(value_type ch)
    const noexcept
      { return (1 <= this->size()) && traits_type::eq(this->front(), ch);  }

    bool
    ends_with(const basic_cow_string& other)
    const noexcept
      { return this->ends_with(other.data(), other.size());  }

    // N.B. This is a non-standard extension.
    bool
    ends_with(const basic_cow_string& other, size_type pos, size_type n = npos)
    const
      { return this->ends_with(other.data() + pos, other.do_clamp_substr(pos, n));  }

    bool
    ends_with(const value_type* s, size_type n)
    const noexcept
      { return (n <= this->size()) && (traits_type::compare(this->data() + this->size() - n, s, n) == 0);  }

    bool
    ends_with(const value_type* s)
    const noexcept
      { return this->ends_with(s, traits_type::length(s));  }

    bool
    ends_with(value_type ch)
    const noexcept
      { return (1 <= this->size()) && traits_type::eq(this->back(), ch);  }
  };

#if __cpp_inline_variables + 0 < 201606  // < c++17
template<typename charT, typename traitsT, typename allocT>
const typename basic_cow_string<charT, traitsT, allocT>::size_type basic_cow_string<charT, traitsT, allocT>::npos;
#endif

template<typename charT, typename traitsT, typename allocT>
const charT basic_cow_string<charT, traitsT, allocT>::null_char[1] = { };

template<typename charT, typename traitsT, typename allocT>
struct basic_cow_string<charT, traitsT, allocT>::hash
  {
    using result_type    = size_t;
    using argument_type  = basic_cow_string;

    constexpr
    result_type
    operator()(const argument_type& str)
    const noexcept
      { return details_cow_string::basic_hasher<charT, traitsT>().append(str.data(), str.size()).finish();  }

    constexpr
    result_type
    operator()(const charT* s)
    const noexcept
      { return details_cow_string::basic_hasher<charT, traitsT>().append(s).finish();  }
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
inline
basic_cow_string<charT, traitsT, allocT>
operator+(const basic_cow_string<charT, traitsT, allocT>& lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
  {
    auto res = lhs;
    res.append(rhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(basic_cow_string<charT, traitsT, allocT>&& lhs, basic_cow_string<charT, traitsT, allocT>&& rhs)
  {
    auto ntotal = lhs.size() + rhs.size();
    if(ROCKET_EXPECT((ntotal <= lhs.capacity()) || (ntotal > rhs.capacity())))
      return ::std::move(lhs.append(rhs));
    else
      return ::std::move(rhs.insert(0, lhs));
  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(basic_cow_string<charT, traitsT, allocT>&& lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
  { return ::std::move(lhs.append(rhs));  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(const basic_cow_string<charT, traitsT, allocT>& lhs, basic_cow_string<charT, traitsT, allocT>&& rhs)
  { return ::std::move(rhs.insert(0, lhs));  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs)
  {
    auto res = lhs;
    res.append(rhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(const basic_cow_string<charT, traitsT, allocT>& lhs, charT rhs)
  {
    auto res = lhs;
    res.push_back(rhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
  {
    auto res = rhs;
    res.insert(0, lhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(charT lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
  {
    auto res = rhs;
    res.insert(0, 1, lhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(basic_cow_string<charT, traitsT, allocT>&& lhs, const charT* rhs)
  { return ::std::move(lhs.append(rhs));  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(basic_cow_string<charT, traitsT, allocT>&& lhs, charT rhs)
  { return ::std::move(lhs.append(1, rhs));  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(const charT* lhs, basic_cow_string<charT, traitsT, allocT>&& rhs)
  { return ::std::move(rhs.insert(0, lhs));  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_cow_string<charT, traitsT, allocT>
operator+(charT lhs, basic_cow_string<charT, traitsT, allocT>&& rhs)
  { return ::std::move(rhs.insert(0, 1, lhs));  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator==(const basic_cow_string<charT, traitsT, allocT>& lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) == 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator!=(const basic_cow_string<charT, traitsT, allocT>& lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) != 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator<(const basic_cow_string<charT, traitsT, allocT>& lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) < 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator>(const basic_cow_string<charT, traitsT, allocT>& lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) > 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator<=(const basic_cow_string<charT, traitsT, allocT>& lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) <= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator>=(const basic_cow_string<charT, traitsT, allocT>& lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.data(), rhs.size()) >= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator==(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) == 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator!=(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) != 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator<(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) < 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator>(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) > 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator<=(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) <= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator>=(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) >= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator==(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) == 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator!=(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) != 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator<(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) < 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator>(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) > 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator<=(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) <= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator>=(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs, traitsT::length(lhs), rhs.data(), rhs.size()) >= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator==(const basic_cow_string<charT, traitsT, allocT>& lhs, basic_shallow_string<charT, traitsT> rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) == 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator!=(const basic_cow_string<charT, traitsT, allocT>& lhs, basic_shallow_string<charT, traitsT> rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) != 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator<(const basic_cow_string<charT, traitsT, allocT>& lhs, basic_shallow_string<charT, traitsT> rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) < 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator>(const basic_cow_string<charT, traitsT, allocT>& lhs, basic_shallow_string<charT, traitsT> rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) > 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator<=(const basic_cow_string<charT, traitsT, allocT>& lhs, basic_shallow_string<charT, traitsT> rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) <= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator>=(const basic_cow_string<charT, traitsT, allocT>& lhs, basic_shallow_string<charT, traitsT> rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) >= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator==(basic_shallow_string<charT, traitsT> lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) == 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator!=(basic_shallow_string<charT, traitsT> lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::inequality(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) != 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator<(basic_shallow_string<charT, traitsT> lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) < 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator>(basic_shallow_string<charT, traitsT> lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) > 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator<=(basic_shallow_string<charT, traitsT> lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) <= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
bool
operator>=(basic_shallow_string<charT, traitsT> lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept
  { return details_cow_string::comparator<charT, traitsT>::relation(
               lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) >= 0;  }

template<typename charT, typename traitsT, typename allocT>
inline
void
swap(basic_cow_string<charT, traitsT, allocT>& lhs, basic_cow_string<charT, traitsT, allocT>& rhs)
noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

template<typename charT, typename traitsT, typename allocT>
inline
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const basic_cow_string<charT, traitsT, allocT>& str)
  { return fmt.putn(str.data(), str.size());  }

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
