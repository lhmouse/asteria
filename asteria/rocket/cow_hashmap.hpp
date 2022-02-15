// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_HASHMAP_HPP_
#define ROCKET_COW_HASHMAP_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include "throw.hpp"
#include "reference_counter.hpp"
#include "xallocator.hpp"
#include "xhashtable.hpp"
#include <tuple>  // std::forward_as_tuple()
#include <cstdio>  // std::sprintf()

namespace rocket {

template<typename keyT, typename mappedT,
         typename hashT = hash<keyT>,
         typename eqT = equal_to<void>,
         typename allocT = allocator<pair<const keyT, mappedT>>>
class cow_hashmap;

#include "details/cow_hashmap.ipp"

/* Differences from `std::unordered_map`:
 * 1. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always
 *    return `const_reference`s.
 * 2. Iterators are bidirectional iterators, not just forward iterators.
 * 3. The copy constructor and copy assignment operator will not throw exceptions.
 * 4. Comparison operators are not provided.
 * 5. `emplace()` and `emplace_hint()` functions are not provided. `try_emplace()` is recommended
 *    as an alternative.
 * 6. There are no buckets. Bucket lookups and local iterators are not provided. Multimap cannot
 *    be implemented.
 * 7. The key and mapped types may be incomplete. The mapped type need be neither copy-assignable
 *    nor move-assignable.
 * 8. `erase()` may move elements around and invalidate iterators.
 * 9. `operator[]()` is not provided.
**/

template<typename keyT, typename mappedT, typename hashT, typename eqT, typename allocT>
class cow_hashmap
  {
    static_assert(!is_array<keyT>::value, "invalid key type");
    static_assert(!is_array<mappedT>::value, "invalid mapped value type");
    static_assert(is_same<typename allocT::value_type, pair<const keyT, mappedT>>::value,
                  "inappropriate allocator type");
    static_assert(noexcept(::std::declval<const hashT&>()(::std::declval<const keyT&>())),
                  "hash operations must not throw exceptions");

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

    using const_iterator     = details_cow_hashmap::hashmap_iterator<cow_hashmap, const value_type>;
    using iterator           = details_cow_hashmap::hashmap_iterator<cow_hashmap, value_type>;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

  private:
    using storage_handle = details_cow_hashmap::storage_handle<allocator_type, hasher, key_equal>;
    using bucket_type    = typename storage_handle::bucket_type;

  private:
    storage_handle m_sth;

