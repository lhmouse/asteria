// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRING_
#define ROCKET_COW_STRING_

#include "fwd.hpp"
#include "assert.hpp"
#include "throw.hpp"
#include "reference_counter.hpp"
#include "xallocator.hpp"
#include "xstring.hpp"
namespace rocket {

// Differences from `std::basic_string`:
// 1. All functions guarantee only basic exception safety rather than strong
//    exception safety, hence are more efficient.
// 2. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()`
//    and `back()` always return `const_reference`s.
// 3. The copy constructor and copy assignment operator will not throw
//    exceptions.
// 4. The constructor taking a sole const pointer is made `explicit`.
// 5. The assignment operator taking a character and the one taking a const
//    pointer are not provided.
// 6. It is possible to create strings holding non-owning references of
//    null-terminated character arrays allocated externally.
// 7. `data()` returns a null pointer if the string is empty.
// 8. `erase()` and `substr()` cannot be called without arguments.
template<typename charT>
class basic_shallow_string;

template<typename charT, typename allocT = allocator<charT>>
class basic_cow_string;

template<typename charT>
class basic_tinyfmt;

template<typename charT>
class basic_tinybuf;

#include "details/cow_string.ipp"

template<typename charT>
class basic_shallow_string
  {
    static_assert(!is_array<charT>::value, "invalid character type");
    static_assert(!is_reference<charT>::value, "invalid character type");
    static_assert(is_trivial<charT>::value, "characters must be trivial");
    template<typename, typename> friend class basic_cow_string;

  private:
    const charT* m_ptr;
    size_t m_len;

  public:
    ROCKET_ALWAYS_INLINE  // https://gcc.gnu.org/PR109464
    explicit constexpr
    basic_shallow_string(const charT* ptr) noexcept
      : m_ptr(ptr), m_len(noadl::xstrlen(ptr))
      { }

  public:
    constexpr
    const charT*
    c_str() const noexcept
      { return this->m_ptr;  }

    constexpr
    size_t
    length() const noexcept
      { return this->m_len;  }

    constexpr
    const charT*
    data() const noexcept
      { return this->m_ptr;  }

    constexpr
    size_t
    size() const noexcept
      { return this->m_len;  }

    constexpr
    const charT*
    begin() const noexcept
      { return this->m_ptr;  }

    constexpr
    const charT*
    end() const noexcept
      { return this->m_ptr + this->m_len;  }
  };

template<typename charT>
constexpr
basic_shallow_string<charT>
sref(const charT* ptr) noexcept
  {
    return basic_shallow_string<charT>(ptr);
  }

extern template class basic_shallow_string<char>;
extern template class basic_shallow_string<wchar_t>;
extern template class basic_shallow_string<char16_t>;
extern template class basic_shallow_string<char32_t>;

using shallow_string     = basic_shallow_string<char>;
using shallow_wstring    = basic_shallow_string<wchar_t>;
using shallow_u16string  = basic_shallow_string<char16_t>;
using shallow_u32string  = basic_shallow_string<char32_t>;

template<typename charT, typename allocT>
class basic_cow_string
  {
    static_assert(!is_array<charT>::value, "invalid character type");
    static_assert(!is_reference<charT>::value, "invalid character type");
    static_assert(is_trivial<charT>::value, "characters must be trivial");
    static_assert(is_same<typename allocT::value_type, charT>::value, "inappropriate allocator type");

  public:
    // types
    using value_type      = charT;
    using allocator_type  = allocT;
    using shallow_type    = basic_shallow_string<charT>;

    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using difference_type  = typename allocator_traits<allocator_type>::difference_type;
    using const_reference  = const value_type&;
    using reference        = value_type&;

    using const_iterator          = details_cow_string::string_iterator<basic_cow_string, const value_type>;
    using iterator                = details_cow_string::string_iterator<basic_cow_string, value_type>;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

    struct hash;
    static constexpr size_type npos = size_type(-1);

  private:
    static constexpr value_type s_zcstr[1] = { };
    static constexpr shallow_type s_zstr = noadl::sref(s_zcstr);
    static_assert(s_zstr.m_len == 0);

    using storage_handle = details_cow_string::storage_handle<allocator_type>;
    shallow_type m_ref;
    storage_handle m_sth;

  public:
    // 24.3.2.2, construct/copy/destroy
    constexpr
    basic_cow_string() noexcept(is_nothrow_constructible<allocator_type>::value)
      : m_ref(s_zstr), m_sth()
      { }

    explicit constexpr
    basic_cow_string(const allocator_type& alloc) noexcept
      : m_ref(s_zstr), m_sth(alloc)
      { }

    constexpr
    basic_cow_string(shallow_type sh, const allocator_type& alloc = allocator_type()) noexcept
      : m_ref(sh), m_sth(alloc)
      { }

    basic_cow_string(const basic_cow_string& other) noexcept
      : m_ref(other.m_ref),
        m_sth(allocator_traits<allocator_type>::select_on_container_copy_construction(
                                                    other.m_sth.as_allocator()))
      { this->m_sth.share_with(other.m_sth);  }

    basic_cow_string(const basic_cow_string& other, const allocator_type& alloc) noexcept
      : m_ref(other.m_ref), m_sth(alloc)
      { this->m_sth.share_with(other.m_sth);  }

    basic_cow_string(basic_cow_string&& other) noexcept
      : m_ref(noadl::exchange(other.m_ref, s_zstr)), m_sth(::std::move(other.m_sth.as_allocator()))
      { this->m_sth.exchange_with(other.m_sth);  }

    basic_cow_string(basic_cow_string&& other, const allocator_type& alloc) noexcept
      : m_ref(noadl::exchange(other.m_ref, s_zstr)), m_sth(alloc)
      { this->m_sth.exchange_with(other.m_sth);  }

    basic_cow_string(initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(init);  }

    basic_cow_string(const basic_cow_string& other, size_type pos, size_type n = npos,
                     const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(other, pos, n);  }

    explicit  // no implicit conversion from string literal; see `sref()`.
    basic_cow_string(const value_type* s, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(s);  }

    basic_cow_string(const value_type* s, size_type n, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(s, n);  }

    basic_cow_string(size_type n, value_type c, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(n, c);  }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string(inputT first, inputT last, const allocator_type& alloc = allocator_type())
      : basic_cow_string(alloc)
      { this->append(::std::move(first), ::std::move(last));  }

    basic_cow_string&
    operator=(shallow_type sh) & noexcept
      {
        this->m_ref = sh;
        return *this;
      }

    basic_cow_string&
    operator=(const basic_cow_string& other) & noexcept
      {
        noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.share_with(other.m_sth);
        this->m_ref = other.m_ref;
        return *this;
      }

    basic_cow_string&
    operator=(basic_cow_string&& other) & noexcept
      {
        noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        this->m_ref = noadl::exchange(other.m_ref, s_zstr);
        return *this;
      }

    basic_cow_string&
    operator=(initializer_list<value_type> init) &
      {
        this->clear();
        this->append(init);
        return *this;
      }

    basic_cow_string&
    swap(basic_cow_string& other) noexcept
      {
        noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        ::std::swap(this->m_ref.m_ptr, other.m_ref.m_ptr);
        ::std::swap(this->m_ref.m_len, other.m_ref.m_len);
        return *this;
      }

  private:
    basic_cow_string&
    do_deallocate() noexcept
      {
        this->m_sth.deallocate();
        this->m_ref = s_zstr;
        return *this;
      }

    void
    do_set_data_and_size(value_type* ptr, size_type n) noexcept
      {
        ptr[n] = value_type();
        this->m_ref.m_ptr = ptr;
        this->m_ref.m_len = n;
      }

    [[noreturn]] ROCKET_NEVER_INLINE
    void
    do_throw_subscript_out_of_range(size_type pos, unsigned char rel) const
      {
        static constexpr char opstr[6][3] = { "==", "<", "<=", ">", ">=", "!=" };
        unsigned int inv = 5U - rel;

        noadl::sprintf_and_throw<out_of_range>(
            "basic_cow_string: subscript out of range (`%lld` %s `%lld`)",
            static_cast<long long>(pos), opstr[inv], static_cast<long long>(this->size()));
      }

#define ROCKET_COW_STRING_VALIDATE_SUBSCRIPT_(pos, op)  \
        if(!(pos op this->size()))  \
          this->do_throw_subscript_out_of_range(pos,  \
                  ((2 op 1) * 4 + (1 op 2) * 2 + (1 op 1) - 1))

