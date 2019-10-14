// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_HASHMAP_HPP_
#define ROCKET_COW_HASHMAP_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"
#include "hash_table_utilities.hpp"
#include "reference_counter.hpp"
#include <iterator>  // std::iterator_traits<>, std::forward_iterator_tag
#include <tuple>  // std::forward_as_tuple()
#include <cstring>  // std::memset()

namespace rocket {

template<typename keyT, typename mappedT, typename hashT = hash<keyT>, typename eqT = equal_to<keyT>,
         typename allocT = allocator<pair<const keyT, mappedT>>> class cow_hashmap;

/* Differences from `std::unordered_map`:
 * 1. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always return `const_reference`s.
 * 2. The copy constructor and copy assignment operator will not throw exceptions.
 * 3. Comparison operators are not provided.
 * 4. `emplace()` and `emplace_hint()` functions are not provided. `try_emplace()` is recommended as an alternative.
 * 5. There are no buckets. Bucket lookups and local iterators are not provided. Multimap cannot be implemented.
 * 6. The key and mapped types may be incomplete. The mapped type need be neither copy-assignable nor move-assignable.
 * 7. `erase()` may move elements around and invalidate iterators.
 */

    namespace details_cow_hashmap {

    struct storage_header
      {
        void (*dtor)(...);
        mutable reference_counter<long> nref;
        size_t nelem;

        explicit storage_header(void (*xdtor)(...)) noexcept
          :
            dtor(xdtor), nref()
            // `nelem` is uninitialized.
          {
          }
      };

    template<typename allocT> class bucket
      {
      public:
        using allocator_type   = allocT;
        using value_type       = typename allocT::value_type;
        using const_reference  = const value_type&;
        using reference        = value_type&;
        using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
        using pointer          = typename allocator_traits<allocator_type>::pointer;

      private:
        pointer m_ptr;

      public:
        bucket() noexcept
          = default;

        bucket(const bucket&)
          = delete;
        bucket& operator=(const bucket&)
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
        pointer reset(pointer ptr = pointer()) noexcept
          {
            return ::std::exchange(this->m_ptr, ptr);
          }

        explicit operator bool () const noexcept
          {
            return bool(this->get());
          }
        const_reference operator*() const noexcept
          {
            return *(this->get());
          }
        reference operator*() noexcept
          {
            return *(this->get());
          }
        const_pointer operator->() const noexcept
          {
            return this->m_ptr;
          }
        pointer operator->() noexcept
          {
            return this->m_ptr;
          }
      };

    template<typename allocT> struct pointer_storage : storage_header
      {
        using allocator_type   = allocT;
        using bucket_type      = bucket<allocator_type>;
        using size_type        = typename allocator_traits<allocator_type>::size_type;

        static constexpr size_type min_nblk_for_nbkt(size_type nbkt) noexcept
          {
            return (sizeof(bucket_type) * nbkt + sizeof(pointer_storage) - 1) / sizeof(pointer_storage) + 1;
          }
        static constexpr size_type max_nbkt_for_nblk(size_type nblk) noexcept
          {
            return sizeof(pointer_storage) * (nblk - 1) / sizeof(bucket_type);
          }

        allocator_type alloc;
        size_type nblk;
        union { bucket_type data[0];  };

        pointer_storage(void (*xdtor)(...), const allocator_type& xalloc, size_type xnblk) noexcept
          :
            storage_header(xdtor),
            alloc(xalloc), nblk(xnblk)
          {
            auto nbkt = pointer_storage::max_nbkt_for_nblk(this->nblk);
            if(is_trivially_default_constructible<bucket_type>::value) {
              // Zero-initialize everything.
              ::std::memset(static_cast<void*>(this->data), 0, sizeof(bucket_type) * nbkt);
            }
            else {
              // The C++ standard requires that value-initialization of such an object shall not throw exceptions
              // and shall result in a null pointer.
              for(size_type i = 0; i < nbkt; ++i)
                noadl::construct_at(this->data + i);
            }
            this->nelem = 0;
          }
        ~pointer_storage()
          {
            auto nbkt = pointer_storage::max_nbkt_for_nblk(this->nblk);
            for(size_type i = 0; i < nbkt; ++i) {
              auto eptr = this->data[i].reset();
              if(!eptr) {
                  continue;
              }
              allocator_traits<allocator_type>::destroy(this->alloc, noadl::unfancy(eptr));
              allocator_traits<allocator_type>::deallocate(this->alloc, eptr, size_t(1));
            }
            // `allocator_type::pointer` need not be a trivial type.
            for(size_type i = 0; i < nbkt; ++i) {
              noadl::destroy_at(this->data + i);
            }
#ifdef ROCKET_DEBUG
            this->nelem = 0xEECD;
#endif
          }

        pointer_storage(const pointer_storage&)
          = delete;
        pointer_storage& operator=(const pointer_storage&)
          = delete;
      };

