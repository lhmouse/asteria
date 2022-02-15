// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_STATIC_VECTOR_HPP_
#define ROCKET_STATIC_VECTOR_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include "throw.hpp"
#include "xallocator.hpp"

namespace rocket {

template<typename valueT, size_t capacityT,
         typename allocT = allocator<valueT>>
class static_vector;

#include "details/static_vector.ipp"

/* Differences from `std::vector`:
 * 1. The storage of elements are allocated inside the vector object, which eliminates dynamic
 *    allocation.
 * 2. An additional capacity template parameter is required.
 * 3. The specialization for `bool` is not provided.
 * 4. `emplace()` is not provided.
 * 5. `capacity()` is a `static constexpr` member function.
 * 6. Comparison operators are not provided.
 * 7. Incomplete element types are not supported.
**/

template<typename valueT, size_t capacityT, typename allocT>
class static_vector
  {
    static_assert(!is_array<valueT>::value, "invalid element type");
    static_assert(is_same<typename allocT::value_type, valueT>::value, "inappropriate allocator type");

  public:
    // types
    using value_type      = valueT;
    using allocator_type  = allocT;

    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using difference_type  = typename allocator_traits<allocator_type>::difference_type;
    using const_reference  = const value_type&;
    using reference        = value_type&;

    using const_iterator      = details_static_vector::vector_iterator<static_vector, const value_type>;
    using iterator            = details_static_vector::vector_iterator<static_vector, value_type>;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

  private:
    using storage_handle = details_static_vector::storage_handle<allocator_type, capacityT>;

  private:
    storage_handle m_sth;

  public:
    // 26.3.11.2, construct/copy/destroy
    explicit
    static_vector(const allocator_type& alloc) noexcept
      : m_sth(alloc)
      { }

    static_vector(const static_vector& other)
      noexcept(is_nothrow_copy_constructible<value_type>::value)
      : m_sth(allocator_traits<allocator_type>::select_on_container_copy_construction(
                                                      other.m_sth.as_allocator()))
      { this->m_sth.copy_from(other.m_sth);  }

    static_vector(const static_vector& other, const allocator_type& alloc)
      noexcept(is_nothrow_copy_constructible<value_type>::value)
      : m_sth(alloc)
      { this->m_sth.copy_from(other.m_sth);  }

    static_vector(static_vector&& other) noexcept(is_nothrow_move_constructible<value_type>::value)
      : m_sth(::std::move(other.m_sth.as_allocator()))
      { this->m_sth.move_from(::std::move(other.m_sth));  }

    static_vector(static_vector&& other, const allocator_type& alloc)
      noexcept(is_nothrow_move_constructible<value_type>::value)
      : m_sth(alloc)
      { this->m_sth.move_from(::std::move(other.m_sth));  }

    static_vector() noexcept(is_nothrow_constructible<allocator_type>::value)
      : static_vector(allocator_type())
      { }

    explicit
    static_vector(size_type n, const allocator_type& alloc = allocator_type())
      : static_vector(alloc)
      { this->append(n);  }

    static_vector(size_type n, const value_type& value, const allocator_type& alloc = allocator_type())
      : static_vector(alloc)
      { this->append(n, value);  }

    // N.B. This is a non-standard extension.
    template<typename firstT, typename... restT,
    ROCKET_DISABLE_IF(is_same<typename decay<firstT>::type, allocator_type>::value)>
    static_vector(size_type n, const firstT& first, const restT&... rest)
      : static_vector()
      { this->append(n, first, rest...);  }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    static_vector(inputT first, inputT last, const allocator_type& alloc = allocator_type())
      : static_vector(alloc)
      { this->append(::std::move(first), ::std::move(last));  }