    // This function ensures `tpos` is within range and returns the number of
    // elements that start there.
    constexpr
    size_type
    do_clamp_substr(size_type tpos, size_type tn) const
      {
        ROCKET_COW_STRING_VALIDATE_SUBSCRIPT_(tpos, <=);
        return noadl::min(this->size() - tpos, tn);
      }

    // This function is used to implement `replace()` after new characters
    // have been appended. `tpos` and `tlen` denote the interval to replace.
    // `old_size` is the old size before `append()`.
    value_type*
    do_swizzle_unchecked(size_type tpos, size_type tlen, size_type old_size)
      {
        auto ptr = this->mut_data();
        noadl::rotate(ptr, tpos, tpos + tlen, this->m_ref.m_len + 1);  // with null terminator
        this->m_ref.m_len -= tlen;
        noadl::rotate(ptr, tpos, old_size - tlen, this->m_ref.m_len);
        return ptr + tpos;
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
    constexpr
    bool
    empty() const noexcept
      { return this->m_ref.m_len == 0;  }

    constexpr
    size_type
    size() const noexcept
      { return this->m_ref.m_len;  }

    constexpr
    size_type
    length() const noexcept
      { return this->m_ref.m_len;  }

    // N.B. This is a non-standard extension.
    constexpr
    difference_type
    ssize() const noexcept
      { return static_cast<difference_type>(this->size());  }

    constexpr
    size_type
    max_size() const noexcept
      { return this->m_sth.max_size();  }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    resize(size_type n, value_type c = value_type())
      {
        return (this->size() < n)
                 ? this->append(n - this->size(), c)
                 : this->pop_back(this->size() - n);
      }

    constexpr
    size_type
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
        this->m_sth.exchange_with(sth);
        this->m_ref.m_ptr = ptr;
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
        this->m_sth.exchange_with(sth);
        this->m_ref.m_ptr = ptr;
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    clear() noexcept
      {
        this->m_ref = s_zstr;
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
        ROCKET_COW_STRING_VALIDATE_SUBSCRIPT_(pos, <);
        return this->data()[pos];
      }

    // N.B. This is a non-standard extension.
    const value_type*
    ptr(size_type pos) const noexcept
      {
        return (pos <= this->size())
                 ? (this->data() + pos) : nullptr;
      }

    constexpr
    const_reference
    operator[](size_type pos) const noexcept
      {
        // Note reading from the character at `size()` is permitted.
        ROCKET_ASSERT(pos <= this->size());
        return this->c_str()[pos];
      }

    constexpr
    const_reference
    front() const noexcept
      {
        ROCKET_ASSERT(!this->empty());
        return this->data()[0];
      }

    constexpr
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
        ROCKET_COW_STRING_VALIDATE_SUBSCRIPT_(pos, <);
        return this->mut_data()[pos];
      }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    value_type*
    mut_ptr(size_type pos)
      {
        return (pos <= this->size())
                 ? (this->mut_data() + pos) : nullptr;
      }

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
    append(shallow_type sh)
      {
        return this->append(sh.data(), sh.size());
      }

    basic_cow_string&
    append(initializer_list<value_type> init)
      {
        return this->append(init.begin(), init.size());
      }

    basic_cow_string&
    append(const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        return this->append(other.data() + pos, other.do_clamp_substr(pos, n));
      }

    basic_cow_string&
    append(const value_type* s)
      {
        return this->append(s, noadl::xstrlen(s));
      }