    template<typename pointerT, typename hashT, typename allocT,
             bool copyableT = is_copy_constructible<typename allocT::value_type>::value>
        struct copy_storage_helper
      {
        void operator()(pointerT ptr, const hashT& hf, pointerT ptr_old, size_t off, size_t cnt) const
          {
            // Get table bounds.
            auto data = ptr->data;
            auto end = data + pointer_storage<allocT>::max_nbkt_for_nblk(ptr->nblk);
            // Copy elements one by one.
            for(auto i = off; i != off + cnt; ++i) {
              auto eptr_old = ptr_old->data[i].get();
              if(!eptr_old) {
                continue;
              }
              // Find a bucket for the new element.
              auto origin = noadl::get_probing_origin(data, end, hf(eptr_old->first));
              auto bkt = noadl::linear_probe(data, origin, origin, end, [&](const auto&) { return false;  });
              ROCKET_ASSERT(bkt);
              // Allocate a new element by copy-constructing from the old one.
              auto eptr = allocator_traits<allocT>::allocate(ptr->alloc, size_t(1));
              try {
                allocator_traits<allocT>::construct(ptr->alloc, noadl::unfancy(eptr), *eptr_old);
              }
              catch(...) {
                allocator_traits<allocT>::deallocate(ptr->alloc, eptr, size_t(1));
                throw;
              }
              // Insert it into the new bucket.
              ROCKET_ASSERT(!*bkt);
              bkt->reset(eptr);
              ptr->nelem++;
            }
          }
      };
    template<typename pointerT, typename hashT, typename allocT>
        struct copy_storage_helper<pointerT, hashT, allocT,
                                   false>     // copyableT
      {
        [[noreturn]] void operator()(pointerT /*ptr*/, const hashT& /*hf*/, pointerT /*ptr_old*/, size_t /*off*/, size_t /*cnt*/) const
          {
            // `allocT::value_type` is not copy-constructible.
            // Throw an exception unconditionally, even when there is nothing to copy.
            noadl::sprintf_and_throw<domain_error>("cow_hashmap: `%s` not copy-constructible",
                                                   typeid(typename allocT::value_type).name());
          }
      };

    template<typename pointerT, typename hashT, typename allocT>
        struct move_storage_helper
      {
        void operator()(pointerT ptr, const hashT& hf, pointerT ptr_old, size_t off, size_t cnt) const
          {
            // Get table bounds.
            auto data = ptr->data;
            auto end = data + pointer_storage<allocT>::max_nbkt_for_nblk(ptr->nblk);
            // Move elements one by one.
            for(auto i = off; i != off + cnt; ++i) {
              auto eptr_old = ptr_old->data[i].get();
              if(!eptr_old) {
                continue;
              }
              // Find a bucket for the new element.
              auto origin = noadl::get_probing_origin(data, end, hf(eptr_old->first));
              auto bkt = noadl::linear_probe(data, origin, origin, end, [&](const auto&) { return false;  });
              ROCKET_ASSERT(bkt);
              // Detach the old element.
              auto eptr = ptr_old->data[i].reset();
              ptr_old->nelem--;
              // Insert it into the new bucket.
              ROCKET_ASSERT(!*bkt);
              bkt->reset(eptr);
              ptr->nelem++;
            }
          }
      };

    // This struct is used as placeholders for EBO'd bases that would otherwise be duplicate, in order to prevent ambiguity.
    template<int indexT> struct ebo_placeholder
      {
        template<typename anythingT> explicit constexpr ebo_placeholder(anythingT&&) noexcept
          {
          }
      };

    template<typename allocT, typename hashT, typename eqT>
        class storage_handle : private allocator_wrapper_base_for<allocT>::type,
                               private conditional<is_same<hashT, allocT>::value,
                                                   ebo_placeholder<0>, typename allocator_wrapper_base_for<hashT>::type>::type,
                               private conditional<is_same<eqT, allocT>::value || is_same<eqT, hashT>::value,
                                                   ebo_placeholder<1>, typename allocator_wrapper_base_for<eqT>::type>::type
      {
      public:
        using allocator_type   = allocT;
        using value_type       = typename allocator_type::value_type;
        using hasher           = hashT;
        using key_equal        = eqT;
        using bucket_type      = bucket<allocator_type>;
        using size_type        = typename allocator_traits<allocator_type>::size_type;
        using difference_type  = typename allocator_traits<allocator_type>::difference_type;

        static constexpr size_type max_load_factor_reciprocal = 2;

      private:
        using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;
        using hasher_base       = typename allocator_wrapper_base_for<hasher>::type;
        using key_equal_base    = typename allocator_wrapper_base_for<key_equal>::type;
        using storage           = pointer_storage<allocator_type>;
        using storage_allocator = typename allocator_traits<allocator_type>::template rebind_alloc<storage>;
        using storage_pointer   = typename allocator_traits<storage_allocator>::pointer;

      private:
        storage_pointer m_ptr;

      public:
        constexpr storage_handle(const allocator_type& alloc, const hasher& hf, const key_equal& eq)
          :
            allocator_base(alloc),
            conditional<is_same<hashT, allocT>::value,
                        ebo_placeholder<0>, hasher_base>::type(hf),
            conditional<is_same<eqT, allocT>::value || is_same<eqT, hashT>::value,
                        ebo_placeholder<1>, key_equal_base>::type(eq),
            m_ptr()
          {
          }
        constexpr storage_handle(allocator_type&& alloc, const hasher& hf, const key_equal& eq)
          :
            allocator_base(noadl::move(alloc)),
            conditional<is_same<hashT, allocT>::value,
                        ebo_placeholder<0>, hasher_base>::type(hf),
            conditional<is_same<eqT, allocT>::value || is_same<eqT, hashT>::value,
                        ebo_placeholder<1>, key_equal_base>::type(eq),
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
            storage_allocator st_alloc(ptr->alloc);
            auto nblk = ptr->nblk;
            noadl::destroy_at(noadl::unfancy(ptr));
#ifdef ROCKET_DEBUG
            ::std::memset(static_cast<void*>(noadl::unfancy(ptr)), '~', sizeof(storage) * nblk);
#endif
            allocator_traits<storage_allocator>::deallocate(st_alloc, ptr, nblk);
          }

