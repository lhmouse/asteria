// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_STATIC_VECTOR_HPP_
#define ROCKET_STATIC_VECTOR_HPP_

#include <iterator> // std::iterator_traits<>, std::random_access_iterator_tag
#include <cstring> // std::memset()
#include "compatibility.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"

/* Differences from `std::vector`:
 * 1. The storage of elements are allocated inside the vector object, which eliminates dynamic allocation.
 * 2. An additional capacity template parameter is required.
 * 3. The specialization for `bool` is not provided.
 * 4. `emplace()` is not provided.
 * 5. `capacity()` is a `static constexpr` member function.
 * 6. Comparison operators are not provided.
 * 7. Incomplete element types are not supported.
 */

namespace rocket {

template<typename valueT, size_t capacityT, typename allocatorT = allocator<valueT>>
  class static_vector;

    namespace details_static_vector {

    template<typename allocatorT, size_t capacityT>
      class storage_handle : private allocator_wrapper_base_for<allocatorT>::type
      {
      public:
        using allocator_type   = allocatorT;
        using value_type       = typename allocator_type::value_type;
        using size_type        = typename allocator_traits<allocator_type>::size_type;

      private:
        using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;

      private:
        size_type m_nelem;
        union { value_type m_ebase[capacityT]; };

      public:
        explicit storage_handle(const allocator_type &alloc) noexcept
          : allocator_base(alloc),
            m_nelem(0)
          {
#ifdef ROCKET_DEBUG
            ::std::memset(this->m_ebase, '*', sizeof(this->m_ebase));
#endif
          }
        explicit storage_handle(allocator_type &&alloc) noexcept
          : allocator_base(::std::move(alloc)),
            m_nelem(0)
          {
#ifdef ROCKET_DEBUG
            ::std::memset(this->m_ebase, '*', sizeof(this->m_ebase));
#endif
          }
        ~storage_handle()
          {
            const auto ebase = this->m_ebase;
            auto nrem = this->m_nelem;
            ROCKET_ASSERT(nrem <= capacityT);
            while(nrem != 0) {
              --nrem;
              allocator_traits<allocator_type>::destroy(this->as_allocator(), ebase + nrem);
            }
#ifdef ROCKET_DEBUG
            this->m_nelem = 0xCD99;
            ::std::memset(this->m_ebase, '~', sizeof(this->m_ebase));
#endif
          }

        storage_handle(const storage_handle &)
          = delete;
        storage_handle & operator=(const storage_handle &)
          = delete;

      public:
        const allocator_type & as_allocator() const noexcept
          {
            return static_cast<const allocator_base &>(*this);
          }
        allocator_type & as_allocator() noexcept
          {
            return static_cast<allocator_base &>(*this);
          }

        static constexpr size_type capacity() noexcept
          {
            return capacityT;
          }
        size_type max_size() const noexcept
          {
            return noadl::min(allocator_traits<allocator_type>::max_size(this->as_allocator()), this->capacity());
          }
        size_type check_size_add(size_type base, size_type add) const
          {
            const auto cap_max = this->max_size();
            ROCKET_ASSERT(base <= cap_max);
            if(cap_max - base < add) {
              noadl::throw_length_error("static_vector: Increasing `%lld` by `%lld` would exceed the max size `%lld`.",
                                        static_cast<long long>(base), static_cast<long long>(add), static_cast<long long>(cap_max));
            }
            return base + add;
          }
        const value_type * data() const noexcept
          {
            return this->m_ebase;
          }
        value_type * data() noexcept
          {
            return this->m_ebase;
          }
        size_type size() const noexcept
          {
            return this->m_nelem;
          }

        operator const storage_handle * () const noexcept
          {
            return this;
          }
        operator storage_handle * () noexcept
          {
            return this;
          }