    basic_cow_string&
    append(const value_type* s, size_type n)
      {
        if(n == 0)
          return *this;

        // Check whether the storage is unique and there is enough space.
        auto ptr = this->m_sth.mut_data_opt();
        size_type cap = this->capacity();
        size_type len = this->size();

        if(ROCKET_EXPECT(ptr && (n <= cap - len))) {
          ::memmove(ptr + len, s, n * sizeof(value_type));
          len += n;
          this->do_set_data_and_size(ptr, len);
          return *this;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        ptr = sth.reallocate_more(this->m_ref.m_ptr, len, n | cap / 2);
        ::memcpy(ptr + len, s, n * sizeof(value_type));
        len += n;
        this->m_sth.exchange_with(sth);
        this->do_set_data_and_size(ptr, len);
        return *this;
      }

    basic_cow_string&
    append(size_type n, value_type c)
      {
        if(n == 0)
          return *this;

        // Check whether the storage is unique and there is enough space.
        auto ptr = this->m_sth.mut_data_opt();
        size_type cap = this->capacity();
        size_type len = this->size();

        if(ROCKET_EXPECT(ptr && (n <= cap - len))) {
          noadl::xmempset(ptr + len, c, n);
          len += n;
          this->do_set_data_and_size(ptr, len);
          return *this;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        ptr = sth.reallocate_more(this->m_ref.m_ptr, len, n | cap / 2);
        noadl::xmempset(ptr + len, c, n);
        len += n;
        this->m_sth.exchange_with(sth);
        this->do_set_data_and_size(ptr, len);
        return *this;
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string&
    append(inputT first, inputT last)
      {
        if(first == last)
          return *this;

        size_t dist = noadl::estimate_distance(first, last);
        size_type n = static_cast<size_type>(dist);

        // Check whether the storage is unique and there is enough space.
        auto ptr = this->m_sth.mut_data_opt();
        size_type cap = this->capacity();
        size_type len = this->size();

        if(ROCKET_EXPECT(dist && (dist == n) && ptr && (n <= cap - len))) {
          for(auto it = ::std::move(first);  it != last;  ++it)
            ptr[len++] = *it;
          this->do_set_data_and_size(ptr, len);
          return *this;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        if(ROCKET_EXPECT(dist && (dist == n))) {
          // The length is known.
          ptr = sth.reallocate_more(this->m_ref.m_ptr, len, n | cap / 2);
          for(auto it = ::std::move(first);  it != last;  ++it)
            ptr[len++] = *it;
        }
        else {
          // The length is not known.
          ptr = sth.reallocate_more(this->m_ref.m_ptr, len, 17 | cap / 2);
          cap = sth.capacity();
          for(auto it = ::std::move(first);  it != last;  ++it) {
            if(ROCKET_UNEXPECT(len >= cap)) {
              ptr = sth.reallocate_more(ptr, len, cap / 2);
              cap = sth.capacity();
            }
            ptr[len++] = *it;
          }
        }
        this->m_sth.exchange_with(sth);
        this->do_set_data_and_size(ptr, len);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    basic_cow_string&
    push_back(value_type c)
      {
        this->append(size_type(1), c);
        return *this;
      }

    basic_cow_string&
    operator+=(shallow_type sh) &
      {
        return this->append(sh);
      }

    basic_cow_string&
    operator+=(initializer_list<value_type> init) &
      {
        return this->append(init);
      }

    basic_cow_string&
    operator+=(const basic_cow_string& other) &
      {
        return this->append(other);
      }

    basic_cow_string&
    operator+=(const value_type* s) &
      {
        return this->append(s);
      }

    basic_cow_string&
    operator+=(value_type c) &
      {
        return this->push_back(c);
      }

    basic_cow_string&
    operator<<(shallow_type sh) &
      {
        return this->append(sh);
      }

    basic_cow_string&
    operator<<(initializer_list<value_type> init) &
      {
        return this->append(init);
      }

    basic_cow_string&
    operator<<(const basic_cow_string& other) &
      {
        return this->append(other);
      }

    basic_cow_string&
    operator<<(const value_type* s) &
      {
        return this->append(s);
      }

    basic_cow_string&
    operator<<(value_type c) &
      {
        return this->push_back(c);
      }

    basic_cow_string&&
    operator<<(shallow_type sh) &&
      {
        return ::std::move(this->append(sh));
      }

    basic_cow_string&&
    operator<<(initializer_list<value_type> init) &&
      {
        return ::std::move(this->append(init));
      }

    basic_cow_string&&
    operator<<(const basic_cow_string& other) &&
      {
        return ::std::move(this->append(other));
      }

    basic_cow_string&&
    operator<<(const value_type* s) &&
      {
        return ::std::move(this->append(s));
      }

    basic_cow_string&&
    operator<<(value_type c) &&
      {
        return ::std::move(this->push_back(c));
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, shallow_type sh)
      {
        return this->replace(tpos, tn, sh.data(), sh.size());
      }

    basic_cow_string&
    replace(size_type tpos, shallow_type sh)
      {
        return this->replace(tpos, npos, sh);
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, initializer_list<value_type> init)
      {
        return this->replace(tpos, tn, init.begin(), init.size());
      }

    basic_cow_string&
    replace(size_type tpos, initializer_list<value_type> init)
      {
        return this->replace(tpos, npos, init);
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        return this->replace(tpos, tn, other.data() + pos, other.do_clamp_substr(pos, n));
      }

    basic_cow_string&
    replace(size_type tpos, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        return this->replace(tpos, npos, other, pos, n);
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, const value_type* s)
      {
        return this->replace(tpos, tn, s, noadl::xstrlen(s));
      }

    basic_cow_string&
    replace(size_type tpos, const value_type* s)
      {
        return this->replace(tpos, npos, s);
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, const value_type* s, size_type n)
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);
        size_type old_size = this->size();
        this->append(s, n);
        this->do_swizzle_unchecked(tpos, tlen, old_size);
        return *this;
      }

    basic_cow_string&
    replace(size_type tpos, const value_type* s, size_type n)
      {
        return this->replace(tpos, npos, s, n);
      }

    basic_cow_string&
    replace(size_type tpos, size_type tn, size_type n, value_type c)
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);
        size_type old_size = this->size();
        this->append(n, c);
        this->do_swizzle_unchecked(tpos, tlen, old_size);
        return *this;
      }

    basic_cow_string&
    replace(size_type tpos, size_type n, value_type c)
      {
        return this->replace(tpos, npos, n, c);
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string&
    replace(size_type tpos, size_type tn, inputT first, inputT last)
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);
        size_type old_size = this->size();
        this->append(::std::move(first), ::std::move(last));
        this->do_swizzle_unchecked(tpos, tlen, old_size);
        return *this;
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string&
    replace(size_type tpos, inputT first, inputT last)
      {
        return this->replace(tpos, npos, ::std::move(first), ::std::move(last));
      }

    iterator
    replace(const_iterator tfirst, const_iterator tlast, shallow_type sh)
      {
        return this->replace(tfirst, tlast, sh.data(), sh.size());
      }

    iterator
    replace(const_iterator tfirst, const_iterator tlast, initializer_list<value_type> init)
      {
        return this->replace(tfirst, tlast, init.begin(), init.size());
      }

    iterator
    replace(const_iterator tfirst, const_iterator tlast, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        return this->replace(tfirst, tlast, other.data() + pos, other.do_clamp_substr(pos, n));
      }

    iterator
    replace(const_iterator tfirst, const_iterator tlast, const value_type* s)
      {
        return this->replace(tfirst, tlast, s, noadl::xstrlen(s));
      }

    iterator
    replace(const_iterator tfirst, const_iterator tlast, const value_type* s, size_type n)
      {
        ROCKET_ASSERT_MSG(tfirst <= tlast, "invalid range");
        size_type tpos = static_cast<size_type>(tfirst - this->begin());
        size_type tlen = static_cast<size_type>(tlast - tfirst);
        size_type old_size = this->size();
        this->append(s, n);
        auto tptr = this->do_swizzle_unchecked(tpos, tlen, old_size);
        return iterator(tptr - tpos, tpos, this->size());
      }

    iterator
    replace(const_iterator tfirst, const_iterator tlast, size_type n, value_type c)
      {
        ROCKET_ASSERT_MSG(tfirst <= tlast, "invalid range");
        size_type tpos = static_cast<size_type>(tfirst - this->begin());
        size_type tlen = static_cast<size_type>(tlast - tfirst);
        size_type old_size = this->size();
        this->append(n, c);
        auto tptr = this->do_swizzle_unchecked(tpos, tlen, old_size);
        return iterator(tptr - tpos, tpos, this->size());
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    iterator
    replace(const_iterator tfirst, const_iterator tlast, inputT first, inputT last)
      {
        ROCKET_ASSERT_MSG(tfirst <= tlast, "invalid range");
        size_type tpos = static_cast<size_type>(tfirst - this->begin());
        size_type tlen = static_cast<size_type>(tlast - tfirst);
        size_type old_size = this->size();
        this->append(::std::move(first), ::std::move(last));
        auto tptr = this->do_swizzle_unchecked(tpos, tlen, old_size);
        return iterator(tptr - tpos, tpos, this->size());
      }

    basic_cow_string&
    insert(size_type tpos, shallow_type sh)
      {
        return this->replace(tpos, size_type(0), sh);
      }

    basic_cow_string&
    insert(size_type tpos, initializer_list<value_type> init)
      {
        return this->replace(tpos, size_type(0), init);
      }

    basic_cow_string&
    insert(size_type tpos, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        return this->replace(tpos, size_type(0), other, pos, n);
      }

    basic_cow_string&
    insert(size_type tpos, const value_type* s)
      {
        return this->replace(tpos, size_type(0), s);
      }

    basic_cow_string&
    insert(size_type tpos, const value_type* s, size_type n)
      {
        return this->replace(tpos, size_type(0), s, n);
      }

    basic_cow_string&
    insert(size_type tpos, size_type n, value_type c)
      {
        return this->replace(tpos, size_type(0), n, c);
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string&
    insert(size_type tpos, inputT first, inputT last)
      {
        return this->replace(tpos, size_type(0), ::std::move(first), ::std::move(last));
      }

    iterator
    insert(const_iterator tins, shallow_type sh)
      {
        return this->replace(tins, tins, sh);
      }

    iterator
    insert(const_iterator tins, initializer_list<value_type> init)
      {
        return this->replace(tins, tins, init);
      }

    iterator
    insert(const_iterator tins, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        return this->replace(tins, tins, other, pos, n);
      }

    iterator
    insert(const_iterator tins, const value_type* s)
      {
        return this->replace(tins, tins, s);
      }

    iterator
    insert(const_iterator tins, const value_type* s, size_type n)
      {
        return this->replace(tins, tins, s, n);
      }

    iterator
    insert(const_iterator tins, size_type n, value_type c)
      {
        return this->replace(tins, tins, n, c);
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    iterator
    insert(const_iterator tins, inputT first, inputT last)
      {
        return this->replace(tins, tins, ::std::move(first), ::std::move(last));
      }

    basic_cow_string&
    erase(size_type tpos, size_type tn = npos)
      {
        return this->replace(tpos, tn, size_type(0), value_type());
      }

    iterator
    erase(const_iterator pos)
      {
        return this->replace(pos, pos + 1, size_type(0), value_type());
      }

    iterator
    erase(const_iterator first, const_iterator last)
      {
        return this->replace(first, last, size_type(0), value_type());
      }

    // N.B. The return type and parameter are non-standard extensions.
    basic_cow_string&
    pop_back(size_type n = 1)
      {
        ROCKET_ASSERT_MSG(n <= this->size(), "no enough characters to pop");

        // If the string is empty, `n` must be zero, so there is nothing to do.
        if(this->empty())
          return *this;

        // If the storage is unique, modify it in place.
        auto ptr = this->m_sth.mut_data_opt();
        if(ROCKET_EXPECT(ptr)) {
          this->do_set_data_and_size(ptr, this->size() - n);
          return *this;
        }

        // Reallocate the storage.
        ptr = this->m_sth.reallocate_more(this->data(), this->size() - n, 0);
        this->do_set_data_and_size(ptr, this->size() - n);
        return *this;
      }

    // N.B. There is no default argument for `tpos`.
    basic_cow_string
    substr(size_type tpos, size_type tn = npos) const
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);
        basic_cow_string res(this->m_sth.as_allocator());
        res.append(this->data() + tpos, tlen);
        return res;
      }

    // N.B. This is a non-standard extension.
    size_type
    copy(size_type tpos, value_type* s, size_type tn) const
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);
        ::memmove(s, this->data() + tpos, tlen * sizeof(value_type));
        return tlen;
      }

