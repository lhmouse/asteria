// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_STATIC_VECTOR_HPP_
#define ROCKET_STATIC_VECTOR_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"

namespace rocket {

template<typename valueT, size_t capacityT, typename allocT = allocator<valueT>>
class static_vector;

#include "details/static_vector.ipp"

/* Differences from `std::vector`:
 * 1. The storage of elements are allocated inside the vector object, which eliminates dynamic allocation.
 * 2. An additional capacity template parameter is required.
 * 3. The specialization for `bool` is not provided.
 * 4. `emplace()` is not provided.
 * 5. `capacity()` is a `static constexpr` member function.
 * 6. Comparison operators are not provided.
 * 7. Incomplete element types are not supported.
 */

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

    using const_iterator          = details_static_vector::vector_iterator<static_vector, const value_type>;
    using iterator                = details_static_vector::vector_iterator<static_vector, value_type>;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

  private:
    details_static_vector::storage_handle<allocator_type, capacityT> m_sth;

  public:
    // 26.3.11.2, construct/copy/destroy
    explicit
    static_vector(const allocator_type& alloc)
    noexcept
      : m_sth(alloc)
      { }

    static_vector(const static_vector& other)
    noexcept(is_nothrow_copy_constructible<value_type>::value)
      : m_sth(allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
      { this->assign(other);  }

    static_vector(const static_vector& other, const allocator_type& alloc)
    noexcept(is_nothrow_copy_constructible<value_type>::value)
      : m_sth(alloc)
      { this->assign(other);  }

    static_vector(static_vector&& other)
    noexcept(is_nothrow_move_constructible<value_type>::value)
      : m_sth(::std::move(other.m_sth.as_allocator()))
      { this->assign(::std::move(other));  }

    static_vector(static_vector&& other, const allocator_type& alloc)
    noexcept(is_nothrow_move_constructible<value_type>::value)
      : m_sth(alloc)
      { this->assign(::std::move(other));  }

    static_vector(nullopt_t = nullopt_t())
    noexcept(is_nothrow_constructible<allocator_type>::value)
      : static_vector(allocator_type())
      { }

    explicit
    static_vector(size_type n, const allocator_type& alloc = allocator_type())
      : static_vector(alloc)
      { this->assign(n);  }

    static_vector(size_type n, const value_type& value, const allocator_type& alloc = allocator_type())
      : static_vector(alloc)
      { this->assign(n, value);  }

    template<typename firstT, typename... restT,
    ROCKET_DISABLE_IF(is_same<typename decay<firstT>::type, allocator_type>::value)>
    static_vector(size_type n, const firstT& first, const restT&... rest)
      : static_vector()
      { this->assign(n, first, rest...);  }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    static_vector(inputT first, inputT last, const allocator_type& alloc = allocator_type())
      : static_vector(alloc)
      { this->assign(::std::move(first), ::std::move(last));  }

    static_vector(initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
      : static_vector(alloc)
      { this->assign(init);  }

    static_vector&
    operator=(nullopt_t)
    noexcept
      {
        this->clear();
        return *this;
      }

    static_vector&
    operator=(const static_vector& other)
    noexcept(conjunction<is_nothrow_copy_assignable<value_type>,
                         is_nothrow_copy_constructible<value_type>>::value)
      {
        noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->assign(other);
        return *this;
      }

    static_vector&
    operator=(static_vector&& other)
    noexcept(conjunction<is_nothrow_move_assignable<value_type>,
                         is_nothrow_move_constructible<value_type>>::value)
      {
        noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->assign(::std::move(other));
        return *this;
      }

    static_vector&
    operator=(initializer_list<value_type> init)
      {
        this->assign(init);
        return *this;
      }

  private:
    // Reallocate more storage as needed, without shrinking.
    void
    do_reserve_more(size_type cap_add)
      {
        auto cnt = this->size();
        auto cap = this->m_sth.check_size_add(cnt, cap_add);
        ROCKET_ASSERT(this->capacity() >= cap);
      }

    [[noreturn]] ROCKET_NOINLINE
    void
    do_throw_subscript_out_of_range(size_type pos)
    const
      {
        noadl::sprintf_and_throw<out_of_range>("static_vector: subscript out of range (`%llu` > `%llu`)",
                                               static_cast<unsigned long long>(pos),
                                               static_cast<unsigned long long>(this->size()));
      }

    // This function works the same way as `std::string::substr()`.
    // Ensure `tpos` is in `[0, size()]` and return `min(tn, size() - tpos)`.
    size_type
    do_clamp_subvec(size_type tpos, size_type tn)
    const
      {
        auto tcnt = this->size();
        if(tpos > tcnt)
          this->do_throw_subscript_out_of_range(tpos);
        return noadl::min(tcnt - tpos, tn);
      }

    template<typename... paramsT>
    value_type*
    do_insert_no_bound_check(size_type tpos, paramsT&&... params)
      {
        auto cnt_old = this->size();
        ROCKET_ASSERT(tpos <= cnt_old);
        details_static_vector::tagged_append(this, ::std::forward<paramsT>(params)...);
        auto cnt_add = this->size() - cnt_old;
        auto ptr = this->m_sth.mut_data();
        noadl::rotate(ptr, tpos, cnt_old, cnt_old + cnt_add);
        return ptr + tpos;
      }

    value_type*
    do_erase_no_bound_check(size_type tpos, size_type tn)
      {
        auto cnt_old = this->size();
        ROCKET_ASSERT(tpos <= cnt_old);
        ROCKET_ASSERT(tn <= cnt_old - tpos);
        auto ptr = this->m_sth.mut_data();
        noadl::rotate(ptr, tpos, tpos + tn, cnt_old);
        this->m_sth.pop_back_n_unchecked(tn);
        return ptr + tpos;
      }

  public:
    // iterators
    const_iterator
    begin()
    const
    noexcept
      { return const_iterator(this->m_sth, this->data());  }

    const_iterator
    end()
    const
    noexcept
      { return const_iterator(this->m_sth, this->data() + this->size());  }

    const_reverse_iterator
    rbegin()
    const
    noexcept
      { return const_reverse_iterator(this->end());  }

    const_reverse_iterator
    rend()
    const
    noexcept
      { return const_reverse_iterator(this->begin());  }

    const_iterator
    cbegin()
    const
    noexcept
      { return this->begin();  }

    const_iterator
    cend()
    const
    noexcept
      { return this->end();  }

    const_reverse_iterator
    crbegin()
    const
    noexcept
      { return this->rbegin();  }

    const_reverse_iterator
    crend()
    const
    noexcept
      { return this->rend();  }

    // N.B. This is a non-standard extension.
    iterator
    mut_begin()
      { return iterator(this->m_sth, this->mut_data());  }

    // N.B. This is a non-standard extension.
    iterator
    mut_end()
      { return iterator(this->m_sth, this->mut_data() + this->size());  }

    // N.B. This is a non-standard extension.
    reverse_iterator
    mut_rbegin()
      { return reverse_iterator(this->mut_end());  }

    // N.B. This is a non-standard extension.
    reverse_iterator
    mut_rend()
      { return reverse_iterator(this->mut_begin());  }

    // 26.3.11.3, capacity
    constexpr
    bool
    empty()
    const
    noexcept
      { return this->m_sth.empty();  }

    constexpr
    size_type
    size()
    const
    noexcept
      { return this->m_sth.size();  }

    // N.B. This is a non-standard extension.
    constexpr
    difference_type
    ssize()
    const
    noexcept
      { return static_cast<difference_type>(this->size());  }

    constexpr
    size_type
    max_size()
    const
    noexcept
      { return this->m_sth.max_size();  }

    // N.B. The return type and the parameter pack are non-standard extensions.
    template<typename... paramsT>
    static_vector&
    resize(size_type n, const paramsT&... params)
      {
        auto cnt_old = this->size();
        if(cnt_old < n)
          this->append(n - cnt_old, params...);
        else if(cnt_old > n)
          this->pop_back(cnt_old - n);
        ROCKET_ASSERT(this->size() == n);
        return *this;
      }

    static constexpr
    size_type
    capacity()
    noexcept
      { return capacityT;  }

    // N.B. The return type is a non-standard extension.
    static_vector&
    reserve(size_type res_arg)
      {
        this->m_sth.check_size_add(0, res_arg);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    static_vector&
    shrink_to_fit()
    noexcept
      { return *this;  }

    // N.B. The return type is a non-standard extension.
    static_vector&
    clear()
    noexcept
      {
        if(this->empty())
          return *this;

        this->m_sth.pop_back_n_unchecked(this->m_sth.size());
        return *this;
      }

    // element access
    const_reference
    at(size_type pos)
    const
      {
        auto cnt = this->size();
        if(pos >= cnt)
          this->do_throw_subscript_out_of_range(pos);
        return this->data()[pos];
      }

    const_reference
    operator[](size_type pos)
    const
    noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(pos < cnt);
        return this->data()[pos];
      }

    const_reference
    front()
    const
    noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[0];
      }

    const_reference
    back()
    const
    noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[cnt - 1];
      }

