// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_VECTOR_HPP_
#define ROCKET_COW_VECTOR_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"
#include "reference_counter.hpp"
#include <iterator>  // std::iterator_traits<>, std::random_access_iterator_tag
#include <cstring>  // std::memset()

/* Differences from `std::vector`:
 * 1. All functions guarantee only basic exception safety rather than strong exception safety, hence are more efficient.
 * 2. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always return `const_reference`s.
 * 3. The copy constructor and copy assignment operator will not throw exceptions.
 * 4. The specialization for `bool` is not provided.
 * 5. `emplace()` is not provided.
 * 6. Comparison operators are not provided.
 * 7. The value type may be incomplete. It need be neither copy-assignable nor move-assignable, but must be swappable.
 */

namespace rocket {

template<typename valueT, typename allocatorT = allocator<valueT>> class cow_vector;

    namespace details_cow_vector {

    struct storage_header
      {
        void (*dtor)(...);
        mutable reference_counter<long> nref;
        size_t nelem;

        explicit storage_header(void (*xdtor)(...)) noexcept
          : dtor(xdtor), nref()
            // `nelem` is uninitialized.
          {
          }
      };

    template<typename allocatorT> struct basic_storage : storage_header
      {
        using allocator_type   = allocatorT;
        using value_type       = typename allocator_type::value_type;
        using size_type        = typename allocator_traits<allocator_type>::size_type;

        static constexpr size_type min_nblk_for_nelem(size_type nelem) noexcept
          {
            return (sizeof(value_type) * nelem + sizeof(basic_storage) - 1) / sizeof(basic_storage) + 1;
          }
        static constexpr size_type max_nelem_for_nblk(size_type nblk) noexcept
          {
            return sizeof(basic_storage) * (nblk - 1) / sizeof(value_type);
          }

        allocator_type alloc;
        size_type nblk;
        union { value_type data[0];  };

        basic_storage(void (*xdtor)(...), const allocator_type& xalloc, size_type xnblk) noexcept
          : storage_header(xdtor),
            alloc(xalloc), nblk(xnblk)
          {
            this->nelem = 0;
          }
        ~basic_storage()
          {
            auto nrem = this->nelem;
            while(nrem != 0) {
              --nrem;
              allocator_traits<allocator_type>::destroy(this->alloc, this->data + nrem);
            }
#ifdef ROCKET_DEBUG
            this->nelem = 0xCCAA;
#endif
          }

        basic_storage(const basic_storage&)
          = delete;
        basic_storage& operator=(const basic_storage&)
          = delete;
      };

    template<typename pointerT, typename allocatorT,
             bool copyableT = is_copy_constructible<typename allocatorT::value_type>::value,
             bool memcpyT = conjunction<is_trivially_copy_constructible<typename allocatorT::value_type>,
                                        is_std_allocator<allocatorT>>::value> struct copy_storage_helper
      {
        void operator()(pointerT ptr, pointerT ptr_old, size_t off, size_t cnt) const
          {
            // Copy elements one by one.
            auto nelem = ptr->nelem;
            auto cap = basic_storage<allocatorT>::max_nelem_for_nblk(ptr->nblk);
            ROCKET_ASSERT(cnt <= cap - nelem);
            for(auto i = off; i != off + cnt; ++i) {
              allocator_traits<allocatorT>::construct(ptr->alloc, ptr->data + nelem, ptr_old->data[i]);
              ptr->nelem = ++nelem;
            }
          }
      };
    template<typename pointerT, typename allocatorT, bool memcpyT> struct copy_storage_helper<pointerT, allocatorT,
                                                                                              false,  // copyableT
                                                                                              memcpyT>  // trivial && std::allocator
      {
        [[noreturn]] void operator()(pointerT /*ptr*/, pointerT /*ptr_old*/, size_t /*off*/, size_t /*cnt*/) const
          {
            // Throw an exception unconditionally, even when there is nothing to copy.
            noadl::sprintf_and_throw<domain_error>("cow_vector: `%s` is not copy-constructible.",
                                                   typeid(typename allocatorT::value_type).name());
          }
      };
    template<typename pointerT, typename allocatorT> struct copy_storage_helper<pointerT, allocatorT,
                                                                                true,  // copyableT
                                                                                true>  // trivial && std::allocator
      {
        void operator()(pointerT ptr, pointerT ptr_old, size_t off, size_t cnt) const
          {
            // Optimize it using `std::memcpy()`, as the source and destination locations can't overlap.
            auto nelem = ptr->nelem;
            auto cap = basic_storage<allocatorT>::max_nelem_for_nblk(ptr->nblk);
            ROCKET_ASSERT(cnt <= cap - nelem);
            ::std::memcpy(static_cast<void*>(ptr->data + nelem), ptr_old->data + off, sizeof(typename allocatorT::value_type) * cnt);
            ptr->nelem = (nelem += cnt);
          }
      };

