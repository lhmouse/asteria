// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_HASHMAP_HPP_
#  error Please include <rocket/cow_hashmap.hpp> instead.
#endif

namespace details_cow_hashmap {

struct storage_header
  {
    void (*dtor)(...);
    mutable reference_counter<long> nref;
    size_t nelem;

    explicit
    storage_header(void (*xdtor)(...))
    noexcept
      : dtor(xdtor), nref()  // `nelem` is uninitialized
      { }
  };

template<typename allocT>
class bucket
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
    // This cannot be `constexpr` as `m_ptr` may be left uninitialized.
    bucket()
    noexcept
      = default;

    bucket(const bucket&)
      = delete;

    bucket&
    operator=(const bucket&)
      = delete;

  public:
    const_pointer
    get()
    const noexcept
      { return this->m_ptr;  }

    pointer
    get()
    noexcept
      { return this->m_ptr;  }

    pointer
    reset(pointer ptr = pointer())
    noexcept
      { return ::std::exchange(this->m_ptr, ptr);  }

    explicit operator
    bool()
    const noexcept
      { return bool(this->get());  }

    const_reference
    operator*()
    const noexcept
      { return *(this->get());  }

    reference
    operator*()
    noexcept
      { return *(this->get());  }

    const_pointer
    operator->()
    const noexcept
      { return this->m_ptr;  }

    pointer
    operator->()
    noexcept
      { return this->m_ptr;  }
  };

template<typename allocT>
struct pointer_storage : storage_header
  {
    using allocator_type   = allocT;
    using bucket_type      = bucket<allocator_type>;
    using size_type        = typename allocator_traits<allocator_type>::size_type;

    static constexpr
    size_type
    min_nblk_for_nbkt(size_type nbkt)
    noexcept
      { return (sizeof(bucket_type) * nbkt + sizeof(pointer_storage) - 1) / sizeof(pointer_storage) + 1;  }

    static constexpr
    size_type
    max_nbkt_for_nblk(size_type nblk)
    noexcept
      { return sizeof(pointer_storage) * (nblk - 1) / sizeof(bucket_type);  }

    allocator_type alloc;
    size_type nblk;
    bucket_type data[0];

    pointer_storage(void (*xdtor)(...), const allocator_type& xalloc, size_type xnblk)
    noexcept
      : storage_header(xdtor), alloc(xalloc), nblk(xnblk)
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
          // Deallocate the element.
          auto eptr = this->data[i].get();
          if(eptr) {
            allocator_traits<allocator_type>::destroy(this->alloc, noadl::unfancy(eptr));
            allocator_traits<allocator_type>::deallocate(this->alloc, eptr, size_t(1));
          }

          // `allocator_type::pointer` need not be a trivial type.
          noadl::destroy_at(this->data + i);
        }

#ifdef ROCKET_DEBUG
        this->nelem = 0xEECD;
#endif
      }

    pointer_storage(const pointer_storage&)
      = delete;

    pointer_storage&
    operator=(const pointer_storage&)
      = delete;
  };