    static_vector(initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
      : static_vector(alloc)
      { this->append(init.begin(), init.end());  }

    static_vector&
    operator=(const static_vector& other)
      noexcept(conjunction<is_nothrow_copy_assignable<value_type>,
                           is_nothrow_copy_constructible<value_type>>::value)
      { noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.copy_from(other.m_sth);
        return *this;  }

    static_vector&
    operator=(static_vector&& other)
      noexcept(conjunction<is_nothrow_move_assignable<value_type>,
                           is_nothrow_move_constructible<value_type>>::value)
      { noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.move_from(::std::move(other.m_sth));
        return *this;  }

    static_vector&
    operator=(initializer_list<value_type> init)
      { return this->assign(init.begin(), init.end());  }

    static_vector&
    swap(static_vector& other)
      noexcept(conjunction<is_nothrow_swappable<value_type>,
                           is_nothrow_move_constructible<value_type>>::value)
      { noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        return *this;  }

  private:
    [[noreturn]] ROCKET_NEVER_INLINE void
    do_throw_subscript_out_of_range(size_type pos, const char* rel) const
      {
        noadl::sprintf_and_throw<out_of_range>(
              "static_vector: subscript out of range (`%llu` %s `%llu`)",
              static_cast<unsigned long long>(pos), rel,
              static_cast<unsigned long long>(this->size()));
      }

    // This function works the same way as `substr()`.
    // Ensure `tpos` is in `[0, size()]` and return `min(tn, size() - tpos)`.
    size_type
    do_clamp_subvec(size_type tpos, size_type tn) const
      {
        size_type len = this->size();
        if(tpos > len)
          this->do_throw_subscript_out_of_range(tpos, ">");
        return noadl::min(tn, len - tpos);
      }

    // This function is used to implement `insert()` after new elements has been appended.
    // `tpos` is the position to insert. `kpos` is the old length prior to `append()`.
    value_type*
    do_swizzle_unchecked(size_type tpos, size_type kpos)
      {
        // Get a pointer to mutable storage.
        auto ptr = this->mut_data() + tpos;
        size_type klen = kpos - tpos;
        size_type slen = this->size() - tpos;

        // Swap the intervals [`tpos`,`kpos`) and [`kpos`,`size`).
        noadl::rotate(ptr, 0, klen, slen);
        return ptr;
      }

    // This function is used to implement `erase()`.
    value_type*
    do_erase_unchecked(size_type tpos, size_type tlen)
      {
        // Get a pointer to mutable storage.
        auto ptr = this->mut_data() + tpos;
        size_type slen = this->size() - tpos;

        // Swap the intervals [`tpos`,`tpos+tlen`) and [`tpos+tlen`,`size`).
        noadl::rotate(ptr, 0, tlen, slen);

        // Purge unwanted elements.
        this->m_sth.pop_back_unchecked(tlen);
        return ptr;
      }

  public:
    // iterators
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

    // N.B. This is a non-standard extension.
    iterator
    mut_begin()
      { return iterator(this->mut_data(), 0, this->size());  }

    // N.B. This is a non-standard extension.
    iterator
    mut_end()
      { return iterator(this->mut_data(), this->size(), this->size());  }

    // N.B. This is a non-standard extension.
    reverse_iterator
    mut_rbegin()
      { return reverse_iterator(this->mut_end());  }

    // N.B. This is a non-standard extension.
    reverse_iterator
    mut_rend()
      { return reverse_iterator(this->mut_begin());  }

    // N.B. This is a non-standard extension.
    ::std::move_iterator<iterator>
    move_begin()
      { return ::std::move_iterator<iterator>(this->mut_begin());  }

    // N.B. This is a non-standard extension.
    ::std::move_iterator<iterator>
    move_end()
      { return ::std::move_iterator<iterator>(this->mut_end());  }

    // N.B. This is a non-standard extension.
    ::std::move_iterator<reverse_iterator>
    move_rbegin()
      { return ::std::move_iterator<reverse_iterator>(this->mut_rbegin());  }

    // N.B. This is a non-standard extension.
    ::std::move_iterator<reverse_iterator>
    move_rend()
      { return ::std::move_iterator<reverse_iterator>(this->mut_rend());  }

    // 26.3.11.3, capacity
    constexpr bool
    empty() const noexcept
      { return this->m_sth.size() == 0;  }

    constexpr size_type
    size() const noexcept
      { return this->m_sth.size();  }

    // N.B. This is a non-standard extension.
    constexpr difference_type
    ssize() const noexcept
      { return static_cast<difference_type>(this->size());  }

    constexpr size_type
    max_size() const noexcept
      { return this->m_sth.max_size();  }

    // N.B. The return type and the parameter pack are non-standard extensions.
    template<typename... paramsT>
    static_vector&
    resize(size_type n, const paramsT&... params)
      {
        if(this->size() < n)
          return this->append(n - this->size(), params...);
        else
          return this->pop_back(this->size() - n);
      }

    static constexpr size_type
    capacity() noexcept
      { return storage_handle::capacity();  }

    // N.B. The return type is a non-standard extension.
    static_vector&
    reserve(size_type res_arg)
      {
        this->m_sth.check_size_add(0, res_arg);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    static_vector&
    shrink_to_fit() noexcept
      { return *this;  }

    // N.B. The return type is a non-standard extension.
    static_vector&
    clear() noexcept
      {
        this->m_sth.pop_back_unchecked(this->size());
        return *this;
      }

    // element access
    const_reference
    at(size_type pos) const
      {
        if(pos >= this->size())
          this->do_throw_subscript_out_of_range(pos, ">=");
        return this->data()[pos];
      }

    template<typename subscriptT,
    ROCKET_ENABLE_IF(is_integral<subscriptT>::value && (sizeof(subscriptT) <= sizeof(size_type)))>
    const_reference
    at(subscriptT pos) const
      { return this->at(static_cast<size_type>(pos));  }

    const_reference
    operator[](size_type pos) const noexcept
      {
        ROCKET_ASSERT(pos < this->size());
        return this->data()[pos];
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

    // N.B. This is a non-standard extension.
    const value_type*
    ptr(size_type pos) const noexcept
      {
        if(pos >= this->size())
          return nullptr;
        return this->data() + pos;
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

    // N.B. This is a non-standard extension.
    value_type*
    mut_ptr(size_type pos)
      {
        if(pos >= this->size())
          return nullptr;
        return this->mut_data() + pos;
      }

    // N.B. This is a non-standard extension.
    template<typename... paramsT>
    static_vector&
    append(size_type n, const paramsT&... params)
      {
        if(n == 0)
          return *this;

        // Check whether there is room for new elements.
        this->m_sth.check_size_add(this->size(), n);

        // The storage can't be reallocated, so we may append all elements in place.
        for(size_type k = 0;  k < n;  ++k)
          this->m_sth.emplace_back_unchecked(params...);

        // The return type aligns with `std::string::append()`.
        return *this;
      }

    // N.B. This is a non-standard extension.
    static_vector&
    append(initializer_list<value_type> init)
      { return this->append(init.begin(), init.end());  }

    // N.B. This is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    static_vector&
    append(inputT first, inputT last)
      {
        if(first == last)
          return *this;

        size_type n = noadl::estimate_distance(first, last);

        // Check whether there is room for new elements if `inputT` is a forward iterator type.
        this->m_sth.check_size_add(this->size(), n);

        // The storage can't be reallocated, so we may append all elements in place.
        for(auto it = ::std::move(first);  it != last;  ++it)
          this->m_sth.emplace_back_unchecked(*it);

        // The return type aligns with `std::string::append()`.
        return *this;
      }

    // 26.3.11.5, modifiers
    template<typename... paramsT>
    reference
    emplace_back(paramsT&&... params)
      {
        // Check whether there is room for new elements.
        this->m_sth.check_size_add(this->size(), 1);

        // The storage can't be reallocated, so we may append the element in place.
        return this->m_sth.emplace_back_unchecked(::std::forward<paramsT>(params)...);
      }

    // N.B. The return type is a non-standard extension.
    static_vector&
    push_back(const value_type& value)
      {
        this->emplace_back(value);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    static_vector&
    push_back(value_type&& value)
      {
        this->emplace_back(::std::move(value));
        return *this;
      }

    // N.B. This is a non-standard extension.
    static_vector&
    insert(size_type tpos, const value_type& value)
      {
        this->do_clamp_subvec(tpos, 0);  // just check

        // Note `value` may reference an element in `*this`.
        size_type kpos = this->size();
        this->push_back(value);
        this->do_swizzle_unchecked(tpos, kpos);
        return *this;
      }

    // N.B. This is a non-standard extension.
    static_vector&
    insert(size_type tpos, value_type&& value)
      {
        this->do_clamp_subvec(tpos, 0);  // just check

        // Note `value` may reference an element in `*this`.
        size_type kpos = this->size();
        this->push_back(::std::move(value));
        this->do_swizzle_unchecked(tpos, kpos);
        return *this;
      }

    // N.B. This is a non-standard extension.
    template<typename... paramsT>
    static_vector&
    insert(size_type tpos, size_type n, const paramsT&... params)
      {
        this->do_clamp_subvec(tpos, 0);  // just check

        // Note `params...` may reference an element in `*this`.
        size_type kpos = this->size();
        this->append(n, params...);
        this->do_swizzle_unchecked(tpos, kpos);
        return *this;
      }

    // N.B. This is a non-standard extension.
    static_vector&
    insert(size_type tpos, initializer_list<value_type> init)
      {
        this->do_clamp_subvec(tpos, 0);  // just check

        // XXX: This can be optimized *a lot*.
        size_type kpos = this->size();
        this->append(init);
        this->do_swizzle_unchecked(tpos, kpos);
        return *this;
      }

    // N.B. This is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    static_vector&
    insert(size_type tpos, inputT first, inputT last)
      {
        this->do_clamp_subvec(tpos, 0);  // just check

        // Note `first` may overlap with `this->begin()`.
        size_type kpos = this->size();
        this->append(::std::move(first), ::std::move(last));
        this->do_swizzle_unchecked(tpos, kpos);
        return *this;
      }

    iterator
    insert(const_iterator tins, const value_type& value)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // Note `value` may reference an element in `*this`.
        size_type kpos = this->size();
        this->push_back(value);
        auto ptr = this->do_swizzle_unchecked(tpos, kpos);
        return iterator(ptr - tpos, tpos, this->size());
      }

    iterator
    insert(const_iterator tins, value_type&& value)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // Note `value` may reference an element in `*this`.
        size_type kpos = this->size();
        this->push_back(::std::move(value));
        auto ptr = this->do_swizzle_unchecked(tpos, kpos);
        return iterator(ptr - tpos, tpos, this->size());
      }

    // N.B. The parameter pack is a non-standard extension.
    template<typename... paramsT>
    iterator
    insert(const_iterator tins, size_type n, const paramsT&... params)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // Note `params...` may reference an element in `*this`.
        size_type kpos = this->size();
        this->append(n, params...);
        auto ptr = this->do_swizzle_unchecked(tpos, kpos);
        return iterator(ptr - tpos, tpos, this->size());
      }

    iterator
    insert(const_iterator tins, initializer_list<value_type> init)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // XXX: This can be optimized *a lot*.
        size_type kpos = this->size();
        this->append(init);
        auto ptr = this->do_swizzle_unchecked(tpos, kpos);
        return iterator(ptr - tpos, tpos, this->size());
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    iterator
    insert(const_iterator tins, inputT first, inputT last)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());

        // Note `first` may overlap with `this->begin()`.
        size_type kpos = this->size();
        this->append(::std::move(first), ::std::move(last));
        auto ptr = this->do_swizzle_unchecked(tpos, kpos);
        return iterator(ptr - tpos, tpos, this->size());
      }