  public:
    // 26.5.4.2, construct/copy/destroy
    explicit constexpr
    cow_hashmap(const allocator_type& alloc, const hasher& hf = hasher(),
                const key_equal& eq = key_equal())
      noexcept(conjunction<is_nothrow_constructible<hasher>,
                           is_nothrow_copy_constructible<hasher>,
                           is_nothrow_constructible<key_equal>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(alloc, hf, eq)
      { }

    cow_hashmap(const cow_hashmap& other)
      noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(allocator_traits<allocator_type>::select_on_container_copy_construction(
                                                   other.m_sth.as_allocator()),
              other.m_sth.as_hasher(), other.m_sth.as_key_equal())
      { this->m_sth.share_with(other.m_sth);  }

    cow_hashmap(const cow_hashmap& other, const allocator_type& alloc)
      noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(alloc, other.m_sth.as_hasher(), other.m_sth.as_key_equal())
      { this->m_sth.share_with(other.m_sth);  }

    cow_hashmap(cow_hashmap&& other)
      noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(::std::move(other.m_sth.as_allocator()), other.m_sth.as_hasher(),
              other.m_sth.as_key_equal())
      { this->m_sth.exchange_with(other.m_sth);  }

    cow_hashmap(cow_hashmap&& other, const allocator_type& alloc)
      noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(alloc, other.m_sth.as_hasher(), other.m_sth.as_key_equal())
      { this->m_sth.exchange_with(other.m_sth);  }

    constexpr
    cow_hashmap()
      noexcept(conjunction<is_nothrow_constructible<allocator_type>,
                           is_nothrow_constructible<hasher>,
                           is_nothrow_copy_constructible<hasher>,
                           is_nothrow_constructible<key_equal>,
                           is_nothrow_copy_constructible<key_equal>>::value)
      : cow_hashmap(allocator_type())
      { }

    explicit
    cow_hashmap(size_type res_arg, const hasher& hf = hasher(), const key_equal& eq = key_equal(),
                const allocator_type& alloc = allocator_type())
      : cow_hashmap(alloc, hf, eq)
      { this->reserve(res_arg);  }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_hashmap(inputT first, inputT last, size_type res_arg = 0, const hasher& hf = hasher(),
                const key_equal& eq = key_equal(), const allocator_type& alloc = allocator_type())
      : cow_hashmap(res_arg, hf, eq, alloc)
      { this->assign(::std::move(first), ::std::move(last));  }

    cow_hashmap(initializer_list<value_type> init, size_type res_arg = 0,
                const hasher& hf = hasher(), const key_equal& eq = key_equal(),
                const allocator_type& alloc = allocator_type())
      : cow_hashmap(res_arg, hf, eq, alloc)
      { this->assign(init.begin(), init.end());  }

    cow_hashmap(size_type res_arg, const hasher& hf, const allocator_type& alloc)
      : cow_hashmap(res_arg, hf, key_equal(), alloc)
      { }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_hashmap(inputT first, inputT last, size_type res_arg, const hasher& hf,
                const allocator_type& alloc)
      : cow_hashmap(::std::move(first), ::std::move(last), res_arg, hf, key_equal(), alloc)
      { }

    cow_hashmap(initializer_list<value_type> init, size_type res_arg, const hasher& hf,
                const allocator_type& alloc)
      : cow_hashmap(init, res_arg, hf, key_equal(), alloc)
      { }

    cow_hashmap(size_type res_arg, const allocator_type& alloc)
      : cow_hashmap(res_arg, hasher(), key_equal(), alloc)
      { }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_hashmap(inputT first, inputT last, size_type res_arg, const allocator_type& alloc)
      : cow_hashmap(::std::move(first), ::std::move(last), res_arg, hasher(), key_equal(), alloc)
      { }

    cow_hashmap(initializer_list<value_type> init, size_type res_arg, const allocator_type& alloc)
      : cow_hashmap(init, res_arg, hasher(), key_equal(), alloc)
      { }

    cow_hashmap&
    operator=(const cow_hashmap& other) noexcept
      { noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.share_with(other.m_sth);
        return *this;  }

    cow_hashmap&
    operator=(cow_hashmap&& other) noexcept
      { noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        return *this;  }

    cow_hashmap&
    operator=(initializer_list<value_type> init)
      { return this->assign(init.begin(), init.end());  }

    cow_hashmap&
    swap(cow_hashmap& other) noexcept
      { noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        return *this;  }

  private:
    cow_hashmap&
    do_deallocate() noexcept
      {
        this->m_sth.deallocate();
        return *this;
      }

    [[noreturn]] ROCKET_NEVER_INLINE void
    do_throw_key_not_found(const details_cow_hashmap::stringified_key& skey) const
      {
        noadl::sprintf_and_throw<out_of_range>(
              "cow_hashmap: key not found (key `%s`)",
              skey.c_str());
      }

    const bucket_type*
    do_buckets() const noexcept
      { return this->m_sth.buckets();  }

    bucket_type*
    do_mut_buckets()
      {
        auto bkts = this->m_sth.mut_buckets_opt();
        if(ROCKET_EXPECT(bkts))
          return bkts;

        // If the hashmap is empty, return a pointer to constant storage.
        if(this->empty())
          return const_cast<bucket_type*>(this->do_buckets());

        // Reallocate the storage. The length is left intact.
        // Note that this function shall preserve indices of buckets of cloned elements
        // in the new table.
        bkts = this->m_sth.reallocate_clone(this->m_sth);
        return bkts;
      }

    // This function is used to implement `erase()`.
    bucket_type*
    do_erase_unchecked(size_type tpos, size_type tlen)
      {
        // Get a pointer to mutable storage.
        auto bkts = this->do_mut_buckets();

        // Purge unwanted elements.
        this->m_sth.erase_range_unchecked(tpos, tlen);

        // Return a pointer next to erased elements.
        return bkts + tpos;
      }

  public:
    // iterators
    const_iterator
    begin() const noexcept
      { return const_iterator(this->do_buckets(), 0, this->bucket_count());  }

    const_iterator
    end() const noexcept
      { return const_iterator(this->do_buckets(), this->bucket_count(), this->bucket_count());  }

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
      { return iterator(this->do_mut_buckets(), 0, this->bucket_count());  }

    // N.B. This is a non-standard extension.
    // N.B. This function may throw `std::bad_alloc`.
    iterator
    mut_end()
      { return iterator(this->do_mut_buckets(), this->bucket_count(), this->bucket_count());  }

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

    // capacity
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

    size_type
    capacity() const noexcept
      { return this->m_sth.capacity();  }

    // N.B. The return type is a non-standard extension.
    cow_hashmap&
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
        storage_handle sth(this->m_sth.as_allocator(), this->m_sth.as_hasher(),
                           this->m_sth.as_key_equal());
        sth.reallocate_reserve(this->m_sth, true, rcap - this->size());

        // Set the new storage up. The size is left intact.
        this->m_sth.exchange_with(sth);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    cow_hashmap&
    shrink_to_fit()
      {
        // If the hashmap is empty, deallocate any dynamic storage.
        if(this->empty())
          return this->do_deallocate();

        // Calculate the minimum capacity to reserve. This must include all existent elements.
        // Don't reallocate if the storage is shared or tight.
        size_type rcap = this->m_sth.round_up_capacity(this->size());
        if(!this->unique() || (this->capacity() <= rcap))
          return *this;

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator(), this->m_sth.as_hasher(),
                           this->m_sth.as_key_equal());
        sth.reallocate_reserve(this->m_sth, true, 0);

        // Set the new storage up. The size is left intact.
        this->m_sth.exchange_with(sth);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    cow_hashmap&
    clear() noexcept
      {
        // If storage is shared, detach it.
        if(!this->m_sth.unique())
          return this->do_deallocate();

        this->m_sth.erase_range_unchecked(0, this->size());
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

    // hash policy
    // N.B. This is a non-standard extension.
    size_type
    bucket_count() const noexcept
      { return this->m_sth.bucket_count();  }

    // N.B. The return type differs from `std::unordered_map`.
    double
    load_factor() const noexcept
      { return static_cast<double>(this->ssize()) /
                   static_cast<double>(static_cast<difference_type>(this->bucket_count()));  }

    // N.B. The return type differs from `std::unordered_map`.
    double
    max_load_factor() const noexcept
      { return 1.0 / storage_handle::max_load_factor_reciprocal;  }

    // N.B. The return type is a non-standard extension.
    cow_hashmap&
    rehash(size_type n)
      {
        // Calculate the minimum bucket count to reserve. This must include all existent elements.
        // Don't reallocate if the storage is unique and there is enough room.
        size_type rcap = this->m_sth.round_up_capacity(noadl::max(
                             this->size(), n / storage_handle::max_load_factor_reciprocal));
        if(this->capacity() == rcap)
          return *this;

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator(), this->m_sth.as_hasher(),
                           this->m_sth.as_key_equal());
        sth.reallocate_reserve(this->m_sth, true, rcap - this->size());

        // Set the new storage up. The size is left intact.
        this->m_sth.exchange_with(sth);
        return *this;
      }

    // 26.5.4.4, modifiers
    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename ymappedT>
    pair<iterator, bool>
    insert(const pair<ykeyT, ymappedT>& value)
      { return this->try_emplace(value.first, value.second);  }

    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename ymappedT>
    pair<iterator, bool>
    insert(pair<ykeyT, ymappedT>&& value)
      { return this->try_emplace(::std::move(value.first), ::std::move(value.second));  }

    // N.B. The return type is a non-standard extension.
    cow_hashmap&
    insert(initializer_list<value_type> init)
      { return this->insert(init.begin(), init.end());  }

    // N.B. The return type is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_hashmap&
    insert(inputT first, inputT last)
      {
        if(first == last)
          return *this;

        size_t dist = noadl::estimate_distance(first, last);
        size_type n = static_cast<size_type>(dist);

        // Check whether the storage is unique and there is enough space.
        auto bkts = this->m_sth.mut_buckets_opt();
        size_type cap = this->capacity();
        size_type tpos;
        if(ROCKET_EXPECT(dist && bkts && (dist <= cap - this->size()))) {
          // Insert new elements in place.
          for(auto it = ::std::move(first);  it != last;  ++it)
            this->m_sth.keyed_try_emplace(tpos, it->first, it->first, it->second);

          // The return type aligns with `std::string::append()`.
          return *this;
        }

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator(), this->m_sth.as_hasher(),
                           this->m_sth.as_key_equal());
        if(ROCKET_EXPECT(n && (n == dist))) {
          // The length is known.
          bkts = sth.reallocate_reserve(this->m_sth, false, n | cap / 2);

          // Insert new elements into the new storage.
          for(auto it = ::std::move(first);  it != last;  ++it)
            if(!this->m_sth.find(tpos, it->first))
              sth.keyed_try_emplace(tpos, it->first, it->first, it->second);
        }
        else {
          // The length is not known.
          bkts = sth.reallocate_reserve(this->m_sth, false, 17 | cap / 2);
          cap = sth.capacity();

          // Insert new elements into the new storage.
          for(auto it = ::std::move(first);  it != last;  ++it) {
            // Reallocate the storage if necessary.
            if(ROCKET_UNEXPECT(sth.size() >= cap)) {
              bkts = sth.reallocate_reserve(sth, this->size(), cap / 2);
              cap = sth.capacity();
            }
            if(!this->m_sth.find(tpos, it->first))
              sth.keyed_try_emplace(tpos, it->first, it->first, it->second);
          }
        }
        sth.reallocate_finish(this->m_sth);

        // Set the new storage up.
        this->m_sth.exchange_with(sth);
        return *this;
      }