    template<typename pointerT, typename allocatorT,
             bool movableT = is_move_constructible<typename allocatorT::value_type>::value,
             bool memcpyT = conjunction<is_trivially_move_constructible<typename allocatorT::value_type>,
                                        is_std_allocator<allocatorT>>::value> struct move_storage_helper
      {
        void operator()(pointerT ptr, pointerT ptr_old, size_t off, size_t cnt) const
          {
            // Move elements one by one.
            auto nelem = ptr->nelem;
            auto cap = basic_storage<allocatorT>::max_nelem_for_nblk(ptr->nblk);
            ROCKET_ASSERT(cnt <= cap - nelem);
            for(auto i = off; i != off + cnt; ++i) {
              allocator_traits<allocatorT>::construct(ptr->alloc, ptr->data + nelem, noadl::move(ptr_old->data[i]));
              ptr->nelem = ++nelem;
            }
          }
      };
    template<typename pointerT, typename allocatorT, bool memcpyT> struct move_storage_helper<pointerT, allocatorT,
                                                                                              false,  // movableT
                                                                                              memcpyT>  // trivial && std::allocator
      {
        [[noreturn]] void operator()(pointerT /*ptr*/, pointerT /*ptr_old*/, size_t /*off*/, size_t /*cnt*/) const
          {
            // Throw an exception unconditionally, even when there is nothing to move.
            noadl::sprintf_and_throw<domain_error>("cow_vector: `%s` is not move-constructible.",
                                                   typeid(typename allocatorT::value_type).name());
          }
      };
    template<typename pointerT, typename allocatorT> struct move_storage_helper<pointerT, allocatorT,
                                                                                true,  // movableT
                                                                                true>  // trivial && std::allocator
      {
        void operator()(pointerT ptr, pointerT ptr_old, size_t off, size_t cnt) const
          {
            // Optimize it using `std::memcpy()`, as the source and destination locations can't overlap.
            auto nelem = ptr->nelem;
            auto cap = basic_storage<allocatorT>::max_nelem_for_nblk(ptr->nblk);
            ROCKET_ASSERT(cnt <= cap - nelem);
            ::std::memcpy(static_cast<void*>(ptr->data + nelem), ptr_old->data + off, sizeof(typename allocatorT::value_type) * cnt);
            ptr->nelem = (nelem += cnt);
          }
      };

