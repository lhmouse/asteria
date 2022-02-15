// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_HASHMAP_HPP_
#  error Please include <rocket/cow_hashmap.hpp> instead.
#endif

namespace details_cow_hashmap {

using unknown_function  = void (...);

struct storage_header
  {
    mutable reference_counter<long> nref;

    unknown_function* dtor;
    size_t nelem;

    explicit
    storage_header() noexcept
      : nref()
      { }
  };

// This struct is used as placeholders for EBO'd bases that would otherwise
// be duplicate, in order to prevent ambiguity.
template<size_t indexT>
struct ebo_placeholder
  {
    template<typename noopT>
    constexpr
    ebo_placeholder(noopT&&) noexcept
      { }
  };

template<typename baseT, size_t indexT, typename... othersT>
struct ebo_select_aux
  : allocator_wrapper_base_for<baseT>  // no duplicate
  { };

template<typename baseT, size_t indexT, typename... othersT>
struct ebo_select_aux<baseT, indexT, baseT, othersT...>
  : identity<ebo_placeholder<indexT>>  // duplicate
  { };

template<typename baseT, size_t indexT, typename firstT, typename... restT>
struct ebo_select_aux<baseT, indexT, firstT, restT...>
  : ebo_select_aux<baseT, indexT, restT...>  // recursive
  { };

template<typename baseT, typename... othersT>
using ebo_select  = typename ebo_select_aux<baseT, sizeof...(othersT), othersT...>::type;

// This is a pointer wrapper that preserves const-ness.
template<typename allocT>
class basic_bucket
  {
  public:
    using allocator_type   = allocT;
    using value_type       = typename allocator_type::value_type;
    using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
    using pointer          = typename allocator_traits<allocator_type>::pointer;

  private:
    pointer m_qval = nullptr;

  public:
    constexpr
    basic_bucket() noexcept
      = default;

    basic_bucket(const basic_bucket&)
      = delete;

    basic_bucket&
    operator=(const basic_bucket&)
      = delete;

  public:
    constexpr const_pointer
    get() const noexcept
      { return this->m_qval;  }

    pointer
    get() noexcept
      { return this->m_qval;  }

    pointer
    exchange(pointer qval) noexcept
      { return ::std::exchange(this->m_qval, qval);  }

    explicit constexpr operator
    bool() const noexcept
      { return bool(this->m_qval);  }

    constexpr const value_type&
    operator*() const
      { return *(this->m_qval);  }

    value_type&
    operator*()
      { return *(this->m_qval);  }

    constexpr const value_type*
    operator->() const noexcept
      { return noadl::unfancy(this->m_qval);  }

