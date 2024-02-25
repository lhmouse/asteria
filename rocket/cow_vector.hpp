// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_VECTOR_
#define ROCKET_COW_VECTOR_

#include "fwd.hpp"
#include "xassert.hpp"
#include "xthrow.hpp"
#include "reference_counter.hpp"
#include "xallocator.hpp"
namespace rocket {

// Differences from `std::vector`:
// 1. All functions guarantee only basic exception safety rather than strong
//    exception safety, hence are more efficient.
// 2. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()`
//    and `back()` always return `const_reference`s.
// 3. The copy constructor and copy assignment operator will not throw
//    exceptions.
// 4. The specialization for `bool` is not provided.
// 5. `emplace()` is not provided.
// 6. Comparison operators are not provided.
// 7. The value type may be incomplete. It need be neither copy-assignable nor
//    move-assignable, but must be swappable.
template<typename valueT, typename allocT = allocator<valueT>>
class cow_vector;

#include "details/cow_vector.ipp"

template<typename valueT, typename allocT>
class cow_vector
  {
    static_assert(!is_array<valueT>::value, "invalid element type");
    static_assert(!is_reference<valueT>::value, "invalid element type");

  public:
    // types
    using value_type      = valueT;
    using allocator_type  = allocT;

    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using difference_type  = typename allocator_traits<allocator_type>::difference_type;
    using const_reference  = const value_type&;
    using reference        = value_type&;

    using const_iterator          = details_cow_vector::iterator<cow_vector, const value_type>;
    using iterator                = details_cow_vector::iterator<cow_vector, value_type>;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

  private:
    using storage_handle = details_cow_vector::storage_handle<allocator_type>;
    storage_handle m_sth;

  public:
    // 26.3.11.2, construct/copy/destroy
    ROCKET_ALWAYS_INLINE  // https://gcc.gnu.org/PR109464
    constexpr cow_vector() noexcept(is_nothrow_constructible<allocator_type>::value)
      :
        m_sth()
      { }

    explicit constexpr cow_vector(const allocator_type& alloc) noexcept
      :
        m_sth(alloc)
      { }

    cow_vector(const cow_vector& other) noexcept
      :
        m_sth(allocator_traits<allocator_type>::select_on_container_copy_construction(
                                                    other.m_sth.as_allocator()))
      { this->m_sth.share_with(other.m_sth);  }

    cow_vector(const cow_vector& other, const allocator_type& alloc) noexcept
      :
        m_sth(alloc)
      { this->m_sth.share_with(other.m_sth);  }

    cow_vector(cow_vector&& other) noexcept
      :
        m_sth(move(other.m_sth.as_allocator()))
      { this->m_sth.exchange_with(other.m_sth);  }

    cow_vector(cow_vector&& other, const allocator_type& alloc) noexcept
      :
        m_sth(alloc)
      { this->m_sth.exchange_with(other.m_sth);  }

    explicit cow_vector(size_type n, const allocator_type& alloc = allocator_type())
      :
        cow_vector(alloc)
      { this->assign(n);  }

    cow_vector(size_type n, const value_type& value, const allocator_type& alloc = allocator_type())
      :
        cow_vector(alloc)
      { this->assign(n, value);  }

    // N.B. This is a non-standard extension.
    template<typename firstT, typename... restT,
    ROCKET_ENABLE_IF(is_constructible<value_type, const firstT&, const restT&...>::value),
    ROCKET_DISABLE_IF(is_convertible<const firstT&, allocator_type>::value)>
    cow_vector(size_type n, const firstT& first, const restT&... rest)
      :
        cow_vector()
      { this->assign(n, first, rest...);  }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_vector(inputT first, inputT last, const allocator_type& alloc = allocator_type())
      :
        cow_vector(alloc)
      { this->assign(move(first), move(last));  }

