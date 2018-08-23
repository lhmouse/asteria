// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_HASHMAP_HPP_
#define ROCKET_COW_HASHMAP_HPP_

#include <memory> // std::allocator<>, std::allocator_traits<>
#include <atomic> // std::atomic<>
#include <type_traits> // so many...
#include <iterator> // std::iterator_traits<>, std::forward_iterator_tag
#include <initializer_list> // std::initializer_list<>
#include <utility> // std::move(), std::forward(), std::declval(), std::pair<>
#include <functional> // std::hash<>
#include <cstddef> // std::size_t, std::ptrdiff_t, std::nullptr_t
#include <cstring> // std::memset()
#include "compatibility.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"
#include "transparent_comparators.hpp"

/* Differences from `std::unordered_map`:
 * 1. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always return `const_reference`s.
 * 2. The copy constructor and copy assignment operator will not throw exceptions.
 * 3. Comparison operators are not provided.
 * 4. `emplace()` and `emplace_hint()` functions are not provided. `try_emplace()` is recommended as an alternative.
 * 5. There are no buckets. Bucket lookups and local iterators are not provided. The non-unique (`unordered_multimap`) equivalent cannot be implemented.
 * 6. `equal_range()` functions are not provided.
 * 7. The key and mapped types may be incomplete. The mapped type need be neither copy-assignable nor move-assignable.
 * 8. `erase()` may move elements around and invalidate iterators.
 */

namespace rocket {

using ::std::allocator;
using ::std::allocator_traits;
using ::std::atomic;
using ::std::is_same;
using ::std::decay;
using ::std::remove_reference;
using ::std::is_array;
using ::std::is_trivial;
using ::std::is_const;
using ::std::enable_if;
using ::std::is_convertible;
using ::std::is_copy_constructible;
using ::std::is_nothrow_copy_constructible;
using ::std::is_nothrow_move_constructible;
using ::std::remove_const;
using ::std::conditional;
using ::std::integral_constant;
using ::std::iterator_traits;
using ::std::initializer_list;
using ::std::pair;
using ::std::hash;
using ::std::size_t;
using ::std::ptrdiff_t;
using ::std::nullptr_t;

template<typename keyT, typename mappedT, typename hashT = hash<keyT>, typename eqT = transparent_equal_to, typename allocatorT = allocator<pair<const keyT, mappedT>>>
  class cow_hashmap;

namespace details_cow_hashmap {
  template<typename allocatorT>
    class value_handle
      {
      public:
        using allocator_type   = allocatorT;
        using value_type       = typename allocatorT::value_type;
        using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
        using pointer          = typename allocator_traits<allocator_type>::pointer;

      private:
        pointer m_ptr;

      public:
        value_handle() noexcept
          : m_ptr()
          {
          }

        value_handle(const value_handle &)
          = delete;
        value_handle & operator=(const value_handle &)
          = delete;

      public:
        const_pointer get() const noexcept
          {
            return this->m_ptr;
          }
        pointer get() noexcept
          {
            return this->m_ptr;
          }
        pointer set(pointer ptr) noexcept
          {
            return noadl::exchange(this->m_ptr, ptr);
          }
      };

  template<typename allocatorT>
    struct handle_storage
      {
        using allocator_type   = allocatorT;
        using handle_type      = value_handle<allocator_type>;
        using size_type        = typename allocator_traits<allocator_type>::size_type;

        static constexpr size_type min_nblk_for_nbkt(size_type nbkt) noexcept
          {
            return (nbkt * sizeof(handle_type) + sizeof(handle_storage) - 1) / sizeof(handle_storage) + 1;
          }
        static constexpr size_type max_nbkt_for_nblk(size_type nblk) noexcept
          {
            return (nblk - 1) * sizeof(handle_storage) / sizeof(handle_type);
          }

        atomic<long> nref;
        allocator_type alloc;
        size_type nblk;
        size_type nelem;
        union { handle_type data[0]; };

        handle_storage(const allocator_type &xalloc, size_type xnblk) noexcept
          : alloc(xalloc), nblk(xnblk)
          {
            const auto nbkt = this->max_nbkt_for_nblk(this->nblk);
            // `allocator_type::pointer` need not be a trivial type.
            // The C++ standard requires that value-initialization of such an object shall not throw exceptions,
            // and shall result in a null pointer.
            for(auto i = size_type(0); i < nbkt; ++i) {
              allocator_traits<allocator_type>::construct(this->alloc, this->data + i);
            }
            this->nelem = 0;
            this->nref.store(1, ::std::memory_order_release);
          }
        ~handle_storage()
          {
            const auto nbkt = this->max_nbkt_for_nblk(this->nblk);
            for(auto i = size_type(0); i < nbkt; ++i) {
              const auto eptr = this->data[i].set(nullptr);
              if(!eptr) {
                  continue;
              }
              allocator_traits<allocator_type>::destroy(this->alloc, noadl::unfancy(eptr));
              allocator_traits<allocator_type>::deallocate(this->alloc, eptr, size_t(1));
            }
            // `allocator_type::pointer` need not be a trivial type.
            for(auto i = size_type(0); i < nbkt; ++i) {
              allocator_traits<allocator_type>::destroy(this->alloc, this->data + i);
            }
#ifdef ROCKET_DEBUG
            this->nelem = 0xEECD;
#endif
          }

        handle_storage(const handle_storage &)
          = delete;
        handle_storage & operator=(const handle_storage &)
          = delete;
      };

  // Copies the `const` qualifier from `otherT`, which may be a reference type, to `typeT`.
  template<typename typeT, typename otherT>
    struct copy_const_from
      : conditional<is_const<typename remove_reference<otherT>::type>::value, const typeT, typeT>
      {
      };

  template<typename allocatorT>
    struct linear_prober
      {
        using allocator_type   = allocatorT;
        using handle_type      = value_handle<allocator_type>;
        using size_type        = typename allocator_traits<allocator_type>::size_type;
        using difference_type  = typename allocator_traits<allocator_type>::difference_type;

        template<typename xpointerT>
          static size_type origin(xpointerT ptr, size_t hval)
            {
              static_assert(is_same<typename decay<decltype(*ptr)>::type, handle_storage<allocatorT>>::value, "???");
              const auto nbkt = handle_storage<allocatorT>::max_nbkt_for_nblk(ptr->nblk);
              ROCKET_ASSERT(nbkt != 0);
              // Conversion between an unsigned integer type and a floating point type results in performance penalty.
              // For a value known to be non-negative, an intermediate cast to some signed integer type will mitigate this.
              const auto seed = hval * static_cast<size_t>(0xA17870F5D4F51B49);
              const auto ratio = static_cast<double>(static_cast<ptrdiff_t>(seed / 2)) / static_cast<double>(static_cast<size_t>(-1) / 2 + 1);
              ROCKET_ASSERT((0.0 <= ratio) && (ratio < 1.0));
              const auto rorigin = static_cast<double>(static_cast<ptrdiff_t>(nbkt)) * ratio;
              ROCKET_ASSERT((0 <= rorigin) && (rorigin < static_cast<double>(nbkt)));
              return static_cast<size_type>(static_cast<ptrdiff_t>(rorigin));
            }