    value_type*
    operator->() noexcept
      { return noadl::unfancy(this->m_qval);  }
  };

template<typename allocT, typename hashT>
struct basic_storage
  : public storage_header,
    public allocator_wrapper_base_for<allocT>::type,
    public ebo_select<hashT, allocT>
  {
    using allocator_type   = allocT;
    using hasher           = hashT;
    using bucket_type      = basic_bucket<allocator_type>;
    using pointer          = typename allocator_traits<allocator_type>::pointer;
    using size_type        = typename allocator_traits<allocator_type>::size_type;

    static constexpr size_type
    min_nblk_for_nbkt(size_t nbkt) noexcept
      {
        // Note this is correct even when `nbkt` is zero, as long as
        // `basic_storage` is larger than `bucket_type`.
        return ((nbkt - 1) * sizeof(bucket_type) + sizeof(basic_storage) - 1)
                / sizeof(basic_storage) + 1;
      }

    static constexpr size_t
    max_nbkt_for_nblk(size_type nblk) noexcept
      { return (nblk - 1) * sizeof(basic_storage) / sizeof(bucket_type) + 1;  }

    size_type nblk;
    union { bucket_type bkts[1];  };

    basic_storage(unknown_function* xdtor, const allocator_type& xalloc,
                  const hasher& hf, size_type xnblk) noexcept
      : allocator_wrapper_base_for<allocT>::type(xalloc),
        ebo_select<hashT, allocT>(hf),
        nblk(xnblk)
      {
        this->dtor = xdtor;
        this->nelem = 0;

        // Initialize an empty table.
        size_t nbkts = this->bucket_count();
        for(size_t k = 0;  k != nbkts;  ++k)
          noadl::construct(this->bkts + k);
      }

    ~basic_storage()
      {
        // Destroy all buckets.
        size_t nbkts = this->bucket_count();
        for(size_t k = 0;  k != nbkts;  ++k)
          if(auto qval = this->bkts[k].exchange(nullptr))
            this->free_value(qval);

        for(size_t k = 0;  k != nbkts;  ++k)
          noadl::destroy(this->bkts + k);

#ifdef ROCKET_DEBUG
        this->nelem = static_cast<size_type>(0xBAD1BEEF);
        ::std::memset(static_cast<void*>(this->bkts), '~',
                 sizeof(bucket_type) + (this->nblk - 1) * sizeof(basic_storage));
#endif
      }

    basic_storage(const basic_storage&)
      = delete;

    basic_storage&
    operator=(const basic_storage&)
      = delete;

    constexpr bool
    compatible(const basic_storage& other) const noexcept
      { return static_cast<const allocator_type&>(*this) ==
               static_cast<const allocator_type&>(other);  }

    size_t
    bucket_count() const noexcept
      { return this->max_nbkt_for_nblk(this->nblk);  }

    template<typename... paramsT>
    pointer
    allocate_value(paramsT&&... params)
      {
        auto qval = allocator_traits<allocator_type>::allocate(*this, size_type(1));
        try {
          allocator_traits<allocator_type>::construct(*this,
              noadl::unfancy(qval), ::std::forward<paramsT>(params)...);
        }
        catch(...) {
          allocator_traits<allocator_type>::deallocate(*this, qval, size_type(1));
          throw;
        }
        return qval;
      }

    void
    free_value(pointer qval) noexcept
      {
        ROCKET_ASSERT(qval);

        allocator_traits<allocator_type>::destroy(*this, noadl::unfancy(qval)),
        allocator_traits<allocator_type>::deallocate(*this, qval, size_type(1));
      }

    template<typename ykeyT>
    constexpr size_t
    hash(const ykeyT& ykey) const noexcept
      { return static_cast<const hasher&>(*this)(ykey);  }

    // This function does not check for duplicate keys.
    bucket_type*
    adopt_value_unchecked(pointer qval) noexcept
      {
        ROCKET_ASSERT(qval);
        ROCKET_ASSERT_MSG(this->nref.unique(), "shared storage shall not be modified");

        // Get table bounds.
        auto bptr = this->bkts;
        auto eptr = this->bkts + this->bucket_count();

        // Find an empty bucket for the new element.
        auto orig = noadl::get_probing_origin(bptr, eptr, this->hash(qval->first));
        auto qbkt = noadl::linear_probe(bptr, orig, orig, eptr,
                                        [&](const bucket_type&) { return false;  });
        ROCKET_ASSERT(qbkt);

        // Insert it into the new bucket.
        return this->adopt_value_unchecked(static_cast<size_t>(qbkt - bptr), qval);
      }

    // This function does not check for duplicate keys.
    // The bucket must be empty prior to this call.
    bucket_type*
    adopt_value_unchecked(size_t k, pointer qval) noexcept
      {
        ROCKET_ASSERT(!this->bkts[k]);
        ROCKET_ASSERT(qval);
        ROCKET_ASSERT_MSG(this->nref.unique(), "shared storage shall not be modified");

        // Insert the value into this bucket.
        this->bkts[k].exchange(qval);
        this->nelem += 1;
        return this->bkts + k;
      }

    // This function does not relocate elements and may corrupt the table.
    pointer
    extract_value_opt(size_t k) noexcept
      {
        ROCKET_ASSERT_MSG(this->nref.unique(), "shared storage shall not be modified");

        // Try extracting an element.
        auto qval = this->bkts[k].exchange(nullptr);
        this->nelem -= bool(qval);
        return qval;
      }
  };

template<typename allocT, typename storageT>
struct storage_traits
  {
    using allocator_type   = allocT;
    using storage_type     = storageT;
    using value_type       = typename allocator_type::value_type;