        template<typename ...paramsT>
          value_type * emplace_back_unchecked(paramsT &&...params)
          {
            ROCKET_ASSERT(this->size() < this->capacity());
            const auto ebase = this->m_ebase;
            auto nelem = this->m_nelem;
            allocator_traits<allocator_type>::construct(this->as_allocator(), ebase + nelem, ::std::forward<paramsT>(params)...);
            this->m_nelem = (nelem += 1);
            return ebase + nelem - 1;
          }
        void pop_back_n_unchecked(size_type n) noexcept
          {
            ROCKET_ASSERT(n <= this->size());
            if(n == 0) {
              return;
            }
            const auto ebase = this->m_ebase;
            auto nelem = this->m_nelem;
            for(auto i = n; i != 0; --i) {
              this->m_nelem = (nelem -= 1);
              allocator_traits<allocator_type>::destroy(this->as_allocator(), ebase + nelem);
            }
          }
      };

    template<typename vectorT, typename valueT>
      class vector_iterator
      {
        template<typename, typename>
          friend class vector_iterator;
        friend vectorT;

      public:
        using iterator_category  = ::std::random_access_iterator_tag;
        using value_type         = valueT;
        using pointer            = value_type *;
        using reference          = value_type &;
        using difference_type    = ptrdiff_t;

        using parent_type   = storage_handle<typename vectorT::allocator_type, vectorT::capacity()>;

      private:
        const parent_type *m_ref;
        value_type *m_ptr;

      private:
        constexpr vector_iterator(const parent_type *ref, value_type *ptr) noexcept
          : m_ref(ref), m_ptr(ptr)
          {
          }

      public:
        constexpr vector_iterator() noexcept
          : vector_iterator(nullptr, nullptr)
          {
          }
        template<typename yvalueT, ROCKET_ENABLE_IF(is_convertible<yvalueT *, valueT *>::value)>
          constexpr vector_iterator(const vector_iterator<vectorT, yvalueT> &other) noexcept
          : vector_iterator(other.m_ref, other.m_ptr)
          {
          }

      private:
        value_type * do_assert_valid_pointer(value_type *ptr, bool to_dereference) const noexcept
          {
            const auto ref = this->m_ref;
            ROCKET_ASSERT_MSG(ref, "This iterator has not been initialized.");
            const auto dist = static_cast<size_t>(ptr - ref->data());
            ROCKET_ASSERT_MSG(dist <= ref->size(), "This iterator has been invalidated.");
            ROCKET_ASSERT_MSG(!(to_dereference && (dist == ref->size())), "This iterator contains a past-the-end value and cannot be dereferenced.");
            return ptr;
          }

      public:
        const parent_type * parent() const noexcept
          {
            return this->m_ref;
          }

        value_type * tell() const noexcept
          {
            const auto ptr = this->do_assert_valid_pointer(this->m_ptr, false);
            return ptr;
          }
        value_type * tell_owned_by(const parent_type *ref) const noexcept
          {
            ROCKET_ASSERT_MSG(this->m_ref == ref, "This iterator does not refer to an element in the same container.");
            return this->tell();
          }
        vector_iterator & seek(value_type *ptr) noexcept
          {
            this->m_ptr = this->do_assert_valid_pointer(ptr, false);
            return *this;
          }

        reference operator*() const noexcept
          {
            const auto ptr = this->do_assert_valid_pointer(this->m_ptr, true);
            return *ptr;
          }
        pointer operator->() const noexcept
          {
            const auto ptr = this->do_assert_valid_pointer(this->m_ptr, true);
            return ptr;
          }
        reference operator[](difference_type off) const noexcept
          {
            const auto ptr = this->do_assert_valid_pointer(this->m_ptr + off, true);
            return *ptr;
          }
      };

    template<typename vectorT, typename valueT>
      inline vector_iterator<vectorT, valueT> & operator++(vector_iterator<vectorT, valueT> &rhs) noexcept
      {
        return rhs.seek(rhs.tell() + 1);
      }
    template<typename vectorT, typename valueT>
      inline vector_iterator<vectorT, valueT> & operator--(vector_iterator<vectorT, valueT> &rhs) noexcept
      {
        return rhs.seek(rhs.tell() - 1);
      }

