// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_HASHMAP_HPP_
#define ROCKET_COW_HASHMAP_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"
#include "hash_table_utilities.hpp"
#include "reference_counter.hpp"
#include <tuple>  // std::forward_as_tuple()

namespace rocket {

template<typename keyT, typename mappedT, typename hashT = hash<keyT>, typename eqT = equal_to<keyT>,
         typename allocT = allocator<pair<const keyT, mappedT>>>
class cow_hashmap;

#include "details/cow_hashmap.ipp"

/* Differences from `std::unordered_map`:
 * 1. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always
      return `const_reference`s.
 * 2. The copy constructor and copy assignment operator will not throw exceptions.
 * 3. Comparison operators are not provided.
 * 4. `emplace()` and `emplace_hint()` functions are not provided. `try_emplace()` is recommended
      as an alternative.
 * 5. There are no buckets. Bucket lookups and local iterators are not provided. Multimap cannot
      be implemented.
 * 6. The key and mapped types may be incomplete. The mapped type need be neither copy-assignable
      nor move-assignable.
 * 7. `erase()` may move elements around and invalidate iterators.
 * 8. `operator[]()` is not provided.
 */

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

    using const_iterator  = details_cow_hashmap::hashmap_iterator<cow_hashmap, const value_type>;
    using iterator        = details_cow_hashmap::hashmap_iterator<cow_hashmap, value_type>;

  private:
    details_cow_hashmap::storage_handle<allocator_type, hasher, key_equal> m_sth;