    static void
    do_copy_insert(false_type,      // 1. copyable?
                   bool,            // 2. cloning?
                   storage_type&, const storage_type&)
      {
        noadl::sprintf_and_throw<domain_error>(
              "cow_hashmap: `%s` not copy-constructible",
              typeid(value_type).name());
      }

    static void
    do_copy_insert(true_type,      // 1. copyable?
                   false_type,     // 2. cloning?
                   storage_type& st_new, const storage_type& st_old)
      {
        size_t nbkts = st_old.bucket_count();
        for(size_t k = 0;  k != nbkts;  ++k)
          if(auto qval = st_old.bkts[k].get())
            st_new.adopt_value_unchecked(st_new.allocate_value(*qval));
      }

    static void
    do_copy_insert(true_type,      // 1. copyable?
                   true_type,      // 2. cloning?
                   storage_type& st_new, const storage_type& st_old)
      {
        size_t nbkts = st_old.bucket_count();
        for(size_t k = 0;  k != nbkts;  ++k)
          if(auto qval = st_old.bkts[k].get())
            st_new.adopt_value_unchecked(k, st_new.allocate_value(*qval));
      }

    static void
    dispatch_transfer(storage_type& st_new, storage_type& st_old)
      {
        if(st_new.compatible(st_old) && st_old.nref.unique()) {
          // Values may be moved if allocators compare equal and the old
          // storage is exclusively owned.
          size_t nbkts = st_old.bucket_count();
          for(size_t k = 0;  k != nbkts;  ++k)
            if(auto qval = st_old.extract_value_opt(k))
              st_new.adopt_value_unchecked(qval);

          // After moving all values, `st_old` shall be empty.
          ROCKET_ASSERT(st_old.nelem == 0);
        }
        else
          do_copy_insert(is_copy_constructible<value_type>(),   // 1. copyable?
                         false_type(),                          // 2. cloning?
                         st_new, st_old);
      }

    static void
    dispatch_clone(storage_type& st_new, const storage_type& st_old)
      {
        ROCKET_ASSERT(st_new.nelem == 0);
        ROCKET_ASSERT(st_new.nblk == st_old.nblk);

        do_copy_insert(is_copy_constructible<value_type>(),   // 1. copyable?
                       true_type(),                           // 2. cloning?
                       st_new, st_old);
      }
  };

template<typename allocT, typename hashT, typename eqT>
class storage_handle
  : private allocator_wrapper_base_for<allocT>::type,
    private ebo_select<hashT, allocT>,
    private ebo_select<eqT, allocT, hashT>
  {
  public:
    using allocator_type   = allocT;
    using value_type       = typename allocator_type::value_type;
    using value_pointer    = typename allocator_traits<allocator_type>::pointer;
    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using hasher           = hashT;
    using key_equal        = eqT;
    using bucket_type      = basic_bucket<allocator_type>;

    static constexpr size_type max_load_factor_reciprocal = 2;

  private:
    using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;
    using hasher_base       = typename allocator_wrapper_base_for<hasher>::type;
    using key_equal_base    = typename allocator_wrapper_base_for<key_equal>::type;
    using storage           = basic_storage<allocator_type, hasher>;
    using storage_allocator = typename allocator_traits<allocator_type>::
                                             template rebind_alloc<storage>;
    using storage_pointer   = typename allocator_traits<storage_allocator>::pointer;

  private:
    storage_pointer m_qstor = nullptr;

  public:
    constexpr
    storage_handle(const allocator_type& alloc, const hasher& hf, const key_equal& eq)
      : allocator_base(alloc),
        ebo_select<hashT, allocT>(hf),
        ebo_select<eqT, allocT, hashT>(eq)
      { }

    constexpr
    storage_handle(allocator_type&& alloc, const hasher& hf, const key_equal& eq)
      : allocator_base(::std::move(alloc)),
        ebo_select<hashT, allocT>(hf),
        ebo_select<eqT, allocT, hashT>(eq)
      { }

    ~storage_handle()
      { this->deallocate();  }

    storage_handle(const storage_handle&)
      = delete;

    storage_handle&
    operator=(const storage_handle&)
      = delete;

  private:
    void
    do_reset(storage_pointer qstor_new) noexcept
      {
        // Decrement the reference count with acquire-release semantics to prevent
        // races on `*qstor`.
        auto qstor = ::std::exchange(this->m_qstor, qstor_new);
        if(ROCKET_EXPECT(!qstor))
          return;

        auto qhead = reinterpret_cast<storage_header*>(noadl::unfancy(qstor));
        if(ROCKET_EXPECT(qhead->nref.decrement() != 0))
          return;

        // This indirect call is paramount for incomplete type support.
        reinterpret_cast<void (*)(storage_pointer)>(qhead->dtor)(qstor);
      }

    ROCKET_NEVER_INLINE static void
    do_destroy_storage(storage_pointer qstor) noexcept
      {
        auto nblk = qstor->nblk;
        storage_allocator st_alloc(*qstor);
        noadl::destroy(noadl::unfancy(qstor));
        allocator_traits<storage_allocator>::deallocate(st_alloc, qstor, nblk);
      }

  public:
    constexpr const hasher&
    as_hasher() const noexcept
      { return static_cast<const hasher_base&>(*this);  }

    hasher&
    as_hasher() noexcept
      { return static_cast<hasher_base&>(*this);  }

    constexpr const key_equal&
    as_key_equal() const noexcept
      { return static_cast<const key_equal_base&>(*this);  }

    key_equal&
    as_key_equal() noexcept
      { return static_cast<key_equal_base&>(*this);  }

    constexpr const allocator_type&
    as_allocator() const noexcept
      { return static_cast<const allocator_base&>(*this);  }

    allocator_type&
    as_allocator() noexcept
      { return static_cast<allocator_base&>(*this);  }

    ROCKET_PURE bool
    unique() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return false;
        return qstor->nref.unique();
      }