    // N.B. This is a non-standard extension.
    // N.B. The hint is ignored.
    template<typename ykeyT, typename ymappedT>
    iterator
    insert(const_iterator /*hint*/, const pair<ykeyT, ymappedT>& value)
      { return this->insert(value).first;  }

    // N.B. This is a non-standard extension.
    // N.B. The hint is ignored.
    template<typename ykeyT, typename ymappedT>
    iterator
    insert(const_iterator /*hint*/, pair<ykeyT, ymappedT>&& value)
      { return this->insert(::std::move(value)).first;  }

    template<typename ykeyT, typename... paramsT>
    pair<iterator, bool>
    try_emplace(ykeyT&& ykey, paramsT&&... params)
      {
        // Check whether the storage is unique and there is enough space.
        auto bkts = this->m_sth.mut_buckets_opt();
        size_type cap = this->capacity();
        size_type tpos;
        if(ROCKET_EXPECT(bkts && (this->size() < cap))) {
          // Insert the new element in place.
          bool inserted = this->m_sth.keyed_try_emplace(tpos, ykey,
                    ::std::piecewise_construct,
                    ::std::forward_as_tuple(::std::forward<ykeyT>(ykey)),
                    ::std::forward_as_tuple(::std::forward<paramsT>(params)...));

          // The return type aligns with `std::unordered_map::try_emplace()`.
          return { iterator(bkts, tpos, this->bucket_count()), inserted };
        }

        // Check for equivalent keys before reallocation.
        // If one is found, `tpos` will point at that bucket, so we can return an iterator to it.
        if(this->m_sth.find(tpos, ykey))
          return { iterator(this->do_mut_buckets(), tpos, this->bucket_count()), false };

        // Allocate new storage.
        storage_handle sth(this->m_sth.as_allocator(), this->m_sth.as_hasher(),
                           this->m_sth.as_key_equal());
        bkts = sth.reallocate_reserve(this->m_sth, false, 17 | cap / 2);

        // Insert the new element into the new storage.
        sth.keyed_try_emplace(tpos, ykey,
                    ::std::piecewise_construct,
                    ::std::forward_as_tuple(::std::forward<ykeyT>(ykey)),
                    ::std::forward_as_tuple(::std::forward<paramsT>(params)...));
        sth.reallocate_finish(this->m_sth);

        // Set the new storage up.
        this->m_sth.exchange_with(sth);
        return { iterator(bkts, tpos, this->bucket_count()), true };
      }