    template<typename vectorT, typename valueT>
      inline vector_iterator<vectorT, valueT> operator++(vector_iterator<vectorT, valueT> &lhs, int) noexcept
      {
        auto res = lhs;
        lhs.seek(lhs.tell() + 1);
        return res;
      }
    template<typename vectorT, typename valueT>
      inline vector_iterator<vectorT, valueT> operator--(vector_iterator<vectorT, valueT> &lhs, int) noexcept
      {
        auto res = lhs;
        lhs.seek(lhs.tell() - 1);
        return res;
      }

    template<typename vectorT, typename valueT>
      inline vector_iterator<vectorT, valueT> & operator+=(vector_iterator<vectorT, valueT> &lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
      {
        return lhs.seek(lhs.tell() + rhs);
      }
    template<typename vectorT, typename valueT>
      inline vector_iterator<vectorT, valueT> & operator-=(vector_iterator<vectorT, valueT> &lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
      {
        return lhs.seek(lhs.tell() - rhs);
      }

    template<typename vectorT, typename valueT>
      inline vector_iterator<vectorT, valueT> operator+(const vector_iterator<vectorT, valueT> &lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
      {
        auto res = lhs;
        res.seek(res.tell() + rhs);
        return res;
      }
    template<typename vectorT, typename valueT>
      inline vector_iterator<vectorT, valueT> operator-(const vector_iterator<vectorT, valueT> &lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
      {
        auto res = lhs;
        res.seek(res.tell() - rhs);
        return res;
      }

    template<typename vectorT, typename valueT>
      inline vector_iterator<vectorT, valueT> operator+(typename vector_iterator<vectorT, valueT>::difference_type lhs, const vector_iterator<vectorT, valueT> &rhs) noexcept
      {
        auto res = rhs;
        res.seek(res.tell() + lhs);
        return res;
      }
    template<typename vectorT, typename xvalueT, typename yvalueT>
      inline typename vector_iterator<vectorT, xvalueT>::difference_type operator-(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) - rhs.tell();
      }

    template<typename vectorT, typename xvalueT, typename yvalueT>
      inline bool operator==(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept
      {
        return lhs.tell() == rhs.tell();
      }
    template<typename vectorT, typename xvalueT, typename yvalueT>
      inline bool operator!=(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept
      {
        return lhs.tell() != rhs.tell();
      }

    template<typename vectorT, typename xvalueT, typename yvalueT>
      inline bool operator<(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) < rhs.tell();
      }
    template<typename vectorT, typename xvalueT, typename yvalueT>
      inline bool operator>(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) > rhs.tell();
      }
    template<typename vectorT, typename xvalueT, typename yvalueT>
      inline bool operator<=(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) <= rhs.tell();
      }
    template<typename vectorT, typename xvalueT, typename yvalueT>
      inline bool operator>=(const vector_iterator<vectorT, xvalueT> &lhs, const vector_iterator<vectorT, yvalueT> &rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) >= rhs.tell();
      }

    // Insertion helpers.
    struct append_tag
      {
      }
    constexpr append;

    template<typename vectorT, typename ...paramsT>
      inline void tagged_append(vectorT *vec, append_tag, paramsT &&...params)
      {
        vec->append(::std::forward<paramsT>(params)...);
      }

    struct emplace_back_tag
      {
      }
    constexpr emplace_back;

    template<typename vectorT, typename ...paramsT>
      inline void tagged_append(vectorT *vec, emplace_back_tag, paramsT &&...params)
      {
        vec->emplace_back(::std::forward<paramsT>(params)...);
      }

    struct push_back_tag
      {
      }
    constexpr push_back;

    template<typename vectorT, typename ...paramsT>
      inline void tagged_append(vectorT *vec, push_back_tag, paramsT &&...params)
      {
        vec->push_back(::std::forward<paramsT>(params)...);
      }

    }

template<typename valueT, size_t capacityT, typename allocatorT>
  class static_vector
  {
    static_assert(!is_array<valueT>::value, "`valueT` must not be an array type.");
    static_assert(is_same<typename allocatorT::value_type, valueT>::value, "`allocatorT::value_type` must denote the same type as `valueT`.");