  public:
    // 26.5.4.2, construct/copy/destroy
    explicit constexpr
    cow_hashmap(const allocator_type& alloc, const hasher& hf = hasher(), const key_equal& eq = key_equal())
    noexcept(conjunction<is_nothrow_constructible<hasher>,
                         is_nothrow_copy_constructible<hasher>,
                         is_nothrow_constructible<key_equal>,
                         is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(alloc, hf, eq)
      { }

    cow_hashmap(const cow_hashmap& other)
    noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                         is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()),
              other.m_sth.as_hasher(), other.m_sth.as_key_equal())
      { this->assign(other);  }

    cow_hashmap(const cow_hashmap& other, const allocator_type& alloc)
    noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                         is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(alloc, other.m_sth.as_hasher(), other.m_sth.as_key_equal())
      { this->assign(other);  }

    cow_hashmap(cow_hashmap&& other)
    noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                         is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(::std::move(other.m_sth.as_allocator()), other.m_sth.as_hasher(), other.m_sth.as_key_equal())
      { this->assign(::std::move(other));  }

    cow_hashmap(cow_hashmap&& other, const allocator_type& alloc)
    noexcept(conjunction<is_nothrow_copy_constructible<hasher>,
                         is_nothrow_copy_constructible<key_equal>>::value)
      : m_sth(alloc, other.m_sth.as_hasher(), other.m_sth.as_key_equal())
      { this->assign(::std::move(other));  }

    constexpr
    cow_hashmap(nullopt_t = nullopt_t())
    noexcept(conjunction<is_nothrow_constructible<hasher>,
                         is_nothrow_copy_constructible<hasher>,
                         is_nothrow_constructible<key_equal>,
                         is_nothrow_copy_constructible<key_equal>,
                         is_nothrow_constructible<allocator_type>>::value)
      : cow_hashmap(allocator_type())
      { }

    explicit
    cow_hashmap(size_type res_arg, const hasher& hf = hasher(), const key_equal& eq = key_equal(),
                const allocator_type& alloc = allocator_type())
      : cow_hashmap(alloc, hf, eq)
      { this->m_sth.reallocate(0, 0, 0, res_arg);  }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_hashmap(inputT first, inputT last, size_type res_arg = 0, const hasher& hf = hasher(),
                const key_equal& eq = key_equal(), const allocator_type& alloc = allocator_type())
      : cow_hashmap(res_arg, hf, eq, alloc)
      { this->assign(::std::move(first), ::std::move(last));  }

    cow_hashmap(initializer_list<value_type> init, size_type res_arg = 0, const hasher& hf = hasher(),
                const key_equal& eq = key_equal(), const allocator_type& alloc = allocator_type())
      : cow_hashmap(res_arg, hf, eq, alloc)
      { this->assign(init);  }

    cow_hashmap(size_type res_arg, const hasher& hf, const allocator_type& alloc)
      : cow_hashmap(alloc, hf, key_equal())
      { this->m_sth.reallocate(0, 0, 0, res_arg);  }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_hashmap(inputT first, inputT last, size_type res_arg, const hasher& hf, const allocator_type& alloc)
      : cow_hashmap(res_arg, hf, alloc)
      { this->assign(::std::move(first), ::std::move(last));  }

    cow_hashmap(initializer_list<value_type> init, size_type res_arg, const hasher& hf, const allocator_type& alloc)
      : cow_hashmap(res_arg, hf, alloc)
      { this->assign(init);  }

    cow_hashmap(size_type res_arg, const allocator_type& alloc)
      : cow_hashmap(alloc, hasher(), key_equal())
      { this->m_sth.reallocate(0, 0, 0, res_arg);  }

    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_hashmap(inputT first, inputT last, size_type res_arg, const allocator_type& alloc)
      : cow_hashmap(res_arg, alloc)
      { this->assign(::std::move(first), ::std::move(last));  }

    cow_hashmap(initializer_list<value_type> init, size_type res_arg, const allocator_type& alloc)
      : cow_hashmap(res_arg, alloc)
      { this->assign(init);  }

    cow_hashmap&
    operator=(nullopt_t)
    noexcept
      {
        this->clear();
        return *this;
      }

    cow_hashmap&
    operator=(const cow_hashmap& other)
    noexcept
      {
        noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->assign(other);
        return *this;
      }

    cow_hashmap&
    operator=(cow_hashmap&& other)
    noexcept
      {
        noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->assign(::std::move(other));
        return *this;
      }

    cow_hashmap&
    operator=(initializer_list<value_type> init)
      {
        this->assign(init);
        return *this;
      }

  private:
    // Reallocate the storage to `res_arg` elements.
    // The storage is owned by the current hashmap exclusively after this function returns normally.
    details_cow_hashmap::bucket<allocator_type>*
    do_reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
      {
        ROCKET_ASSERT(cnt_one <= off_two);
        ROCKET_ASSERT(off_two <= this->m_sth.bucket_count());
        ROCKET_ASSERT(cnt_two <= this->m_sth.bucket_count() - off_two);
        auto ptr = this->m_sth.reallocate(cnt_one, off_two, cnt_two, res_arg);
        ROCKET_ASSERT(!ptr || this->m_sth.unique());
        return ptr;
      }

    // Clear contents. Deallocate the storage if it is shared at all.
    void
    do_clear()
    noexcept
      {
        if(!this->unique())
          this->m_sth.deallocate();
        else
          this->m_sth.erase_range_unchecked(0, this->m_sth.bucket_count());
      }

    // Reallocate more storage as needed, without shrinking.
    void
    do_reserve_more(size_type cap_add)
      {
        auto cnt = this->size();
        auto cap = this->m_sth.check_size_add(cnt, cap_add);
        if(!this->unique() || ROCKET_UNEXPECT(this->capacity() < cap)) {
#ifndef ROCKET_DEBUG
          // Reserve more space for non-debug builds.
          cap |= cnt / 2 + 7;
#endif
          this->do_reallocate(0, 0, this->bucket_count(), cap | 1);
        }
        ROCKET_ASSERT(this->capacity() >= cap);
      }

    [[noreturn]] ROCKET_NOINLINE
    void
    do_throw_key_not_found()
    const
      { noadl::sprintf_and_throw<out_of_range>("cow_hashmap: key not found");  }

    const details_cow_hashmap::bucket<allocator_type>*
    do_get_table()
    const noexcept
      { return this->m_sth.buckets();  }

    details_cow_hashmap::bucket<allocator_type>*
    do_mut_table()
      {
        if(ROCKET_UNEXPECT(!this->empty() && !this->unique())) {
          return this->do_reallocate(0, 0, this->bucket_count(), this->size() | 1);
        }
        return this->m_sth.mut_buckets_unchecked();
      }

    details_cow_hashmap::bucket<allocator_type>*
    do_erase_no_bound_check(size_type tpos, size_type tn)
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
    const_iterator
    begin()
    const noexcept
      { return const_iterator(this->m_sth, details_cow_hashmap::needs_adjust, this->do_get_table());  }

    const_iterator
    end()
    const noexcept
      { return const_iterator(this->m_sth, this->do_get_table() + this->bucket_count());  }

    const_iterator
    cbegin()
    const noexcept
      { return this->begin();  }

    const_iterator
    cend()
    const noexcept
      { return this->end();  }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator
    mut_begin()
      { return iterator(this->m_sth, details_cow_hashmap::needs_adjust, this->do_mut_table());  }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator
    mut_end()
      { return iterator(this->m_sth, this->do_mut_table() + this->bucket_count());  }

    // capacity
    constexpr
    bool
    empty()
    const noexcept
      { return this->m_sth.empty();  }

    constexpr
    size_type
    size()
    const noexcept
      { return this->m_sth.element_count();  }

    // N.B. This is a non-standard extension.
    constexpr
    difference_type
    ssize()
    const noexcept
      { return static_cast<difference_type>(this->size());  }

    constexpr
    size_type
    max_size()
    const noexcept
      { return this->m_sth.max_size();  }

    constexpr
    size_type
    capacity()
    const noexcept
      { return this->m_sth.capacity();  }

    // N.B. The return type is a non-standard extension.
    cow_hashmap&
    reserve(size_type res_arg)
      {
        auto cnt = this->size();
        auto cap_new = this->m_sth.round_up_capacity(noadl::max(cnt, res_arg));
        // If the storage is shared with other hashmaps, force rellocation to prevent copy-on-write
        // upon modification.
        if(this->unique() && (this->capacity() >= cap_new))
          return *this;

        this->do_reallocate(0, 0, this->bucket_count(), cap_new);
        ROCKET_ASSERT(this->capacity() >= res_arg);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    cow_hashmap&
    shrink_to_fit()
      {
        auto cnt = this->size();
        auto cap_min = this->m_sth.round_up_capacity(cnt);
        // Don't increase memory usage.
        if(!this->unique() || (this->capacity() <= cap_min))
          return *this;

        this->do_reallocate(0, 0, cnt, cnt);
        ROCKET_ASSERT(this->capacity() <= cap_min);
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    cow_hashmap&
    clear()
    noexcept
      {
        if(!this->empty())
          this->do_clear();
        return *this;
      }

    // N.B. This is a non-standard extension.
    bool
    unique()
    const noexcept
      { return this->m_sth.unique();  }

    // N.B. This is a non-standard extension.
    long
    use_count()
    const noexcept
      { return this->m_sth.use_count();  }

    // hash policy
    // N.B. This is a non-standard extension.
    constexpr
    size_type
    bucket_count()
    const noexcept
      { return this->m_sth.bucket_count();  }

    // N.B. The return type differs from `std::unordered_map`.
    constexpr
    double
    load_factor()
    const noexcept
      { return static_cast<double>(static_cast<difference_type>(this->size())) /
               static_cast<double>(static_cast<difference_type>(this->bucket_count()));  }

    // N.B. The `constexpr` specifier is a non-standard extension.
    // N.B. The return type differs from `std::unordered_map`.
    constexpr
    double
    max_load_factor()
    const noexcept
      { return this->m_sth.max_load_factor();  }

    void
    rehash(size_type n)
      { this->do_reallocate(0, 0, this->bucket_count(), noadl::max(this->size(), n));  }

    // 26.5.4.4, modifiers
    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename yvalueT>
    pair<iterator, bool>
    insert(const pair<ykeyT, yvalueT>& value)
      { return this->try_emplace(value.first, value.second);  }

    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename yvalueT>
    pair<iterator, bool>
    insert(pair<ykeyT, yvalueT>&& value)
      { return this->try_emplace(::std::move(value.first), ::std::move(value.second));  }

    // N.B. The return type is a non-standard extension.
    template<typename inputT,
    ROCKET_ENABLE_IF(is_input_iterator<inputT>::value)>
    cow_hashmap&
    insert(inputT first, inputT last)
      {
        if(first == last)
          return *this;

        auto dist = noadl::estimate_distance(first, last);
        if(dist == 0) {
          noadl::ranged_do_while(::std::move(first), ::std::move(last),
                                 [&](const inputT& it) { this->insert(*it);  });
          return *this;
        }
        this->do_reserve_more(dist);
        noadl::ranged_do_while(::std::move(first), ::std::move(last),
                               [&](const inputT& it) { this->m_sth.keyed_emplace_unchecked(it->first, *it);  });
        return *this;
      }

    // N.B. The return type is a non-standard extension.
    cow_hashmap&
    insert(initializer_list<value_type> init)
      {
        return this->insert(init.begin(), init.end());
      }

    // N.B. This is a non-standard extension.
    // N.B. The hint is ignored.
    template<typename ykeyT, typename yvalueT>
    iterator
    insert(const_iterator /*hint*/, const pair<ykeyT, yvalueT>& value)
      {
        return this->insert(value).first;
      }

    // N.B. This is a non-standard extension.
    // N.B. The hint is ignored.
    template<typename ykeyT, typename yvalueT>
    iterator
    insert(const_iterator /*hint*/, pair<ykeyT, yvalueT>&& value)
      {
        return this->insert(::std::move(value)).first;
      }

    template<typename ykeyT, typename... paramsT>
    pair<iterator, bool>
    try_emplace(ykeyT&& key, paramsT&&... params)
      {
        this->do_reserve_more(1);
        auto result = this->m_sth.keyed_emplace_unchecked(key,
                          ::std::piecewise_construct, ::std::forward_as_tuple(::std::forward<ykeyT>(key)),
                                                      ::std::forward_as_tuple(::std::forward<paramsT>(params)...));
        return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
      }

    // N.B. The hint is ignored.
    template<typename ykeyT, typename... paramsT>
    iterator
    try_emplace(const_iterator /*hint*/, ykeyT&& key, paramsT&&... params)
      {
        return this->try_emplace(::std::forward<ykeyT>(key), ::std::forward<paramsT>(params)...).first;
      }

    template<typename ykeyT, typename yvalueT> pair<iterator, bool> insert_or_assign(ykeyT&& key, yvalueT&& yvalue)
      {
        this->do_reserve_more(1);
        auto result = this->m_sth.keyed_emplace_unchecked(key,
                          ::std::forward<ykeyT>(key), ::std::forward<yvalueT>(yvalue));
        if(!result.second) {
          result.first->get()->second = ::std::forward<yvalueT>(yvalue);
        }
        return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
      }

    // N.B. The hint is ignored.
    template<typename ykeyT, typename yvalueT>
    iterator
    insert_or_assign(const_iterator /*hint*/, ykeyT&& key, yvalueT&& yvalue)
      {
        return this->insert_or_assign(::std::forward<ykeyT>(key), ::std::forward<yvalueT>(yvalue)).first;
      }

    // N.B. This function may throw `std::bad_alloc`.
    iterator
    erase(const_iterator tfirst, const_iterator tlast)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->do_get_table());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this->m_sth) - tfirst.tell());
        auto ptr = this->do_erase_no_bound_check(tpos, tn);
        return iterator(this->m_sth, details_cow_hashmap::needs_adjust, ptr);
      }

    // N.B. This function may throw `std::bad_alloc`.
    iterator
    erase(const_iterator tfirst)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->do_get_table());
        auto ptr = this->do_erase_no_bound_check(tpos, 1);
        return iterator(this->m_sth, details_cow_hashmap::needs_adjust, ptr);
      }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. The return type differs from `std::unordered_map`.
    template<typename ykeyT,
    ROCKET_DISABLE_IF(is_convertible<ykeyT, const_iterator>::value)>
    bool
    erase(const ykeyT& key)
      {
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key))
          return false;
        this->do_erase_no_bound_check(tpos, 1);
        return true;
      }

    // map operations
    template<typename ykeyT>
    const_iterator
    find(const ykeyT& key)
    const
      {
        auto ptr = this->do_get_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key))
          return this->end();
        return const_iterator(this->m_sth, ptr + tpos);
      }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    template<typename ykeyT>
    iterator
    find_mut(const ykeyT& key)
      {
        auto ptr = this->do_mut_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key))
          return this->mut_end();
        return iterator(this->m_sth, ptr + tpos);
      }

    template<typename ykeyT>
    size_t
    count(const ykeyT& key)
    const
      {
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key))
          return 0;
        return 1;
      }

    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename ydefaultT>
    typename select_type<const mapped_type&, ydefaultT&&>::type
    get_or(const ykeyT& key, ydefaultT&& ydef)
    const
      {
        auto ptr = this->do_get_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key))
          return ::std::forward<ydefaultT>(ydef);
        return ptr[tpos]->second;
      }

    // N.B. This is a non-standard extension.
    template<typename ykeyT, typename ydefaultT>
    typename select_type<mapped_type&&, ydefaultT&&>::type
    move_or(const ykeyT& key, ydefaultT&& ydef)
    const
      {
        auto ptr = this->do_mut_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key))
          return ::std::forward<ydefaultT>(ydef);
        return ::std::move(ptr[tpos]->second);
      }

    // 26.5.4.3, element access
    template<typename ykeyT>
    const mapped_type&
    at(const ykeyT& key)
    const
      {
        auto ptr = this->do_get_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key))
          this->do_throw_key_not_found();
        return ptr[tpos]->second;
      }

    // N.B. This is a non-standard extension.
    template<typename ykeyT>
    const mapped_type*
    get_ptr(const ykeyT& key)
    const
      {
        auto ptr = this->do_get_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key))
          return nullptr;
        return ::std::addressof(ptr[tpos]->second);
      }

    // N.B. This is a non-standard extension.
    template<typename ykeyT>
    mapped_type&
    mut(const ykeyT& key)
      {
        auto ptr = this->do_mut_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key))
          this->do_throw_key_not_found();
        return ptr[tpos]->second;
      }

    // N.B. This is a non-standard extension.
    template<typename ykeyT>
    mapped_type*
    mut_ptr(const ykeyT& key)
      {
        auto ptr = this->do_mut_table();
        size_type tpos;
        if(!this->m_sth.index_of(tpos, key))
          return nullptr;
        return ::std::addressof(ptr[tpos]->second);
      }

    // N.B. This function is a non-standard extension.
    cow_hashmap&
    assign(const cow_hashmap& other)
    noexcept
      {
        this->m_sth.share_with(other.m_sth);
        return *this;
      }

    // N.B. This function is a non-standard extension.
    cow_hashmap&
    assign(cow_hashmap&& other)
    noexcept
      {
        this->m_sth.share_with(::std::move(other.m_sth));
        return *this;
      }

    // N.B. This function is a non-standard extension.
    cow_hashmap&
    assign(initializer_list<value_type> init)
      {
        this->clear();
        this->insert(init);
        return *this;
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

    cow_hashmap&
    swap(cow_hashmap& other)
    noexcept
      {
        noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.exchange_with(other.m_sth);
        return *this;
      }

    // N.B. The return type differs from `std::unordered_map`.
    constexpr
    const
    allocator_type&
    get_allocator()
    const noexcept
      { return this->m_sth.as_allocator();  }

    allocator_type&
    get_allocator()
    noexcept
      { return this->m_sth.as_allocator();  }

    // N.B. The return type differs from `std::unordered_map`.
    constexpr
    const hasher&
    hash_function()
    const noexcept
      { return this->m_sth.as_hasher();  }

    hasher&
    hash_function()
    noexcept
      { return this->m_sth.as_hasher();  }

    // N.B. The return type differs from `std::unordered_map`.
    constexpr
    const
    key_equal&
    key_eq()
    const noexcept
      { return this->m_sth.as_key_equal();  }

    key_equal&
    key_eq()
    noexcept
      { return this->m_sth.as_key_equal();  }
  };

template<typename keyT, typename mappedT, typename hashT, typename eqT, typename allocT>
inline
void
swap(cow_hashmap<keyT, mappedT, hashT, eqT, allocT>& lhs, cow_hashmap<keyT, mappedT, hashT, eqT, allocT>& rhs)
noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