    cow_vector(initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
      :
        cow_vector(alloc)
      { this->assign(init.begin(), init.end());  }

    cow_vector&
    operator=(const cow_vector& other) & noexcept
      {
        noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.share_with(other.m_sth);
        return *this;
      }

    cow_vector&
    operator=(cow_vector&& other) & noexcept
      {
        noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }

    cow_vector&
    operator=(initializer_list<value_type> init) &
      {
        this->assign(init.begin(), init.end());
        return *this;
      }

    cow_vector&
    swap(cow_vector& other) noexcept
      {
        noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }

  private:
    cow_vector&
    do_deallocate() noexcept
      {
        this->m_sth.deallocate();
        return *this;
      }

    [[noreturn]] ROCKET_NEVER_INLINE
    void
    do_throw_subscript_out_of_range(size_type pos, unsigned char rel) const
      {
        static constexpr char opstr[6][3] = { "==", "<", "<=", ">", ">=", "!=" };
        unsigned int inv = 5U - rel;

        noadl::sprintf_and_throw<out_of_range>(
            "cow_vector: subscript out of range (`%lld` %s `%lld`)",
            static_cast<long long>(pos), opstr[inv], static_cast<long long>(this->size()));
      }

#define ROCKET_COW_VECTOR_VALIDATE_SUBSCRIPT_(pos, op)  \
        if(!(pos op this->size()))  \
          this->do_throw_subscript_out_of_range(pos,  \
                  ((2 op 1) * 4 + (1 op 2) * 2 + (1 op 1) - 1))

    // This function works the same way as `substr()`. It ensures `tpos` is
    // within range and returns the number of elements that start there.
    constexpr
    size_type
    do_clamp_subvec(size_type tpos, size_type tn) const
      {
        ROCKET_COW_VECTOR_VALIDATE_SUBSCRIPT_(tpos, <=);
        return noadl::min(this->size() - tpos, tn);
      }

    // This function is used to implement `insert()` after new elements have
    // been appended. `tpos` is the position to insert. `old_size` is the old
    // size before `append()`.
    value_type*
    do_swizzle_unchecked(size_type tpos, size_type old_size)
      {
        auto ptr = this->mut_data();
        noadl::rotate(ptr, tpos, old_size, this->size());
        return ptr + tpos;
      }

    // This function is used to implement `erase()`.
    value_type*
    do_erase_unchecked(size_type tpos, size_type tlen)
      {
        auto ptr = this->mut_data();
        noadl::rotate(ptr, tpos, tpos + tlen, this->size());
        this->m_sth.pop_n_unchecked(tlen);
        return ptr + tpos;
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
    // N.B. This function may throw `std::bad_alloc`.
    iterator
    mut_begin()
      { return iterator(this->mut_data(), 0, this->size());  }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    iterator
    mut_end()
      { return iterator(this->mut_data(), this->size(), this->size());  }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    reverse_iterator
    mut_rbegin()
      { return reverse_iterator(this->mut_end());  }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
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

    // 26.3.11.3, capacity
    bool
    empty() const noexcept
      { return this->m_sth.size() == 0;  }

    size_type
    size() const noexcept
      { return this->m_sth.size();  }

    // N.B. This is a non-standard extension.
    difference_type
    ssize() const noexcept
      { return static_cast<difference_type>(this->size());  }

    size_type
    max_size() const noexcept
      { return this->m_sth.max_size();  }

    // N.B. The return type and the parameter pack are non-standard extensions.
    template<typename... paramsT>
    cow_vector&
    resize(size_type n, const paramsT&... params)
      {
        return (this->size() < n)
                 ? this->append(n - this->size(), params...)
                 : this->pop_back(this->size() - n);
      }

    size_type
    capacity() const noexcept
      { return this->m_sth.capacity();  }

    // N.B. The return type is a non-standard extension.
    cow_vector&
    reserve(size_type res_arg)
      {
        // Note zero is a special request to reduce capacity.
        if(res_arg == 0)
          return this->shrink_to_fit();

        // Calculate the minimum capacity to reserve. This must include all existent elements.
        // Don't reallocate if the storage is unique and there is enough room.
        size_type rcap = this->m_sth.round_up_capacity(noadl::max(this->size(), res_arg));
        if(this->unique() && (this->capacity() >= rcap))
          return *this;

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        sth.reallocate_prepare(this->m_sth, 0, rcap - this->size());
        this->m_sth.exchange_with(sth);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    cow_vector&
    shrink_to_fit()
      {
        // If the vector is empty, deallocate any dynamic storage.
        if(this->empty())
          return this->do_deallocate();

        // Calculate the minimum capacity to reserve. This must include all existent elements.
        // Don't reallocate if the storage is shared or tight.
        size_type rcap = this->m_sth.round_up_capacity(this->size());
        if(!this->unique() || (this->capacity() <= rcap))
          return *this;

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        sth.reallocate_prepare(this->m_sth, 0, 0);
        this->m_sth.exchange_with(sth);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    cow_vector&
    clear() noexcept
      {
        // If storage is shared, detach it.
        if(!this->m_sth.unique())
          return this->do_deallocate();

        this->m_sth.pop_n_unchecked(this->size());
        return *this;
      }

    // N.B. This is a non-standard extension.
    bool
    unique() const noexcept
      { return this->m_sth.unique();  }

    // N.B. This is a non-standard extension.
    int
    use_count() const noexcept
      { return this->m_sth.use_count();  }

    // element access
    const_reference
    at(size_type pos) const
      {
        ROCKET_COW_VECTOR_VALIDATE_SUBSCRIPT_(pos, <);
        return this->data()[pos];
      }

    // N.B. This is a non-standard extension.
    const value_type*
    ptr(size_type pos) const noexcept
      {
        return (pos < this->size())
                 ? (this->data() + pos) : nullptr;
      }

    const_reference
    operator[](size_type pos) const noexcept
      {
        ROCKET_ASSERT(pos < this->size());
        return this->data()[pos];
      }

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
    // N.B. This function may throw `std::bad_alloc`.
    reference
    mut(size_type pos)
      {
        ROCKET_COW_VECTOR_VALIDATE_SUBSCRIPT_(pos, <);
        return this->mut_data()[pos];
      }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    value_type*
    mut_ptr(size_type pos)
      {
        return (pos < this->size())
                 ? (this->mut_data() + pos) : nullptr;
      }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    reference
    mut_front()
      {
        ROCKET_ASSERT(!this->empty());
        return this->mut_data()[0];
      }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    reference
    mut_back()
      {
        ROCKET_ASSERT(!this->empty());
        return this->mut_data()[this->size() - 1];
      }

    // N.B. This is a non-standard extension.
    template<typename... paramsT,
    ROCKET_ENABLE_IF(is_constructible<value_type, const paramsT&...>::value)>
    cow_vector&
    append(size_type n, const paramsT&... params)
      {
        if(n == 0)
          return *this;

        // Check whether the storage is unique and there is enough space.
        auto ptr = this->m_sth.mut_data_opt();
        size_type cap = this->capacity();
        size_type len = this->size();

        if(ROCKET_EXPECT(ptr && (n <= cap - len))) {
          this->m_sth.append_n_unchecked(n, params...);
          return *this;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        ptr = sth.reallocate_prepare(this->m_sth, len, n | cap / 2);
        sth.append_n_unchecked(n, params...);
        sth.reallocate_finish(this->m_sth);
        this->m_sth.exchange_with(sth);
        return *this;
      }

    // N.B. This is a non-standard extension.
    cow_vector&
    append(initializer_list<value_type> init)
      {
        return this->append(init.begin(), init.end());
      }

    // N.B. This is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_vector&
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
          this->m_sth.append_range_unchecked(move(first), move(last));
          return *this;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        if(ROCKET_EXPECT(dist && (dist == n))) {
          // The length is known.
          ptr = sth.reallocate_prepare(this->m_sth, len, n | cap / 2);
          sth.append_range_unchecked(move(first), move(last));
        }
        else {
          // The length is not known.
          ptr = sth.reallocate_prepare(this->m_sth, len, 17 | cap / 2);
          cap = sth.capacity();
          for(auto it = move(first);  it != last;  ++it) {
            if(ROCKET_UNEXPECT(sth.size() >= cap)) {
              ptr = sth.reallocate_prepare(sth, len, cap / 2);
              cap = sth.capacity();
            }
            sth.emplace_back_unchecked(*it);
          }
        }
        sth.reallocate_finish(this->m_sth);
        this->m_sth.exchange_with(sth);
        return *this;
      }

    // 26.3.11.5, modifiers
    template<typename... paramsT>
    reference
    emplace_back(paramsT&&... params)
      {
        // Check whether the storage is unique and there is enough space.
        auto ptr = this->m_sth.mut_data_opt();
        size_type cap = this->capacity();
        size_type len = this->size();

        if(ROCKET_EXPECT(ptr && (len < cap))) {
          auto& ref = this->m_sth.emplace_back_unchecked(forward<paramsT>(params)...);
          return ref;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator());
        ptr = sth.reallocate_prepare(this->m_sth, len, 17 | cap / 2);
        auto& ref = sth.emplace_back_unchecked(forward<paramsT>(params)...);
        sth.reallocate_finish(this->m_sth);
        this->m_sth.exchange_with(sth);
        return ref;
      }

    // N.B. The return type is a non-standard extension.
    cow_vector&
    push_back(const value_type& value)
      {
        this->emplace_back(value);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    cow_vector&
    push_back(value_type&& value)
      {
        this->emplace_back(move(value));
        return *this;
      }

    // N.B. This is a non-standard extension.
    cow_vector&
    insert(size_type tpos, const value_type& value)
      {
        this->do_clamp_subvec(tpos, 0);  // just check
        size_type kpos = this->size();
        this->push_back(value);
        this->do_swizzle_unchecked(tpos, kpos);
        return *this;
      }

    // N.B. This is a non-standard extension.
    cow_vector&
    insert(size_type tpos, value_type&& value)
      {
        this->do_clamp_subvec(tpos, 0);  // just check
        size_type kpos = this->size();
        this->push_back(move(value));
        this->do_swizzle_unchecked(tpos, kpos);
        return *this;
      }

    // N.B. This is a non-standard extension.
    template<typename... paramsT,
    ROCKET_ENABLE_IF(is_constructible<value_type, const paramsT&...>::value)>
    cow_vector&
    insert(size_type tpos, size_type n, const paramsT&... params)
      {
        this->do_clamp_subvec(tpos, 0);  // just check
        size_type kpos = this->size();
        this->append(n, params...);
        this->do_swizzle_unchecked(tpos, kpos);
        return *this;
      }

    // N.B. This is a non-standard extension.
    cow_vector&
    insert(size_type tpos, initializer_list<value_type> init)
      {
        this->do_clamp_subvec(tpos, 0);  // just check
        size_type kpos = this->size();
        this->append(init);
        this->do_swizzle_unchecked(tpos, kpos);
        return *this;
      }

    // N.B. This is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_vector&
    insert(size_type tpos, inputT first, inputT last)
      {
        this->do_clamp_subvec(tpos, 0);  // just check
        size_type kpos = this->size();
        this->append(move(first), move(last));
        this->do_swizzle_unchecked(tpos, kpos);
        return *this;
      }

    iterator
    insert(const_iterator tins, const value_type& value)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());
        size_type kpos = this->size();
        this->push_back(value);
        auto tptr = this->do_swizzle_unchecked(tpos, kpos);
        return iterator(tptr - tpos, tpos, this->size());
      }

    iterator
    insert(const_iterator tins, value_type&& value)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());
        size_type kpos = this->size();
        this->push_back(move(value));
        auto tptr = this->do_swizzle_unchecked(tpos, kpos);
        return iterator(tptr - tpos, tpos, this->size());
      }

    // N.B. The parameter pack is a non-standard extension.
    template<typename... paramsT,
    ROCKET_ENABLE_IF(is_constructible<value_type, const paramsT&...>::value)>
    iterator
    insert(const_iterator tins, size_type n, const paramsT&... params)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());
        size_type kpos = this->size();
        this->append(n, params...);
        auto tptr = this->do_swizzle_unchecked(tpos, kpos);
        return iterator(tptr - tpos, tpos, this->size());
      }