    long
    use_count() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return qstor->nref.get();
      }

    ROCKET_PURE size_type
    bucket_count() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return qstor->bucket_count();
      }

    ROCKET_PURE size_type
    capacity() const noexcept
      { return this->bucket_count() / max_load_factor_reciprocal;  }

    size_type
    max_size() const noexcept
      {
        storage_allocator st_alloc(this->as_allocator());
        auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
        return storage::max_nbkt_for_nblk(max_nblk / 2) / max_load_factor_reciprocal;
      }

    size_type
    check_size_add(size_type base, size_type add) const
      {
        auto nmax = this->max_size();
        ROCKET_ASSERT(base <= nmax);
        if(nmax - base < add) {
          noadl::sprintf_and_throw<length_error>(
              "cow_hashmap: max size exceeded (`%lld` + `%lld` > `%lld`)",
              static_cast<long long>(base), static_cast<long long>(add),
              static_cast<long long>(nmax));
        }
        return base + add;
      }

    size_type
    round_up_capacity(size_type res_arg) const
      {
        size_type cap = this->check_size_add(0, res_arg);
        auto nblk = storage::min_nblk_for_nbkt(cap * max_load_factor_reciprocal);
        return storage::max_nbkt_for_nblk(nblk) / max_load_factor_reciprocal;
      }

    ROCKET_PURE const bucket_type*
    buckets() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return reinterpret_cast<bucket_type*>(-1);
        return qstor->bkts;
      }

    bucket_type*
    mut_buckets_opt() noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor || !qstor->nref.unique())
          return nullptr;
        return qstor->bkts;
      }

    ROCKET_PURE size_type
    size() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return reinterpret_cast<const storage_header*>(noadl::unfancy(qstor))->nelem;
      }

    template<typename ykeyT>
    const bucket_type*
    find(size_type& tpos, const ykeyT& ykey) const noexcept
      {
        // Get table bounds.
        auto qstor = this->m_qstor;
        if(!qstor)
          return nullptr;

        auto bptr = qstor->bkts;
        auto eptr = qstor->bkts + qstor->bucket_count();

        // Find an equivalent key using linear probing.
        // The load factor is kept below 0.5 so there is always at least one bucket available.
        auto orig = noadl::get_probing_origin(bptr, eptr, qstor->hash(ykey));
        auto qbkt = noadl::linear_probe(bptr, orig, orig, eptr,
                        [&](const bucket_type& r) { return this->as_key_equal()(r->first, ykey);  });

        static_assert(max_load_factor_reciprocal > 1, "");
        ROCKET_ASSERT(qbkt);
        tpos = static_cast<size_type>(qbkt - bptr);

        // If probing stopped due to an empty bucket, there is no equivalent key.
        if(!*qbkt)
          return nullptr;

        // Report that an element has been found.
        // The bucket index is returned via `tpos`.
        return qbkt;
      }

    template<typename ykeyT, typename... paramsT>
    bool
    keyed_try_emplace(size_type& tpos, const ykeyT& ykey, paramsT&&... params)
      {
        auto qstor = this->m_qstor;
        ROCKET_ASSERT_MSG(qstor, "no storage allocated");
        ROCKET_ASSERT_MSG(qstor->nref.unique(), "shared storage shall not be modified");
        ROCKET_ASSERT_MSG(qstor->nelem < this->capacity(), "no space for new elements");

        // Check whether the key exists already.
        if(this->find(tpos, ykey))
          return false;

        // Insert a new element otherwise.
        // Note that `tpos` shall have been set by `find()`.
        qstor->adopt_value_unchecked(tpos, qstor->allocate_value(::std::forward<paramsT>(params)...));
        return true;
      }

    void
    erase_range_unchecked(size_type tpos, size_type tlen) noexcept
      {
        auto qstor = this->m_qstor;
        ROCKET_ASSERT_MSG(qstor, "no storage allocated");
        ROCKET_ASSERT_MSG(qstor->nref.unique(), "shared storage shall not be modified");

        if(tlen == 0)
          return;

        // Clear all buckets in the interval [tpos,tpos+tlen).
        for(size_t k = tpos;  k != tpos + tlen;  ++k)
          if(auto qval = qstor->extract_value_opt(k))
            qstor->free_value(qval);

        // Relocate elements that are not placed in their immediate locations.
        noadl::linear_probe(
          qstor->bkts,
          qstor->bkts + tpos,
          qstor->bkts + tpos + tlen,
          qstor->bkts + qstor->bucket_count(),
          [&](bucket_type& r) {
            // Clear this bucket temporarily.
            auto qval = r.exchange(nullptr);
            ROCKET_ASSERT(qval);
            qstor->nelem -= 1;

            // Insert it back.
            qstor->adopt_value_unchecked(qval);
            return false;
          });
      }

    ROCKET_NEVER_INLINE bucket_type*
    reallocate_clone(storage_handle& sth)
      {
        // Get the number of existent elements.
        // Note that `sth` shall not be empty prior to this call.
        ROCKET_ASSERT(sth.m_qstor);
        auto len = sth.size();

        // Allocate an array of `storage` large enough for a header + `cap` instances of `bucket_type`.
        auto nblk = sth.m_qstor->nblk;
        storage_allocator st_alloc(this->as_allocator());
        auto qstor = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
        noadl::construct(noadl::unfancy(qstor),
                 reinterpret_cast<void (*)(...)>(this->do_destroy_storage),
                 this->as_allocator(), this->as_hasher(), nblk);

        // Copy/move old elements from `sth`.
        try {
          if(len != 0)
            storage_traits<allocator_type, storage>::dispatch_clone(*qstor, *(sth.m_qstor));
        }
        catch(...) {
          this->do_destroy_storage(qstor);
          throw;
        }

        // Set up the new storage.
        this->do_reset(qstor);
        return qstor->bkts;
      }

    ROCKET_NEVER_INLINE bucket_type*
    reallocate_reserve(storage_handle& sth, bool finish, size_type add)
      {
        // Calculate the combined length of hashmap (sth.size() + add).
        // The first part is copied/moved from `sth`. The second part is left uninitialized.
        auto len = sth.size();
        size_type cap = this->check_size_add(len, add);

        // Allocate an array of `storage` large enough for a header + `cap` instances of `bucket_type`.
        auto nblk = storage::min_nblk_for_nbkt(cap * max_load_factor_reciprocal);
        storage_allocator st_alloc(this->as_allocator());
        auto qstor = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
        noadl::construct(noadl::unfancy(qstor),
                 reinterpret_cast<void (*)(...)>(this->do_destroy_storage),
                 this->as_allocator(), this->as_hasher(), nblk);

        // Copy/move old elements from `sth`.
        try {
          if(finish && len)
            ROCKET_ASSERT(sth.m_qstor),
              storage_traits<allocator_type, storage>::dispatch_transfer(*qstor, *(sth.m_qstor));
        }
        catch(...) {
          this->do_destroy_storage(qstor);
          throw;
        }

        // Set up the new storage.
        this->do_reset(qstor);
        return qstor->bkts;
      }

    void
    reallocate_finish(storage_handle& sth)
      {
        auto qstor = this->m_qstor;
        ROCKET_ASSERT(qstor);

#ifdef ROCKET_DEBUG
        // Ensure there are no duplicate keys.
        size_type tpos;
        for(size_t k = 0;  k != qstor->bucket_count();  ++k)
          if(auto qval = qstor->bkts[k].get())
            ROCKET_ASSERT(!sth.find(tpos, qval->first));
#endif

        // Copy/move old elements from `sth`.
        if(sth.m_qstor)
          storage_traits<allocator_type, storage>::dispatch_transfer(*qstor, *(sth.m_qstor));
      }

    void
    deallocate() noexcept
      { this->do_reset(nullptr);  }

    void
    share_with(const storage_handle& other) noexcept
      {
        auto qstor = other.m_qstor;
        if(qstor)
          reinterpret_cast<const storage_header*>(noadl::unfancy(qstor))->nref.increment();
        this->do_reset(qstor);
      }

    void
    exchange_with(storage_handle& other) noexcept
      { ::std::swap(this->m_qstor, other.m_qstor);  }
  };