    // N.B. The hint is ignored.
    template<typename ykeyT, typename... paramsT>
    iterator
    try_emplace(const_iterator /*hint*/, ykeyT&& ykey, paramsT&&... params)
      { return this->try_emplace(::std::forward<ykeyT>(ykey),
                                 ::std::forward<paramsT>(params)...).first;  }

    template<typename ykeyT, typename ymappedT>
    pair<iterator, bool>
    insert_or_assign(ykeyT&& ykey, ymappedT&& ymapped)
      {
        auto r = this->try_emplace(::std::forward<ykeyT>(ykey), ::std::forward<ymappedT>(ymapped));
        if(!r.second)
          r.first->second = ::std::forward<ymappedT>(ymapped);
        return r;
      }

    // N.B. The hint is ignored.
    template<typename ykeyT, typename ymappedT>
    iterator
    insert_or_assign(const_iterator /*hint*/, ykeyT&& ykey, ymappedT&& ymapped)
      { return this->insert_or_assign(::std::forward<ykeyT>(ykey),
                                      ::std::forward<ymappedT>(ymapped)).first;  }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. The return type differs from `std::unordered_map`.
    template<typename ykeyT,
    ROCKET_DISABLE_IF(is_convertible<ykeyT, const_iterator>::value)>
    bool
    erase(const ykeyT& ykey)
      {
        size_type tpos;
        if(!this->m_sth.find(tpos, ykey))
          return false;

        this->do_erase_unchecked(tpos, 1);
        return true;
      }