      public:
        const hasher& as_hasher() const noexcept
          {
            return static_cast<const hasher_base&>(*this);
          }
        hasher& as_hasher() noexcept
          {
            return static_cast<hasher_base&>(*this);
          }

        const key_equal& as_key_equal() const noexcept
          {
            return static_cast<const key_equal_base&>(*this);
          }
        key_equal& as_key_equal() noexcept
          {
            return static_cast<key_equal_base&>(*this);
          }

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
        constexpr double max_load_factor() const noexcept
          {
            return 1.0 / static_cast<double>(static_cast<difference_type>(max_load_factor_reciprocal));
          }
        size_type bucket_count() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return 0;
            }
            return storage::max_nbkt_for_nblk(ptr->nblk);
          }
        size_type capacity() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return 0;
            }
            auto cap = storage::max_nbkt_for_nblk(ptr->nblk) / max_load_factor_reciprocal;
            ROCKET_ASSERT(cap > 0);
            return cap;
          }
        size_type max_size() const noexcept
          {
            storage_allocator st_alloc(this->as_allocator());
            auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
            return storage::max_nbkt_for_nblk(max_nblk / 2) / max_load_factor_reciprocal;
          }
        size_type check_size_add(size_type base, size_type add) const
          {
            auto cap_max = this->max_size();
            ROCKET_ASSERT(base <= cap_max);
            if(cap_max - base < add) {
              noadl::sprintf_and_throw<length_error>("cow_hashmap: max size exceeded (`%llu` + `%llu` > `%llu`)",
                                                     static_cast<unsigned long long>(base), static_cast<unsigned long long>(add),
                                                     static_cast<unsigned long long>(cap_max));
            }
            return base + add;
          }
        size_type round_up_capacity(size_type res_arg) const
          {
            auto cap = this->check_size_add(0, res_arg);
            auto nblk = storage::min_nblk_for_nbkt(cap * max_load_factor_reciprocal);
            return storage::max_nbkt_for_nblk(nblk) / max_load_factor_reciprocal;
          }
        const bucket_type* buckets() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return nullptr;
            }
            return ptr->data;
          }
        size_type element_count() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return 0;
            }
            return reinterpret_cast<const storage_header*>(ptr)->nelem;
          }
        ROCKET_NOINLINE bucket_type* reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
          {
            if(res_arg == 0) {
              // Deallocate the block.
              this->deallocate();
              return nullptr;
            }
            auto cap = this->check_size_add(0, res_arg);
            // Allocate an array of `storage` large enough for a header + `cap` instances of pointers.
            auto nblk = storage::min_nblk_for_nbkt(cap * max_load_factor_reciprocal);
            storage_allocator st_alloc(this->as_allocator());
            auto ptr = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
#ifdef ROCKET_DEBUG
            ::std::memset(static_cast<void*>(noadl::unfancy(ptr)), '*', sizeof(storage) * nblk);
#endif
            noadl::construct_at(noadl::unfancy(ptr), reinterpret_cast<void (*)(...)>(&storage_handle::do_drop_reference),
                                this->as_allocator(), nblk);
            auto ptr_old = this->m_ptr;
            if(ROCKET_UNEXPECT(ptr_old)) {
              try {
                // Copy or move elements into the new block.
                // Moving is only viable if the old and new allocators compare equal and the old block is owned exclusively.
                if((ptr_old->alloc == ptr->alloc) && ptr_old->nref.unique()) {
                  move_storage_helper<storage_pointer, hasher, allocator_type>()(ptr, this->as_hasher(), ptr_old,       0, cnt_one);
                  move_storage_helper<storage_pointer, hasher, allocator_type>()(ptr, this->as_hasher(), ptr_old, off_two, cnt_two);
                }
                else {
                  copy_storage_helper<storage_pointer, hasher, allocator_type>()(ptr, this->as_hasher(), ptr_old,       0, cnt_one);
                  copy_storage_helper<storage_pointer, hasher, allocator_type>()(ptr, this->as_hasher(), ptr_old, off_two, cnt_two);
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

        constexpr operator const storage_handle* () const noexcept
          {
            return this;
          }
        operator storage_handle* () noexcept
          {
            return this;
          }

        template<typename ykeyT> bool index_of(size_type& index, const ykeyT& ykey) const
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return false;
            }
            // Get table bounds.
            auto data = ptr->data;
            auto end = data + storage::max_nbkt_for_nblk(ptr->nblk);
            // Find the desired element using linear probing.
            auto origin = noadl::get_probing_origin(data, end, this->as_hasher()(ykey));
            auto bkt = noadl::linear_probe(data, origin, origin, end,
                                           [&](const bucket_type& rbkt) { return this->as_key_equal()(rbkt->first, ykey);  });
            if(!bkt) {
              // This can only happen if the load factor is 1.0 i.e. no bucket is empty in the table.
              ROCKET_ASSERT(max_load_factor_reciprocal == 1);
              return false;
            }
            if(!*bkt) {
              // The previous probing has stopped due to an empty bucket. No equivalent key has been found so far.
              return false;
            }
            ROCKET_ASSERT(data <= bkt);
            index = static_cast<size_type>(bkt - data);
            return true;
          }
        bucket_type* mut_buckets_unchecked() noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return nullptr;
            }
            ROCKET_ASSERT(this->unique());
            return ptr->data;
          }
        template<typename ykeyT, typename... paramsT>
            pair<bucket_type*, bool> keyed_emplace_unchecked(const ykeyT& ykey, paramsT&&... params)
          {
            ROCKET_ASSERT(this->unique());
            ROCKET_ASSERT(this->element_count() < this->capacity());
            auto ptr = this->m_ptr;
            ROCKET_ASSERT(ptr);
            // Get table bounds.
            auto data = ptr->data;
            auto end = data + storage::max_nbkt_for_nblk(ptr->nblk);
            // Find an empty bucket using linear probing.
            auto origin = noadl::get_probing_origin(data, end, this->as_hasher()(ykey));
            auto bkt = noadl::linear_probe(data, origin, origin, end,
                                           [&](const bucket_type& rbkt) { return this->as_key_equal()(rbkt->first, ykey);  });
            ROCKET_ASSERT(bkt);
            if(*bkt) {
              // A duplicate key has been found.
              return ::std::make_pair(bkt, false);
            }
            // Allocate a new element.
            auto eptr = allocator_traits<allocator_type>::allocate(ptr->alloc, size_t(1));
            try {
              allocator_traits<allocator_type>::construct(ptr->alloc, noadl::unfancy(eptr), noadl::forward<paramsT>(params)...);
            }
            catch(...) {
              allocator_traits<allocT>::deallocate(ptr->alloc, eptr, size_t(1));
              throw;
            }
            // Insert it into the new bucket.
            eptr = bkt->reset(eptr);
            ROCKET_ASSERT(!eptr);
            ptr->nelem++;
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
            auto ptr = this->m_ptr;
            ROCKET_ASSERT(ptr);
            // Erase all elements in [tpos,tpos+tn).
            for(auto i = tpos; i != tpos + tn; ++i) {
              auto eptr = ptr->data[i].reset();
              if(!eptr) {
                continue;
              }
              ptr->nelem--;
              // Destroy the element and deallocate its storage.
              allocator_traits<allocator_type>::destroy(ptr->alloc, noadl::unfancy(eptr));
              allocator_traits<allocator_type>::deallocate(ptr->alloc, eptr, size_t(1));
            }
            // Get table bounds.
            auto data = ptr->data;
            auto end = data + storage::max_nbkt_for_nblk(ptr->nblk);
            // Relocate elements that are not placed in their immediate locations.
            noadl::linear_probe(
              // Only probe non-erased buckets.
              data, data + tpos, data + tpos + tn, end,
              // Relocate every bucket found.
              [&](bucket_type& rbkt)
                {
                  // Release the old element.
                  auto eptr = rbkt.reset();
                  // Find a new bucket for it using linear probing.
                  auto origin = noadl::get_probing_origin(data, end, this->as_hasher()(eptr->first));
                  auto bkt = noadl::linear_probe(data, origin, origin, end, [&](const auto&) { return false;  });
                  ROCKET_ASSERT(bkt);
                  // Insert it into the new bucket.
                  ROCKET_ASSERT(!*bkt);
                  bkt->reset(eptr);
                  return false;
                }
              );
          }

        void swap(storage_handle& other) noexcept
          {
            ::std::swap(this->m_ptr, other.m_ptr);
          }
      };

    // Informs the constructor of an iterator that the `bkt` parameter might point to an empty bucket.
    struct needs_adjust_tag
      {
      }
    constexpr needs_adjust;

    template<typename hashmapT, typename valueT> class hashmap_iterator
      {
        template<typename, typename> friend class hashmap_iterator;
        friend hashmapT;

      public:
        using iterator_category  = forward_iterator_tag;
        using value_type         = valueT;
        using pointer            = value_type*;
        using reference          = value_type&;
        using difference_type    = ptrdiff_t;

        using parent_type   = storage_handle<typename hashmapT::allocator_type,
                                             typename hashmapT::hasher, typename hashmapT::key_equal>;
        using bucket_type   = typename conditional<is_const<valueT>::value,
                                                   const typename parent_type::bucket_type,
                                                   typename parent_type::bucket_type>::type;

      private:
        const parent_type* m_ref;
        bucket_type* m_bkt;

      private:
        constexpr hashmap_iterator(const parent_type* ref, bucket_type* bkt) noexcept
          :
            m_ref(ref), m_bkt(bkt)
          {
          }
        hashmap_iterator(const parent_type* ref, needs_adjust_tag, bucket_type* hint) noexcept
          :
            m_ref(ref), m_bkt(this->do_adjust_forwards(hint))
          {
          }

      public:
        constexpr hashmap_iterator() noexcept
          :
            hashmap_iterator(nullptr, nullptr)
          {
          }
        template<typename yvalueT, ROCKET_ENABLE_IF(is_convertible<yvalueT*, valueT*>::value)>
            constexpr hashmap_iterator(const hashmap_iterator<hashmapT, yvalueT>& other) noexcept
          :
            hashmap_iterator(other.m_ref, other.m_bkt)
          {
          }

      private:
        bucket_type* do_assert_valid_bucket(bucket_type* bkt, bool to_dereference) const noexcept
          {
            auto ref = this->m_ref;
            ROCKET_ASSERT_MSG(ref, "This iterator has not been initialized.");
            auto dist = static_cast<size_t>(bkt - ref->buckets());
            ROCKET_ASSERT_MSG(dist <= ref->bucket_count(), "This iterator has been invalidated.");
            ROCKET_ASSERT_MSG(!((dist < ref->bucket_count()) && !*bkt),
                              "The element referenced by this iterator no longer exists.");
            ROCKET_ASSERT_MSG(!(to_dereference && (dist == ref->bucket_count())),
                              "This iterator contains a past-the-end value and cannot be dereferenced.");
            return bkt;
          }
        bucket_type* do_adjust_forwards(bucket_type* hint) const noexcept
          {
            if(hint == nullptr) {
              return nullptr;
            }
            auto ref = this->m_ref;
            ROCKET_ASSERT_MSG(ref, "This iterator has not been initialized.");
            auto end = ref->buckets() + ref->bucket_count();
            auto bkt = hint;
            while((bkt != end) && !*bkt) {
              ++bkt;
            }
            return bkt;
          }

      public:
        const parent_type* parent() const noexcept
          {
            return this->m_ref;
          }

        bucket_type* tell() const noexcept
          {
            auto bkt = this->do_assert_valid_bucket(this->m_bkt, false);
            return bkt;
          }
        bucket_type* tell_owned_by(const parent_type* ref) const noexcept
          {
            ROCKET_ASSERT_MSG(this->m_ref == ref, "This iterator does not refer to an element in the same container.");
            return this->tell();
          }
        hashmap_iterator& seek_next() noexcept
          {
            auto bkt = this->do_assert_valid_bucket(this->m_bkt, false);
            ROCKET_ASSERT_MSG(bkt != this->m_ref->buckets() + this->m_ref->bucket_count(),
                              "The past-the-end iterator cannot be incremented.");
            bkt = this->do_adjust_forwards(bkt + 1);
            this->m_bkt = this->do_assert_valid_bucket(bkt, false);
            return *this;
          }

        reference operator*() const noexcept
          {
            auto bkt = this->do_assert_valid_bucket(this->m_bkt, true);
            ROCKET_ASSERT(*bkt);
            return **bkt;
          }
        pointer operator->() const noexcept
          {
            auto bkt = this->do_assert_valid_bucket(this->m_bkt, true);
            ROCKET_ASSERT(*bkt);
            return ::std::addressof(**bkt);
          }
      };

    template<typename hashmapT, typename valueT>
        inline hashmap_iterator<hashmapT, valueT>& operator++(hashmap_iterator<hashmapT, valueT>& rhs) noexcept
      {
        return rhs.seek_next();
      }

    template<typename hashmapT, typename valueT>
        inline hashmap_iterator<hashmapT, valueT> operator++(hashmap_iterator<hashmapT, valueT>& lhs, int) noexcept
      {
        auto res = lhs;
        lhs.seek_next();
        return res;
      }

    template<typename hashmapT, typename xvalueT, typename yvalueT>
        inline bool operator==(const hashmap_iterator<hashmapT, xvalueT>& lhs, const hashmap_iterator<hashmapT, yvalueT>& rhs) noexcept
      {
        return lhs.tell() == rhs.tell();
      }
    template<typename hashmapT, typename xvalueT, typename yvalueT>
        inline bool operator!=(const hashmap_iterator<hashmapT, xvalueT>& lhs, const hashmap_iterator<hashmapT, yvalueT>& rhs) noexcept
      {
        return lhs.tell() != rhs.tell();
      }

    }  // namespace details_cow_hashmap