class stringified_key
  {
  private:
    char m_temp[128];

  public:
    stringified_key(bool val) noexcept
      { ::std::strcpy(this->m_temp, val ? "true" : "false");  }

    stringified_key(signed char val) noexcept
      { ::std::sprintf(this->m_temp, "%d", val);  }

    stringified_key(unsigned char val) noexcept
      { ::std::sprintf(this->m_temp, "%u", val);  }

    stringified_key(signed short val) noexcept
      { ::std::sprintf(this->m_temp, "%d", val);  }

    stringified_key(unsigned short val) noexcept
      { ::std::sprintf(this->m_temp, "%u", val);  }

    stringified_key(signed val) noexcept
      { ::std::sprintf(this->m_temp, "%d", val);  }

    stringified_key(unsigned val) noexcept
      { ::std::sprintf(this->m_temp, "%u", val);  }

    stringified_key(signed long val) noexcept
      { ::std::sprintf(this->m_temp, "%ld", val);  }

    stringified_key(unsigned long val) noexcept
      { ::std::sprintf(this->m_temp, "%lu", val);  }

    stringified_key(signed long long val) noexcept
      { ::std::sprintf(this->m_temp, "%lld", val);  }

    stringified_key(unsigned long long val) noexcept
      { ::std::sprintf(this->m_temp, "%llu", val);  }

    template<typename valueT,
    ROCKET_ENABLE_IF(is_enum<valueT>::value)>
    stringified_key(valueT val) noexcept
      { ::std::sprintf(this->m_temp, "%lld", static_cast<long long>(val));  }

    stringified_key(const void* val) noexcept
      { ::std::sprintf(this->m_temp, "%p", val);  }

    template<typename funcT,
    ROCKET_ENABLE_IF(is_function<funcT>::value)>
    stringified_key(funcT* val) noexcept
      { ::std::sprintf(this->m_temp, "%p", reinterpret_cast<void*>(val));  }

    template<typename valueT,
    ROCKET_DISABLE_IF(is_scalar<valueT>::value)>
    stringified_key(const valueT&) noexcept
      { ::std::strcpy(this->m_temp, "[not printable]");  }

  public:
    const char*
    c_str() const noexcept
      { return this->m_temp;  }
  };