    iterator
    insert(const_iterator tins, initializer_list<value_type> init)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());
        size_type kpos = this->size();
        this->append(init);
        auto tptr = this->do_swizzle_unchecked(tpos, kpos);
        return iterator(tptr - tpos, tpos, this->size());
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    iterator
    insert(const_iterator tins, inputT first, inputT last)
      {
        size_type tpos = static_cast<size_type>(tins - this->begin());
        size_type kpos = this->size();
        this->append(move(first), move(last));
        auto tptr = this->do_swizzle_unchecked(tpos, kpos);
        return iterator(tptr - tpos, tpos, this->size());
      }

    // N.B. This is a non-standard extension.
    cow_vector&
    erase(size_type tpos, size_type tn = size_type(-1))
      {
        size_type tlen = this->do_clamp_subvec(tpos, tn);
        this->do_erase_unchecked(tpos, tlen);
        return *this;
      }

    iterator
    erase(const_iterator pos)
      {
        return this->erase(pos, pos + 1);
      }

    iterator
    erase(const_iterator first, const_iterator last)
      {
        ROCKET_ASSERT_MSG(first <= last, "invalid range");
        size_type tpos = static_cast<size_type>(first - this->begin());
        size_type tlen = static_cast<size_type>(last - first);
        auto tptr = this->do_erase_unchecked(tpos, tlen);
        return iterator(tptr - tpos, tpos, this->size());
      }

    // N.B. The return type and parameter are non-standard extensions.
    cow_vector&
    pop_back(size_type n = 1)
      {
        this->mut_data();
        this->m_sth.pop_n_unchecked(n);
        return *this;
      }

    // N.B. This is a non-standard extension.
    cow_vector
    subvec(size_type tpos, size_type tn = size_type(-1)) const
      {
        size_type tlen = this->do_clamp_subvec(tpos, tn);
        cow_vector res(this->m_sth.as_allocator());

        // Duplicate the subrange.
        res.m_sth.reallocate_prepare(res.m_sth, 0, tlen);
        res.m_sth.append_range_unchecked(this->data() + tpos, this->data() + tpos + tlen);
        return res;
      }

    // N.B. The parameter pack is a non-standard extension.
    // N.B. The return type is a non-standard extension.
    template<typename... paramsT,
    ROCKET_ENABLE_IF(is_constructible<value_type, const paramsT&...>::value)>
    cow_vector&
    assign(size_type n, const paramsT&... params)
      {
        this->clear();
        this->append(n, params...);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_vector&
    assign(inputT first, inputT last)
      {
        this->clear();
        this->append(move(first), move(last));
        return *this;
      }

    // 26.3.11.4, data access
    const value_type*
    data() const noexcept
      { return this->m_sth.data();  }

    // Get a pointer to mutable data.
    // N.B. This is a non-standard extension.
    ROCKET_ALWAYS_INLINE
    value_type*
    mut_data()
      {
        auto ptr = this->m_sth.mut_data_opt();
        if(ROCKET_EXPECT(ptr))
          return ptr;

        // If the vector is empty, return a pointer to constant storage.
        if(this->empty())
          return const_cast<value_type*>(this->data());

        // Reallocate the storage. The length is left intact.
        ptr = this->m_sth.reallocate_clone(this->m_sth);
        return ptr;
      }

    // N.B. The return type differs from `std::vector`.
    constexpr
    const allocator_type&
    get_allocator() const noexcept
      { return this->m_sth.as_allocator();  }

    allocator_type&
    get_allocator() noexcept
      { return this->m_sth.as_allocator();  }
  };

template<typename valueT, typename allocT>
inline
void
swap(cow_vector<valueT, allocT>& lhs, cow_vector<valueT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket
#endif
