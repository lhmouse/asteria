// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_VECTOR_HPP_
#define ROCKET_COW_VECTOR_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"
#include "reference_counter.hpp"

namespace rocket {

/* Differences from `std::vector`:
 * 1. All functions guarantee only basic exception safety rather than strong exception safety, hence are more efficient.
 * 2. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always return `const_reference`s.
 * 3. The copy constructor and copy assignment operator will not throw exceptions.
 * 4. The specialization for `bool` is not provided.
 * 5. `emplace()` is not provided.
 * 6. Comparison operators are not provided.
 * 7. The value type may be incomplete. It need be neither copy-assignable nor move-assignable, but must be swappable.
 */
template<typename valueT, typename allocT = allocator<valueT>> class cow_vector;

#include "details/cow_vector.tcc"

template<typename valueT, typename allocT> class cow_vector
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

    using const_iterator          = details_cow_vector::vector_iterator<cow_vector, const value_type>;
    using iterator                = details_cow_vector::vector_iterator<cow_vector, value_type>;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

  private:
    details_cow_vector::storage_handle<allocator_type> m_sth;

  public:
    // 26.3.11.2, construct/copy/destroy
    explicit constexpr cow_vector(const allocator_type& alloc) noexcept
      :
        m_sth(alloc)
      {
      }
    cow_vector(const cow_vector& other) noexcept
      :
        m_sth(allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
      {
        this->assign(other);
      }
    cow_vector(const cow_vector& other, const allocator_type& alloc) noexcept
      :
        m_sth(alloc)
      {
        this->assign(other);
      }
    cow_vector(cow_vector&& other) noexcept
      :
        m_sth(::std::move(other.m_sth.as_allocator()))
      {
        this->assign(::std::move(other));
      }
    cow_vector(cow_vector&& other, const allocator_type& alloc) noexcept
      :
        m_sth(alloc)
      {
        this->assign(::std::move(other));
      }
    constexpr cow_vector(nullopt_t = nullopt_t()) noexcept(is_nothrow_constructible<allocator_type>::value)
      :
        cow_vector(allocator_type())
      {
      }
    explicit cow_vector(size_type n, const allocator_type& alloc = allocator_type())
      :
        cow_vector(alloc)
      {
        this->assign(n);
      }
    cow_vector(size_type n, const value_type& value, const allocator_type& alloc = allocator_type())
      :
        cow_vector(alloc)
      {
        this->assign(n, value);
      }
    // N.B. This is a non-standard extension.
    template<typename firstT, typename... restT, ROCKET_DISABLE_IF(is_same<typename decay<firstT>::type, allocator_type>::value)>
        cow_vector(size_type n, const firstT& first, const restT&... rest)
      :
        cow_vector()
      {
        this->assign(n, first, rest...);
      }
    template<typename inputT, ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
        cow_vector(inputT first, inputT last, const allocator_type& alloc = allocator_type())
      :
        cow_vector(alloc)
      {
        this->assign(::std::move(first), ::std::move(last));
      }
    cow_vector(initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
      :
        cow_vector(alloc)
      {
        this->assign(init);
      }
    cow_vector& operator=(nullopt_t) noexcept
      {
        this->clear();
        return *this;
      }
    cow_vector& operator=(const cow_vector& other) noexcept
      {
        noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->assign(other);
        return *this;
      }
    cow_vector& operator=(cow_vector&& other) noexcept
      {
        noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->assign(::std::move(other));
        return *this;
      }
    cow_vector& operator=(initializer_list<value_type> init)
      {
        this->assign(init);
        return *this;
      }

  private:
    // Reallocate the storage to `res_arg` elements.
    // The storage is owned by the current vector exclusively after this function returns normally.
    value_type* do_reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
      {
        ROCKET_ASSERT(cnt_one <= off_two);
        ROCKET_ASSERT(off_two <= this->m_sth.size());
        ROCKET_ASSERT(cnt_two <= this->m_sth.size() - off_two);
        auto ptr = this->m_sth.reallocate(cnt_one, off_two, cnt_two, res_arg);
        if(!ptr) {
          // The storage has been deallocated.
          return nullptr;
        }
        ROCKET_ASSERT(this->m_sth.unique());
        return ptr;
      }
    // Clear contents. Deallocate the storage if it is shared at all.
    void do_clear() noexcept
      {
        if(!this->unique()) {
          this->m_sth.deallocate();
          return;
        }
        this->m_sth.pop_back_n_unchecked(this->m_sth.size());
      }

    // Reallocate more storage as needed, without shrinking.
    void do_reserve_more(size_type cap_add)
      {
        auto cnt = this->size();
        auto cap = this->m_sth.check_size_add(cnt, cap_add);
        if(!this->unique() || ROCKET_UNEXPECT(this->capacity() < cap)) {
#ifndef ROCKET_DEBUG
          // Reserve more space for non-debug builds.
          cap = noadl::max(cap, cnt + cnt / 2 + 7);
#endif
          this->do_reallocate(0, 0, cnt, cap | 1);
        }
        ROCKET_ASSERT(this->capacity() >= cap);
      }

    [[noreturn]] ROCKET_NOINLINE void do_throw_subscript_out_of_range(size_type pos) const
      {
        noadl::sprintf_and_throw<out_of_range>("cow_vector: subscript out of range (`%llu` > `%llu`)",
                                               static_cast<unsigned long long>(pos),
                                               static_cast<unsigned long long>(this->size()));
      }

    // This function works the same way as `std::string::substr()`.
    // Ensure `tpos` is in `[0, size()]` and return `min(tn, size() - tpos)`.
    ROCKET_PURE_FUNCTION size_type do_clamp_subvec(size_type tpos, size_type tn) const
      {
        auto tcnt = this->size();
        if(tpos > tcnt) {
          this->do_throw_subscript_out_of_range(tpos);
        }
        return noadl::min(tcnt - tpos, tn);
      }

    template<typename... paramsT> value_type* do_insert_no_bound_check(size_type tpos, paramsT&&... params)
      {
        auto cnt_old = this->size();
        ROCKET_ASSERT(tpos <= cnt_old);
        details_cow_vector::tagged_append(this, ::std::forward<paramsT>(params)...);
        auto cnt_add = this->size() - cnt_old;
        this->do_reserve_more(0);
        auto ptr = this->m_sth.mut_data_unchecked();
        noadl::rotate(ptr, tpos, cnt_old, cnt_old + cnt_add);
        return ptr + tpos;
      }
    value_type* do_erase_no_bound_check(size_type tpos, size_type tn)
      {
        auto cnt_old = this->size();
        ROCKET_ASSERT(tpos <= cnt_old);
        ROCKET_ASSERT(tn <= cnt_old - tpos);
        if(!this->unique()) {
          auto ptr = this->do_reallocate(tpos, tpos + tn, cnt_old - (tpos + tn), cnt_old);
          return ptr + tpos;
        }
        auto ptr = this->m_sth.mut_data_unchecked();
        noadl::rotate(ptr, tpos, tpos + tn, cnt_old);
        this->m_sth.pop_back_n_unchecked(tn);
        return ptr + tpos;
      }

  public:
    // iterators
    const_iterator begin() const noexcept
      {
        return const_iterator(this->m_sth, this->data());
      }
    const_iterator end() const noexcept
      {
        return const_iterator(this->m_sth, this->data() + this->size());
      }
    const_reverse_iterator rbegin() const noexcept
      {
        return const_reverse_iterator(this->end());
      }
    const_reverse_iterator rend() const noexcept
      {
        return const_reverse_iterator(this->begin());
      }

    const_iterator cbegin() const noexcept
      {
        return this->begin();
      }
    const_iterator cend() const noexcept
      {
        return this->end();
      }
    const_reverse_iterator crbegin() const noexcept
      {
        return this->rbegin();
      }
    const_reverse_iterator crend() const noexcept
      {
        return this->rend();
      }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator mut_begin()
      {
        return iterator(this->m_sth, this->mut_data());
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator mut_end()
      {
        return iterator(this->m_sth, this->mut_data() + this->size());
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    reverse_iterator mut_rbegin()
      {
        return reverse_iterator(this->mut_end());
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    reverse_iterator mut_rend()
      {
        return reverse_iterator(this->mut_begin());
      }

    // 26.3.11.3, capacity
    bool empty() const noexcept
      {
        return this->m_sth.empty();
      }
    size_type size() const noexcept
      {
        return this->m_sth.size();
      }
    // N.B. This is a non-standard extension.
    difference_type ssize() const noexcept
      {
        return static_cast<difference_type>(this->size());
      }
    size_type max_size() const noexcept
      {
        return this->m_sth.max_size();
      }
    // N.B. The return type and the parameter pack are non-standard extensions.
    template<typename... paramsT> cow_vector& resize(size_type n, const paramsT&... params)
      {
        auto cnt_old = this->size();
        if(cnt_old < n) {
          this->append(n - cnt_old, params...);
        }
        else if(cnt_old > n) {
          this->pop_back(cnt_old - n);
        }
        ROCKET_ASSERT(this->size() == n);
        return *this;
      }
    size_type capacity() const noexcept
      {
        return this->m_sth.capacity();
      }
    // N.B. The return type is a non-standard extension.
    cow_vector& reserve(size_type res_arg)
      {
        auto cnt = this->size();
        auto cap_new = this->m_sth.round_up_capacity(noadl::max(cnt, res_arg));
        // If the storage is shared with other vectors, force rellocation to prevent copy-on-write upon modification.
        if(this->unique() && (this->capacity() >= cap_new)) {
          return *this;
        }
        this->do_reallocate(0, 0, cnt, cap_new);
        ROCKET_ASSERT(this->capacity() >= res_arg);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    cow_vector& shrink_to_fit()
      {
        auto cnt = this->size();
        auto cap_min = this->m_sth.round_up_capacity(cnt);
        // Don't increase memory usage.
        if(!this->unique() || (this->capacity() <= cap_min)) {
          return *this;
        }
        this->do_reallocate(0, 0, cnt, cnt);
        ROCKET_ASSERT(this->capacity() <= cap_min);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    cow_vector& clear() noexcept
      {
        if(this->empty()) {
          return *this;
        }
        this->do_clear();
        return *this;
      }
    // N.B. This is a non-standard extension.
    bool unique() const noexcept
      {
        return this->m_sth.unique();
      }
    // N.B. This is a non-standard extension.
    long use_count() const noexcept
      {
        return this->m_sth.use_count();
      }

    // element access
    const_reference at(size_type pos) const
      {
        auto cnt = this->size();
        if(pos >= cnt) {
          this->do_throw_subscript_out_of_range(pos);
        }
        return this->data()[pos];
      }
    const_reference operator[](size_type pos) const noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(pos < cnt);
        return this->data()[pos];
      }
    const_reference front() const noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[0];
      }
    const_reference back() const noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[cnt - 1];
      }
    // N.B. This is a non-standard extension.
    const value_type* get_ptr(size_type pos) const noexcept
      {
        auto cnt = this->size();
        if(pos >= cnt) {
          return nullptr;
        }
        return this->data() + pos;
      }

    // There is no `at()` overload that returns a non-const reference. This is the consequent overload which does that.
    // N.B. This is a non-standard extension.
    reference mut(size_type pos)
      {
        auto cnt = this->size();
        if(pos >= cnt) {
          this->do_throw_subscript_out_of_range(pos);
        }
        return this->mut_data()[pos];
      }
    // N.B. This is a non-standard extension.
    reference mut_front()
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->mut_data()[0];
      }
    // N.B. This is a non-standard extension.
    reference mut_back()
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->mut_data()[cnt - 1];
      }
    // N.B. This is a non-standard extension.
    value_type* mut_ptr(size_type pos) noexcept
      {
        auto cnt = this->size();
        if(pos >= cnt) {
          return nullptr;
        }
        return this->mut_data() + pos;
      }

    // N.B. This is a non-standard extension.
    template<typename... paramsT> cow_vector& append(size_type n, const paramsT&... params)
      {
        if(n == 0) {
          return *this;
        }
        this->do_reserve_more(n);
        noadl::ranged_do_while(size_type(0), n, [&](size_type) { this->m_sth.emplace_back_unchecked(params...);  });
        return *this;
      }
    // N.B. This is a non-standard extension.
    cow_vector& append(initializer_list<value_type> init)
      {
        return this->append(init.begin(), init.end());
      }
    // N.B. This is a non-standard extension.
    template<typename inputT, ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)> cow_vector& append(inputT first, inputT last)
      {
        if(first == last) {
          return *this;
        }
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
    template<typename... paramsT> reference emplace_back(paramsT&&... params)
      {
        this->do_reserve_more(1);
        auto ptr = this->m_sth.emplace_back_unchecked(::std::forward<paramsT>(params)...);
        return *ptr;
      }
    // N.B. The return type is a non-standard extension.
    reference push_back(const value_type& value)
      {
        auto cnt_old = this->size();
        // Check for overlapped elements before `do_reserve_more()`.
        auto srpos = static_cast<uintptr_t>(::std::addressof(value) - this->data());
        this->do_reserve_more(1);
        if(srpos < cnt_old) {
          auto ptr = this->m_sth.emplace_back_unchecked(this->data()[srpos]);
          return *ptr;
        }
        auto ptr = this->m_sth.emplace_back_unchecked(value);
        return *ptr;
      }
    // N.B. The return type is a non-standard extension.
    reference push_back(value_type&& value)
      {
        this->do_reserve_more(1);
        auto ptr = this->m_sth.emplace_back_unchecked(::std::move(value));
        return *ptr;
      }

    // N.B. This is a non-standard extension.
    cow_vector& insert(size_type tpos, const value_type& value)
      {
        this->do_insert_no_bound_check(tpos + this->do_clamp_subvec(tpos, 0), details_cow_vector::push_back, value);
        return *this;
      }
    // N.B. This is a non-standard extension.
    cow_vector& insert(size_type tpos, value_type&& value)
      {
        this->do_insert_no_bound_check(tpos + this->do_clamp_subvec(tpos, 0), details_cow_vector::push_back, ::std::move(value));
        return *this;
      }
    // N.B. This is a non-standard extension.
    template<typename... paramsT> cow_vector& insert(size_type tpos, size_type n, const paramsT&... params)
      {
        this->do_insert_no_bound_check(tpos + this->do_clamp_subvec(tpos, 0), details_cow_vector::append, n, params...);
        return *this;
      }
    // N.B. This is a non-standard extension.
    cow_vector& insert(size_type tpos, initializer_list<value_type> init)
      {
        this->do_insert_no_bound_check(tpos + this->do_clamp_subvec(tpos, 0), details_cow_vector::append, init);
        return *this;
      }
    // N.B. This is a non-standard extension.
    template<typename inputT, ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)> cow_vector& insert(size_type tpos, inputT first, inputT last)
      {
        this->do_insert_no_bound_check(tpos + this->do_clamp_subvec(tpos, 0), details_cow_vector::append, ::std::move(first), ::std::move(last));
        return *this;
      }
    iterator insert(const_iterator tins, const value_type& value)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_cow_vector::push_back, value);
        return iterator(this->m_sth, ptr);
      }
    iterator insert(const_iterator tins, value_type&& value)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_cow_vector::push_back, ::std::move(value));
        return iterator(this->m_sth, ptr);
      }
    // N.B. The parameter pack is a non-standard extension.
    template<typename... paramsT> iterator insert(const_iterator tins, size_type n, const paramsT&... params)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_cow_vector::append, n, params...);
        return iterator(this->m_sth, ptr);
      }
    iterator insert(const_iterator tins, initializer_list<value_type> init)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_cow_vector::append, init);
        return iterator(this->m_sth, ptr);
      }
    template<typename inputT, ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)> iterator insert(const_iterator tins, inputT first, inputT last)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_cow_vector::append, ::std::move(first), ::std::move(last));
        return iterator(this->m_sth, ptr);
      }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    cow_vector& erase(size_type tpos, size_type tn = size_type(-1))
      {
        this->do_erase_no_bound_check(tpos, this->do_clamp_subvec(tpos, tn));
        return *this;
      }
    // N.B. This function may throw `std::bad_alloc`.
    iterator erase(const_iterator tfirst, const_iterator tlast)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this->m_sth) - tfirst.tell());
        auto ptr = this->do_erase_no_bound_check(tpos, tn);
        return iterator(this->m_sth, ptr);
      }
    // N.B. This function may throw `std::bad_alloc`.
    iterator erase(const_iterator tfirst)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_erase_no_bound_check(tpos, 1);
        return iterator(this->m_sth, ptr);
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. The return type and parameter are non-standard extensions.
    cow_vector& pop_back(size_type n = 1)
      {
        auto cnt_old = this->size();
        ROCKET_ASSERT(n <= cnt_old);
        if(n == 0) {
          return *this;
        }
        if(!this->unique()) {
          this->do_reallocate(0, 0, cnt_old - n, cnt_old);
          return *this;
        }
        this->m_sth.pop_back_n_unchecked(n);
        return *this;
      }

    // N.B. This is a non-standard extension.
    cow_vector subvec(size_type tpos, size_type tn = size_type(-1)) const
      {
        if((tpos == 0) && (tn >= this->size())) {
          // Utilize reference counting.
          return cow_vector(*this, this->m_sth.as_allocator());
        }
        return cow_vector(this->data() + tpos, this->data() + tpos + this->do_clamp_subvec(tpos, tn),
                          this->m_sth.as_allocator());
      }

    // N.B. The return type is a non-standard extension.
    cow_vector& assign(const cow_vector& other) noexcept
      {
        this->m_sth.share_with(other.m_sth);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    cow_vector& assign(cow_vector&& other) noexcept
      {
        this->m_sth.share_with(::std::move(other.m_sth));
        return *this;
      }
    // N.B. The parameter pack is a non-standard extension.
    // N.B. The return type is a non-standard extension.
    template<typename... paramsT> cow_vector& assign(size_type n, const paramsT&... params)
      {
        this->clear();
        this->append(n, params...);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    cow_vector& assign(initializer_list<value_type> init)
      {
        this->clear();
        this->append(init);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    template<typename inputT, ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)> cow_vector& assign(inputT first, inputT last)
      {
        this->clear();
        this->append(::std::move(first), ::std::move(last));
        return *this;
      }

    cow_vector& swap(cow_vector& other) noexcept
      {
        noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }

    // 26.3.11.4, data access
    const value_type* data() const noexcept
      {
        return this->m_sth.data();
      }

    // Get a pointer to mutable data. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    value_type* mut_data()
      {
        if(!this->unique()) {
          this->do_reallocate(0, 0, this->size(), this->size() | 1);
        }
        return this->m_sth.mut_data_unchecked();
      }

    // N.B. The return type differs from `std::vector`.
    const allocator_type& get_allocator() const noexcept
      {
        return this->m_sth.as_allocator();
      }
    allocator_type& get_allocator() noexcept
      {
        return this->m_sth.as_allocator();
      }
  };

template<typename valueT, typename allocT>
    inline void swap(cow_vector<valueT, allocT>& lhs,
                     cow_vector<valueT, allocT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

}  // namespace rocket

#endif
