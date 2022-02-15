// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_VECTOR_HPP_
#  error Please include <rocket/cow_vector.hpp> instead.
#endif

namespace details_cow_vector {

using unknown_function  = void (...);

struct storage_header
  {
    mutable reference_counter<long> nref;

    unknown_function* dtor;
    size_t nelem;

    // This is the number of uninitialized elements in the beginning.
    // This field is paramount for exception safety.
    // It shall be zero for a fully initialized block.
    size_t nskip;

    explicit
    storage_header() noexcept
      : nref()
      { }
  };

template<typename allocT>
struct basic_storage
  : public storage_header,
    public allocator_wrapper_base_for<allocT>::type
  {
    using allocator_type   = allocT;
    using value_type       = typename allocator_type::value_type;
    using size_type        = typename allocator_traits<allocator_type>::size_type;

    static constexpr size_type
    min_nblk_for_nelem(size_t nelem) noexcept
      {
        // Note this is correct even when `nelem` is zero, as long as
        // `basic_storage` is larger than `value_type`.
        return ((nelem - 1) * sizeof(value_type) + sizeof(basic_storage) - 1)
                / sizeof(basic_storage) + 1;
      }

    static constexpr size_t
    max_nelem_for_nblk(size_type nblk) noexcept
      { return (nblk - 1) * sizeof(basic_storage) / sizeof(value_type) + 1;  }

    size_type nblk;
    union { value_type data[1];  };

    basic_storage(unknown_function* xdtor, size_t xnskip,
                  const allocator_type& xalloc, size_type xnblk) noexcept
      : allocator_wrapper_base_for<allocT>::type(xalloc),
        nblk(xnblk)
      {
        this->dtor = xdtor;
        this->nelem = xnskip;
        this->nskip = xnskip;

#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this->data), '*',
                 sizeof(value_type) + (this->nblk - 1) * sizeof(basic_storage));
#endif
      }

    ~basic_storage()
      {
        // Destroy all elements backwards.
        size_t off = this->nelem;
        while(off-- != this->nskip)
          allocator_traits<allocator_type>::destroy(*this, this->data + off);

#ifdef ROCKET_DEBUG
        this->nelem = static_cast<size_type>(0xBAD1BEEF);
        ::std::memset(static_cast<void*>(this->data), '~',
                 sizeof(value_type) + (this->nblk - 1) * sizeof(basic_storage));
#endif
      }

    basic_storage(const basic_storage&)
      = delete;

    basic_storage&
    operator=(const basic_storage&)
      = delete;

    template<typename... paramsT>
    value_type&
    emplace_back_unchecked(paramsT&&... params)
      {
        ROCKET_ASSERT_MSG(this->nref.unique(), "shared storage shall not be modified");
        ROCKET_ASSERT_MSG(this->nelem < this->max_nelem_for_nblk(this->nblk),
                          "no space for new elements");

        size_t off = this->nelem;
        allocator_traits<allocator_type>::construct(*this, this->data + off,
                                                    ::std::forward<paramsT>(params)...);
        this->nelem = static_cast<size_type>(off + 1);

        return this->data[off];
      }

    void
    pop_back_unchecked() noexcept
      {
        ROCKET_ASSERT_MSG(this->nref.unique(), "shared storage shall not be modified");
        ROCKET_ASSERT_MSG(this->nelem > 0, "no element to pop");

        size_t off = this->nelem - 1;
        this->nelem = static_cast<size_type>(off);
        allocator_traits<allocator_type>::destroy(*this, this->data + off);
      }
  };