        template<typename xpointerT, typename predT>
          static typename copy_const_from<handle_type, decltype(::std::declval<xpointerT &>())>::type * probe(xpointerT ptr, size_type first, size_type last, predT &&pred)
            {
              static_assert(is_same<typename decay<decltype(*ptr)>::type, handle_storage<allocatorT>>::value, "???");
              const auto nbkt = ptr->max_nbkt_for_nblk(ptr->nblk);
              ROCKET_ASSERT(nbkt != 0);
              // Phase one: Probe from `first` to the end of the table.
              for(auto i = first; i != nbkt; ++i) {
                const auto bkt = ptr->data + i;
                if(!(bkt->get()) || ::std::forward<predT>(pred)(bkt)) {
                  return bkt;
                }
              }
              // Phase two: Probe from the beginning of the table to `last`.
              for(auto i = size_type(0); i != last; ++i) {
                const auto bkt = ptr->data + i;
                if(!(bkt->get()) || ::std::forward<predT>(pred)(bkt)) {
                  return bkt;
                }
              }
              // The table is full and no desired element has been found so far.
              return nullptr;
            }
      };

  template<typename allocatorT, typename hashT, bool copyableT = is_copy_constructible<typename allocatorT::value_type>::value>
    struct copy_storage_helper
      {
        // This is the generic version.
        template<typename xpointerT, typename ypointerT>
          void operator()(xpointerT ptr, const hashT &hf, ypointerT ptr_old, size_t off, size_t cnt) const
            {
              static_assert(is_same<typename decay<decltype(*ptr)>::type, handle_storage<allocatorT>>::value, "???");
              static_assert(is_same<typename decay<decltype(*ptr_old)>::type, handle_storage<allocatorT>>::value, "???");
              for(auto i = off; i != off + cnt; ++i) {
                const auto eptr_old = ptr_old->data[i].get();
                if(!eptr_old) {
                  continue;
                }
                // Find a bucket for the new element.
                const auto origin = linear_prober<allocatorT>::origin(ptr, hf(eptr_old->first));
                const auto bkt = linear_prober<allocatorT>::probe(ptr, origin, origin, [](const void *) { return false; });
                ROCKET_ASSERT(bkt);
                // Allocate a new element by copy-constructing from the old one.
                auto eptr = allocator_traits<allocatorT>::allocate(ptr->alloc, size_t(1));
                try {
                  allocator_traits<allocatorT>::construct(ptr->alloc, noadl::unfancy(eptr), *eptr_old);
                } catch(...) {
                  allocator_traits<allocatorT>::deallocate(ptr->alloc, eptr, size_t(1));
                  throw;
                }
                // Insert it into the new bucket.
                eptr = bkt->set(eptr);
                ROCKET_ASSERT(!eptr);
                ptr->nelem += 1;
              }
            }
      };
  template<typename allocatorT, typename hashT>
    struct copy_storage_helper<allocatorT, hashT, false>
      {
        // This specialization is used when `allocatorT::value_type` is not copy-constructible.
        template<typename xpointerT, typename ypointerT>
          [[noreturn]] void operator()(xpointerT /*ptr*/, const hashT & /*hf*/, ypointerT /*ptr_old*/, size_t /*off*/, size_t /*cnt*/) const
            {
              // `allocatorT::value_type` is not copy-constructible.
              // Throw an exception unconditionally, even when there is nothing to copy.
              noadl::throw_domain_error("cow_hashmap: The `keyT` or `mappedT` of this `cow_hashmap` is not copy-constructible.");
            }
      };

  template<typename allocatorT, typename hashT>
    struct move_storage_helper
      {
        // This is the generic version.
        template<typename xpointerT, typename ypointerT>
          void operator()(xpointerT ptr, const hashT &hf, ypointerT ptr_old, size_t off, size_t cnt) const
            {
              static_assert(is_same<typename decay<decltype(*ptr)>::type, handle_storage<allocatorT>>::value, "???");
              static_assert(is_same<typename decay<decltype(*ptr_old)>::type, handle_storage<allocatorT>>::value, "???");
              for(auto i = off; i != off + cnt; ++i) {
                const auto eptr_old = ptr_old->data[i].get();
                if(!eptr_old) {
                  continue;
                }
                // Find a bucket for the new element.
                const auto origin = linear_prober<allocatorT>::origin(ptr, hf(eptr_old->first));
                const auto bkt = linear_prober<allocatorT>::probe(ptr, origin, origin, [](const void *) { return false; });
                ROCKET_ASSERT(bkt);
                // Detach the old element.
                auto eptr = ptr_old->data[i].set(nullptr);
                ptr_old->nelem -= 1;
                // Insert it into the new bucket.
                eptr = bkt->set(eptr);
                ROCKET_ASSERT(!eptr);
                ptr->nelem += 1;
              }
            }
      };

  // This struct is used as placeholders for EBO'd bases that would otherwise be duplicate, in order to prevent ambiguity.
  template<int indexT>
    struct ebo_placeholder
      {
        template<typename anythingT>
          explicit constexpr ebo_placeholder(anythingT &&) noexcept
          {
          }
      };