struct storage_traits_helpers
  {
    template<typename ptrT, typename allocT, typename hashT>
    [[noreturn]] static
    void
    dispatch_copy(false_type, ptrT /*ptr*/, const hashT& /*hf*/, ptrT /*ptr_old*/, size_t, size_t)
      {
        // Throw an exception unconditionally, even when there is nothing to copy.
        noadl::sprintf_and_throw<domain_error>("cow_hashmap: `%s` not copy-constructible",
                                               typeid(typename allocT::value_type).name());
      }

    template<typename ptrT, typename allocT, typename hashT>
    static
    void
    dispatch_copy(true_type, ptrT ptr, const hashT& hf, ptrT ptr_old, size_t off, size_t cnt)
      {
        // Get table bounds.
        auto data = ptr->data;
        auto end = data + pointer_storage<allocT>::max_nbkt_for_nblk(ptr->nblk);

        // Copy elements one by one.
        for(size_t i = off; i != off + cnt; ++i) {
          auto eptr_old = ptr_old->data[i].get();
          if(!eptr_old)
            continue;

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

    template<typename ptrT, typename allocT, typename hashT>
    static
    void
    dispatch_move(ptrT ptr, const hashT& hf, ptrT ptr_old, size_t off, size_t cnt)
      {
        // Get table bounds.
        auto data = ptr->data;
        auto end = data + pointer_storage<allocT>::max_nbkt_for_nblk(ptr->nblk);

        // Move elements one by one.
        for(size_t i = off; i != off + cnt; ++i) {
          auto eptr_old = ptr_old->data[i].get();
          if(!eptr_old)
            continue;

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

template<typename ptrT, typename allocT, typename hashT>
struct storage_traits
  {
    static
    void
    copy(ptrT ptr, const hashT& hf, ptrT ptr_old, size_t off, size_t cnt)
      {
        storage_traits_helpers::template dispatch_copy<ptrT, allocT>(
           is_copy_constructible<typename allocT::value_type>(),  // copyable
           ptr, hf, ptr_old, off, cnt);
      }

    static
    void
    move(ptrT ptr, const hashT& hf, ptrT ptr_old, size_t off, size_t cnt)
      {
        storage_traits_helpers::template dispatch_move<ptrT, allocT>(
           ptr, hf, ptr_old, off, cnt);
      }
  };

// This struct is used as placeholders for EBO'd bases that would otherwise be duplicate, in order to
// prevent ambiguity.
template<int indexT>
struct ebo_placeholder
  {
    template<typename anyT>
    constexpr
    ebo_placeholder(anyT&&)
    noexcept
      { }
  };

template<typename allocT, typename hashT, typename eqT>
class storage_handle
  : private allocator_wrapper_base_for<allocT>::type,
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
    constexpr
    storage_handle(const allocator_type& alloc, const hasher& hf, const key_equal& eq)
      : allocator_base(alloc),
        conditional<is_same<hashT, allocT>::value,
                    ebo_placeholder<0>, hasher_base>::type(hf),
        conditional<is_same<eqT, allocT>::value || is_same<eqT, hashT>::value,
                    ebo_placeholder<1>, key_equal_base>::type(eq),
        m_ptr()
      { }

    constexpr
    storage_handle(allocator_type&& alloc, const hasher& hf, const key_equal& eq)
      : allocator_base(::std::move(alloc)),
        conditional<is_same<hashT, allocT>::value,
                    ebo_placeholder<0>, hasher_base>::type(hf),
        conditional<is_same<eqT, allocT>::value || is_same<eqT, hashT>::value,
                    ebo_placeholder<1>, key_equal_base>::type(eq),
        m_ptr()
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
    do_reset(storage_pointer ptr_new)
    noexcept
      {
        auto ptr = ::std::exchange(this->m_ptr, ptr_new);
        if(ROCKET_EXPECT(!ptr))
          return;

        // This is needed for incomplete type support.
        auto dtor = reinterpret_cast<const storage_header*>(noadl::unfancy(ptr))->dtor;
        reinterpret_cast<void (*)(storage_pointer)>(dtor)(ptr);
      }

    ROCKET_NOINLINE static
    void
    do_drop_reference(storage_pointer ptr)
    noexcept
      {
        // Decrement the reference count with acquire-release semantics to prevent races on `ptr->alloc`.
        if(ROCKET_EXPECT(!ptr->nref.decrement()))
          return;

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
    const hasher&
    as_hasher()
    const noexcept
      { return static_cast<const hasher_base&>(*this);  }

    hasher&
    as_hasher()
    noexcept
      { return static_cast<hasher_base&>(*this);  }

    const key_equal&
    as_key_equal()
    const noexcept
      { return static_cast<const key_equal_base&>(*this);  }

    key_equal&
    as_key_equal()
    noexcept
      { return static_cast<key_equal_base&>(*this);  }

    const allocator_type&
    as_allocator()
    const noexcept
      { return static_cast<const allocator_base&>(*this);  }

    allocator_type&
    as_allocator()
    noexcept
      { return static_cast<allocator_base&>(*this);  }

    bool
    unique()
    const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return false;

        return ptr->nref.unique();
      }

    long
    use_count()
    const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return 0;

        auto nref = ptr->nref.get();
        ROCKET_ASSERT(nref > 0);
        return nref;
      }

    constexpr
    double
    max_load_factor()
    const noexcept
      { return 1.0 / static_cast<double>(static_cast<difference_type>(max_load_factor_reciprocal));  }

    size_type
    bucket_count()
    const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return 0;

        return storage::max_nbkt_for_nblk(ptr->nblk);
      }

    size_type
    capacity()
    const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return 0;

        auto cap = storage::max_nbkt_for_nblk(ptr->nblk) / max_load_factor_reciprocal;
        ROCKET_ASSERT(cap > 0);
        return cap;
      }

    size_type
    max_size()
    const noexcept
      {
        storage_allocator st_alloc(this->as_allocator());
        auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
        return storage::max_nbkt_for_nblk(max_nblk / 2) / max_load_factor_reciprocal;
      }

    size_type
    check_size_add(size_type base, size_type add)
    const
      {
        auto nmax = this->max_size();
        ROCKET_ASSERT(base <= nmax);
        if(nmax - base < add)
          noadl::sprintf_and_throw<length_error>("cow_hashmap: max size exceeded (`%lld` + `%lld` > `%lld`)",
                                                 static_cast<long long>(base), static_cast<long long>(add),
                                                 static_cast<long long>(nmax));
        return base + add;
      }

    size_type
    round_up_capacity(size_type res_arg)
    const
      {
        auto cap = this->check_size_add(0, res_arg);
        auto nblk = storage::min_nblk_for_nbkt(cap * max_load_factor_reciprocal);
        return storage::max_nbkt_for_nblk(nblk) / max_load_factor_reciprocal;
      }

    const bucket_type*
    buckets()
    const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return nullptr;

        return ptr->data;
      }

    bool
    empty()
    const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return true;

        return reinterpret_cast<const storage_header*>(ptr)->nelem == 0;
      }

    size_type
    element_count()
    const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return 0;

        return reinterpret_cast<const storage_header*>(ptr)->nelem;
      }

    ROCKET_NOINLINE
    bucket_type*
    reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
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
        auto dtor = reinterpret_cast<void (*)(...)>(storage_handle::do_drop_reference);
        noadl::construct_at(noadl::unfancy(ptr), dtor, this->as_allocator(), nblk);

        // Copy or move elements into the new block.
        auto ptr_old = this->m_ptr;
        if(!ptr_old) {
          // Replace the current block.
          this->do_reset(ptr);
          return ptr->data;
        }

        try {
          // Moving is only viable if the old and new allocators compare equal and the old block
          // is owned exclusively.
          using traits = storage_traits<storage_pointer, allocator_type, hasher>;
          if((ptr_old->alloc != ptr->alloc) || !ptr_old->nref.unique())
            traits::copy(ptr, this->as_hasher(), ptr_old,       0, cnt_one),
            traits::copy(ptr, this->as_hasher(), ptr_old, off_two, cnt_two);
          else
            traits::move(ptr, this->as_hasher(), ptr_old,       0, cnt_one),
            traits::move(ptr, this->as_hasher(), ptr_old, off_two, cnt_two);
        }
        catch(...) {
          // If an exception is thrown, deallocate the new block, then rethrow the exception.
          noadl::destroy_at(noadl::unfancy(ptr));
          allocator_traits<storage_allocator>::deallocate(st_alloc, ptr, nblk);
          throw;
        }

        // Replace the current block.
        this->do_reset(ptr);
        return ptr->data;
      }

    void
    deallocate()
    noexcept
      { this->do_reset(storage_pointer());  }

    void
    share_with(const storage_handle& other)
    noexcept
      {
        auto ptr = other.m_ptr;
        if(ptr)
          reinterpret_cast<storage_header*>(noadl::unfancy(ptr))->nref.increment();
        this->do_reset(ptr);
      }

    void
    share_with(storage_handle&& other)
    noexcept
      {
        auto ptr = other.m_ptr;
        if(ptr)
          other.m_ptr = storage_pointer();
        this->do_reset(ptr);
      }

    void
    exchange_with(storage_handle& other)
    noexcept
      {
        noadl::xswap(this->m_ptr, other.m_ptr);
      }

    constexpr operator
    const storage_handle*()
    const noexcept
      { return this;  }

    operator
    storage_handle*()
    noexcept
      { return this;  }

    template<typename ykeyT>
    bool
    index_of(size_type& index, const ykeyT& ykey)
    const
      {
#ifdef ROCKET_DEBUG
        index = size_type(0xDEADBEEF);
#endif
        auto ptr = this->m_ptr;
        if(!ptr)
          return false;

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

        if(!*bkt)
          // The previous probing has stopped due to an empty bucket. No equivalent key has been found so far.
          return false;

        ROCKET_ASSERT(data <= bkt);
        index = static_cast<size_type>(bkt - data);
        return true;
      }

    bucket_type*
    mut_buckets_unchecked()
    noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return nullptr;

        ROCKET_ASSERT(this->unique());
        return ptr->data;
      }

    template<typename ykeyT, typename... paramsT>
    pair<bucket_type*, bool>
    keyed_emplace_unchecked(const ykeyT& ykey, paramsT&&... params)
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
        if(*bkt)
          // A duplicate key has been found.
          return ::std::make_pair(bkt, false);

        // Allocate a new element.
        auto eptr = allocator_traits<allocator_type>::allocate(ptr->alloc, size_t(1));
        try {
          allocator_traits<allocator_type>::construct(ptr->alloc, noadl::unfancy(eptr),
                                                      ::std::forward<paramsT>(params)...);
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

    void
    erase_range_unchecked(size_type tpos, size_type tn)
    noexcept
      {
        ROCKET_ASSERT(this->unique());
        ROCKET_ASSERT(tpos <= this->bucket_count());
        ROCKET_ASSERT(tn <= this->bucket_count() - tpos);
        if(tn == 0)
          return;

        auto ptr = this->m_ptr;
        ROCKET_ASSERT(ptr);

        // Erase all elements in [tpos,tpos+tn).
        for(size_type i = tpos; i != tpos + tn; ++i) {
          auto eptr = ptr->data[i].reset();
          if(!eptr)
            continue;

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
          [&](bucket_type& rbkt) {
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
  };

// Informs the constructor of an iterator that the `bkt` parameter might point to an empty bucket.
struct needs_adjust_tag
  { }
  constexpr needs_adjust;

template<typename hashmapT, typename valueT>
class hashmap_iterator
  {
    template<typename, typename>
    friend class hashmap_iterator;

    friend hashmapT;

  public:
    using iterator_category  = forward_iterator_tag;
    using value_type         = typename remove_cv<valueT>::type;
    using pointer            = valueT*;
    using reference          = valueT&;
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
    // These constructors are called by the container.
    constexpr
    hashmap_iterator(const parent_type* ref, bucket_type* bkt)
    noexcept
      : m_ref(ref), m_bkt(bkt)
      { }

    hashmap_iterator(const parent_type* ref, needs_adjust_tag, bucket_type* bkt)
    noexcept
      : m_ref(ref), m_bkt(bkt)
      { this->do_adjust_unchecked();  }

  public:
    constexpr
    hashmap_iterator()
    noexcept
      : hashmap_iterator(nullptr, nullptr)
      { }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_convertible<yvalueT*, valueT*>::value)>
    constexpr
    hashmap_iterator(const hashmap_iterator<hashmapT, yvalueT>& other)
    noexcept
      : hashmap_iterator(other.m_ref, other.m_bkt)
      { }

  private:
    bucket_type*
    do_assert_valid_bucket(bucket_type* bkt, bool deref)
    const noexcept
      {
        auto ref = this->m_ref;
        ROCKET_ASSERT_MSG(ref, "Iterator not initialized");
        auto dist = static_cast<size_t>(bkt - ref->buckets());
        ROCKET_ASSERT_MSG(dist <= ref->bucket_count(), "Iterator invalidated");
        ROCKET_ASSERT_MSG(!deref || ((dist < ref->bucket_count()) && *bkt),
                          "Past-the-end iterator not dereferenceable");
        return bkt;
      }

    bucket_type*
    do_adjust_unchecked()
    noexcept
      {
        // Don't advance singular iterators.
        auto bkt = this->m_bkt;
        if(!bkt)
          return nullptr;

        auto ref = this->m_ref;
        ROCKET_ASSERT_MSG(ref, "Iterator not initialized");
        const auto end = ref->buckets() + ref->bucket_count();

        // Find the first non-empty bucket.
        while((bkt != end) && !*bkt)
          ++bkt;

        this->m_bkt = this->do_assert_valid_bucket(bkt, false);
        return bkt;
      }

  public:
    const parent_type*
    parent()
    const noexcept
      { return this->m_ref;  }

    bucket_type*
    tell()
    const noexcept
      {
        return this->do_assert_valid_bucket(this->m_bkt, false);
      }

    bucket_type*
    tell_owned_by(const parent_type* ref)
    const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_ref == ref, "Dangling iterator");
        return this->tell();
      }

    hashmap_iterator&
    next()
    noexcept
      {
        this->m_bkt = this->do_assert_valid_bucket(this->m_bkt + 1, false);
        this->do_adjust_unchecked();
        return *this;
      }

    reference
    operator*()
    const noexcept
      {
        auto bkt = this->do_assert_valid_bucket(this->m_bkt, true);
        ROCKET_ASSERT(*bkt);
        return **bkt;
      }

    pointer
    operator->()
    const noexcept
      {
        auto bkt = this->do_assert_valid_bucket(this->m_bkt, true);
        ROCKET_ASSERT(*bkt);
        return ::std::addressof(**bkt);
      }
  };

template<typename hashmapT, typename valueT>
inline
hashmap_iterator<hashmapT, valueT>&
operator++(hashmap_iterator<hashmapT, valueT>& rhs)
noexcept
  { return rhs.next();  }

template<typename hashmapT, typename valueT>
inline
hashmap_iterator<hashmapT, valueT>
operator++(hashmap_iterator<hashmapT, valueT>& lhs, int)
noexcept
  {
    auto res = lhs;
    lhs.next();
    return res;
  }

template<typename hashmapT, typename xvalueT, typename yvalueT>
inline
bool
operator==(const hashmap_iterator<hashmapT, xvalueT>& lhs, const hashmap_iterator<hashmapT, yvalueT>& rhs)
noexcept
  { return lhs.tell() == rhs.tell();  }

template<typename hashmapT, typename xvalueT, typename yvalueT>
inline
bool
operator!=(const hashmap_iterator<hashmapT, xvalueT>& lhs, const hashmap_iterator<hashmapT, yvalueT>& rhs)
noexcept
  { return lhs.tell() != rhs.tell();  }

}  // namespace details_cow_hashmap