    template<typename allocatorT> class storage_handle : private allocator_wrapper_base_for<allocatorT>::type
      {
      public:
        using allocator_type   = allocatorT;
        using value_type       = typename allocator_type::value_type;
        using size_type        = typename allocator_traits<allocator_type>::size_type;

      private:
        using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;
        using storage           = basic_storage<allocator_type>;
        using storage_allocator = typename allocator_traits<allocator_type>::template rebind_alloc<storage>;
        using storage_pointer   = typename allocator_traits<storage_allocator>::pointer;

      private:
        storage_pointer m_ptr;

      public:
        explicit constexpr storage_handle(const allocator_type& alloc) noexcept
          : allocator_base(alloc),
            m_ptr()
          {
          }
        explicit constexpr storage_handle(allocator_type&& alloc) noexcept
          : allocator_base(noadl::move(alloc)),
            m_ptr()
          {
          }
        ~storage_handle()
          {
            this->deallocate();
          }

        storage_handle(const storage_handle&)
          = delete;
        storage_handle& operator=(const storage_handle&)
          = delete;

      private:
        void do_reset(storage_pointer ptr_new) noexcept
          {
            auto ptr = ::std::exchange(this->m_ptr, ptr_new);
            if(ROCKET_EXPECT(!ptr)) {
              return;
            }
            // This is needed for incomplete type support.
            (*reinterpret_cast<void (*)(storage_pointer)>(reinterpret_cast<const storage_header*>(noadl::unfancy(ptr))->dtor))(ptr);
          }

        ROCKET_NOINLINE static void do_drop_reference(storage_pointer ptr) noexcept
          {
            // Decrement the reference count with acquire-release semantics to prevent races on `ptr->alloc`.
            if(ROCKET_EXPECT(!ptr->nref.decrement())) {
              return;
            }
            // If it has been decremented to zero, deallocate the block.
            auto st_alloc = storage_allocator(ptr->alloc);
            auto nblk = ptr->nblk;
            noadl::destroy_at(noadl::unfancy(ptr));
#ifdef ROCKET_DEBUG
            ::std::memset(static_cast<void*>(noadl::unfancy(ptr)), '~', sizeof(storage) * nblk);
#endif
            allocator_traits<storage_allocator>::deallocate(st_alloc, ptr, nblk);
          }

        [[noreturn]] ROCKET_NOINLINE void do_throw_size_overflow(size_type base, size_type add) const
          {
            noadl::sprintf_and_throw<length_error>("cow_vector: Increasing `%lld` by `%lld` would exceed the max size `%lld`.",
                                                   static_cast<long long>(base), static_cast<long long>(add), static_cast<long long>(this->max_size()));
          }

      public:
        const allocator_type& as_allocator() const noexcept
          {
            return static_cast<const allocator_base&>(*this);
          }
        allocator_type& as_allocator() noexcept
          {
            return static_cast<allocator_base&>(*this);
          }

        bool unique() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return false;
            }
            return ptr->nref.unique();
          }
        long use_count() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return 0;
            }
            auto nref = ptr->nref.get();
            ROCKET_ASSERT(nref > 0);
            return nref;
          }
        size_type capacity() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return 0;
            }
            auto cap = storage::max_nelem_for_nblk(ptr->nblk);
            ROCKET_ASSERT(cap > 0);
            return cap;
          }
        size_type max_size() const noexcept
          {
            auto st_alloc = storage_allocator(this->as_allocator());
            auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
            return storage::max_nelem_for_nblk(max_nblk / 2);
          }
        size_type check_size_add(size_type base, size_type add) const
          {
            auto cap_max = this->max_size();
            ROCKET_ASSERT(base <= cap_max);
            if(cap_max - base < add) {
              this->do_throw_size_overflow(base, add);
            }
            return base + add;
          }
        size_type round_up_capacity(size_type res_arg) const
          {
            auto cap = this->check_size_add(0, res_arg);
            auto nblk = storage::min_nblk_for_nelem(cap);
            return storage::max_nelem_for_nblk(nblk);
          }
        const value_type* data() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return nullptr;
            }
            return ptr->data;
          }
        size_type size() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return 0;
            }
            return reinterpret_cast<const storage_header*>(ptr)->nelem;
          }
        ROCKET_NOINLINE value_type* reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
          {
            if(res_arg == 0) {
              // Deallocate the block.
              this->deallocate();
              return nullptr;
            }
            auto cap = this->check_size_add(0, res_arg);
            // Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
            auto nblk = storage::min_nblk_for_nelem(cap);
            auto st_alloc = storage_allocator(this->as_allocator());
            auto ptr = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
#ifdef ROCKET_DEBUG
            ::std::memset(static_cast<void*>(noadl::unfancy(ptr)), '*', sizeof(storage) * nblk);
#endif
            noadl::construct_at(noadl::unfancy(ptr), reinterpret_cast<void (*)(...)>(&storage_handle::do_drop_reference), this->as_allocator(), nblk);
            auto ptr_old = this->m_ptr;
            if(ROCKET_UNEXPECT(ptr_old)) {
              try {
                // Copy or move elements into the new block.
                // Moving is only viable if the old and new allocators compare equal and the old block is owned exclusively.
                if((ptr_old->alloc == ptr->alloc) && ptr_old->nref.unique()) {
                  move_storage_helper<storage_pointer, allocator_type>()(ptr, ptr_old,       0, cnt_one);
                  move_storage_helper<storage_pointer, allocator_type>()(ptr, ptr_old, off_two, cnt_two);
                }
                else {
                  copy_storage_helper<storage_pointer, allocator_type>()(ptr, ptr_old,       0, cnt_one);
                  copy_storage_helper<storage_pointer, allocator_type>()(ptr, ptr_old, off_two, cnt_two);
                }
              }
              catch(...) {
                // If an exception is thrown, deallocate the new block, then rethrow the exception.
                noadl::destroy_at(noadl::unfancy(ptr));
                allocator_traits<storage_allocator>::deallocate(st_alloc, ptr, nblk);
                throw;
              }
            }
            // Replace the current block.
            this->do_reset(ptr);
            return ptr->data;
          }
        void deallocate() noexcept
          {
            this->do_reset(storage_pointer());
          }

        void share_with(const storage_handle& other) noexcept
          {
            auto ptr = other.m_ptr;
            if(ptr) {
              // Increment the reference count.
              reinterpret_cast<storage_header*>(noadl::unfancy(ptr))->nref.increment();
            }
            this->do_reset(ptr);
          }
        void share_with(storage_handle&& other) noexcept
          {
            auto ptr = other.m_ptr;
            if(ptr) {
              // Detach the block.
              other.m_ptr = storage_pointer();
            }
            this->do_reset(ptr);
          }
        void exchange_with(storage_handle& other) noexcept
          {
            ::std::swap(this->m_ptr, other.m_ptr);
          }

        constexpr operator const storage_handle* () const noexcept
          {
            return this;
          }
        operator storage_handle* () noexcept
          {
            return this;
          }

        value_type* mut_data_unchecked() noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return nullptr;
            }
            ROCKET_ASSERT(this->unique());
            return ptr->data;
          }
        template<typename... paramsT> value_type* emplace_back_unchecked(paramsT&&... params)
          {
            ROCKET_ASSERT(this->unique());
            ROCKET_ASSERT(this->size() < this->capacity());
            auto ptr = this->m_ptr;
            ROCKET_ASSERT(ptr);
            auto nelem = ptr->nelem;
            allocator_traits<allocator_type>::construct(ptr->alloc, ptr->data + nelem, noadl::forward<paramsT>(params)...);
            ptr->nelem = ++nelem;
            return ptr->data + nelem - 1;
          }
        void pop_back_n_unchecked(size_type n) noexcept
          {
            ROCKET_ASSERT(this->unique());
            ROCKET_ASSERT(n <= this->size());
            if(n == 0) {
              return;
            }
            auto ptr = this->m_ptr;
            ROCKET_ASSERT(ptr);
            auto nelem = ptr->nelem;
            for(auto i = n; i != 0; --i) {
              ptr->nelem = --nelem;
              allocator_traits<allocator_type>::destroy(ptr->alloc, ptr->data + nelem);
            }
          }
      };

    template<typename vectorT, typename valueT> class vector_iterator
      {
        template<typename, typename> friend class vector_iterator;
        friend vectorT;

      public:
        using iterator_category  = random_access_iterator_tag;
        using value_type         = valueT;
        using pointer            = value_type*;
        using reference          = value_type&;
        using difference_type    = ptrdiff_t;

        using parent_type   = storage_handle<typename vectorT::allocator_type>;

      private:
        const parent_type* m_ref;
        value_type* m_ptr;

      private:
        constexpr vector_iterator(const parent_type* ref, value_type* ptr) noexcept
          : m_ref(ref), m_ptr(ptr)
          {
          }

      public:
        constexpr vector_iterator() noexcept
          : vector_iterator(nullptr, nullptr)
          {
          }
        template<typename yvalueT, ROCKET_ENABLE_IF(is_convertible<yvalueT*, valueT*>::value)> constexpr vector_iterator(const vector_iterator<vectorT, yvalueT>& other) noexcept
          : vector_iterator(other.m_ref, other.m_ptr)
          {
          }

      private:
        value_type* do_assert_valid_pointer(value_type* ptr, bool to_dereference) const noexcept
          {
            auto ref = this->m_ref;
            ROCKET_ASSERT_MSG(ref, "This iterator has not been initialized.");
            auto dist = static_cast<size_t>(ptr - ref->data());
            ROCKET_ASSERT_MSG(dist <= ref->size(), "This iterator has been invalidated.");
            ROCKET_ASSERT_MSG(!(to_dereference && (dist == ref->size())), "This iterator contains a past-the-end value and cannot be dereferenced.");
            return ptr;
          }

      public:
        const parent_type* parent() const noexcept
          {
            return this->m_ref;
          }

        value_type* tell() const noexcept
          {
            auto ptr = this->do_assert_valid_pointer(this->m_ptr, false);
            return ptr;
          }
        value_type* tell_owned_by(const parent_type* ref) const noexcept
          {
            ROCKET_ASSERT_MSG(this->m_ref == ref, "This iterator does not refer to an element in the same container.");
            return this->tell();
          }
        vector_iterator& seek(value_type* ptr) noexcept
          {
            this->m_ptr = this->do_assert_valid_pointer(ptr, false);
            return *this;
          }

        reference operator*() const noexcept
          {
            auto ptr = this->do_assert_valid_pointer(this->m_ptr, true);
            ROCKET_ASSERT(ptr);
            return *ptr;
          }
        pointer operator->() const noexcept
          {
            auto ptr = this->do_assert_valid_pointer(this->m_ptr, true);
            ROCKET_ASSERT(ptr);
            return ptr;
          }
        reference operator[](difference_type off) const noexcept
          {
            auto ptr = this->do_assert_valid_pointer(this->m_ptr + off, true);
            ROCKET_ASSERT(ptr);
            return *ptr;
          }
      };

    template<typename vectorT, typename valueT> inline vector_iterator<vectorT, valueT>& operator++(vector_iterator<vectorT, valueT>& rhs) noexcept
      {
        return rhs.seek(rhs.tell() + 1);
      }
    template<typename vectorT, typename valueT> inline vector_iterator<vectorT, valueT>& operator--(vector_iterator<vectorT, valueT>& rhs) noexcept
      {
        return rhs.seek(rhs.tell() - 1);
      }

    template<typename vectorT, typename valueT> inline vector_iterator<vectorT, valueT> operator++(vector_iterator<vectorT, valueT>& lhs, int) noexcept
      {
        auto res = lhs;
        lhs.seek(lhs.tell() + 1);
        return res;
      }
    template<typename vectorT, typename valueT> inline vector_iterator<vectorT, valueT> operator--(vector_iterator<vectorT, valueT>& lhs, int) noexcept
      {
        auto res = lhs;
        lhs.seek(lhs.tell() - 1);
        return res;
      }

    template<typename vectorT, typename valueT> inline vector_iterator<vectorT, valueT>& operator+=(vector_iterator<vectorT, valueT>& lhs,
                                                                                                     typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
      {
        return lhs.seek(lhs.tell() + rhs);
      }
    template<typename vectorT, typename valueT> inline vector_iterator<vectorT, valueT>& operator-=(vector_iterator<vectorT, valueT>& lhs,
                                                                                                     typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
      {
        return lhs.seek(lhs.tell() - rhs);
      }

    template<typename vectorT, typename valueT> inline vector_iterator<vectorT, valueT> operator+(const vector_iterator<vectorT, valueT>& lhs,
                                                                                                  typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
      {
        auto res = lhs;
        res.seek(res.tell() + rhs);
        return res;
      }
    template<typename vectorT, typename valueT> inline vector_iterator<vectorT, valueT> operator-(const vector_iterator<vectorT, valueT>& lhs,
                                                                                                  typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
      {
        auto res = lhs;
        res.seek(res.tell() - rhs);
        return res;
      }

    template<typename vectorT, typename valueT> inline vector_iterator<vectorT, valueT> operator+(typename vector_iterator<vectorT, valueT>::difference_type lhs,
                                                                                                  const vector_iterator<vectorT, valueT>& rhs) noexcept
      {
        auto res = rhs;
        res.seek(res.tell() + lhs);
        return res;
      }
    template<typename vectorT, typename xvalueT,
                               typename yvalueT> inline typename vector_iterator<vectorT, xvalueT>::difference_type operator-(const vector_iterator<vectorT, xvalueT>& lhs,
                                                                                                                              const vector_iterator<vectorT, yvalueT>& rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) - rhs.tell();
      }

    template<typename vectorT, typename xvalueT,
                               typename yvalueT> inline bool operator==(const vector_iterator<vectorT, xvalueT>& lhs,
                                                                        const vector_iterator<vectorT, yvalueT>& rhs) noexcept
      {
        return lhs.tell() == rhs.tell();
      }
    template<typename vectorT, typename xvalueT,
                               typename yvalueT> inline bool operator!=(const vector_iterator<vectorT, xvalueT>& lhs,
                                                                        const vector_iterator<vectorT, yvalueT>& rhs) noexcept
      {
        return lhs.tell() != rhs.tell();
      }

    template<typename vectorT, typename xvalueT,
                               typename yvalueT> inline bool operator<(const vector_iterator<vectorT, xvalueT>& lhs,
                                                                       const vector_iterator<vectorT, yvalueT>& rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) < rhs.tell();
      }
    template<typename vectorT, typename xvalueT,
                               typename yvalueT> inline bool operator>(const vector_iterator<vectorT, xvalueT>& lhs,
                                                                       const vector_iterator<vectorT, yvalueT>& rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) > rhs.tell();
      }
    template<typename vectorT, typename xvalueT,
                               typename yvalueT> inline bool operator<=(const vector_iterator<vectorT, xvalueT>& lhs,
                                                                        const vector_iterator<vectorT, yvalueT>& rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) <= rhs.tell();
      }
    template<typename vectorT, typename xvalueT,
                               typename yvalueT> inline bool operator>=(const vector_iterator<vectorT, xvalueT>& lhs,
                                                                        const vector_iterator<vectorT, yvalueT>& rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) >= rhs.tell();
      }

    // Insertion helpers.
    struct append_tag
      {
      }
    constexpr append;

    template<typename vectorT, typename... paramsT> inline void tagged_append(vectorT* vec, append_tag, paramsT&&... params)
      {
        vec->append(noadl::forward<paramsT>(params)...);
      }

    struct emplace_back_tag
      {
      }
    constexpr emplace_back;

    template<typename vectorT, typename... paramsT> inline void tagged_append(vectorT* vec, emplace_back_tag, paramsT&&... params)
      {
        vec->emplace_back(noadl::forward<paramsT>(params)...);
      }

    struct push_back_tag
      {
      }
    constexpr push_back;

    template<typename vectorT, typename... paramsT> inline void tagged_append(vectorT* vec, push_back_tag, paramsT&&... params)
      {
        vec->push_back(noadl::forward<paramsT>(params)...);
      }

    }  // namespace details_cow_vector