    // N.B. This is a non-standard extension.
    static_vector&
    erase(size_type tpos, size_type tn = size_type(-1))
      {
        size_type tlen = this->do_clamp_subvec(tpos, tn);

        this->do_erase_unchecked(tpos, tlen);
        return *this;
      }

    iterator
    erase(const_iterator first, const_iterator last)
      {
        ROCKET_ASSERT_MSG(first <= last, "invalid range");
        size_type tpos = static_cast<size_type>(first - this->begin());
        size_type tlen = static_cast<size_type>(last - first);

        auto ptr = this->do_erase_unchecked(tpos, tlen);
        return iterator(ptr - tpos, tpos, this->size());
      }

    iterator
    erase(const_iterator pos)
      {
        size_type tpos = static_cast<size_type>(pos - this->begin());

        auto ptr = this->do_erase_unchecked(tpos, 1);
        return iterator(ptr - tpos, tpos, this->size());
      }

    // N.B. The return type and parameter are non-standard extensions.
    static_vector&
    pop_back(size_type n = 1)
      {
        this->m_sth.pop_back_unchecked(n);
        return *this;
      }

    // N.B. This is a non-standard extension.
    static_vector
    subvec(size_type tpos, size_type tn = size_type(-1)) const
      {
        return static_vector(this->data() + tpos,
                             this->data() + tpos + this->do_clamp_subvec(tpos, tn),
                             this->m_sth.as_allocator());
      }

    // N.B. The parameter pack is a non-standard extension.
    // N.B. The return type is a non-standard extension.
    template<typename... paramsT>
    static_vector&
    assign(size_type n, const paramsT&... params)
      {
        this->clear();
        this->append(n, params...);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    static_vector&
    assign(inputT first, inputT last)
      {
        this->clear();
        this->append(::std::move(first), ::std::move(last));
        return *this;
      }

    // 26.3.11.4, data access
    const value_type*
    data() const noexcept
      { return this->m_sth.data();  }

    // Get a pointer to mutable data.
    // N.B. This is a non-standard extension.
    ROCKET_ALWAYS_INLINE value_type*
    mut_data()
      { return this->m_sth.mut_data();  }

    // N.B. The return type differs from `std::vector`.
    constexpr const allocator_type&
    get_allocator() const noexcept
      { return this->m_sth.as_allocator();  }

    allocator_type&
    get_allocator() noexcept
      { return this->m_sth.as_allocator();  }
  };

template<typename valueT, size_t capacityT, typename allocT>
inline void
swap(static_vector<valueT, capacityT, allocT>& lhs, static_vector<valueT, capacityT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