    // N.B. This function may throw `std::bad_alloc`.
    iterator
    erase(const_iterator first, const_iterator last)
      {
        ROCKET_ASSERT_MSG(first.m_cur <= last.m_cur, "invalid range");
        size_type tpos = static_cast<size_type>(first.do_this_pos(this->do_buckets()));
        size_type tlen = static_cast<size_type>(last.do_this_len(first));

        auto bkt = this->do_erase_unchecked(tpos, tlen);
        return iterator(bkt - tpos, tpos, this->bucket_count());
      }

    // N.B. This function may throw `std::bad_alloc`.
    iterator
    erase(const_iterator pos)
      {
        size_type tpos = static_cast<size_type>(pos.do_this_pos(this->do_buckets()));

        auto bkt = this->do_erase_unchecked(tpos, 1);
        return iterator(bkt - tpos, tpos, this->bucket_count());
      }

    // map operations
    template<typename ykeyT>
    const_iterator
    find(const ykeyT& ykey) const
      {
        size_type tpos;
        if(!this->m_sth.find(tpos, ykey))
          return this->end();
        return const_iterator(this->do_buckets(), tpos, this->bucket_count());
      }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    template<typename ykeyT>
    iterator
    mut_find(const ykeyT& ykey)
      {
        size_type tpos;
        if(!this->m_sth.find(tpos, ykey))
          return this->mut_end();
        return iterator(this->do_mut_buckets(), tpos, this->bucket_count());
      }