template<typename valueT, typename allocatorT> class cow_vector
  {
    static_assert(!is_array<valueT>::value, "`valueT` must not be an array type.");
    static_assert(is_same<typename allocatorT::value_type, valueT>::value, "`allocatorT::value_type` must denote the same type as `valueT`.");

  public:
    // types
    using value_type      = valueT;
    using allocator_type  = allocatorT;

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
      : m_sth(alloc)
      {
      }
    constexpr cow_vector(clear_t = clear_t()) noexcept(is_nothrow_constructible<allocator_type>::value)
      : cow_vector(allocator_type())
      {
      }
    cow_vector(const cow_vector& other) noexcept
      : cow_vector(allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
      {
        this->assign(other);
      }
    cow_vector(const cow_vector& other, const allocator_type& alloc) noexcept
      : cow_vector(alloc)
      {
        this->assign(other);
      }
    cow_vector(cow_vector&& other) noexcept
      : cow_vector(noadl::move(other.m_sth.as_allocator()))
      {
        this->assign(noadl::move(other));
      }
    cow_vector(cow_vector&& other, const allocator_type& alloc) noexcept
      : cow_vector(alloc)
      {
        this->assign(noadl::move(other));
      }
    explicit cow_vector(size_type n, const allocator_type& alloc = allocator_type())
      : cow_vector(alloc)
      {
        this->assign(n);
      }
    cow_vector(size_type n, const value_type& value, const allocator_type& alloc = allocator_type())
      : cow_vector(alloc)
      {
        this->assign(n, value);
      }
    // N.B. This is a non-standard extension.
    template<typename firstT, typename... restT, ROCKET_DISABLE_IF(is_same<typename decay<firstT>::type,
                                                                           allocator_type>::value)> cow_vector(size_type n, const firstT& first, const restT&... rest)
      : cow_vector(allocator_type())
      {
        this->assign(n, first, rest...);
      }
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)> cow_vector(inputT first, inputT last,
                                                                                                                const allocator_type& alloc = allocator_type())
      : cow_vector(alloc)
      {
        this->assign(noadl::move(first), noadl::move(last));
      }
    cow_vector(initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
      : cow_vector(alloc)
      {
        this->assign(init);
      }
    cow_vector& operator=(clear_t) noexcept
      {
        this->clear();
        return *this;
      }
    cow_vector& operator=(const cow_vector& other) noexcept
      {
        this->assign(other);
        noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        return *this;
      }
    cow_vector& operator=(cow_vector&& other) noexcept
      {
        this->assign(noadl::move(other));
        noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), noadl::move(other.m_sth.as_allocator()));
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

    [[noreturn]] ROCKET_NOINLINE void do_throw_subscript_of_range(size_type pos) const
      {
        noadl::sprintf_and_throw<out_of_range>("cow_vector: The subscript `%lld` is not a valid position within this vector of size `%lld`.",
                                               static_cast<long long>(pos), static_cast<long long>(this->size()));
      }

    // This function works the same way as `std::string::substr()`.
    // Ensure `tpos` is in `[0, size()]` and return `min(tn, size() - tpos)`.
    ROCKET_PURE_FUNCTION size_type do_clamp_subrange(size_type tpos, size_type tn) const
      {
        auto tcnt = this->size();
        if(tpos > tcnt) {
          this->do_throw_subscript_of_range(tpos);
        }
        return noadl::min(tcnt - tpos, tn);
      }

    template<typename... paramsT> value_type* do_insert_no_bound_check(size_type tpos, paramsT&&... params)
      {
        auto cnt_old = this->size();
        ROCKET_ASSERT(tpos <= cnt_old);
        details_cow_vector::tagged_append(this, noadl::forward<paramsT>(params)...);
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
    // N.B. This is a non-standard extension.
    difference_type ssize() const noexcept
      {
        return static_cast<difference_type>(this->size());
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
          this->do_throw_subscript_of_range(pos);
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
          this->do_throw_subscript_of_range(pos);
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
        noadl::ranged_do_while(size_type(0), n, [&](size_type, const paramsT&... fwdp) { this->m_sth.emplace_back_unchecked(fwdp...);  }, params...);
        return *this;
      }
    // N.B. This is a non-standard extension.
    cow_vector& append(initializer_list<value_type> init)
      {
        return this->append(init.begin(), init.end());
      }
    // N.B. This is a non-standard extension.
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)> cow_vector& append(inputT first, inputT last)
      {
        if(first == last) {
          return *this;
        }
        auto dist = noadl::estimate_distance(first, last);
        if(dist == 0) {
          noadl::ranged_do_while(noadl::move(first), noadl::move(last), [&](const inputT& it) { this->emplace_back(*it);  });
          return *this;
        }
        this->do_reserve_more(dist);
        noadl::ranged_do_while(noadl::move(first), noadl::move(last), [&](const inputT& it) { this->m_sth.emplace_back_unchecked(*it);  });
        return *this;
      }
    // 26.3.11.5, modifiers
    template<typename... paramsT> reference emplace_back(paramsT&&... params)
      {
        this->do_reserve_more(1);
        auto ptr = this->m_sth.emplace_back_unchecked(noadl::forward<paramsT>(params)...);
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
        auto ptr = this->m_sth.emplace_back_unchecked(noadl::move(value));
        return *ptr;
      }

    // N.B. This is a non-standard extension.
    cow_vector& insert(size_type tpos, const value_type& value)
      {
        this->do_insert_no_bound_check(this->do_clamp_subrange(tpos, 0), details_cow_vector::push_back, value);
        return *this;
      }
    // N.B. This is a non-standard extension.
    cow_vector& insert(size_type tpos, value_type&& value)
      {
        this->do_insert_no_bound_check(this->do_clamp_subrange(tpos, 0), details_cow_vector::push_back, noadl::move(value));
        return *this;
      }
    // N.B. This is a non-standard extension.
    template<typename... paramsT> cow_vector& insert(size_type tpos, size_type n, const paramsT&... params)
      {
        this->do_insert_no_bound_check(this->do_clamp_subrange(tpos, 0), details_cow_vector::append, n, params...);
        return *this;
      }
    // N.B. This is a non-standard extension.
    cow_vector& insert(size_type tpos, initializer_list<value_type> init)
      {
        this->do_insert_no_bound_check(this->do_clamp_subrange(tpos, 0), details_cow_vector::append, init);
        return *this;
      }
    // N.B. This is a non-standard extension.
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)> cow_vector& insert(size_type tpos, inputT first, inputT last)
      {
        this->do_insert_no_bound_check(this->do_clamp_subrange(tpos, 0), details_cow_vector::append, noadl::move(first), noadl::move(last));
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
        auto ptr = this->do_insert_no_bound_check(tpos, details_cow_vector::push_back, noadl::move(value));
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
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)> iterator insert(const_iterator tins, inputT first, inputT last)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this->m_sth) - this->data());
        auto ptr = this->do_insert_no_bound_check(tpos, details_cow_vector::append, noadl::move(first), noadl::move(last));
        return iterator(this->m_sth, ptr);
      }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    cow_vector& erase(size_type tpos = 0, size_type tn = size_type(-1))
      {
        this->do_erase_no_bound_check(tpos, this->do_clamp_subrange(tpos, tn));
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

    // N.B. The return type is a non-standard extension.
    cow_vector& assign(const cow_vector& other) noexcept
      {
        this->m_sth.share_with(other.m_sth);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    cow_vector& assign(cow_vector&& other) noexcept
      {
        this->m_sth.share_with(noadl::move(other.m_sth));
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
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)> cow_vector& assign(inputT first, inputT last)
      {
        this->clear();
        this->append(noadl::move(first), noadl::move(last));
        return *this;
      }

    void swap(cow_vector& other) noexcept
      {
        this->m_sth.exchange_with(other.m_sth);
        noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
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

template<typename valueT, typename allocatorT> inline void swap(cow_vector<valueT, allocatorT>& lhs,
                                                                cow_vector<valueT, allocatorT>& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace rocket

#endif