    size_type
    copy(value_type* s, size_type tn) const
      {
        return this->copy(size_type(0), s, tn);
      }

    basic_cow_string&
    assign(shallow_type sh)
      {
        this->replace(this->begin(), this->end(), sh);
        return *this;
      }

    basic_cow_string&
    assign(initializer_list<value_type> init)
      {
        this->replace(this->begin(), this->end(), init);
        return *this;
      }

    basic_cow_string&
    assign(const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        this->replace(this->begin(), this->end(), other, pos, n);
        return *this;
      }

    basic_cow_string&
    assign(const value_type* s)
      {
        this->replace(this->begin(), this->end(), s);
        return *this;
      }

    basic_cow_string&
    assign(const value_type* s, size_type n)
      {
        this->replace(this->begin(), this->end(), s, n);
        return *this;
      }

    basic_cow_string&
    assign(size_type n, value_type c)
      {
        this->replace(this->begin(), this->end(), n, c);
        return *this;
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    basic_cow_string&
    assign(inputT first, inputT last)
      {
        this->replace(this->begin(), this->end(), ::std::move(first), ::std::move(last));
        return *this;
      }

    // 24.3.2.7, string operations
    constexpr
    const value_type*
    data() const noexcept
      { return this->m_ref.m_ptr;  }

    constexpr
    const value_type*
    c_str() const noexcept
      { return this->m_ref.m_ptr;  }

    // N.B. This is a non-standard extension.
    const value_type*
    safe_c_str() const
      {
        size_type clen = noadl::xstrlen(this->c_str());
        if(clen != this->length())
          noadl::sprintf_and_throw<domain_error>(
              "basic_cow_string: embedded null character detected at `%lld`",
              static_cast<long long>(clen));

        return this->c_str();
      }

    // Get a pointer to mutable data. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    ROCKET_ALWAYS_INLINE
    value_type*
    mut_data()
      {
        auto ptr = this->m_sth.mut_data_opt();
        if(ROCKET_EXPECT(ptr))
          return ptr;

        // If the string is empty, return a pointer to constant storage. The
        // null terminator shall not be modified.
        if(this->empty())
          return const_cast<value_type*>(s_zstr.m_ptr);

        // Reallocate the storage. The length is left intact.
        ptr = this->m_sth.reallocate_more(this->data(), this->size(), 0);
        this->m_ref.m_ptr = ptr;
        return ptr;
      }

    // N.B. The return type differs from `std::basic_string`.
    constexpr
    const allocator_type&
    get_allocator() const noexcept
      { return this->m_sth.as_allocator();  }

    allocator_type&
    get_allocator() noexcept
      { return this->m_sth.as_allocator();  }

    constexpr
    size_type
    find(size_type from, shallow_type sh) const noexcept
      {
        return this->find(from, sh.data(), sh.size());
      }

    constexpr
    size_type
    find(shallow_type sh) const noexcept
      {
        return this->find(size_type(0), sh);
      }

    constexpr
    size_type
    find(size_type from, initializer_list<value_type> init) const noexcept
      {
        return this->find(from, init.begin(), init.size());
      }

    constexpr
    size_type
    find(initializer_list<value_type> init) const noexcept
      {
        return this->find(size_type(0), init);
      }

    constexpr
    size_type
    find(size_type from, const basic_cow_string& other) const noexcept
      {
        return this->find(from, other.data(), other.size());
      }

    constexpr
    size_type
    find(const basic_cow_string& other) const noexcept
      {
        return this->find(size_type(0), other);
      }

    constexpr
    size_type
    find(size_type from, const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->find(from, other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    size_type
    find(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->find(size_type(0), other, pos, n);
      }

    constexpr
    size_type
    find(size_type from, const value_type* s) const noexcept
      {
// TODO
        return this->find(from, s, noadl::xstrlen(s));
      }

    constexpr
    size_type
    find(const value_type* s) const noexcept
      {
        return this->find(size_type(0), s);
      }

    constexpr
    size_type
    find(size_type from, const value_type* s, size_type n) const noexcept
      {
// TODO
for(size_type k = from;  (k <= this->size()) && (this->size() - k >= n);  ++k)
  if(noadl::xmemeq(this->data() + k, s, n))
    return k;
return npos;
      }

    constexpr
    size_type
    find(const value_type* s, size_type n) const noexcept
      {
        return this->find(size_type(0), s, n);
      }

    constexpr
    size_type
    find(size_type from, value_type c) const noexcept
      {
        return this->find_of(from, c);
      }

    constexpr
    size_type
    find(value_type c) const noexcept
      {
        return this->find(size_type(0), c);
      }

    constexpr
    size_type
    rfind(size_type to, shallow_type sh) const noexcept
      {
        return this->rfind(to, sh.data(), sh.size());
      }

    constexpr
    size_type
    rfind(shallow_type sh) const noexcept
      {
        return this->rfind(npos, sh);
      }

    constexpr
    size_type
    rfind(size_type to, initializer_list<value_type> init) const noexcept
      {
        return this->rfind(to, init.begin(), init.size());
      }

    constexpr
    size_type
    rfind(initializer_list<value_type> init) const noexcept
      {
        return this->rfind(npos, init);
      }

    constexpr
    size_type
    rfind(size_type to, const basic_cow_string& other) const noexcept
      {
        return this->rfind(to, other.data(), other.size());
      }

    constexpr
    size_type
    rfind(const basic_cow_string& other) const noexcept
      {
        return this->rfind(npos, other);
      }

    constexpr
    size_type
    rfind(size_type to, const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->rfind(to, other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    size_type
    rfind(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->rfind(npos, other, pos, n);
      }

    constexpr
    size_type
    rfind(size_type to, const value_type* s) const noexcept
      {
// TODO
        return this->rfind(to, s, noadl::xstrlen(s));
      }

    constexpr
    size_type
    rfind(const value_type* s) const noexcept
      {
        return this->rfind(npos, s);
      }

    constexpr
    size_type
    rfind(size_type to, const value_type* s, size_type n) const noexcept
      {
// TODO
if(this->size() < n)
  return npos;
for(size_type k = noadl::min(to, this->size() - n);  k != npos;  --k)
  if(noadl::xmemeq(this->data() + k, s, n))
    return k;
return npos;
      }

    constexpr
    size_type
    rfind(const value_type* s, size_type n) const noexcept
      {
        return this->rfind(npos, s, n);
      }

    constexpr
    size_type
    rfind(size_type to, value_type c) const noexcept
      {
        return this->rfind_of(to, c);
      }

    constexpr
    size_type
    rfind(value_type c) const noexcept
      {
        return this->rfind(npos, c);
      }

    constexpr
    size_type
    find_of(size_type from, shallow_type sh) const noexcept
      {
        return this->find_of(from, sh.data(), sh.size());
      }

    constexpr
    size_type
    find_of(shallow_type sh) const noexcept
      {
        return this->find_of(size_type(0), sh);
      }

    constexpr
    size_type
    find_of(size_type from, initializer_list<value_type> init) const noexcept
      {
        return this->find_of(from, init.begin(), init.size());
      }

    constexpr
    size_type
    find_of(initializer_list<value_type> init) const noexcept
      {
        return this->find_of(size_type(0), init);
      }

    constexpr
    size_type
    find_of(size_type from, const basic_cow_string& other) const noexcept
      {
        return this->find_of(from, other.data(), other.size());
      }

    constexpr
    size_type
    find_of(const basic_cow_string& other) const noexcept
      {
        return this->find_of(size_type(0), other);
      }

    constexpr
    size_type
    find_of(size_type from, const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->find_of(from, other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    size_type
    find_of(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->find_of(size_type(0), other, pos, n);
      }

    constexpr
    size_type
    find_of(size_type from, const value_type* s) const noexcept
      {
        if(this->empty())
          return npos;

        bool bitmap[256] = { };
        int overflow = 0;

        for(auto sptr = s;  *sptr != 0;  ++sptr) {
          // Hash the lowest 8 bits into `bitmap`. If other non-zero bits exist,
          // accumulate them into `overflow`.
          int ch = noadl::xchrtoint(*sptr);
          bitmap[uint8_t(ch)] = true;
          overflow |= ch >> 8;
        }

        for(size_type k = from;  k < this->size();  ++k) {
          ROCKET_ASSERT(k != npos);

          // A null character is not part of the target string.
          if(this->data()[k] == 0)
            continue;

          // If `bitmap` doesn't contain this character, then it cannot be a match.
          int ch = noadl::xchrtoint(this->data()[k]);
          if(bitmap[uint8_t(ch)] == false)
            continue;

          // If `bitmap` has not been overflowed, then it is not necessary to look
          // for it in the target string.
          if(overflow == 0)
            return k;

          // Check whether this is really a match using plain comparison, unlike
          // `std::basic_string` which uses `std::char_traits`.
          if(noadl::xstrchr(s, this->data()[k]) != nullptr)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    find_of(const value_type* s) const noexcept
      {
        return this->find_of(size_type(0), s);
      }

    constexpr
    size_type
    find_of(size_type from, const value_type* s, size_type n) const noexcept
      {
        if(this->empty())
          return npos;

        bool bitmap[256] = { };
        int overflow = 0;

        for(auto sptr = s;  sptr != s + n;  ++sptr) {
          // Hash the lowest 8 bits into `bitmap`. If other non-zero bits exist,
          // accumulate them into `overflow`.
          int ch = noadl::xchrtoint(*sptr);
          bitmap[uint8_t(ch)] = true;
          overflow |= ch >> 8;
        }

        for(size_type k = from;  k < this->size();  ++k) {
          ROCKET_ASSERT(k != npos);

          // If `bitmap` doesn't contain this character, then it cannot be a match.
          int ch = noadl::xchrtoint(this->data()[k]);
          if(bitmap[uint8_t(ch)] == false)
            continue;

          // If `bitmap` has not been overflowed, then it is not necessary to look
          // for it in the target string.
          if(overflow == 0)
            return k;

          // Check whether this is really a match using plain comparison, unlike
          // `std::basic_string` which uses `std::char_traits`.
          if(noadl::xmemchr(s, this->data()[k], n))
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    find_of(const value_type* s, size_type n) const noexcept
      {
        return this->find_of(size_type(0), s, n);
      }

    constexpr
    size_type
    find_of(size_type from, value_type c) const noexcept
      {
        if(this->empty())
          return npos;

        for(size_type k = from;  k < this->size();  ++k) {
          ROCKET_ASSERT(k != npos);

          // Perform plain comparison, unlike `std::basic_string` which uses
          // `std::char_traits`.
          if(this->data()[k] == c)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    find_of(value_type c) const noexcept
      {
        return this->find_of(size_type(0), c);
      }

    constexpr
    size_type
    rfind_of(size_type to, shallow_type sh) const noexcept
      {
        return this->rfind_of(to, sh.data(), sh.size());
      }

    constexpr
    size_type
    rfind_of(shallow_type sh) const noexcept
      {
        return this->rfind_of(npos, sh);
      }

    constexpr
    size_type
    rfind_of(size_type to, initializer_list<value_type> init) const noexcept
      {
        return this->rfind_of(to, init.begin(), init.size());
      }

    constexpr
    size_type
    rfind_of(initializer_list<value_type> init) const noexcept
      {
        return this->rfind_of(npos, init);
      }

    constexpr
    size_type
    rfind_of(size_type to, const basic_cow_string& other) const noexcept
      {
        return this->rfind_of(to, other.data(), other.size());
      }

    constexpr
    size_type
    rfind_of(const basic_cow_string& other) const noexcept
      {
        return this->rfind_of(npos, other);
      }

    constexpr
    size_type
    rfind_of(size_type to, const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->rfind_of(to, other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    size_type
    rfind_of(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->rfind_of(npos, other, pos, n);
      }

    constexpr
    size_type
    rfind_of(size_type to, const value_type* s) const noexcept
      {
        if(this->empty())
          return npos;

        bool bitmap[256] = { };
        int overflow = 0;

        for(auto sptr = s;  *sptr != 0;  ++sptr) {
          // Hash the lowest 8 bits into `bitmap`. If other non-zero bits exist,
          // accumulate them into `overflow`.
          int ch = noadl::xchrtoint(*sptr);
          bitmap[uint8_t(ch)] = true;
          overflow |= ch >> 8;
        }

        for(size_type k = noadl::min(to, this->size() - 1);  k != size_type(-1);  --k) {
          ROCKET_ASSERT(k != npos);

          // A null character is not part of the target string.
          if(this->data()[k] == 0)
            continue;

          // If `bitmap` doesn't contain this character, then it cannot be a match.
          int ch = noadl::xchrtoint(this->data()[k]);
          if(bitmap[uint8_t(ch)] == false)
            continue;

          // If `bitmap` has not been overflowed, then it is not necessary to look
          // for it in the target string.
          if(overflow == 0)
            return k;

          // Check whether this is really a match using plain comparison, unlike
          // `std::basic_string` which uses `std::char_traits`.
          if(noadl::xstrchr(s, this->data()[k]) != nullptr)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    rfind_of(const value_type* s) const noexcept
      {
        return this->rfind_of(npos, s);
      }

    constexpr
    size_type
    rfind_of(size_type to, const value_type* s, size_type n) const noexcept
      {
        if(this->empty())
          return npos;

        bool bitmap[256] = { };
        int overflow = 0;

        for(auto sptr = s;  sptr != s + n;  ++sptr) {
          // Hash the lowest 8 bits into `bitmap`. If other non-zero bits exist,
          // accumulate them into `overflow`.
          int ch = noadl::xchrtoint(*sptr);
          bitmap[uint8_t(ch)] = true;
          overflow |= ch >> 8;
        }

        for(size_type k = noadl::min(to, this->size() - 1);  k != size_type(-1);  --k) {
          ROCKET_ASSERT(k != npos);

          // If `bitmap` doesn't contain this character, then it cannot be a match.
          int ch = noadl::xchrtoint(this->data()[k]);
          if(bitmap[uint8_t(ch)] == false)
            continue;

          // If `bitmap` has not been overflowed, then it is not necessary to look
          // for it in the target string.
          if(overflow == 0)
            return k;

          // Check whether this is really a match using plain comparison, unlike
          // `std::basic_string` which uses `std::char_traits`.
          if(noadl::xmemchr(s, this->data()[k], n) != nullptr)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    rfind_of(const value_type* s, size_type n) const noexcept
      {
        return this->rfind_of(npos, s, n);
      }

    constexpr
    size_type
    rfind_of(size_type to, value_type c) const noexcept
      {
        if(this->empty())
          return npos;

        for(size_type k = noadl::min(to, this->size() - 1);  k != size_type(-1);  --k) {
          ROCKET_ASSERT(k != npos);

          // Perform plain comparison, unlike `std::basic_string` which uses
          // `std::char_traits`.
          if(this->data()[k] == c)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    rfind_of(value_type c) const noexcept
      {
        return this->rfind_of(npos, c);
      }

    constexpr
    size_type
    find_not_of(size_type from, shallow_type sh) const noexcept
      {
        return this->find_not_of(from, sh.data(), sh.size());
      }

    constexpr
    size_type
    find_not_of(shallow_type sh) const noexcept
      {
        return this->find_not_of(size_type(0), sh);
      }

    constexpr
    size_type
    find_not_of(size_type from, initializer_list<value_type> init) const noexcept
      {
        return this->find_not_of(from, init.begin(), init.size());
      }

    constexpr
    size_type
    find_not_of(initializer_list<value_type> init) const noexcept
      {
        return this->find_not_of(size_type(0), init);
      }

    constexpr
    size_type
    find_not_of(size_type from, const basic_cow_string& other) const noexcept
      {
        return this->find_not_of(from, other.data(), other.size());
      }

    constexpr
    size_type
    find_not_of(const basic_cow_string& other) const noexcept
      {
        return this->find_not_of(size_type(0), other);
      }

    constexpr
    size_type
    find_not_of(size_type from, const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->find_not_of(from, other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    size_type
    find_not_of(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->find_not_of(size_type(0), other, pos, n);
      }

    constexpr
    size_type
    find_not_of(size_type from, const value_type* s) const noexcept
      {
        if(this->empty())
          return npos;

        bool bitmap[256] = { };
        int overflow = 0;

        for(auto sptr = s;  *sptr != 0;  ++sptr) {
          // Hash the lowest 8 bits into `bitmap`. If other non-zero bits exist,
          // accumulate them into `overflow`.
          int ch = noadl::xchrtoint(*sptr);
          bitmap[uint8_t(ch)] = true;
          overflow |= ch >> 8;
        }

        for(size_type k = from;  k < this->size();  ++k) {
          ROCKET_ASSERT(k != npos);

          // A null character is not part of the target string.
          if(this->data()[k] == 0)
            return k;

          // If `bitmap` doesn't contain this character, then it cannot be a match.
          int ch = noadl::xchrtoint(this->data()[k]);
          if(bitmap[uint8_t(ch)] == false)
            return k;

          // If `bitmap` has not been overflowed, then it is not necessary to look
          // for it in the target string.
          if(overflow == 0)
            continue;

          // Check whether this is really a match using plain comparison, unlike
          // `std::basic_string` which uses `std::char_traits`.
          if(noadl::xstrchr(s, this->data()[k]) == nullptr)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    find_not_of(const value_type* s) const noexcept
      {
        return this->find_not_of(size_type(0), s);
      }

    constexpr
    size_type
    find_not_of(size_type from, const value_type* s, size_type n) const noexcept
      {
        if(this->empty())
          return npos;

        bool bitmap[256] = { };
        int overflow = 0;

        for(auto sptr = s;  sptr != s + n;  ++sptr) {
          // Hash the lowest 8 bits into `bitmap`. If other non-zero bits exist,
          // accumulate them into `overflow`.
          int ch = noadl::xchrtoint(*sptr);
          bitmap[uint8_t(ch)] = true;
          overflow |= ch >> 8;
        }

        for(size_type k = from;  k < this->size();  ++k) {
          ROCKET_ASSERT(k != npos);

          // If `bitmap` doesn't contain this character, then it cannot be a match.
          int ch = noadl::xchrtoint(this->data()[k]);
          if(bitmap[uint8_t(ch)] == false)
            return k;

          // If `bitmap` has not been overflowed, then it is not necessary to look
          // for it in the target string.
          if(overflow == 0)
            continue;

          // Check whether this is really a match using plain comparison, unlike
          // `std::basic_string` which uses `std::char_traits`.
          if(noadl::xmemchr(s, this->data()[k], n) == nullptr)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    find_not_of(const value_type* s, size_type n) const noexcept
      {
        return this->find_not_of(size_type(0), s, n);
      }

    constexpr
    size_type
    find_not_of(size_type from, value_type c) const noexcept
      {
        if(this->empty())
          return npos;

        for(size_type k = from;  k < this->size();  ++k) {
          ROCKET_ASSERT(k != npos);

          // Perform plain comparison, unlike `std::basic_string` which uses
          // `std::char_traits`.
          if(this->data()[k] != c)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    find_not_of(value_type c) const noexcept
      {
        return this->find_not_of(size_type(0), c);
      }

    constexpr
    size_type
    rfind_not_of(size_type to, shallow_type sh) const noexcept
      {
        return this->rfind_not_of(to, sh.data(), sh.size());
      }

    constexpr
    size_type
    rfind_not_of(shallow_type sh) const noexcept
      {
        return this->rfind_not_of(npos, sh);
      }

    constexpr
    size_type
    rfind_not_of(size_type to, initializer_list<value_type> init) const noexcept
      {
        return this->rfind_not_of(to, init.begin(), init.size());
      }

    constexpr
    size_type
    rfind_not_of(initializer_list<value_type> init) const noexcept
      {
        return this->rfind_not_of(npos, init);
      }

    constexpr
    size_type
    rfind_not_of(size_type to, const basic_cow_string& other) const noexcept
      {
        return this->rfind_not_of(to, other.data(), other.size());
      }

    constexpr
    size_type
    rfind_not_of(const basic_cow_string& other) const noexcept
      {
        return this->rfind_not_of(npos, other);
      }

    constexpr
    size_type
    rfind_not_of(size_type to, const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->rfind_not_of(to, other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    size_type
    rfind_not_of(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->rfind_not_of(npos, other, pos, n);
      }

    constexpr
    size_type
    rfind_not_of(size_type to, const value_type* s) const noexcept
      {
        if(this->empty())
          return npos;

        bool bitmap[256] = { };
        int overflow = 0;

        for(auto sptr = s;  *sptr != 0;  ++sptr) {
          // Hash the lowest 8 bits into `bitmap`. If other non-zero bits exist,
          // accumulate them into `overflow`.
          int ch = noadl::xchrtoint(*sptr);
          bitmap[uint8_t(ch)] = true;
          overflow |= ch >> 8;
        }

        for(size_type k = noadl::min(to, this->size() - 1);  k != size_type(-1);  --k) {
          ROCKET_ASSERT(k != npos);

          // A null character is not part of the target string.
          if(this->data()[k] == 0)
            return k;

          // If `bitmap` doesn't contain this character, then it cannot be a match.
          int ch = noadl::xchrtoint(this->data()[k]);
          if(bitmap[uint8_t(ch)] == false)
            return k;

          // If `bitmap` has not been overflowed, then it is not necessary to look
          // for it in the target string.
          if(overflow == 0)
            continue;

          // Check whether this is really a match using plain comparison, unlike
          // `std::basic_string` which uses `std::char_traits`.
          if(noadl::xstrchr(s, this->data()[k]) == nullptr)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    rfind_not_of(const value_type* s) const noexcept
      {
        return this->rfind_not_of(npos, s);
      }

    constexpr
    size_type
    rfind_not_of(size_type to, const value_type* s, size_type n) const noexcept
      {
        if(this->empty())
          return npos;

        bool bitmap[256] = { };
        int overflow = 0;

        for(auto sptr = s;  sptr != s + n;  ++sptr) {
          // Hash the lowest 8 bits into `bitmap`. If other non-zero bits exist,
          // accumulate them into `overflow`.
          int ch = noadl::xchrtoint(*sptr);
          bitmap[uint8_t(ch)] = true;
          overflow |= ch >> 8;
        }

        for(size_type k = noadl::min(to, this->size() - 1);  k != size_type(-1);  --k) {
          ROCKET_ASSERT(k != npos);

          // If `bitmap` doesn't contain this character, then it cannot be a match.
          int ch = noadl::xchrtoint(this->data()[k]);
          if(bitmap[uint8_t(ch)] == false)
            return k;

          // If `bitmap` has not been overflowed, then it is not necessary to look
          // for it in the target string.
          if(overflow == 0)
            continue;

          // Check whether this is really a match using plain comparison, unlike
          // `std::basic_string` which uses `std::char_traits`.
          if(noadl::xmemchr(s, this->data()[k], n) == nullptr)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    rfind_not_of(const value_type* s, size_type n) const noexcept
      {
        return this->rfind_not_of(npos, s, n);
      }

    constexpr
    size_type
    rfind_not_of(size_type to, value_type c) const noexcept
      {
        if(this->empty())
          return npos;

        for(size_type k = noadl::min(to, this->size() - 1);  k != size_type(-1);  --k) {
          ROCKET_ASSERT(k != npos);

          // Perform plain comparison, unlike `std::basic_string` which uses
          // `std::char_traits`.
          if(this->data()[k] != c)
            return k;
        }

        return npos;
      }

    constexpr
    size_type
    rfind_not_of(value_type c) const noexcept
      {
        return this->rfind_not_of(npos, c);
      }

    constexpr
    bool
    equals(shallow_type sh) const noexcept
      {
        return this->equals(sh.data(), sh.size());
      }

    constexpr
    bool
    equals(initializer_list<value_type> init) const noexcept
      {
        return this->equals(init.begin(), init.size());
      }

    constexpr
    bool
    equals(const basic_cow_string& other) const noexcept
      {
        return this->equals(other.data(), other.size());
      }

    constexpr
    bool
    equals(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->equals(other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    bool
    equals(const value_type* s) const noexcept
      {
        return this->equals(s, noadl::xstrlen(s));
      }

    constexpr
    bool
    equals(const value_type* s, size_type n) const noexcept
      {
        return (this->size() == n) && noadl::xmemeq(this->data(), s, n);
      }

    constexpr
    bool
    substr_equals(size_type tpos, size_type tn, shallow_type sh) const
      {
        return this->substr_equals(tpos, tn, sh.data(), sh.size());
      }

    constexpr
    bool
    substr_equals(size_type tpos, shallow_type sh) const
      {
        return this->substr_equals(tpos, npos, sh);
      }

    constexpr
    bool
    substr_equals(size_type tpos, size_type tn, initializer_list<value_type> init) const
      {
        return this->substr_equals(tpos, tn, init.begin(), init.size());
      }

    constexpr
    bool
    substr_equals(size_type tpos, initializer_list<value_type> init) const
      {
        return this->substr_equals(tpos, npos, init);
      }

    constexpr
    bool
    substr_equals(size_type tpos, size_type tn, const basic_cow_string& other, size_type pos = 0, size_type n = npos) const
      {
        return this->substr_equals(tpos, tn, other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    bool
    substr_equals(size_type tpos, const basic_cow_string& other, size_type pos = 0, size_type n = npos) const
      {
        return this->substr_equals(tpos, npos, other, pos, n);
      }

    constexpr
    bool
    substr_equals(size_type tpos, size_type tn, const value_type* s) const
      {
        return this->substr_equals(tpos, tn, s, noadl::xstrlen(s));
      }

    constexpr
    bool
    substr_equals(size_type tpos, const value_type* s) const
      {
        return this->substr_equals(tpos, npos, s);
      }

    constexpr
    bool
    substr_equals(size_type tpos, size_type tn, const value_type* s, size_type n) const
      {
        return (this->do_clamp_substr(tpos, tn) == n) && noadl::xmemeq(this->data() + tpos, s, n);
      }

    constexpr
    bool
    substr_equals(size_type tpos, const value_type* s, size_type n) const
      {
        return this->substr_equals(tpos, npos, s, n);
      }

    constexpr
    int
    compare(shallow_type sh) const noexcept
      {
        return this->compare(sh.data(), sh.size());
      }

    constexpr
    int
    compare(initializer_list<value_type> init) const noexcept
      {
        return this->compare(init.begin(), init.size());
      }

    constexpr
    int
    compare(const basic_cow_string& other) const noexcept
      {
        return this->compare(other.data(), other.size());
      }

    constexpr
    int
    compare(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->compare(other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    int
    compare(const value_type* s) const noexcept
      {
        return this->compare(s, noadl::xstrlen(s));
      }

    constexpr
    int
    compare(const value_type* s, size_type n) const noexcept
      {
        return (this->size() >= n)
                 ? (noadl::xmemcmp(this->data(), s, n) | (this->size() > n))
                 : ~(noadl::xmemcmp(s, this->data(), this->size()) | 1);
      }

    constexpr
    int
    substr_compare(size_type tpos, size_type tn, shallow_type sh) const
      {
        return this->substr_compare(tpos, tn, sh.data(), sh.size());
      }

    constexpr
    int
    substr_compare(size_type tpos, shallow_type sh) const
      {
        return this->substr_compare(tpos, npos, sh);
      }

    constexpr
    int
    substr_compare(size_type tpos, size_type tn, initializer_list<value_type> init) const
      {
        return this->substr_compare(tpos, tn, init.begin(), init.size());
      }

    constexpr
    int
    substr_compare(size_type tpos, initializer_list<value_type> init) const
      {
        return this->substr_compare(tpos, npos, init);
      }

    constexpr
    int
    substr_compare(size_type tpos, size_type tn, const basic_cow_string& other, size_type pos = 0, size_type n = npos) const
      {
        return this->substr_compare(tpos, tn, other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    int
    substr_compare(size_type tpos, const basic_cow_string& other, size_type pos = 0, size_type n = npos) const
      {
        return this->substr_compare(tpos, npos, other, pos, n);
      }

    constexpr
    int
    substr_compare(size_type tpos, size_type tn, const value_type* s) const
      {
        return this->substr_compare(tpos, tn, s, noadl::xstrlen(s));
      }

    constexpr
    int
    substr_compare(size_type tpos, const value_type* s) const
      {
        return this->substr_compare(tpos, npos, s);
      }

    constexpr
    int
    substr_compare(size_type tpos, size_type tn, const value_type* s, size_type n) const
      {
        size_type tlen = this->do_clamp_substr(tpos, tn);
        return (tlen >= n)
                 ? (noadl::xmemcmp(this->data() + tpos, s, n) | (tlen > n))
                 : ~(noadl::xmemcmp(s, this->data() + tpos, tlen) | 1);
      }

    constexpr
    int
    substr_compare(size_type tpos, const value_type* s, size_type n) const
      {
        return this->substr_compare(tpos, npos, s, n);
      }

    // N.B. These are extensions but might be standardized in C++20.
    constexpr
    bool
    starts_with(shallow_type sh) const noexcept
      {
        return this->starts_with(sh.data(), sh.size());
      }

    constexpr
    bool
    starts_with(initializer_list<value_type> init) const noexcept
      {
        return this->starts_with(init.begin(), init.size());
      }

    constexpr
    bool
    starts_with(const basic_cow_string& other) const noexcept
      {
        return this->starts_with(other.data(), other.size());
      }

    constexpr
    bool
    starts_with(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->starts_with(other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    bool
    starts_with(const value_type* s) const noexcept
      {
        return this->starts_with(s, noadl::xstrlen(s));
      }

    constexpr
    bool
    starts_with(const value_type* s, size_type n) const noexcept
      {
        return (this->size() >= n) && noadl::xmemeq(this->data(), s, n);
      }

    constexpr
    bool
    ends_with(shallow_type sh) const noexcept
      {
        return this->ends_with(sh.data(), sh.size());
      }

    constexpr
    bool
    ends_with(initializer_list<value_type> init) const noexcept
      {
        return this->ends_with(init.begin(), init.size());
      }

    constexpr
    bool
    ends_with(const basic_cow_string& other) const noexcept
      {
        return this->ends_with(other.data(), other.size());
      }

    constexpr
    bool
    ends_with(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->ends_with(other.data() + pos, other.do_clamp_substr(pos, n));
      }

    constexpr
    bool
    ends_with(const value_type* s) const noexcept
      {
        return this->ends_with(s, noadl::xstrlen(s));
      }

    constexpr
    bool
    ends_with(const value_type* s, size_type n) const noexcept
      {
        return (this->size() >= n) && noadl::xmemeq(this->data() + this->size() - n, s, n);
      }
  };

#ifndef __cpp_inline_variables
template<typename charT, typename allocT>
const typename allocator_traits<allocT>::size_type basic_cow_string<charT, allocT>::npos;

template<typename charT, typename allocT>
const charT basic_cow_string<charT, allocT>::s_zcstr[1];

template<typename charT, typename allocT>
const basic_shallow_string<charT> basic_cow_string<charT, allocT>::s_zstr;
#endif  // __cpp_inline_variables

template<typename charT, typename allocT>
struct basic_cow_string<charT, allocT>::hash
  {
    using result_type    = size_t;
    using argument_type  = basic_cow_string;

    constexpr
    result_type
    operator()(const argument_type& str) const noexcept
      {
        // Implement the FNV-1a hashing algorithm.
        uint32_t reg = 2166136261U;
        for(charT c : str) {
          int ch = noadl::xchrtoint(c);

          // Accumulate bytes in little-endian byte order.
          for(size_t k = 0;  k != sizeof(c);  ++k) {
            reg ^= static_cast<uint8_t>(ch);
            reg *= 16777619U;
            ch >>= 8;
          }
        }
        return reg;
      }
  };

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(const basic_cow_string<charT, allocT>& lhs, basic_shallow_string<charT> rhs)
  {
    auto str = lhs;
    str.append(rhs);
    return str;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(const basic_cow_string<charT, allocT>& lhs, initializer_list<charT> rhs)
  {
    auto str = lhs;
    str.append(rhs);
    return str;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(const basic_cow_string<charT, allocT>& lhs, const basic_cow_string<charT, allocT>& rhs)
  {
    auto str = lhs;
    str.append(rhs);
    return str;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(const basic_cow_string<charT, allocT>& lhs, const charT* rhs)
  {
    auto str = lhs;
    str.append(rhs);
    return str;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(const basic_cow_string<charT, allocT>& lhs, charT rhs)
  {
    auto str = lhs;
    str.push_back(rhs);
    return str;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(basic_cow_string<charT, allocT>&& lhs, basic_shallow_string<charT> rhs)
  {
    lhs.append(rhs);
    return lhs;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(basic_cow_string<charT, allocT>&& lhs, initializer_list<charT> rhs)
  {
    lhs.append(rhs);
    return lhs;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(basic_cow_string<charT, allocT>&& lhs, const basic_cow_string<charT, allocT>& rhs)
  {
    lhs.append(rhs);
    return lhs;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(basic_cow_string<charT, allocT>&& lhs, basic_cow_string<charT, allocT>&& rhs)
  {
    lhs.append(rhs);
    return lhs;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(basic_cow_string<charT, allocT>&& lhs, const charT* rhs)
  {
    lhs.append(rhs);
    return lhs;
  }


template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(basic_cow_string<charT, allocT>&& lhs, charT rhs)
  {
    lhs.push_back(rhs);
    return lhs;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(basic_shallow_string<charT> lhs, const basic_cow_string<charT, allocT>& rhs)
  {
    auto str = rhs;
    str.insert(str.begin(), lhs);
    return str;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(initializer_list<charT> lhs, const basic_cow_string<charT, allocT>& rhs)
  {
    auto str = rhs;
    str.insert(str.begin(), lhs);
    return str;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(const charT* lhs, const basic_cow_string<charT, allocT>& rhs)
  {
    auto str = rhs;
    str.insert(str.begin(), lhs);
    return str;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(charT lhs, const basic_cow_string<charT, allocT>& rhs)
  {
    auto str = rhs;
    str.insert(str.begin(), size_type(1), lhs);
    return str;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(basic_shallow_string<charT> lhs, basic_cow_string<charT, allocT>&& rhs)
  {
    rhs.insert(rhs.begin(), lhs);
    return rhs;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(initializer_list<charT> lhs, basic_cow_string<charT, allocT>&& rhs)
  {
    rhs.insert(rhs.begin(), lhs);
    return rhs;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(const basic_cow_string<charT, allocT>& lhs, basic_cow_string<charT, allocT>&& rhs)
  {
    rhs.insert(rhs.begin(), lhs);
    return rhs;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(const charT* lhs, basic_cow_string<charT, allocT>&& rhs)
  {
    rhs.insert(rhs.begin(), lhs);
    return rhs;
  }

template<typename charT, typename allocT>
inline
basic_cow_string<charT, allocT>
operator+(charT lhs, basic_cow_string<charT, allocT>&& rhs)
  {
    rhs.insert(rhs.begin(), size_type(1), lhs);
    return rhs;
  }

template<typename charT, typename allocT>
constexpr
bool
operator==(const basic_cow_string<charT, allocT>& lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return lhs.equals(rhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator==(const basic_cow_string<charT, allocT>& lhs, basic_shallow_string<charT> rhs) noexcept
  {
    return lhs.equals(rhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator==(const basic_cow_string<charT, allocT>& lhs, initializer_list<charT> rhs) noexcept
  {
    return lhs.equals(rhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator==(const basic_cow_string<charT, allocT>& lhs, const charT* rhs) noexcept
  {
    return lhs.equals(rhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator==(basic_shallow_string<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.equals(lhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator==(initializer_list<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.equals(lhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator==(const charT* lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.equals(lhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator!=(const basic_cow_string<charT, allocT>& lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return ! lhs.equals(rhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator!=(const basic_cow_string<charT, allocT>& lhs, basic_shallow_string<charT> rhs) noexcept
  {
    return ! lhs.equals(rhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator!=(const basic_cow_string<charT, allocT>& lhs, initializer_list<charT> rhs) noexcept
  {
    return ! lhs.equals(rhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator!=(const basic_cow_string<charT, allocT>& lhs, const charT* rhs) noexcept
  {
    return ! lhs.equals(rhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator!=(basic_shallow_string<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return ! rhs.equals(lhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator!=(initializer_list<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return ! rhs.equals(lhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator!=(const charT* lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return ! rhs.equals(lhs);
  }

template<typename charT, typename allocT>
constexpr
bool
operator<(const basic_cow_string<charT, allocT>& lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return lhs.compare(rhs) < 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<(const basic_cow_string<charT, allocT>& lhs, basic_shallow_string<charT> rhs) noexcept
  {
    return lhs.compare(rhs) < 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<(const basic_cow_string<charT, allocT>& lhs, initializer_list<charT> rhs) noexcept
  {
    return lhs.compare(rhs) < 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<(const basic_cow_string<charT, allocT>& lhs, const charT* rhs) noexcept
  {
    return lhs.compare(rhs) < 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<(basic_shallow_string<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) > 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<(initializer_list<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) > 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<(const charT* lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) > 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<=(const basic_cow_string<charT, allocT>& lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return lhs.compare(rhs) <= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<=(const basic_cow_string<charT, allocT>& lhs, basic_shallow_string<charT> rhs) noexcept
  {
    return lhs.compare(rhs) <= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<=(const basic_cow_string<charT, allocT>& lhs, initializer_list<charT> rhs) noexcept
  {
    return lhs.compare(rhs) <= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<=(const basic_cow_string<charT, allocT>& lhs, const charT* rhs) noexcept
  {
    return lhs.compare(rhs) <= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<=(basic_shallow_string<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) >= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<=(initializer_list<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) >= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator<=(const charT* lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) >= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>(const basic_cow_string<charT, allocT>& lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return lhs.compare(rhs) > 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>(const basic_cow_string<charT, allocT>& lhs, basic_shallow_string<charT> rhs) noexcept
  {
    return lhs.compare(rhs) > 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>(const basic_cow_string<charT, allocT>& lhs, initializer_list<charT> rhs) noexcept
  {
    return lhs.compare(rhs) > 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>(const basic_cow_string<charT, allocT>& lhs, const charT* rhs) noexcept
  {
    return lhs.compare(rhs) > 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>(basic_shallow_string<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) < 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>(initializer_list<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) < 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>(const charT* lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) < 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>=(const basic_cow_string<charT, allocT>& lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return lhs.compare(rhs) >= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>=(const basic_cow_string<charT, allocT>& lhs, basic_shallow_string<charT> rhs) noexcept
  {
    return lhs.compare(rhs) >= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>=(const basic_cow_string<charT, allocT>& lhs, initializer_list<charT> rhs) noexcept
  {
    return lhs.compare(rhs) >= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>=(const basic_cow_string<charT, allocT>& lhs, const charT* rhs) noexcept
  {
    return lhs.compare(rhs) >= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>=(basic_shallow_string<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) <= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>=(initializer_list<charT> lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) <= 0;
  }

template<typename charT, typename allocT>
constexpr
bool
operator>=(const charT* lhs, const basic_cow_string<charT, allocT>& rhs) noexcept
  {
    return rhs.compare(lhs) <= 0;
  }

template<typename charT, typename allocT>
inline
void
swap(basic_cow_string<charT, allocT>& lhs, basic_cow_string<charT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

template<typename charT, typename allocT>
inline
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const basic_cow_string<charT, allocT>& str)
  {
    return fmt.putn(str.data(), str.size());
  }

template<typename charT, typename allocT>
inline
bool
getline(basic_cow_string<charT, allocT>& str, basic_tinybuf<charT>& buf)
  {
    int ch;
    str.clear();
    for(;;)
      if((ch = buf.getc()) < 0)
        return !str.empty();  // end of stream
      else if(ch == '\n')
        return true;  // new line
      else
        str.push_back(static_cast<charT>(ch));
  }

extern template class basic_cow_string<char>;
extern template class basic_cow_string<wchar_t>;
extern template class basic_cow_string<char16_t>;
extern template class basic_cow_string<char32_t>;

using cow_string     = basic_cow_string<char>;
using cow_wstring    = basic_cow_string<wchar_t>;
using cow_u16string  = basic_cow_string<char16_t>;
using cow_u32string  = basic_cow_string<char32_t>;

}  // namespace rocket
#endif