template<typename allocT, typename storageT>
struct storage_traits
  {
    using allocator_type   = allocT;
    using storage_type     = storageT;
    using value_type       = typename allocator_type::value_type;

    [[noreturn]] static void
    do_transfer(false_type,   // 1. movable?
                bool,         // 2. trivial?
                bool,         // 3. copyable?
                storage_type&, storage_type&, size_t)
      {
        noadl::sprintf_and_throw<domain_error>(
              "cow_vector: `%s` not move-constructible",
              typeid(value_type).name());
      }

    static void
    do_transfer(true_type,    // 1. movable?
                true_type,    // 2. trivial?
                bool,         // 3. copyable?
                storage_type& st_new, storage_type& st_old, size_t nskip)
      {
        ::std::memcpy(st_new.data + nskip, st_old.data + nskip,
                                           sizeof(value_type) * (st_old.nelem - nskip));
        st_new.nskip = st_old.nskip;
      }

    static void
    do_transfer(true_type,    // 1. movable?
                false_type,   // 2. trivial?
                true_type,    // 3. copyable?
                storage_type& st_new, storage_type& st_old, size_t nskip)
      {
        if(st_old.nref.unique())
          for(size_t k = st_old.nelem - 1;  k != nskip - 1;  --k)
            allocator_traits<allocator_type>::construct(st_new,
                                     st_new.data + k, ::std::move(st_old.data[k])),
            st_new.nskip = k;
        else
          for(size_t k = st_old.nelem - 1;  k != nskip - 1;  --k)
            allocator_traits<allocator_type>::construct(st_new,
                                     st_new.data + k, st_old.data[k]),
            st_new.nskip = k;
      }

    static void
    do_transfer(true_type,    // 1. movable?
                false_type,   // 2. trivial?
                false_type,   // 3. copyable?
                storage_type& st_new, storage_type& st_old, size_t nskip)
      {
        if(st_old.nref.unique())
          for(size_t k = st_old.nelem - 1;  k != nskip - 1;  --k)
            allocator_traits<allocator_type>::construct(st_new,
                                     st_new.data + k, ::std::move(st_old.data[k])),
            st_new.nskip = k;
        else
          noadl::sprintf_and_throw<domain_error>(
                "cow_vector: `%s` not copy-constructible",
                typeid(value_type).name());
      }

    static void
    dispatch_transfer(storage_type& st_new, storage_type& st_old, size_t nskip)
      {
        ROCKET_ASSERT(st_old.nskip <= st_old.nelem);
        ROCKET_ASSERT(st_old.nelem == st_new.nskip);
        ROCKET_ASSERT(st_new.nskip <= st_new.nelem);
        ROCKET_ASSERT(st_old.nskip <= nskip);

        return do_transfer(
            is_move_constructible<value_type>(),              // 1. movable
            conjunction<is_trivially_copyable<value_type>,
                        is_std_allocator<allocator_type>>(),  // 2. trivial
            is_copy_constructible<value_type>(),              // 3. copyable
            st_new, st_old, nskip);
      }
  };