    // N.B. The return type differs from `std::unordered_map`.
    template<typename ykeyT>
    bool
    count(const ykeyT& ykey) const
      {
        size_type tpos;
        return this->m_sth.find(tpos, ykey);
      }

    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename ydefaultT>
    typename select_type<const mapped_type&, ydefaultT&&>::type
    get_or(const ykeyT& ykey, ydefaultT&& ydef) const
      {
        size_type tpos;
        if(!this->m_sth.find(tpos, ykey))
          return ::std::forward<ydefaultT>(ydef);
        return this->do_buckets()[tpos]->second;
      }

    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename ydefaultT>
    typename select_type<mapped_type&&, ydefaultT&&>::type
    move_or(const ykeyT& ykey, ydefaultT&& ydef) const
      {
        size_type tpos;
        if(!this->m_sth.find(tpos, ykey))
          return ::std::forward<ydefaultT>(ydef);
        return ::std::move(this->do_mut_buckets()[tpos]->second);
      }

    // 26.5.4.3, element access
    template<typename ykeyT>
    const mapped_type&
    at(const ykeyT& ykey) const
      {
        size_type tpos;
        if(!this->m_sth.find(tpos, ykey))
          this->do_throw_key_not_found(ykey);
        return this->do_buckets()[tpos]->second;
      }

    // N.B. This is a non-standard extension.
    template<typename ykeyT>
    const mapped_type*
    ptr(const ykeyT& ykey) const
      {
        size_type tpos;
        if(!this->m_sth.find(tpos, ykey))
          return nullptr;
        return ::std::addressof(this->do_buckets()[tpos]->second);
      }

    // N.B. This is a non-standard extension.
    template<typename ykeyT>
    mapped_type&
    mut(const ykeyT& ykey)
      {
        size_type tpos;
        if(!this->m_sth.find(tpos, ykey))
          this->do_throw_key_not_found(ykey);
        return this->do_mut_buckets()[tpos]->second;
      }

    // N.B. This is a non-standard extension.
    template<typename ykeyT>
    mapped_type*
    mut_ptr(const ykeyT& ykey)
      {
        size_type tpos;
        if(!this->m_sth.find(tpos, ykey))
          return nullptr;
        return ::std::addressof(this->do_mut_buckets()[tpos]->second);
      }

    // N.B. This function is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_hashmap&
    assign(inputT first, inputT last)
      {
        this->clear();
        this->insert(::std::move(first), ::std::move(last));
        return *this;
      }

    // N.B. The return type differs from `std::unordered_map`.
    constexpr const
    allocator_type&
    get_allocator() const noexcept
      { return this->m_sth.as_allocator();  }

    allocator_type&
    get_allocator() noexcept
      { return this->m_sth.as_allocator();  }

    // N.B. The return type differs from `std::unordered_map`.
    constexpr const hasher&
    hash_function() const noexcept
      { return this->m_sth.as_hasher();  }

    hasher&
    hash_function() noexcept
      { return this->m_sth.as_hasher();  }

    // N.B. The return type differs from `std::unordered_map`.
    constexpr const
    key_equal&
    key_eq() const noexcept
      { return this->m_sth.as_key_equal();  }

    key_equal&
    key_eq() noexcept
      { return this->m_sth.as_key_equal();  }
  };

template<typename keyT, typename mappedT, typename hashT, typename eqT, typename allocT>
inline void
swap(cow_hashmap<keyT, mappedT, hashT, eqT, allocT>& lhs,
     cow_hashmap<keyT, mappedT, hashT, eqT, allocT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