template<typename keyT, typename mappedT, typename hashT, typename eqT, typename allocT>
    class cow_hashmap
  {
    static_assert(!is_array<keyT>::value, "`keyT` must not be an array type.");
    static_assert(!is_array<mappedT>::value, "`mappedT` must not be an array type.");
    static_assert(is_same<typename allocT::value_type, pair<const keyT, mappedT>>::value,
                  "`allocT::value_type` must denote the same type as `pair<const keyT, mappedT>`.");
    static_assert(noexcept(::std::declval<const hashT&>()(::std::declval<const keyT&>())),
                  "The hash operation shall not throw exceptions.");

  public:
    // types
    using key_type        = keyT;
    using mapped_type     = mappedT;
    using value_type      = pair<const key_type, mapped_type>;
    using hasher          = hashT;
    using key_equal       = eqT;
    using allocator_type  = allocT;

    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using difference_type  = typename allocator_traits<allocator_type>::difference_type;
    using const_reference  = const value_type&;
    using reference        = value_type&;

    using const_iterator  = details_cow_hashmap::hashmap_iterator<cow_hashmap, const value_type>;
    using iterator        = details_cow_hashmap::hashmap_iterator<cow_hashmap, value_type>;

  private:
    details_cow_hashmap::storage_handle<allocator_type, hasher, key_equal> m_sth;

  public:
    // 26.5.4.2, construct/copy/destroy
    explicit constexpr cow_hashmap(const allocator_type& alloc)
        noexcept(conjunction<is_nothrow_constructible<hasher>, is_nothrow_copy_constructible<hasher>,
                             is_nothrow_constructible<key_equal>, is_nothrow_copy_constructible<key_equal>>::value)
      :
        m_sth(alloc, hasher(), key_equal())
      {
      }
    constexpr cow_hashmap(clear_t = clear_t())
        noexcept(conjunction<is_nothrow_constructible<hasher>, is_nothrow_copy_constructible<hasher>,
                             is_nothrow_constructible<key_equal>, is_nothrow_copy_constructible<key_equal>,
                             is_nothrow_constructible<allocator_type>>::value)
      :
        cow_hashmap(allocator_type())
      {
      }
    explicit cow_hashmap(size_type res_arg, const hasher& hf = hasher(), const key_equal& eq = key_equal(),
                         const allocator_type& alloc = allocator_type())
      :
        m_sth(alloc, hf, eq)
      {
        this->m_sth.reallocate(0, 0, 0, res_arg);
      }
    cow_hashmap(size_type res_arg, const allocator_type& alloc)
      :
        cow_hashmap(res_arg, hasher(), key_equal(), alloc)
      {
      }
    cow_hashmap(size_type res_arg, const hasher& hf, const allocator_type& alloc)
      :
        cow_hashmap(res_arg, hf, key_equal(), alloc)
      {
      }
    cow_hashmap(const cow_hashmap& other) noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                                                               is_nothrow_copy_constructible<key_equal>>::value)
      :
        cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(),
                    allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
      {
        this->assign(other);
      }
    cow_hashmap(const cow_hashmap& other, const allocator_type& alloc) noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                                                                                            is_nothrow_copy_constructible<key_equal>>::value)
      :
        cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), alloc)
      {
        this->assign(other);
      }
    cow_hashmap(cow_hashmap&& other) noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                                                          is_nothrow_copy_constructible<key_equal>>::value)
      :
        cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), noadl::move(other.m_sth.as_allocator()))
      {
        this->assign(noadl::move(other));
      }
    cow_hashmap(cow_hashmap&& other, const allocator_type& alloc) noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                                                                                       is_nothrow_copy_constructible<key_equal>>::value)
      :
        cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), alloc)
      {
        this->assign(noadl::move(other));
      }
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)>
        cow_hashmap(inputT first, inputT last, size_type res_arg = 0,
                    const hasher& hf = hasher(), const key_equal& eq = key_equal(), const allocator_type& alloc = allocator_type())
      :
        cow_hashmap(res_arg, hf, eq, alloc)
      {
        this->assign(noadl::move(first), noadl::move(last));
      }
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)>
        cow_hashmap(inputT first, inputT last, size_type res_arg, const hasher& hf, const allocator_type& alloc)
      :
        cow_hashmap(res_arg, hf, key_equal(), alloc)
      {
        this->assign(noadl::move(first), noadl::move(last));
      }
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)>
        cow_hashmap(inputT first, inputT last, size_type res_arg, const allocator_type& alloc)
      :
        cow_hashmap(res_arg, hasher(), key_equal(), alloc)
      {
        this->assign(noadl::move(first), noadl::move(last));
      }
    cow_hashmap(initializer_list<value_type> init, size_type res_arg = 0,
                const hasher& hf = hasher(), const key_equal& eq = key_equal(), const allocator_type& alloc = allocator_type())
      :
        cow_hashmap(res_arg, hf, eq, alloc)
      {
        this->assign(init);
      }
    cow_hashmap(initializer_list<value_type> init, size_type res_arg, const hasher& hf, const allocator_type& alloc)
      :
        cow_hashmap(res_arg, hf, key_equal(), alloc)
      {
        this->assign(init);
      }
    cow_hashmap(initializer_list<value_type> init, size_type res_arg, const allocator_type& alloc)
      :
        cow_hashmap(res_arg, hasher(), key_equal(), alloc)
      {
        this->assign(init);
      }
    cow_hashmap& operator=(clear_t) noexcept
      {
        this->clear();
        return *this;
      }
    cow_hashmap& operator=(const cow_hashmap& other) noexcept
      {
        this->assign(other);
        noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        return *this;
      }
    cow_hashmap& operator=(cow_hashmap&& other) noexcept
      {
        this->assign(noadl::move(other));
        noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), noadl::move(other.m_sth.as_allocator()));
        return *this;
      }
    cow_hashmap& operator=(initializer_list<value_type> init)
      {
        this->assign(init);
        return *this;
      }

  private:
    // Reallocate the storage to `res_arg` elements.
    // The storage is owned by the current hashmap exclusively after this function returns normally.
    details_cow_hashmap::bucket<allocator_type>* do_reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
      {
        ROCKET_ASSERT(cnt_one <= off_two);
        ROCKET_ASSERT(off_two <= this->m_sth.bucket_count());
        ROCKET_ASSERT(cnt_two <= this->m_sth.bucket_count() - off_two);
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
        this->m_sth.erase_range_unchecked(0, this->m_sth.bucket_count());
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
          this->do_reallocate(0, 0, this->bucket_count(), cap | 1);
        }
        ROCKET_ASSERT(this->capacity() >= cap);
      }

    [[noreturn]] ROCKET_NOINLINE void do_throw_key_not_found() const
      {
        noadl::sprintf_and_throw<out_of_range>("cow_hashmap: key not found");
      }

    const details_cow_hashmap::bucket<allocator_type>* do_get_table() const noexcept
      {
        return this->m_sth.buckets();
      }
    details_cow_hashmap::bucket<allocator_type>* do_mut_table()
      {
        if(!this->unique()) {
          this->do_reallocate(0, 0, this->bucket_count(), this->size() | 1);
        }
        return this->m_sth.mut_buckets_unchecked();
      }

    details_cow_hashmap::bucket<allocator_type>* do_erase_no_bound_check(size_type tpos, size_type tn)
      {
        auto cnt_old = this->size();
        auto nbkt_old = this->bucket_count();
        ROCKET_ASSERT(tpos <= nbkt_old);
        ROCKET_ASSERT(tn <= nbkt_old - tpos);
        if(!this->unique()) {
          auto ptr = this->do_reallocate(tpos, tpos + tn, nbkt_old - (tpos + tn), cnt_old);
          return ptr;
        }
        auto ptr = this->m_sth.mut_buckets_unchecked();
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

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator mut_begin()
      {
        return iterator(this->m_sth, details_cow_hashmap::needs_adjust, this->do_mut_table());
      }
    // N.B. This function may throw `std::bad_alloc`.
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
    // N.B. This is a non-standard extension.
    difference_type ssize() const noexcept
      {
        return static_cast<difference_type>(this->size());
      }
    size_type capacity() const noexcept
      {
        return this->m_sth.capacity();
      }
    // N.B. The return type is a non-standard extension.
    cow_hashmap& reserve(size_type res_arg)
      {
        auto cnt = this->size();
        auto cap_new = this->m_sth.round_up_capacity(noadl::max(cnt, res_arg));
        // If the storage is shared with other hashmaps, force rellocation to prevent copy-on-write upon modification.
        if(this->unique() && (this->capacity() >= cap_new)) {
          return *this;
        }
        this->do_reallocate(0, 0, this->bucket_count(), cap_new);
        ROCKET_ASSERT(this->capacity() >= res_arg);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    cow_hashmap& shrink_to_fit()
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
    cow_hashmap& clear() noexcept
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

    // hash policy
    // N.B. This is a non-standard extension.
    size_type bucket_count() const noexcept
      {
        return this->m_sth.bucket_count();
      }
    // N.B. The return type differs from `std::unordered_map`.
    double load_factor() const noexcept
      {
        return static_cast<double>(static_cast<difference_type>(this->size()))
               / static_cast<double>(static_cast<difference_type>(this->bucket_count()));
      }
    // N.B. The `constexpr` specifier is a non-standard extension.
    // N.B. The return type differs from `std::unordered_map`.
    constexpr double max_load_factor() const noexcept
      {
        return this->m_sth.max_load_factor();
      }
    void rehash(size_type n)
      {
        this->do_reallocate(0, 0, this->bucket_count(), noadl::max(this->size(), n));
      }

    // 26.5.4.4, modifiers
    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename yvalueT> pair<iterator, bool> insert(const pair<ykeyT, yvalueT>& value)
      {
        return this->try_emplace(value.first, value.second);
      }
    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename yvalueT> pair<iterator, bool> insert(pair<ykeyT, yvalueT>&& value)
      {
        return this->try_emplace(noadl::move(value.first), noadl::move(value.second));
      }
    // N.B. The return type is a non-standard extension.
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)>
        cow_hashmap& insert(inputT first, inputT last)
      {
        if(first == last) {
          return *this;
        }
        auto dist = noadl::estimate_distance(first, last);
        if(dist == 0) {
          noadl::ranged_do_while(noadl::move(first), noadl::move(last),
                                 [&](const inputT& it) { this->insert(*it);  });
          return *this;
        }
        this->do_reserve_more(dist);
        noadl::ranged_do_while(noadl::move(first), noadl::move(last),
                               [&](const inputT& it) { this->m_sth.keyed_emplace_unchecked(it->first, *it);  });
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    cow_hashmap& insert(initializer_list<value_type> init)
      {
        return this->insert(init.begin(), init.end());
      }
    // N.B. This is a non-standard extension.
    // N.B. The hint is ignored.
    template<typename ykeyT, typename yvalueT> iterator insert(const_iterator /*hint*/, const pair<ykeyT, yvalueT>& value)
      {
        return this->insert(value).first;
      }
    // N.B. This is a non-standard extension.
    // N.B. The hint is ignored.
    template<typename ykeyT, typename yvalueT> iterator insert(const_iterator /*hint*/, pair<ykeyT, yvalueT>&& value)
      {
        return this->insert(noadl::move(value)).first;
      }

    template<typename ykeyT, typename... paramsT> pair<iterator, bool> try_emplace(ykeyT&& key, paramsT&&... params)
      {
        this->do_reserve_more(1);
        auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::piecewise_construct,
                                                               ::std::forward_as_tuple(noadl::forward<ykeyT>(key)),
                                                               ::std::forward_as_tuple(noadl::forward<paramsT>(params)...));
        return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
      }
    // N.B. The hint is ignored.
    template<typename ykeyT, typename... paramsT> iterator try_emplace(const_iterator /*hint*/, ykeyT&& key, paramsT&&... params)
      {
        return this->try_emplace(noadl::forward<ykeyT>(key), noadl::forward<paramsT>(params)...).first;
      }

    template<typename ykeyT, typename yvalueT> pair<iterator, bool> insert_or_assign(ykeyT&& key, yvalueT&& yvalue)
      {
        this->do_reserve_more(1);
        auto result = this->m_sth.keyed_emplace_unchecked(key, noadl::forward<ykeyT>(key), noadl::forward<yvalueT>(yvalue));
        if(!result.second) {
          result.first->get()->second = noadl::forward<yvalueT>(yvalue);
        }
        return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
      }
    // N.B. The hint is ignored.
    template<typename ykeyT, typename yvalueT> iterator insert_or_assign(const_iterator /*hint*/, ykeyT&& key, yvalueT&& yvalue)
      {
        return this->insert_or_assign(noadl::forward<ykeyT>(key), noadl::forward<yvalueT>(yvalue)).first;
      }

    // N.B. This function may throw `std::bad_alloc`.
    iterator erase(const_iterator tfirst, const_iterator tlast)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->do_get_table());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this->m_sth) - tfirst.tell());
        auto ptr = this->do_erase_no_bound_check(tpos, tn);
        return iterator(this->m_sth, details_cow_hashmap::needs_adjust, ptr);
      }
    // N.B. This function may throw `std::bad_alloc`.
    iterator erase(const_iterator tfirst)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->do_get_table());
        auto ptr = this->do_erase_no_bound_check(tpos, 1);
        return iterator(this->m_sth, details_cow_hashmap::needs_adjust, ptr);
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. The return type differs from `std::unordered_map`.
    template<typename ykeyT, ROCKET_DISABLE_IF(is_convertible<ykeyT, const_iterator>::value)> bool erase(const ykeyT& key)
      {
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key)) {
          return false;
        }
        this->do_erase_no_bound_check(tpos, 1);
        return true;
      }

    // map operations
    template<typename ykeyT> const_iterator find(const ykeyT& key) const
      {
        auto ptr = this->do_get_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key)) {
          return this->end();
        }
        return const_iterator(this->m_sth, ptr + tpos);
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    template<typename ykeyT> iterator find_mut(const ykeyT& key)
      {
        auto ptr = this->do_mut_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key)) {
          return this->mut_end();
        }
        return iterator(this->m_sth, ptr + tpos);
      }
    template<typename ykeyT> size_t count(const ykeyT& key) const
      {
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key)) {
          return 0;
        }
        return 1;
      }
    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename ydefaultT>
        typename common_type<const mapped_type&, ydefaultT&&>::type get_or(const ykeyT& key, ydefaultT&& ydef) const
      {
        auto ptr = this->do_get_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key)) {
          return noadl::forward<ydefaultT>(ydef);
        }
        return ptr[tpos]->second;
      }

    // 26.5.4.3, element access
    template<typename ykeyT> const mapped_type& at(const ykeyT& key) const
      {
        auto ptr = this->do_get_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key)) {
          this->do_throw_key_not_found();
        }
        return ptr[tpos]->second;
      }
    // N.B. This is a non-standard extension.
    template<typename ykeyT> const mapped_type* get_ptr(const ykeyT& key) const
      {
        auto ptr = this->do_get_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key)) {
          return nullptr;
        }
        return ::std::addressof(ptr[tpos]->second);
      }

    template<typename ykeyT> mapped_type& operator[](ykeyT&& key)
      {
        this->do_reserve_more(1);
        auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::piecewise_construct,
                                                               ::std::forward_as_tuple(noadl::forward<ykeyT>(key)),
                                                               ::std::forward_as_tuple());
        return result.first->get()->second;
      }
    // N.B. This is a non-standard extension.
    template<typename ykeyT> mapped_type& mut(const ykeyT& key)
      {
        auto ptr = this->do_mut_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key)) {
          this->do_throw_key_not_found();
        }
        return ptr[tpos]->second;
      }
    // N.B. This is a non-standard extension.
    template<typename ykeyT> mapped_type* mut_ptr(const ykeyT& key)
      {
        auto ptr = this->do_mut_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key)) {
          return nullptr;
        }
        return ::std::addressof(ptr[tpos]->second);
      }

    // N.B. This function is a non-standard extension.
    cow_hashmap& assign(const cow_hashmap& other) noexcept
      {
        this->m_sth.share_with(other.m_sth);
        return *this;
      }
    // N.B. This function is a non-standard extension.
    cow_hashmap& assign(cow_hashmap&& other) noexcept
      {
        this->m_sth.share_with(noadl::move(other.m_sth));
        return *this;
      }
    // N.B. This function is a non-standard extension.
    cow_hashmap& assign(initializer_list<value_type> init)
      {
        this->clear();
        this->insert(init);
        return *this;
      }
    // N.B. This function is a non-standard extension.
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)>
        cow_hashmap& assign(inputT first, inputT last)
      {
        this->clear();
        this->insert(noadl::move(first), noadl::move(last));
        return *this;
      }

    void swap(cow_hashmap& other) noexcept
      {
        this->m_sth.swap(other.m_sth);
        noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
      }

    // N.B. The return type differs from `std::unordered_map`.
    const allocator_type& get_allocator() const noexcept
      {
        return this->m_sth.as_allocator();
      }
    allocator_type& get_allocator() noexcept
      {
        return this->m_sth.as_allocator();
      }
    // N.B. The return type differs from `std::unordered_map`.
    const hasher& hash_function() const noexcept
      {
        return this->m_sth.as_hasher();
      }
    hasher& hash_function() noexcept
      {
        return this->m_sth.as_hasher();
      }
    // N.B. The return type differs from `std::unordered_map`.
    const key_equal& key_eq() const noexcept
      {
        return this->m_sth.as_key_equal();
      }
    key_equal& key_eq() noexcept
      {
        return this->m_sth.as_key_equal();
      }
  };

template<typename keyT, typename mappedT, typename hashT, typename eqT, typename allocT>
    void swap(cow_hashmap<keyT, mappedT, hashT, eqT, allocT>& lhs, cow_hashmap<keyT, mappedT, hashT, eqT, allocT>& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace rocket

#endif