  public:
    // types
    using value_type      = valueT;
    using allocator_type  = allocatorT;

    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using difference_type  = typename allocator_traits<allocator_type>::difference_type;
    using const_reference  = const value_type &;
    using reference        = value_type &;

    using const_iterator          = details_static_vector::vector_iterator<static_vector, const value_type>;
    using iterator                = details_static_vector::vector_iterator<static_vector, value_type>;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

  private:
    details_static_vector::storage_handle<allocator_type, capacityT> m_sth;

  public:
    // 26.3.11.2, construct/copy/destroy
    explicit static_vector(const allocator_type &alloc) noexcept
      : m_sth(alloc)
      {
      }
    static_vector() noexcept(is_nothrow_constructible<allocator_type>::value)
      : static_vector(allocator_type())
      {
      }
    static_vector(const static_vector &other) noexcept(is_nothrow_copy_constructible<value_type>::value)
      : static_vector(allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
      {
        this->assign(other);
      }
    static_vector(const static_vector &other, const allocator_type &alloc) noexcept(is_nothrow_copy_constructible<value_type>::value)
      : static_vector(alloc)
      {
        this->assign(other);
      }
    static_vector(static_vector &&other) noexcept(is_nothrow_move_constructible<value_type>::value)
      : static_vector(::std::move(other.m_sth.as_allocator()))
      {
        this->assign(::std::move(other));
      }
    static_vector(static_vector &&other, const allocator_type &alloc) noexcept(is_nothrow_move_constructible<value_type>::value)
      : static_vector(alloc)
      {
        this->assign(::std::move(other));
      }
    static_vector(size_type n, const allocator_type &alloc = allocator_type())
      : static_vector(alloc)
      {
        this->assign(n);
      }
    static_vector(size_type n, const value_type &value, const allocator_type &alloc = allocator_type())
      : static_vector(alloc)
      {
        this->assign(n, value);
      }
    template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
      static_vector(inputT first, inputT last, const allocator_type &alloc = allocator_type())
      : static_vector(alloc)
      {
        this->assign(::std::move(first), ::std::move(last));
      }
    static_vector(initializer_list<value_type> init, const allocator_type &alloc = allocator_type())
      : static_vector(alloc)
      {
        this->assign(init);
      }
    static_vector & operator=(const static_vector &other) noexcept(is_nothrow_copy_assignable<value_type>::value && is_nothrow_copy_constructible<value_type>::value)
      {
        this->assign(other);
        allocator_copy_assigner<allocator_type>()(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        return *this;
      }
    static_vector & operator=(static_vector &&other) noexcept(is_nothrow_move_assignable<value_type>::value && is_nothrow_move_constructible<value_type>::value)
      {
        this->assign(::std::move(other));
        allocator_move_assigner<allocator_type>()(this->m_sth.as_allocator(), ::std::move(other.m_sth.as_allocator()));
        return *this;
      }
    static_vector & operator=(initializer_list<value_type> init)
      {
        this->assign(init);
        return *this;
      }

  private:
    // Reallocate more storage as needed, without shrinking.
    void do_reserve_more(size_type cap_add)
      {
        const auto cnt = this->size();
        auto cap = this->m_sth.check_size_add(cnt, cap_add);
        ROCKET_ASSERT(this->capacity() >= cap);
      }

    template<typename ...paramsT>
      value_type * do_insert_no_bound_check(size_type tpos, paramsT &&...params)
      {
        const auto cnt_old = this->size();
        ROCKET_ASSERT(tpos <= cnt_old);
        details_static_vector::tagged_append(this, ::std::forward<paramsT>(params)...);
        const auto cnt_add = this->size() - cnt_old;
        const auto ptr = this->m_sth.data();
        noadl::rotate(ptr, tpos, cnt_old, cnt_old + cnt_add);
        return ptr + tpos;
      }
    value_type * do_erase_no_bound_check(size_type tpos, size_type tn)
      {
        const auto cnt_old = this->size();
        ROCKET_ASSERT(tpos <= cnt_old);
        ROCKET_ASSERT(tn <= cnt_old - tpos);
        const auto ptr = this->m_sth.data();
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

    iterator begin() noexcept
      {
        return iterator(this->m_sth, this->data());
      }
    iterator end() noexcept
      {
        return iterator(this->m_sth, this->data() + this->size());
      }
    reverse_iterator rbegin() noexcept
      {
        return reverse_iterator(this->end());
      }
    reverse_iterator rend() noexcept
      {
        return reverse_iterator(this->begin());
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

    // 26.3.11.3, capacity
    bool empty() const noexcept
      {
        return this->m_sth.size() == 0;
      }
    size_type size() const noexcept
      {
        return this->m_sth.size();
      }
    size_type max_size() const noexcept
      {
        return this->m_sth.max_size();
      }
    // N.B. The parameter pack is a non-standard extension.
    template<typename ...paramsT>
      void resize(size_type n, const paramsT &...params)
      {
        const auto cnt_old = this->size();
        if(cnt_old == n) {
          return;
        }
        if(cnt_old < n) {
          this->append(n - cnt_old, params...);
        } else {
          this->pop_back(cnt_old - n);
        }
        ROCKET_ASSERT(this->size() == n);
      }
    static constexpr size_type capacity() noexcept
      {
        return capacityT;
      }
    void reserve(size_type res_arg)
      {
        this->m_sth.check_size_add(0, res_arg);
        // There is nothing to do.
      }
    void shrink_to_fit() noexcept
      {
        // There is nothing to do.
      }
    void clear() noexcept
      {
        this->m_sth.pop_back_n_unchecked(this->size());
      }

    // element access
    const_reference operator[](size_type pos) const noexcept
      {
        const auto cnt = this->size();
        ROCKET_ASSERT(pos < cnt);
        return this->data()[pos];
      }
    const_reference at(size_type pos) const
      {
        const auto cnt = this->size();
        if(pos >= cnt) {
          noadl::throw_out_of_range("static_vector: The subscript `%lld` is not a writable position within a vector of size `%lld`.",
                                    static_cast<long long>(pos), static_cast<long long>(cnt));
        }
        return this->data()[pos];
      }
    const_reference front() const noexcept
      {
        const auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[0];
      }
    const_reference back() const noexcept
      {
        const auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[cnt - 1];
      }

    reference at(size_type pos)
      {
        auto cnt = this->size();
        if(pos >= cnt) {
          noadl::throw_out_of_range("static_vector: The subscript `%lld` is not a writable position within a vector of size `%lld`.",
                                    static_cast<long long>(pos), static_cast<long long>(cnt));
        }
        return this->data()[pos];
      }
    reference operator[](size_type pos) noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(pos < cnt);
        return this->data()[pos];
      }
    reference front() noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[0];
      }
    reference back() noexcept
      {
        auto cnt = this->size();
        ROCKET_ASSERT(cnt > 0);
        return this->data()[cnt - 1];
      }

    // N.B. This is a non-standard extension.
    template<typename ...paramsT>
      static_vector & append(size_type n, const paramsT &...params)
      {
        if(n == 0) {
          return *this;
        }
        this->do_reserve_more(n);
        noadl::ranged_do_while(size_type(0), n, [&](size_type, const paramsT &...fwdp) { this->m_sth.emplace_back_unchecked(fwdp...); }, params...);
        return *this;
      }
    // N.B. This is a non-standard extension.
    static_vector & append(initializer_list<value_type> init)
      {
        return this->append(init.begin(), init.end());
      }
    // N.B. This is a non-standard extension.
    template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
      static_vector & append(inputT first, inputT last)
      {
        if(first == last) {
          return *this;
        }
        const auto dist = noadl::estimate_distance(first, last);
        if(dist == 0) {
          noadl::ranged_do_while(::std::move(first), ::std::move(last), [&](const inputT &it) { this->emplace_back(*it); });
          return *this;
        }
        this->do_reserve_more(dist);
        noadl::ranged_do_while(::std::move(first), ::std::move(last), [&](const inputT &it) { this->m_sth.emplace_back_unchecked(*it); });
        return *this;
      }
    // 26.3.11.5, modifiers
    template<typename ...paramsT>
      reference emplace_back(paramsT &&...params)
      {
        this->do_reserve_more(1);
        const auto ptr = this->m_sth.emplace_back_unchecked(::std::forward<paramsT>(params)...);
        return *ptr;
      }
    // N.B. The return type is a non-standard extension.
    reference push_back(const value_type &value)
      {
        this->do_reserve_more(1);
        const auto ptr = this->m_sth.emplace_back_unchecked(value);
        return *ptr;
      }
    // N.B. The return type is a non-standard extension.
    reference push_back(value_type &&value)
      {
        this->do_reserve_more(1);
        const auto ptr = this->m_sth.emplace_back_unchecked(::std::move(value));
        return *ptr;
      }

    iterator insert(const_iterator tins, const value_type &value)
      {
        const auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        const auto ptr = this->do_insert_no_bound_check(tpos, details_static_vector::push_back, value);
        return iterator(this->m_sth, ptr);
      }
    iterator insert(const_iterator tins, value_type &&value)
      {
        const auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        const auto ptr = this->do_insert_no_bound_check(tpos, details_static_vector::push_back, ::std::move(value));
        return iterator(this->m_sth, ptr);
      }
    // N.B. The parameter pack is a non-standard extension.
    template<typename ...paramsT>
      iterator insert(const_iterator tins, size_type n, const paramsT &...params)
      {
        const auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        const auto ptr = this->do_insert_no_bound_check(tpos, details_static_vector::append, n, params...);
        return iterator(this->m_sth, ptr);
      }
    iterator insert(const_iterator tins, initializer_list<value_type> init)
      {
        const auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        const auto ptr = this->do_insert_no_bound_check(tpos, details_static_vector::append, init);
        return iterator(this->m_sth, ptr);
      }
    template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
      iterator insert(const_iterator tins, inputT first, inputT last)
      {
        const auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        const auto ptr = this->do_insert_no_bound_check(tpos, details_static_vector::append, ::std::move(first), ::std::move(last));
        return iterator(this->m_sth, ptr);
      }

    // N.B. This function may throw `std::bad_alloc`.
    iterator erase(const_iterator tfirst, const_iterator tlast)
      {
        const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->data());
        const auto tn = static_cast<size_type>(tlast.tell_owned_by(this->m_sth) - tfirst.tell());
        const auto ptr = this->do_erase_no_bound_check(tpos, tn);
        return iterator(this->m_sth, ptr);
      }
    // N.B. This function may throw `std::bad_alloc`.
    iterator erase(const_iterator tfirst)
      {
        const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->data());
        const auto ptr = this->do_erase_no_bound_check(tpos, 1);
        return iterator(this->m_sth, ptr);
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. The return type and parameter are non-standard extensions.
    static_vector & pop_back(size_type n = 1)
      {
        const auto cnt_old = this->size();
        ROCKET_ASSERT(n <= cnt_old);
        this->m_sth.pop_back_n_unchecked(n);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    static_vector & assign(const static_vector &other) noexcept(is_nothrow_copy_assignable<value_type>::value && is_nothrow_copy_constructible<value_type>::value)
      {
        const auto sl = this->size();
        const auto sr = other.size();
        if(sl < sr) {
          for(size_type i = 0; i < sl; ++i) {
            this->m_sth.data()[i] = other.m_sth.data()[i];
          }
          for(auto i = sl; i < sr; ++i) {
            this->m_sth.emplace_back_unchecked(other.m_sth.data()[i]);
          }
        } else {
          for(size_type i = 0; i < sr; ++i) {
            this->m_sth.data()[i] = other.m_sth.data()[i];
          }
          this->m_sth.pop_back_n_unchecked(sl - sr);
        }
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    static_vector & assign(static_vector &&other) noexcept(is_nothrow_move_assignable<value_type>::value && is_nothrow_move_constructible<value_type>::value)
      {
        const auto sl = this->size();
        const auto sr = other.size();
        if(sl < sr) {
          for(size_type i = 0; i < sl; ++i) {
            this->m_sth.data()[i] = ::std::move(other.m_sth.data()[i]);
          }
          for(auto i = sl; i < sr; ++i) {
            this->m_sth.emplace_back_unchecked(::std::move(other.m_sth.data()[i]));
          }
        } else {
          for(size_type i = 0; i < sr; ++i) {
            this->m_sth.data()[i] = ::std::move(other.m_sth.data()[i]);
          }
          this->m_sth.pop_back_n_unchecked(sl - sr);
        }
        return *this;
      }
    // N.B. The parameter pack is a non-standard extension.
    // N.B. The return type is a non-standard extension.
    template<typename ...paramsT>
      static_vector & assign(size_type n, const paramsT &...params)
      {
        this->clear();
        this->append(n, params...);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    static_vector & assign(initializer_list<value_type> init)
      {
        this->clear();
        this->append(init);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
      static_vector & assign(inputT first, inputT last)
      {
        this->clear();
        this->append(::std::move(first), ::std::move(last));
        return *this;
      }

    void swap(static_vector &other) noexcept(is_nothrow_swappable<value_type>::value && is_nothrow_move_constructible<value_type>::value)
      {
        const auto sl = this->size();
        const auto sr = other.size();
        if(sl < sr) {
          for(size_type i = 0; i < sl; ++i) {
            noadl::adl_swap(this->m_sth.data()[i], other.m_sth.data()[i]);
          }
          for(auto i = sl; i < sr; ++i) {
            this->m_sth.emplace_back_unchecked(::std::move(other.m_sth.data()[i]));
          }
          other.m_sth.pop_back_n_unchecked(sr - sl);
        } else {
          for(size_type i = 0; i < sr; ++i) {
            noadl::adl_swap(this->m_sth.data()[i], other.m_sth.data()[i]);
          }
          for(auto i = sr; i < sl; ++i) {
            other.m_sth.emplace_back_unchecked(::std::move(this->m_sth.data()[i]));
          }
          this->m_sth.pop_back_n_unchecked(sl - sr);
        }
        allocator_swapper<allocator_type>()(this->m_sth.as_allocator(), other.m_sth.as_allocator());
      }

    // 26.3.11.4, data access
    const value_type * data() const noexcept
      {
        return this->m_sth.data();
      }

    value_type * data() noexcept
      {
        return this->m_sth.data();
      }

    // N.B. The return type differs from `std::vector`.
    const allocator_type & get_allocator() const noexcept
      {
        return this->m_sth.as_allocator();
      }
    allocator_type & get_allocator() noexcept
      {
        return this->m_sth.as_allocator();
      }
  };

template<typename valueT, size_t capacityT, typename allocatorT>
  inline void swap(static_vector<valueT, capacityT, allocatorT> &lhs, static_vector<valueT, capacityT, allocatorT> &rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

}

#endif