    // N.B. This is a non-standard extension.
    const value_type*
    get_ptr(size_type pos)
    const
    noexcept
      {
        auto cnt = this->size();
        if(pos >= cnt)
          return nullptr;
        return this->data() + pos;
      }

    // There is no `at()` overload that returns a non-const reference.
    // This is the consequent overload which does that.
    // N.B. This is a non-standard extension.
    reference
    mut(size_type pos)
      {
        auto cnt = this->size();
        if(pos >= cnt)
          this->do_throw_subscript_out_of_range(pos);
        return this->mut_data()[pos];
      }

    reference
    operator[](size_type pos)
    noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(pos < cnt);
        return this->mut_data()[pos];
      }

    // N.B. This is a non-standard extension.
    reference
    mut_front()
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->mut_data()[0];
      }

    // N.B. This is a non-standard extension.
    reference
    mut_back()
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->mut_data()[cnt - 1];
      }

    // N.B. This is a non-standard extension.
    value_type*
    mut_ptr(size_type pos)
    noexcept
      {
        auto cnt = this->size();
        if(pos >= cnt)
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

        this->do_reserve_more(n);
        noadl::ranged_do_while(size_type(0), n, [&](size_type) { this->m_sth.emplace_back_unchecked(params...);  });
        return *this;
      }

    // N.B. This is a non-standard extension.
    static_vector&
    append(initializer_list<value_type> init)
      {
        return this->append(init.begin(), init.end());
      }

    // N.B. This is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    static_vector&
    append(inputT first, inputT last)
      {
        if(first == last)
          return *this;

        auto dist = noadl::estimate_distance(first, last);
        if(dist == 0) {
          noadl::ranged_do_while(::std::move(first), ::std::move(last),
                                 [&](const inputT& it) { this->emplace_back(*it);  });
          return *this;
        }
        this->do_reserve_more(dist);
        noadl::ranged_do_while(::std::move(first), ::std::move(last),
                               [&](const inputT& it) { this->m_sth.emplace_back_unchecked(*it);  });
        return *this;
      }

    // 26.3.11.5, modifiers
    template<typename... paramsT>
    reference
    emplace_back(paramsT&&... params)
      {
        this->do_reserve_more(1);
        auto ptr = this->m_sth.emplace_back_unchecked(::std::forward<paramsT>(params)...);
        return *ptr;
      }

    // N.B. The return type is a non-standard extension.
    reference
    push_back(const value_type& value)
      {
        auto cnt_old = this->size();
        // Check for overlapped elements before `do_reserve_more()`.
        auto srpos = static_cast<uintptr_t>(::std::addressof(value) - this->data());
        this->do_reserve_more(1);
        if(srpos < cnt_old) {
          auto ptr = this->m_sth.emplace_back_unchecked(this->m_sth.mut_data()[srpos]);
          return *ptr;
        }
        auto ptr = this->m_sth.emplace_back_unchecked(value);
        return *ptr;
      }

    // N.B. The return type is a non-standard extension.
    reference
    push_back(value_type&& value)
      {
        this->do_reserve_more(1);
        auto ptr = this->m_sth.emplace_back_unchecked(::std::move(value));
        return *ptr;
      }

    // N.B. This is a non-standard extension.
    static_vector&
    insert(size_type tpos, const value_type& value)
      {
        this->do_clamp_subvec(tpos, 0);  // just check
        this->do_insert_no_bound_check(tpos, details_static_vector::push_back, value);
        return *this;
      }

    // N.B. This is a non-standard extension.
    static_vector&
    insert(size_type tpos, value_type&& value)
      {
        this->do_clamp_subvec(tpos, 0);  // just check
        this->do_insert_no_bound_check(tpos, details_static_vector::push_back, ::std::move(value));
        return *this;
      }

    // N.B. This is a non-standard extension.
    template<typename... paramsT>
    static_vector&
    insert(size_type tpos, size_type n, const paramsT&... params)
      {
        this->do_clamp_subvec(tpos, 0);  // just check
        this->do_insert_no_bound_check(tpos, details_static_vector::append, n, params...);
        return *this;
      }

    // N.B. This is a non-standard extension.
    static_vector&
    insert(size_type tpos, initializer_list<value_type> init)
      {
        this->do_clamp_subvec(tpos, 0);  // just check
        this->do_insert_no_bound_check(tpos, details_static_vector::append, init);
        return *this;
      }

    // N.B. This is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    static_vector&
    insert(size_type tpos, inputT first, inputT last)
      {
        this->do_clamp_subvec(tpos, 0);  // just check
        this->do_insert_no_bound_check(tpos, details_static_vector::append, ::std::move(first), ::std::move(last));
        return *this;
      }

    iterator
    insert(const_iterator tins, const value_type& value)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_static_vector::push_back, value);
        return iterator(this->m_sth, ptr);
      }

    iterator
    insert(const_iterator tins, value_type&& value)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_static_vector::push_back, ::std::move(value));
        return iterator(this->m_sth, ptr);
      }

    // N.B. The parameter pack is a non-standard extension.
    template<typename... paramsT>
    iterator
    insert(const_iterator tins, size_type n, const paramsT&... params)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_static_vector::append, n, params...);
        return iterator(this->m_sth, ptr);
      }

    iterator
    insert(const_iterator tins, initializer_list<value_type> init)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_static_vector::append, init);
        return iterator(this->m_sth, ptr);
      }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    iterator
    insert(const_iterator tins, inputT first, inputT last)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_static_vector::append,
                                                  ::std::move(first), ::std::move(last));
        return iterator(this->m_sth, ptr);
      }

    // N.B. This is a non-standard extension.
    static_vector&
    erase(size_type tpos, size_type tn = size_type(-1))
      {
        this->do_erase_no_bound_check(tpos, this->do_clamp_subvec(tpos, tn));
        return *this;
      }

    iterator
    erase(const_iterator tfirst, const_iterator tlast)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this->m_sth) - tfirst.tell());
        auto ptr = this->do_erase_no_bound_check(tpos, tn);
        return iterator(this->m_sth, ptr);
      }

    iterator
    erase(const_iterator tfirst)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_erase_no_bound_check(tpos, 1);
        return iterator(this->m_sth, ptr);
      }

    // N.B. The return type and parameter are non-standard extensions.
    static_vector&
    pop_back(size_type n = 1)
      {
        auto cnt_old = this->size();
        ROCKET_ASSERT(n <= cnt_old);
        this->m_sth.pop_back_n_unchecked(n);
        return *this;
      }

    // N.B. This is a non-standard extension.
    static_vector
    subvec(size_type tpos, size_type tn = size_type(-1))
    const
      {
        return static_vector(this->data() + tpos, this->data() + tpos + this->do_clamp_subvec(tpos, tn),
                             this->m_sth.as_allocator());
      }

    // N.B. The return type is a non-standard extension.
    static_vector&
    assign(const static_vector& other)
    noexcept(conjunction<is_nothrow_copy_assignable<value_type>,
                         is_nothrow_copy_constructible<value_type>>::value)
      {
        // Copy-assign the initial sequence.
        auto ncomm = noadl::min(this->size(), other.size());
        for(size_type i = 0; i != ncomm; ++i) {
          this->m_sth.mut_data()[i] = other.m_sth.data()[i];
        }
        if(ncomm < other.size()) {
          // Copy-construct remaining elements from `other` if is longer.
          for(size_type i = ncomm; i != other.size(); ++i)
            this->m_sth.emplace_back_unchecked(other.m_sth.data()[i]);
        }
        else {
          // Truncate `*this` if it is longer.
          this->m_sth.pop_back_n_unchecked(this->size() - ncomm);
        }
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    static_vector&
    assign(static_vector&& other)
    noexcept(conjunction<is_nothrow_move_assignable<value_type>,
                         is_nothrow_move_constructible<value_type>>::value)
      {
        // Move-assign the initial sequence.
        auto ncomm = noadl::min(this->size(), other.size());
        for(size_type i = 0; i != ncomm; ++i) {
          this->m_sth.mut_data()[i] = ::std::move(other.m_sth.mut_data()[i]);
        }
        if(ncomm < other.size()) {
          // Move-construct remaining elements from `other` if is longer.
          for(size_type i = ncomm; i != other.size(); ++i)
            this->m_sth.emplace_back_unchecked(::std::move(other.m_sth.mut_data()[i]));
        }
        else {
          // Truncate `*this` if it is longer.
          this->m_sth.pop_back_n_unchecked(this->size() - ncomm);
        }
        return *this;
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
    static_vector&
    assign(initializer_list<value_type> init)
      {
        this->clear();
        this->append(init);
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

    static_vector&
    swap(static_vector& other)
    noexcept(conjunction<is_nothrow_swappable<value_type>,
                         is_nothrow_move_constructible<value_type>>::value)
      {
        noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        // Swap the initial sequence.
        auto ncomm = noadl::min(this->size(), other.size());
        for(size_type i = 0; i != ncomm; ++i) {
          noadl::xswap(this->m_sth.mut_data()[i], other.m_sth.mut_data()[i]);
        }
        if(ncomm < other.size()) {
          // Move-construct remaining elements from `other` if is longer.
          for(size_type i = ncomm; i != other.size(); ++i)
            this->m_sth.emplace_back_unchecked(::std::move(other.m_sth.mut_data()[i]));
          // Truncate `other`.
          other.m_sth.pop_back_n_unchecked(other.size() - ncomm);
        }
        else {
          // Move-construct remaining elements from `*this` if it is longer.
          for(size_type i = ncomm; i != this->size(); ++i)
            other.m_sth.emplace_back_unchecked(::std::move(this->m_sth.mut_data()[i]));
          // Truncate `*this`.
          this->m_sth.pop_back_n_unchecked(this->size() - ncomm);
        }
        return *this;
      }

    // 26.3.11.4, data access
    const value_type*
    data()
    const
    noexcept
      { return this->m_sth.data();  }

    // Get a pointer to mutable data.
    // N.B. This is a non-standard extension.
    value_type*
    mut_data()
      { return this->m_sth.mut_data();  }

    // N.B. The return type differs from `std::vector`.
    constexpr
    const allocator_type&
    get_allocator()
    const
    noexcept
      { return this->m_sth.as_allocator();  }

    allocator_type&
    get_allocator()
    noexcept
      { return this->m_sth.as_allocator();  }
  };

template<typename valueT, size_t capacityT, typename allocT>
inline
void
swap(static_vector<valueT, capacityT, allocT>& lhs, static_vector<valueT, capacityT, allocT>& rhs)
noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