  template<typename allocatorT, typename hashT, typename eqT>
    class storage_handle
      : private allocator_wrapper_base_for<allocatorT>::type
      , private conditional<is_same<hashT, allocatorT>::value,
                            ebo_placeholder<0>, typename allocator_wrapper_base_for<hashT>::type>::type
      , private conditional<is_same<eqT, allocatorT>::value || is_same<eqT, hashT>::value,
                            ebo_placeholder<1>, typename allocator_wrapper_base_for<eqT>::type>::type
      {
      public:
        using allocator_type   = allocatorT;
        using value_type       = typename allocator_type::value_type;
        using hasher           = hashT;
        using key_equal        = eqT;
        using handle_type      = value_handle<allocator_type>;
        using size_type        = typename allocator_traits<allocator_type>::size_type;
        using difference_type  = typename allocator_traits<allocator_type>::difference_type;

        enum : size_type { max_load_factor_reciprocal = 2 };

      private:
        using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;
        using hasher_base       = typename allocator_wrapper_base_for<hasher>::type;
        using key_equal_base    = typename allocator_wrapper_base_for<key_equal>::type;
        using storage           = handle_storage<allocator_type>;
        using storage_allocator = typename allocator_traits<allocator_type>::template rebind_alloc<storage>;
        using storage_pointer   = typename allocator_traits<storage_allocator>::pointer;

      private:
        storage_pointer m_ptr;

      public:
        storage_handle(const allocator_type &alloc, const hasher &hf, const key_equal &eq)
          : allocator_base(alloc)
          , conditional<is_same<hashT, allocatorT>::value,
                        ebo_placeholder<0>, hasher_base>::type(hf)
          , conditional<is_same<eqT, allocatorT>::value || is_same<eqT, hashT>::value,
                        ebo_placeholder<1>, key_equal_base>::type(eq)
          , m_ptr(nullptr)
          {
          }
        storage_handle(allocator_type &&alloc, const hasher &hf, const key_equal &eq)
          : allocator_base(::std::move(alloc))
          , conditional<is_same<hashT, allocatorT>::value,
                        ebo_placeholder<0>, hasher_base>::type(hf)
          , conditional<is_same<eqT, allocatorT>::value || is_same<eqT, hashT>::value,
                        ebo_placeholder<1>, key_equal_base>::type(eq)
          , m_ptr(nullptr)
          {
          }
        ~storage_handle()
          {
            this->do_reset(nullptr);
          }

        storage_handle(const storage_handle &)
          = delete;
        storage_handle & operator=(const storage_handle &)
          = delete;

      private:
        void do_reset(storage_pointer ptr_new) noexcept
          {
            const auto ptr = noadl::exchange(this->m_ptr, ptr_new);
            if(ptr == nullptr) {
              return;
            }
            // Decrement the reference count with acquire-release semantics to prevent races on `ptr->alloc`.
            const auto nref_old = ptr->nref.fetch_sub(1, ::std::memory_order_acq_rel);
            if(nref_old > 1) {
              return;
            }
            ROCKET_ASSERT(nref_old == 1);
            // If it has been decremented to zero, deallocate the block.
            auto st_alloc = storage_allocator(ptr->alloc);
            const auto nblk = ptr->nblk;
            noadl::destroy_at(noadl::unfancy(ptr));
#ifdef ROCKET_DEBUG
            ::std::memset(static_cast<void *>(noadl::unfancy(ptr)), '~', sizeof(storage) * nblk);
#endif
            allocator_traits<storage_allocator>::deallocate(st_alloc, ptr, nblk);
          }

      public:
        const hasher & as_hasher() const noexcept
          {
            return static_cast<const hasher_base &>(*this);
          }
        hasher & as_hasher() noexcept
          {
            return static_cast<hasher_base &>(*this);
          }

        const key_equal & as_key_equal() const noexcept
          {
            return static_cast<const key_equal_base &>(*this);
          }
        key_equal & as_key_equal() noexcept
          {
            return static_cast<key_equal_base &>(*this);
          }

        const allocator_type & as_allocator() const noexcept
          {
            return static_cast<const allocator_base &>(*this);
          }
        allocator_type & as_allocator() noexcept
          {
            return static_cast<allocator_base &>(*this);
          }

        bool unique() const noexcept
          {
            const auto ptr = this->m_ptr;
            if(ptr == nullptr) {
              return false;
            }
            return ptr->nref.load(::std::memory_order_relaxed) == 1;
          }
        size_type bucket_count() const noexcept
          {
            const auto ptr = this->m_ptr;
            if(ptr == nullptr) {
              return 0;
            }
            return storage::max_nbkt_for_nblk(ptr->nblk);
          }
        size_type capacity() const noexcept
          {
            const auto ptr = this->m_ptr;
            if(ptr == nullptr) {
              return 0;
            }
            return storage::max_nbkt_for_nblk(ptr->nblk) / max_load_factor_reciprocal;
          }
        size_type max_size() const noexcept
          {
            auto st_alloc = storage_allocator(this->as_allocator());
            const auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
            return storage::max_nbkt_for_nblk(max_nblk / 2) / max_load_factor_reciprocal;
          }
        size_type check_size_add(size_type base, size_type add) const
          {
            const auto cap_max = this->max_size();
            ROCKET_ASSERT(base <= cap_max);
            if(cap_max - base < add) {
              noadl::throw_length_error("cow_hashmap: Increasing `%lld` by `%lld` would exceed the max size `%lld`.",
                                        static_cast<long long>(base), static_cast<long long>(add), static_cast<long long>(cap_max));
            }
            return base + add;
          }
        size_type round_up_capacity(size_type res_arg) const
          {
            const auto cap = this->check_size_add(0, res_arg);
            const auto nblk = storage::min_nblk_for_nbkt(cap * max_load_factor_reciprocal);
            return storage::max_nbkt_for_nblk(nblk) / max_load_factor_reciprocal;
          }
        const handle_type * data() const noexcept
          {
            const auto ptr = this->m_ptr;
            if(ptr == nullptr) {
              return nullptr;
            }
            return ptr->data;
          }
        size_type element_count() const noexcept
          {
            const auto ptr = this->m_ptr;
            if(ptr == nullptr) {
              return 0;
            }
            return ptr->nelem;
          }
        handle_type * reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
          {
            if(res_arg == 0) {
              // Deallocate the block.
              this->do_reset(nullptr);
              return nullptr;
            }
            const auto cap = this->check_size_add(0, res_arg);
            // Allocate an array of `storage` large enough for a header + `cap` instances of pointers.
            const auto nblk = storage::min_nblk_for_nbkt(cap * max_load_factor_reciprocal);
            auto st_alloc = storage_allocator(this->as_allocator());
            const auto ptr = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
#ifdef ROCKET_DEBUG
            ::std::memset(static_cast<void *>(noadl::unfancy(ptr)), '*', sizeof(storage) * nblk);
#endif
            noadl::construct_at(noadl::unfancy(ptr), this->as_allocator(), nblk);
            const auto ptr_old = this->m_ptr;
            if(ptr_old) {
              try {
                // Copy or move elements into the new block.
                // Moving is only viable if the old and new allocators compare equal and the old block is owned exclusively.
                if((ptr_old->alloc != ptr->alloc) || (ptr_old->nref.load(::std::memory_order_relaxed) != 1)) {
                  copy_storage_helper<allocator_type, hasher>()(ptr, this->as_hasher(), ptr_old,       0, cnt_one);
                  copy_storage_helper<allocator_type, hasher>()(ptr, this->as_hasher(), ptr_old, off_two, cnt_two);
                } else {
                  move_storage_helper<allocator_type, hasher>()(ptr, this->as_hasher(), ptr_old,       0, cnt_one);
                  move_storage_helper<allocator_type, hasher>()(ptr, this->as_hasher(), ptr_old, off_two, cnt_two);
                }
              } catch(...) {
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
            this->do_reset(nullptr);
          }

        constexpr operator const storage_handle * () const noexcept
          {
            return this;
          }
        operator storage_handle * () noexcept
          {
            return this;
          }

        void share_with(const storage_handle &other) noexcept
          {
            const auto ptr = other.m_ptr;
            if(ptr) {
              // Increment the reference count.
              const auto nref_old = ptr->nref.fetch_add(1, ::std::memory_order_relaxed);
              ROCKET_ASSERT(nref_old >= 1);
            }
            this->do_reset(ptr);
          }
        void share_with(storage_handle &&other) noexcept
          {
            const auto ptr = other.m_ptr;
            if(ptr) {
              // Detach the block.
              other.m_ptr = nullptr;
            }
            this->do_reset(ptr);
          }
        void exchange_with(storage_handle &other) noexcept
          {
            ::std::swap(this->m_ptr, other.m_ptr);
          }

        template<typename ykeyT>
          difference_type index_of(const ykeyT &ykey) const
            {
              const auto ptr = this->m_ptr;
              if(ptr == nullptr) {
                return -1;
              }
              const auto origin = linear_prober<allocator_type>::origin(ptr, this->as_hasher()(ykey));
              const auto bkt = linear_prober<allocator_type>::probe(ptr, origin, origin,
                [&](const value_handle<allocatorT> *tbkt)
                  { return this->as_key_equal()(tbkt->get()->first, ykey); });
              if(!bkt || !(bkt->get())) {
                return -1;
              }
              const auto toff = bkt - ptr->data;
              ROCKET_ASSERT(toff >= 0);
              return toff;
            }
        handle_type * mut_data_unchecked() noexcept
          {
            const auto ptr = this->m_ptr;
            if(ptr == nullptr) {
              return nullptr;
            }
            ROCKET_ASSERT(this->unique());
            return ptr->data;
          }
        template<typename ykeyT, typename ...paramsT>
          pair<handle_type *, bool> keyed_emplace_unchecked(const ykeyT &ykey, paramsT &&...params)
            {
              ROCKET_ASSERT(this->unique());
              ROCKET_ASSERT(this->element_count() < this->capacity());
              const auto ptr = this->m_ptr;
              ROCKET_ASSERT(ptr);
              // Find a bucket for the new element.
              const auto origin = linear_prober<allocator_type>::origin(ptr, this->as_hasher()(ykey));
              const auto bkt = linear_prober<allocator_type>::probe(ptr, origin, origin,
                [&](const value_handle<allocatorT> *tbkt)
                  { return this->as_key_equal()(tbkt->get()->first, ykey); });
              ROCKET_ASSERT(bkt);
              if(bkt->get()) {
                // A duplicate key has been found.
                return ::std::make_pair(bkt, false);
              }
              // Allocate a new element.
              auto eptr = allocator_traits<allocator_type>::allocate(ptr->alloc, size_t(1));
              try {
                allocator_traits<allocator_type>::construct(ptr->alloc, noadl::unfancy(eptr), ::std::forward<paramsT>(params)...);
              } catch(...) {
                allocator_traits<allocatorT>::deallocate(ptr->alloc, eptr, size_t(1));
                throw;
              }
              // Insert it into the new bucket.
              eptr = bkt->set(eptr);
              ROCKET_ASSERT(!eptr);
              ptr->nelem += 1;
              return ::std::make_pair(bkt, true);
            }
        void erase_range_unchecked(size_type tpos, size_type tn) noexcept
          {
            ROCKET_ASSERT(this->unique());
            ROCKET_ASSERT(tpos <= this->bucket_count());
            ROCKET_ASSERT(tn <= this->bucket_count() - tpos);
            if(tn == 0) {
              return;
            }
            const auto ptr = this->m_ptr;
            ROCKET_ASSERT(ptr);
            const auto nbkt = storage::max_nbkt_for_nblk(ptr->nblk);
            ROCKET_ASSERT(nbkt != 0);
            // Erase all elements in [tpos,tpos+tn).
            for(auto i = tpos; i != tpos + tn; ++i) {
              const auto eptr = ptr->data[i].set(nullptr);
              if(!eptr) {
                continue;
              }
              ptr->nelem -= 1;
              // Destroy the element and deallocate its storage.
              allocator_traits<allocator_type>::destroy(ptr->alloc, noadl::unfancy(eptr));
              allocator_traits<allocator_type>::deallocate(ptr->alloc, eptr, size_t(1));
            }
            // Relocate elements that are not placed in their immediate locations.
            linear_prober<allocator_type>::probe(ptr, tpos + tn, tpos,
              [&](value_handle<allocator_type> *tbkt)
                {
                  // Remove the element from the old bucket.
                  auto eptr = tbkt->set(nullptr);
                  // Find a new bucket for it.
                  const auto origin = linear_prober<allocator_type>::origin(ptr, this->as_hasher()(eptr->first));
                  const auto bkt = linear_prober<allocator_type>::probe(ptr, origin, origin, [&](const void *) { return false; });
                  ROCKET_ASSERT(bkt);
                  // Insert it into the new bucket.
                  eptr = bkt->set(eptr);
                  ROCKET_ASSERT(!eptr);
                  return false;
                });
          }
      };

  // Informs the constructor of an iterator that the `bkt` parameter might point to an empty bucket.
  constexpr struct needs_adjust_tag
    {
    } needs_adjust;

  template<typename hashmapT, typename valueT>
    class hashmap_iterator
      {
        template<typename, typename>
          friend class hashmap_iterator;
        friend hashmapT;

      public:
        using iterator_category  = ::std::forward_iterator_tag;
        using value_type         = valueT;
        using pointer            = value_type *;
        using reference          = value_type &;
        using difference_type    = ptrdiff_t;

        using parent_type   = storage_handle<typename hashmapT::allocator_type, typename hashmapT::hasher, typename hashmapT::key_equal>;
        using handle_type   = typename copy_const_from<typename parent_type::handle_type, value_type>::type;

      private:
        const parent_type *m_ref;
        handle_type *m_bkt;

      private:
        constexpr hashmap_iterator(const parent_type *ref, handle_type *bkt) noexcept
          : m_ref(ref), m_bkt(bkt)
          {
          }
        hashmap_iterator(const parent_type *ref, needs_adjust_tag, handle_type *hint) noexcept
          : m_ref(ref), m_bkt(this->do_adjust_forwards(hint))
          {
          }

      public:
        constexpr hashmap_iterator() noexcept
          : hashmap_iterator(nullptr, nullptr)
          {
          }
        template<typename yvalueT, typename enable_if<is_convertible<yvalueT *, valueT *>::value>::type * = nullptr>
          constexpr hashmap_iterator(const hashmap_iterator<hashmapT, yvalueT> &other) noexcept
            : hashmap_iterator(other.m_ref, other.m_bkt)
            {
            }

      private:
        handle_type * do_assert_valid_bucket(handle_type *bkt, bool to_dereference) const noexcept
          {
            const auto ref = this->m_ref;
            ROCKET_ASSERT_MSG(ref, "This iterator has not been initialized.");
            const auto dist = static_cast<size_t>(bkt - ref->data());
            ROCKET_ASSERT_MSG(dist <= ref->bucket_count(), "This iterator has been invalidated.");
            ROCKET_ASSERT_MSG(!((dist < ref->bucket_count()) && (bkt->get() == nullptr)), "The element referenced by this iterator no longer exists.");
            ROCKET_ASSERT_MSG(!(to_dereference && (dist == ref->bucket_count())), "This iterator contains a past-the-end value and cannot be dereferenced.");
            return bkt;
          }
        handle_type * do_adjust_forwards(handle_type *hint) const noexcept
          {
            if(hint == nullptr) {
              return nullptr;
            }
            const auto ref = this->m_ref;
            ROCKET_ASSERT_MSG(ref, "This iterator has not been initialized.");
            const auto end = ref->data() + ref->bucket_count();
            auto bkt = hint;
            while((bkt != end) && (bkt->get() == nullptr)) {
              ++bkt;
            }
            return bkt;
          }

      public:
        const parent_type * parent() const noexcept
          {
            return this->m_ref;
          }

        handle_type * tell() const noexcept
          {
            return this->do_assert_valid_bucket(this->m_bkt, false);
          }
        handle_type * tell_owned_by(const parent_type *ref) const noexcept
          {
            ROCKET_ASSERT_MSG(this->m_ref == ref, "This iterator does not refer to an element in the same container.");
            return this->tell();
          }
        hashmap_iterator & seek_next() noexcept
          {
            auto bkt = this->do_assert_valid_bucket(this->m_bkt, false);
            ROCKET_ASSERT_MSG(bkt != this->m_ref->data() + this->m_ref->bucket_count(), "The past-the-end iterator cannot be incremented.");
            bkt = this->do_adjust_forwards(bkt + 1);
            this->m_bkt = this->do_assert_valid_bucket(bkt, false);
            return *this;
          }

        reference operator*() const noexcept
          {
            const auto bkt = this->do_assert_valid_bucket(this->m_bkt, true);
            return *(bkt->get());
          }
        pointer operator->() const noexcept
          {
            const auto bkt = this->do_assert_valid_bucket(this->m_bkt, true);
            return noadl::unfancy(bkt->get());
          }
      };

  template<typename hashmapT, typename valueT>
    inline hashmap_iterator<hashmapT, valueT> & operator++(hashmap_iterator<hashmapT, valueT> &rhs) noexcept
      {
        return rhs.seek_next();
      }

  template<typename hashmapT, typename valueT>
    inline hashmap_iterator<hashmapT, valueT> operator++(hashmap_iterator<hashmapT, valueT> &lhs, int) noexcept
      {
        auto res = lhs;
        lhs.seek_next();
        return res;
      }

  template<typename hashmapT, typename xvalueT, typename yvalueT>
    inline bool operator==(const hashmap_iterator<hashmapT, xvalueT> &lhs, const hashmap_iterator<hashmapT, yvalueT> &rhs) noexcept
      {
        return lhs.tell() == rhs.tell();
      }
  template<typename hashmapT, typename xvalueT, typename yvalueT>
    inline bool operator!=(const hashmap_iterator<hashmapT, xvalueT> &lhs, const hashmap_iterator<hashmapT, yvalueT> &rhs) noexcept
      {
        return lhs.tell() != rhs.tell();
      }
}

template<typename keyT, typename mappedT, typename hashT, typename eqT, typename allocatorT>
  class cow_hashmap
    {
      static_assert(is_array<keyT>::value == false, "`keyT` must not be an array type.");
      static_assert(is_array<mappedT>::value == false, "`mappedT` must not be an array type.");
      static_assert(is_same<typename allocatorT::value_type, pair<const keyT, mappedT>>::value, "`allocatorT::value_type` must denote the same type as `pair<const keyT, mappedT>`.");

    public:
      // types
      using key_type        = keyT;
      using mapped_type     = mappedT;
      using value_type      = pair<const key_type, mapped_type>;
      using hasher          = hashT;
      using key_equal       = eqT;
      using allocator_type  = allocatorT;

      using size_type        = typename allocator_traits<allocator_type>::size_type;
      using difference_type  = typename allocator_traits<allocator_type>::difference_type;
      using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
      using pointer          = typename allocator_traits<allocator_type>::pointer;
      using const_reference  = const value_type &;
      using reference        = value_type &;

      using const_iterator          = details_cow_hashmap::hashmap_iterator<cow_hashmap, const value_type>;
      using iterator                = details_cow_hashmap::hashmap_iterator<cow_hashmap, value_type>;

    private:
      details_cow_hashmap::storage_handle<allocator_type, hasher, key_equal> m_sth;

    public:
      // 26.5.4.2, construct/copy/destroy
      explicit cow_hashmap(size_type res_arg, const hasher &hf = hasher(), const key_equal &eq = key_equal(), const allocator_type &alloc = allocator_type())
        : m_sth(alloc, hf, eq)
        {
          this->m_sth.reallocate(0, 0, 0, res_arg);
        }
      cow_hashmap() noexcept(noexcept(hasher()) && is_nothrow_copy_constructible<hasher>::value &&
                             noexcept(key_equal()) && is_nothrow_copy_constructible<key_equal>::value &&
                             noexcept(allocator_type()))
        : cow_hashmap(0, hasher(), key_equal(), allocator_type())
        {
        }
      explicit cow_hashmap(const allocator_type &alloc) noexcept
        : cow_hashmap(0, hasher(), key_equal(), alloc)
        {
        }
      cow_hashmap(size_type res_arg, const allocator_type &alloc)
        : cow_hashmap(res_arg, hasher(), key_equal(), alloc)
        {
        }
      cow_hashmap(size_type res_arg, const hasher &hf, const allocator_type &alloc)
        : cow_hashmap(res_arg, hf, key_equal(), alloc)
        {
        }
      cow_hashmap(const cow_hashmap &other) noexcept(is_nothrow_copy_constructible<hasher>::value && is_nothrow_copy_constructible<key_equal>::value)
        : cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
        {
          this->assign(other);
        }
      cow_hashmap(const cow_hashmap &other, const allocator_type &alloc) noexcept(is_nothrow_copy_constructible<hasher>::value && is_nothrow_copy_constructible<key_equal>::value)
        : cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), alloc)
        {
          this->assign(other);
        }
      cow_hashmap(cow_hashmap &&other) noexcept(is_nothrow_copy_constructible<hasher>::value && is_nothrow_copy_constructible<key_equal>::value)
        : cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
        {
          this->assign(::std::move(other));
        }
      cow_hashmap(cow_hashmap &&other, const allocator_type &alloc) noexcept(is_nothrow_copy_constructible<hasher>::value && is_nothrow_copy_constructible<key_equal>::value)
        : cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), alloc)
        {
          this->assign(::std::move(other));
        }
      template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
        cow_hashmap(inputT first, inputT last, size_type res_arg = 0, const hasher &hf = hasher(), const key_equal &eq = key_equal(), const allocator_type &alloc = allocator_type())
          : cow_hashmap(res_arg, hf, eq, alloc)
          {
            this->assign(::std::move(first), ::std::move(last));
          }
      template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
        cow_hashmap(inputT first, inputT last, size_type res_arg, const hasher &hf, const allocator_type &alloc)
          : cow_hashmap(res_arg, hf, key_equal(), alloc)
          {
            this->assign(::std::move(first), ::std::move(last));
          }
      template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
        cow_hashmap(inputT first, inputT last, size_type res_arg, const allocator_type &alloc)
          : cow_hashmap(res_arg, hasher(), key_equal(), alloc)
          {
            this->assign(::std::move(first), ::std::move(last));
          }
      cow_hashmap(initializer_list<value_type> init, size_type res_arg = 0, const hasher &hf = hasher(), const key_equal &eq = key_equal(), const allocator_type &alloc = allocator_type())
        : cow_hashmap(res_arg, hf, eq, alloc)
        {
          this->assign(init);
        }
      cow_hashmap(initializer_list<value_type> init, size_type res_arg, const hasher &hf, const allocator_type &alloc)
        : cow_hashmap(res_arg, hf, key_equal(), alloc)
        {
          this->assign(init);
        }
      cow_hashmap(initializer_list<value_type> init, size_type res_arg, const allocator_type &alloc)
        : cow_hashmap(res_arg, hasher(), key_equal(), alloc)
        {
          this->assign(init);
        }
      cow_hashmap & operator=(const cow_hashmap &other) noexcept
        {
          allocator_copy_assigner<allocator_type>()(this->m_sth.as_allocator(), other.m_sth.as_allocator());
          return this->assign(other);
        }
      cow_hashmap & operator=(cow_hashmap &&other) noexcept
        {
          allocator_move_assigner<allocator_type>()(this->m_sth.as_allocator(), ::std::move(other.m_sth.as_allocator()));
          return this->assign(::std::move(other));
        }
      cow_hashmap & operator=(initializer_list<value_type> init)
        {
          return this->assign(init);
        }

    private:
      // Reallocate the storage to `res_arg` elements.
      // The storage is owned by the current hashmap exclusively after this function returns normally.
      details_cow_hashmap::value_handle<allocator_type> * do_reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
        {
          ROCKET_ASSERT(cnt_one <= off_two);
          ROCKET_ASSERT(off_two <= this->m_sth.bucket_count());
          ROCKET_ASSERT(cnt_two <= this->m_sth.bucket_count() - off_two);
          const auto ptr = this->m_sth.reallocate(cnt_one, off_two, cnt_two, res_arg);
          if(ptr == nullptr) {
            // The storage has been deallocated.
            return nullptr;
          }
          ROCKET_ASSERT(this->m_sth.unique());
          return ptr;
        }
      // Deallocate any dynamic storage.
      void do_deallocate() noexcept
        {
          this->m_sth.deallocate();
        }

      // Reallocate more storage as needed, without shrinking.
      void do_reserve_more(size_type cap_add)
        {
          const auto cnt = this->size();
          auto cap = this->m_sth.check_size_add(cnt, cap_add);
          if((this->unique() == false) || (this->capacity() < cap)) {
#ifndef ROCKET_DEBUG
            // Reserve more space for non-debug builds.
            cap = noadl::max(cap, cnt + cnt / 2 + 7);
#endif
            this->do_reallocate(0, 0, this->bucket_count(), cap);
          }
          ROCKET_ASSERT(this->capacity() >= cap);
        }

      const details_cow_hashmap::value_handle<allocator_type> * do_get_table() const noexcept
        {
          return this->m_sth.data();
        }
      details_cow_hashmap::value_handle<allocator_type> * do_mut_table()
        {
          if(this->unique() == false) {
            return this->do_reallocate(0, 0, this->bucket_count(), this->size());
          }
          return this->m_sth.mut_data_unchecked();
        }

      details_cow_hashmap::value_handle<allocator_type> * do_erase_no_bound_check(size_type tpos, size_type tn)
        {
          const auto cnt_old = this->size();
          const auto nbkt_old = this->bucket_count();
          ROCKET_ASSERT(tpos <= nbkt_old);
          ROCKET_ASSERT(tn <= nbkt_old - tpos);
          if(this->unique() == false) {
            const auto ptr = this->do_reallocate(tpos, tpos + tn, nbkt_old - (tpos + tn), cnt_old);
            return ptr;
          }
          const auto ptr = this->m_sth.mut_data_unchecked();
          this->m_sth.erase_range_unchecked(tpos, tn);
          return ptr + tn;
        }

    public:
      // iterators
      const_iterator begin() const noexcept
        {
          return const_iterator(this->m_sth, details_cow_hashmap::needs_adjust, this->do_get_table());
        }
      const_iterator end() const noexcept
        {
          return const_iterator(this->m_sth, this->do_get_table() + this->bucket_count());
        }

      const_iterator cbegin() const noexcept
        {
          return this->begin();
        }
      const_iterator cend() const noexcept
        {
          return this->end();
        }

      // N.B. This function may throw `std::bad_alloc()`.
      // N.B. This is a non-standard extension.
      iterator mut_begin()
        {
          return iterator(this->m_sth, details_cow_hashmap::needs_adjust, this->do_mut_table());
        }
      // N.B. This function may throw `std::bad_alloc()`.
      // N.B. This is a non-standard extension.
      iterator mut_end()
        {
          return iterator(this->m_sth, this->do_mut_table() + this->bucket_count());
        }

      // capacity
      bool empty() const noexcept
        {
          return this->m_sth.element_count() == 0;
        }
      size_type size() const noexcept
        {
          return this->m_sth.element_count();
        }
      size_type max_size() const noexcept
        {
          return this->m_sth.max_size();
        }
      size_type capacity() const noexcept
        {
          return this->m_sth.capacity();
        }
      void reserve(size_type res_arg)
        {
          const auto cnt = this->size();
          const auto cap_new = this->m_sth.round_up_capacity(noadl::max(cnt, res_arg));
          // If the storage is shared with other hashmaps, force rellocation to prevent copy-on-write upon modification.
          if((this->unique() != false) && (this->capacity() >= cap_new)) {
            return;
          }
          this->do_reallocate(0, 0, this->bucket_count(), cap_new);
          ROCKET_ASSERT(this->capacity() >= res_arg);
        }
      void shrink_to_fit()
        {
          const auto cnt = this->size();
          const auto cap_min = this->m_sth.round_up_capacity(cnt);
          // Don't increase memory usage.
          if((this->unique() == false) || (this->capacity() <= cap_min)) {
            return;
          }
          this->do_reallocate(0, 0, cnt, cnt);
          ROCKET_ASSERT(this->capacity() <= cap_min);
        }
      void clear() noexcept
        {
          if(this->unique() == false) {
            this->do_deallocate();
            return;
          }
          this->m_sth.erase_range_unchecked(0, this->bucket_count());
        }
      // N.B. This is a non-standard extension.
      bool unique() const noexcept
        {
          return this->m_sth.unique();
        }

      // hash policy
      // N.B. This is a non-standard extension.
      size_type bucket_count() const noexcept
        {
          return this->m_sth.bucket_count();
        }
      // N.B. The return type differs from `std::unordered_map`.
      double load_factor() const noexcept
        {
          return static_cast<double>(static_cast<difference_type>(this->size())) / static_cast<double>(static_cast<difference_type>(this->bucket_count()));
        }
      // N.B. The `constexpr` specifier is a non-standard extension.
      // N.B. The return type differs from `std::unordered_map`.
      constexpr double max_load_factor() const noexcept
        {
          return 1.0 / static_cast<double>(static_cast<difference_type>(decltype(this->m_sth)::max_load_factor_reciprocal));
        }
      void rehash(size_type n)
        {
          this->do_reallocate(0, 0, this->bucket_count(), noadl::max(this->size(), n));
        }

      // 26.5.4.4, modifiers
      pair<iterator, bool> insert(const value_type &value)
        {
          return this->try_emplace(value.first, value.second);
        }
      pair<iterator, bool> insert(value_type &&value)
        {
          return this->try_emplace(value.first, ::std::move(value.second));
        }
      // N.B. This is a non-standard extension.
      template<typename ykeyT, typename yvalueT>
        pair<iterator, bool> insert(const pair<ykeyT, yvalueT> &value)
          {
            return this->try_emplace(value.first, value.second);
          }
      // N.B. This is a non-standard extension.
      template<typename ykeyT, typename yvalueT>
        pair<iterator, bool> insert(pair<ykeyT, yvalueT> &&value)
          {
            return this->try_emplace(::std::move(value.first), ::std::move(value.second));
          }
      // N.B. The return type is a non-standard extension.
      template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
        cow_hashmap & insert(inputT first, inputT last)
          {
            if(first == last) {
              return *this;
            }
            const auto dist = noadl::estimate_distance(first, last);
            if(dist == 0) {
              noadl::ranged_do_while(::std::move(first), ::std::move(last), [&](const inputT &it) { this->insert(*it); });
              return *this;
            }
            this->do_reserve_more(dist);
            noadl::ranged_do_while(::std::move(first), ::std::move(last), [&](const inputT &it) { this->m_sth.keyed_emplace_unchecked(it->first, *it); });
            return *this;
          }
      // N.B. The return type is a non-standard extension.
      cow_hashmap & insert(initializer_list<value_type> init)
        {
          return this->insert(init.begin(), init.end());
        }
      // N.B. The hint is ignored.
      iterator insert(const_iterator /*hint*/, const value_type &value)
        {
          return this->insert(value).first;
        }
      // N.B. The hint is ignored.
      iterator insert(const_iterator /*hint*/, value_type &&value)
        {
          return this->insert(::std::move(value)).first;
        }
      // N.B. This is a non-standard extension.
      // N.B. The hint is ignored.
      template<typename ykeyT, typename yvalueT>
        iterator insert(const_iterator /*hint*/, const pair<ykeyT, yvalueT> &value)
          {
            return this->insert(value).first;
          }
      // N.B. This is a non-standard extension.
      // N.B. The hint is ignored.
      template<typename ykeyT, typename yvalueT>
        iterator insert(const_iterator /*hint*/, pair<ykeyT, yvalueT> &&value)
          {
            return this->insert(::std::move(value)).first;
          }

      template<typename ...paramsT>
        pair<iterator, bool> try_emplace(const key_type &key, paramsT &&...params)
          {
            this->do_reserve_more(1);
            const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::piecewise_construct,
                                                                    ::std::forward_as_tuple(key), ::std::forward_as_tuple(::std::forward<paramsT>(params)...));
            return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
          }
      template<typename ...paramsT>
        pair<iterator, bool> try_emplace(key_type &&key, paramsT &&...params)
          {
            this->do_reserve_more(1);
            const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::piecewise_construct,
                                                                    ::std::forward_as_tuple(::std::move(key)), ::std::forward_as_tuple(::std::forward<paramsT>(params)...));
            return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
          }
      // N.B. The hint is ignored.
      template<typename ...paramsT>
        iterator try_emplace(const_iterator /*hint*/, const key_type &key, paramsT &&...params)
          {
            return this->try_emplace(key, ::std::forward<paramsT>(params)...).first;
          }
      // N.B. The hint is ignored.
      template<typename ...paramsT>
        iterator try_emplace(const_iterator /*hint*/, key_type &&key, paramsT &&...params)
          {
            return this->try_emplace(::std::move(key), ::std::forward<paramsT>(params)...).first;
          }

      template<typename yvalueT>
        pair<iterator, bool> insert_or_assign(const key_type &key, yvalueT &&yvalue)
          {
            this->do_reserve_more(1);
            const auto result = this->m_sth.keyed_emplace_unchecked(key, key, ::std::forward<yvalueT>(yvalue));
            if(result.second == false) {
              result.first->get()->second = ::std::forward<yvalueT>(yvalue);
            }
            return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
          }
      template<typename yvalueT>
        pair<iterator, bool> insert_or_assign(key_type &&key, yvalueT &&yvalue)
          {
            this->do_reserve_more(1);
            const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::move(key), ::std::forward<yvalueT>(yvalue));
            if(result.second == false) {
              result.first->get()->second = ::std::forward<yvalueT>(yvalue);
            }
            return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
          }
      // N.B. The hint is ignored.
      template<typename yvalueT>
        iterator insert_or_assign(const_iterator /*hint*/, const key_type &key, yvalueT &&yvalue)
          {
            return this->insert_or_assign(key, ::std::forward<yvalueT>(yvalue)).first;
          }
      // N.B. The hint is ignored.
      template<typename yvalueT>
        iterator insert_or_assign(const_iterator /*hint*/, key_type &&key, yvalueT &&yvalue)
          {
            return this->insert_or_assign(::std::move(key), ::std::forward<yvalueT>(yvalue)).first;
          }

      // N.B. This function may throw `std::bad_alloc()`.
      iterator erase(const_iterator tfirst, const_iterator tlast)
        {
          const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->do_get_table());
          const auto tn = static_cast<size_type>(tlast.tell_owned_by(this->m_sth) - tfirst.tell());
          const auto ptr = this->do_erase_no_bound_check(tpos, tn);
          return iterator(this->m_sth, details_cow_hashmap::needs_adjust, ptr);
        }
      // N.B. This function may throw `std::bad_alloc()`.
      iterator erase(const_iterator tfirst)
        {
          const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->do_get_table());
          const auto ptr = this->do_erase_no_bound_check(tpos, 1);
          return iterator(this->m_sth, details_cow_hashmap::needs_adjust, ptr);
        }
      // N.B. This function may throw `std::bad_alloc()`.
      // N.B. The return type differs from `std::unordered_map`.
      template<typename ykeyT, typename enable_if<is_convertible<ykeyT, const_iterator>::value == false>::type * = nullptr>
        bool erase(const ykeyT &key)
          {
            const auto toff = this->m_sth.index_of(key);
            if(toff < 0) {
              return false;
            }
            this->do_erase_no_bound_check(static_cast<size_type>(toff), 1);
            return true;
          }

      // map operations
      const_iterator find(const key_type &key) const
        {
          const auto ptr = this->do_get_table();
          const auto toff = this->m_sth.index_of(key);
          if(toff < 0) {
            return this->end();
          }
          return const_iterator(this->m_sth, ptr + toff);
        }
      // N.B. This function may throw `std::bad_alloc()`.
      // N.B. This is a non-standard extension.
      iterator find_mut(const key_type &key)
        {
          const auto ptr = this->do_mut_table();
          const auto toff = this->m_sth.index_of(key);
          if(toff < 0) {
            return this->mut_end();
          }
          return iterator(this->m_sth, ptr + toff);
        }
      pair<const_iterator, const_iterator> equal_range(const key_type &key) const
        {
          const auto ptr = this->do_get_table();
          const auto toff = this->m_sth.index_of(key);
          if(toff < 0) {
            auto tcur = this->end();
            return ::std::make_pair(tcur, tcur);
          }
          auto tnext = const_iterator(this->m_sth, ptr + toff);
          auto tcur = tnext;
          tnext.seek_next();
          return ::std::make_pair(tcur, tnext);
        }
      // N.B. This function may throw `std::bad_alloc()`.
      // N.B. This is a non-standard extension.
      pair<iterator, iterator> mut_equal_range(const key_type &key)
        {
          const auto ptr = this->do_mut_table();
          const auto toff = this->m_sth.index_of(key);
          if(toff < 0) {
            auto tcur = this->mut_end();
            return ::std::make_pair(tcur, tcur);
          }
          auto tnext = iterator(this->m_sth, ptr + toff);
          auto tcur = tnext;
          tnext.seek_next();
          return ::std::make_pair(tcur, tnext);
        }
      // N.B. The return type differs from `std::unordered_map`.
      bool count(const key_type &key) const
        {
          const auto toff = this->m_sth.index_of(key);
          if(toff < 0) {
            return false;
          }
          return true;
        }

      // N.B. This is a non-standard extension.
      const mapped_type * get(const key_type &key) const
        {
          const auto ptr = this->do_get_table();
          const auto toff = this->m_sth.index_of(key);
          if(toff < 0) {
            return nullptr;
          }
          return ::std::addressof(ptr[toff].get()->second);
        }
      // N.B. This is a non-standard extension.
      mapped_type * get_mut(const key_type &key)
        {
          const auto ptr = this->do_mut_table();
          const auto toff = this->m_sth.index_of(key);
          if(toff < 0) {
            return nullptr;
          }
          return ::std::addressof(ptr[toff].get()->second);
        }
      // N.B. This is a non-standard extension.
      template<typename yvalueT>
        pair<mapped_type *, bool> set(const key_type &key, yvalueT &&yvalue)
          {
            this->do_reserve_more(1);
            const auto result = this->m_sth.keyed_emplace_unchecked(key, key, ::std::forward<yvalueT>(yvalue));
            if(result.second == false) {
              result.first->get()->second = ::std::forward<yvalueT>(yvalue);
            }
            return ::std::make_pair(::std::addressof(result.first->get()->second), result.second);
          }
      // N.B. This is a non-standard extension.
      template<typename yvalueT>
        pair<mapped_type *, bool> set(key_type &&key, yvalueT &&yvalue)
          {
            this->do_reserve_more(1);
            const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::move(key), ::std::forward<yvalueT>(yvalue));
            if(result.second == false) {
              result.first->get()->second = ::std::forward<yvalueT>(yvalue);
            }
            return ::std::make_pair(::std::addressof(result.first->get()->second), result.second);
          }

      // 26.5.4.3, element access
      mapped_type & operator[](const key_type &key)
        {
          this->do_reserve_more(1);
          const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::piecewise_construct,
                                                                  ::std::forward_as_tuple(key), ::std::forward_as_tuple());
          return result.first->get()->second;
        }
      mapped_type & operator[](key_type &&key)
        {
          this->do_reserve_more(1);
          const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::piecewise_construct,
                                                                  ::std::forward_as_tuple(::std::move(key)), ::std::forward_as_tuple());
          return result.first->get()->second;
        }
      const mapped_type & at(const key_type &key) const
        {
          const auto ptr = this->do_get_table();
          const auto toff = this->m_sth.index_of(key);
          if(toff < 0) {
            noadl::throw_out_of_range("cow_hashmap: The specified key does not exist in this hashmap.");
          }
          return ptr[toff].get()->second;
        }
      // N.B. This is a non-standard extension.
      mapped_type & mut(const key_type &key)
        {
          const auto ptr = this->do_mut_table();
          const auto toff = this->m_sth.index_of(key);
          if(toff < 0) {
            noadl::throw_out_of_range("cow_hashmap: The specified key does not exist in this hashmap.");
          }
          return ptr[toff].get()->second;
        }

      // N.B. This function is a non-standard extension.
      cow_hashmap & assign(const cow_hashmap &other) noexcept
        {
          this->m_sth.share_with(other.m_sth);
          return *this;
        }
      // N.B. This function is a non-standard extension.
      cow_hashmap & assign(cow_hashmap &&other) noexcept
        {
          this->m_sth.share_with(::std::move(other.m_sth));
          return *this;
        }
      // N.B. This function is a non-standard extension.
      cow_hashmap & assign(initializer_list<value_type> init)
        {
          this->clear();
          this->insert(init);
          return *this;
        }
      // N.B. This function is a non-standard extension.
      template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
      cow_hashmap & assign(inputT first, inputT last)
        {
          this->clear();
          this->insert(::std::move(first), ::std::move(last));
          return *this;
        }

      void swap(cow_hashmap &other) noexcept
        {
          allocator_swapper<allocator_type>()(this->m_sth.as_allocator(), other.m_sth.as_allocator());
          this->m_sth.exchange_with(other.m_sth);
        }

      // N.B. The return type differs from `std::unordered_map`.
      const allocator_type & get_allocator() const noexcept
        {
          return this->m_sth.as_allocator();
        }
      allocator_type & get_allocator() noexcept
        {
          return this->m_sth.as_allocator();
        }
      const hasher & hash_function() const noexcept
        {
          return this->m_sth.as_hasher();
        }
      hasher & hash_function() noexcept
        {
          return this->m_sth.as_hasher();
        }
      const key_equal & key_eq() const noexcept
        {
          return this->m_sth.as_key_equal();
        }
      key_equal & key_eq() noexcept
        {
          return this->m_sth.as_key_equal();
        }
    };

template<typename ...paramsT>
  inline void swap(cow_hashmap<paramsT...> &lhs, cow_hashmap<paramsT...> &rhs) noexcept
    {
      lhs.swap(rhs);
    }

template<typename ...paramsT>
  inline typename cow_hashmap<paramsT...>::const_iterator begin(const cow_hashmap<paramsT...> &rhs) noexcept
    {
      return rhs.begin();
    }
template<typename ...paramsT>
  inline typename cow_hashmap<paramsT...>::const_iterator end(const cow_hashmap<paramsT...> &rhs) noexcept
    {
      return rhs.end();
    }

template<typename ...paramsT>
  inline typename cow_hashmap<paramsT...>::const_iterator cbegin(const cow_hashmap<paramsT...> &rhs) noexcept
    {
      return rhs.cbegin();
    }
template<typename ...paramsT>
  inline typename cow_hashmap<paramsT...>::const_iterator cend(const cow_hashmap<paramsT...> &rhs) noexcept
    {
      return rhs.cend();
    }

}

#endif