template<typename hashmapT, typename valueT,
         typename bucketT = typename copy_cv<basic_bucket<
                                typename hashmapT::allocator_type>, valueT>::type>
class hashmap_iterator
  {
    template<typename, typename, typename>
    friend class hashmap_iterator;

    friend hashmapT;

  public:
    using iterator_category  = ::std::bidirectional_iterator_tag;
    using value_type         = typename remove_cv<valueT>::type;
    using pointer            = valueT*;
    using reference          = valueT&;
    using difference_type    = ptrdiff_t;

  private:
    bucketT* m_begin;
    bucketT* m_cur;
    bucketT* m_end;

  private:
    // This constructor is called by the container.
    hashmap_iterator(bucketT* begin, size_t ncur, size_t nend) noexcept
      : m_begin(begin), m_cur(begin + ncur), m_end(begin + nend)
      {
        // Go to the first following non-empty bucket if any.
        while((this->m_cur != this->m_end) && !*(this->m_cur))
          this->m_cur++;
      }

  public:
    constexpr
    hashmap_iterator() noexcept
      : m_begin(), m_cur(), m_end()
      { }

    template<typename yvalueT, typename ybucketT,
    ROCKET_ENABLE_IF(is_convertible<ybucketT*, bucketT*>::value)>
    constexpr
    hashmap_iterator(const hashmap_iterator<hashmapT, yvalueT, ybucketT>& other) noexcept
      : m_begin(other.m_begin),
        m_cur(other.m_cur),
        m_end(other.m_end)
      { }

    template<typename yvalueT, typename ybucketT,
    ROCKET_ENABLE_IF(is_convertible<ybucketT*, bucketT*>::value)>
    hashmap_iterator&
    operator=(const hashmap_iterator<hashmapT, yvalueT, ybucketT>& other) noexcept
      { this->m_begin = other.m_begin;
        this->m_cur = other.m_cur;
        this->m_end = other.m_end;
        return *this;  }

  private:
    bucketT*
    do_validate(bucketT* cur, bool deref) const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        ROCKET_ASSERT_MSG((this->m_begin <= cur) && (cur <= this->m_end), "iterator out of range");
        ROCKET_ASSERT_MSG(!deref || (cur < this->m_end), "past-the-end iterator not dereferenceable");
        ROCKET_ASSERT_MSG(!deref || *cur, "iterator invalidated");
        return cur;
      }

    difference_type
    do_this_pos(const bucketT* begin) const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        ROCKET_ASSERT_MSG(this->m_begin == begin, "iterator not compatible");
        return this->do_validate(this->m_cur, false) - begin;
      }

    difference_type
    do_this_len(const hashmap_iterator& other) const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        ROCKET_ASSERT_MSG(this->m_begin == other.m_begin, "iterator not compatible");
        ROCKET_ASSERT_MSG(this->m_end == other.m_end, "iterator not compatible");
        return this->do_validate(this->m_cur, false) - other.m_cur;
      }

    hashmap_iterator
    do_next() const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        auto res = *this;
        do {
          ROCKET_ASSERT_MSG(res.m_cur != this->m_end, "past-the-end iterator not incrementable");
          res.m_cur++;
        }
        while((res.m_cur != this->m_end) && !*(res.m_cur));
        return res;
      }

    hashmap_iterator
    do_prev() const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        auto res = *this;
        do {
          ROCKET_ASSERT_MSG(res.m_cur != this->m_begin, "beginning iterator not decrementable");
          res.m_cur--;
        }
        while(!*(res.m_cur));
        return res;
      }

  public:
    reference
    operator*() const noexcept
      { return **(this->do_validate(this->m_cur, true));  }

    pointer
    operator->() const noexcept
      { return ::std::addressof(**this);  }

    hashmap_iterator&
    operator++() noexcept
      { return *this = this->do_next();  }

    hashmap_iterator&
    operator--() noexcept
      { return *this = this->do_prev();  }

    hashmap_iterator
    operator++(int) noexcept
      { return ::std::exchange(*this, this->do_next());  }

    hashmap_iterator
    operator--(int) noexcept
      { return ::std::exchange(*this, this->do_prev());  }

    template<typename ybucketT>
    constexpr bool
    operator==(const hashmap_iterator<hashmapT, ybucketT>& other) const noexcept
      { return this->m_cur == other.m_cur;  }

    template<typename ybucketT>
    constexpr bool
    operator!=(const hashmap_iterator<hashmapT, ybucketT>& other) const noexcept
      { return this->m_cur != other.m_cur;  }
  };

}  // namespace details_cow_hashmap