template<typename allocT>
class storage_handle
  : private allocator_wrapper_base_for<allocT>::type
  {
  public:
    using allocator_type   = allocT;
    using value_type       = typename allocator_type::value_type;
    using size_type        = typename allocator_traits<allocator_type>::size_type;

  private:
    using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;
    using storage           = basic_storage<allocator_type>;
    using storage_allocator = typename allocator_traits<allocator_type>::template rebind_alloc<storage>;
    using storage_pointer   = typename allocator_traits<storage_allocator>::pointer;

  private:
    storage_pointer m_qstor = nullptr;

  public:
    explicit constexpr
    storage_handle(const allocator_type& alloc) noexcept
      : allocator_base(alloc)
      { }

    explicit constexpr
    storage_handle(allocator_type&& alloc) noexcept
      : allocator_base(::std::move(alloc))
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
        // races on `qstor`.
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
    capacity() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return storage::max_nelem_for_nblk(qstor->nblk);
      }

    size_type
    max_size() const noexcept
      {
        storage_allocator st_alloc(this->as_allocator());
        auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
        return storage::max_nelem_for_nblk(max_nblk / 2);
      }

    size_type
    check_size_add(size_type base, size_type add) const
      {
        auto nmax = this->max_size();
        ROCKET_ASSERT(base <= nmax);
        if(nmax - base < add) {
          noadl::sprintf_and_throw<length_error>(
                "cow_vector: max size exceeded (`%lld` + `%lld` > `%lld`)",
                static_cast<long long>(base), static_cast<long long>(add),
                static_cast<long long>(nmax));
        }
        return base + add;
      }

    size_type
    round_up_capacity(size_type res_arg) const
      {
        size_type cap = this->check_size_add(0, res_arg);
        auto nblk = storage::min_nblk_for_nelem(cap);
        return storage::max_nelem_for_nblk(nblk);
      }

    ROCKET_PURE const value_type*
    data() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return reinterpret_cast<value_type*>(-1);
        return qstor->data;
      }

    value_type*
    mut_data_opt() noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor || !qstor->nref.unique())
          return nullptr;
        return qstor->data;
      }

    ROCKET_PURE size_type
    size() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return reinterpret_cast<const storage_header*>(noadl::unfancy(qstor))->nelem;
      }

    template<typename... paramsT>
    value_type&
    emplace_back_unchecked(paramsT&&... params)
      {
        auto qstor = this->m_qstor;
        ROCKET_ASSERT_MSG(qstor, "no storage allocated");
        return qstor->emplace_back_unchecked(::std::forward<paramsT>(params)...);
      }

    void
    pop_back_unchecked() noexcept
      {
        auto qstor = this->m_qstor;
        ROCKET_ASSERT_MSG(qstor, "no storage allocated");
        return qstor->pop_back_unchecked();
      }

    void
    pop_back_unchecked(size_t total) noexcept
      {
        for(size_t k = 0;  k != total;  ++k)
          this->pop_back_unchecked();
      }

    ROCKET_NEVER_INLINE value_type*
    reallocate_clone(storage_handle& sth)
      {
        // Get the number of existent elements.
        // Note that `sth` shall not be empty prior to this call.
        ROCKET_ASSERT(sth.m_qstor);
        auto len = sth.size();

        // Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
        auto nblk = sth.m_qstor->nblk;
        storage_allocator st_alloc(this->as_allocator());
        auto qstor = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
        noadl::construct(noadl::unfancy(qstor),
                 reinterpret_cast<unknown_function*>(this->do_destroy_storage), len,
                 this->as_allocator(), nblk);

        // Copy/move old elements from `sth`.
        try {
          if(len != 0)
            storage_traits<allocator_type, storage>::dispatch_transfer(*qstor, *(sth.m_qstor), 0);
        }
        catch(...) {
          this->do_destroy_storage(qstor);
          throw;
        }

        // Set up the new storage.
        this->do_reset(qstor);
        return qstor->data;
      }

    ROCKET_NEVER_INLINE value_type*
    reallocate_prepare(storage_handle& sth, size_type skip, size_type add)
      {
        // Calculate the combined length of vector (sth.size() + add).
        // The first part is copied/moved from `sth`. The second part is left uninitialized.
        auto len = sth.size();
        size_type cap = this->check_size_add(len, add);

        // Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
        auto nblk = storage::min_nblk_for_nelem(cap);
        storage_allocator st_alloc(this->as_allocator());
        auto qstor = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
        noadl::construct(noadl::unfancy(qstor),
                 reinterpret_cast<unknown_function*>(this->do_destroy_storage), len,
                 this->as_allocator(), nblk);

        // Copy/move old elements from `sth`.
        try {
          if(skip < len)
            ROCKET_ASSERT(sth.m_qstor),
              storage_traits<allocator_type, storage>::dispatch_transfer(*qstor, *(sth.m_qstor), skip);
        }
        catch(...) {
          this->do_destroy_storage(qstor);
          throw;
        }

        // Set up the new storage.
        this->do_reset(qstor);
        return qstor->data;
      }

    void
    reallocate_finish(storage_handle& sth)
      {
        auto qstor = this->m_qstor;
        ROCKET_ASSERT(qstor);

        // Copy/move old elements from `sth`.
        if(qstor->nskip != 0)
          storage_traits<allocator_type, storage>::dispatch_transfer(*qstor, *(sth.m_qstor), 0);

        ROCKET_ASSERT(qstor->nskip == 0);
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

template<typename vectorT, typename valueT>
class vector_iterator
  {
    template<typename, typename>
    friend class vector_iterator;

    friend vectorT;

  public:
    using iterator_category  = ::std::random_access_iterator_tag;
    using value_type         = typename remove_cv<valueT>::type;
    using pointer            = valueT*;
    using reference          = valueT&;
    using difference_type    = ptrdiff_t;

  private:
    valueT* m_begin;
    valueT* m_cur;
    valueT* m_end;

  private:
    // This constructor is called by the container.
    constexpr
    vector_iterator(valueT* begin, size_t ncur, size_t nend) noexcept
      : m_begin(begin), m_cur(begin + ncur), m_end(begin + nend)
      { }

  public:
    constexpr
    vector_iterator() noexcept
      : m_begin(), m_cur(), m_end()
      { }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_convertible<yvalueT*, valueT*>::value)>
    constexpr
    vector_iterator(const vector_iterator<vectorT, yvalueT>& other) noexcept
      : m_begin(other.m_begin),
        m_cur(other.m_cur),
        m_end(other.m_end)
      { }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_convertible<yvalueT*, valueT*>::value)>
    vector_iterator&
    operator=(const vector_iterator<vectorT, yvalueT>& other) noexcept
      { this->m_begin = other.m_begin;
        this->m_cur = other.m_cur;
        this->m_end = other.m_end;
        return *this;  }

  private:
    valueT*
    do_validate(valueT* cur, bool deref) const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        ROCKET_ASSERT_MSG((this->m_begin <= cur) && (cur <= this->m_end), "iterator out of range");
        ROCKET_ASSERT_MSG(!deref || (cur < this->m_end), "past-the-end iterator not dereferenceable");
        return cur;
      }

  public:
    reference
    operator*() const noexcept
      { return *(this->do_validate(this->m_cur, true));  }

    reference
    operator[](difference_type off) const noexcept
      { return *(this->do_validate(this->m_cur + off, true));  }

    pointer
    operator->() const noexcept
      { return ::std::addressof(**this);  }

    vector_iterator&
    operator+=(difference_type off) noexcept
      {
        this->m_cur = this->do_validate(this->m_cur + off, false);
        return *this;
      }

    vector_iterator&
    operator-=(difference_type off) noexcept
      {
        this->m_cur = this->do_validate(this->m_cur - off, false);
        return *this;
      }

    vector_iterator
    operator+(difference_type off) const noexcept
      {
        auto res = *this;
        res += off;
        return res;
      }

    vector_iterator
    operator-(difference_type off) const noexcept
      {
        auto res = *this;
        res -= off;
        return res;
      }

    vector_iterator&
    operator++() noexcept
      { return *this += 1;  }

    vector_iterator&
    operator--() noexcept
      { return *this -= 1;  }

    vector_iterator
    operator++(int) noexcept
      { return ::std::exchange(*this, *this + 1);  }

    vector_iterator
    operator--(int) noexcept
      { return ::std::exchange(*this, *this - 1);  }

    friend
    vector_iterator
    operator+(difference_type off, const vector_iterator& other) noexcept
      { return other + off;  }

    template<typename yvalueT>
    difference_type
    operator-(const vector_iterator<vectorT, yvalueT>& other) const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        ROCKET_ASSERT_MSG(this->m_begin == other.m_begin, "iterator not compatible");
        ROCKET_ASSERT_MSG(this->m_end == other.m_end, "iterator not compatible");
        return this->m_cur - other.m_cur;
      }

    template<typename yvalueT>
    constexpr bool
    operator==(const vector_iterator<vectorT, yvalueT>& other) const noexcept
      { return this->m_cur == other.m_cur;  }

    template<typename yvalueT>
    constexpr bool
    operator!=(const vector_iterator<vectorT, yvalueT>& other) const noexcept
      { return this->m_cur != other.m_cur;  }

    template<typename yvalueT>
    bool
    operator<(const vector_iterator<vectorT, yvalueT>& other) const noexcept
      { return *this - other < 0;  }

    template<typename yvalueT>
    bool
    operator>(const vector_iterator<vectorT, yvalueT>& other) const noexcept
      { return *this - other > 0;  }

    template<typename yvalueT>
    bool
    operator<=(const vector_iterator<vectorT, yvalueT>& other) const noexcept
      { return *this - other <= 0;  }

    template<typename yvalueT>
    bool
    operator>=(const vector_iterator<vectorT, yvalueT>& other) const noexcept
      { return *this - other >= 0;  }
  };

}  // namespace details_cow_vector
