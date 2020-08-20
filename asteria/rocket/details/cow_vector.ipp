// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_VECTOR_HPP_
#  error Please include <rocket/cow_vector.hpp> instead.
#endif

namespace details_cow_vector {

struct storage_header
  {
    mutable reference_counter<long> nref;

    void (*dtor)(...);
    size_t nelem;

    explicit
    storage_header()
    noexcept
      : nref()
      { }
  };

template<typename allocT, typename storageT>
struct storage_traits
  {
    using allocator_type   = allocT;
    using storage_type     = storageT;
    using value_type       = typename allocator_type::value_type;

    [[noreturn]] static
    void
    do_transfer(false_type,   // 1. movable?
                bool,         // 2. trival?
                bool,         // 3. copyable?
                storage_type&, storage_type&)
      {
        noadl::sprintf_and_throw<domain_error>("cow_vector: `%s` not move-constructible",
                                               typeid(value_type).name());
      }

    static
    void
    do_transfer(true_type,    // 1. movable?
                true_type,    // 2. trival?
                bool,         // 3. copyable?
                storage_type& st_new, storage_type& st_old)
      {
        ::std::memcpy(st_new.data + st_new.nelem,
                      st_old.data, st_old.nelem * sizeof(value_type));
        st_new.nelem += st_old.nelem;
      }

    static
    void
    do_transfer(true_type,    // 1. movable?
                false_type,   // 2. trivial?
                true_type,    // 3. copyable?
                storage_type& st_new, storage_type& st_old)
      {
        if(st_old.nref.unique())
          for(size_t k = 0;  k != st_old.nelem;  ++k)
            allocator_traits<allocator_type>::construct(st_new.alloc,
                      st_new.data + st_new.nelem, ::std::move(st_old.data[k])),
            st_new.nelem += 1;
        else
          for(size_t k = 0;  k != st_old.nelem;  ++k)
            allocator_traits<allocator_type>::construct(st_new.alloc,
                      st_new.data + st_new.nelem, st_old.data[k]),
            st_new.nelem += 1;
      }

    static
    void
    do_transfer(true_type,    // 1. movable?
                false_type,   // 2. trivial?
                false_type,   // 3. copyable?
                storage_type& st_new, storage_type& st_old)
      {
        if(st_old.nref.unique())
          for(size_t k = 0;  k != st_old.nelem;  ++k)
            allocator_traits<allocator_type>::construct(st_new.alloc,
                      st_new.data + st_new.nelem, ::std::move(st_old.data[k])),
            st_new.nelem += 1;
        else
          noadl::sprintf_and_throw<domain_error>("cow_vector: `%s` not copy-constructible",
                                                 typeid(value_type).name());
      }

    static
    void
    dispatch_transfer(storage_type& st_new, storage_type& st_old)
      {
        return do_transfer(
            ::std::is_move_constructible<value_type>(),                // 1. movable
            conjunction<is_std_allocator<allocator_type>,
                        ::std::is_trivially_copyable<value_type>>(),   // 2. trivial
            ::std::is_copy_constructible<value_type>(),                // 3. copyable
            st_new, st_old);
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
    struct storage : storage_header
      {
        static constexpr
        size_type
        min_nblk_for_nelem(size_type nelem)
        noexcept
          { return (sizeof(value_type) * nelem + sizeof(storage) - 1) / sizeof(storage) + 1;  }

        static constexpr
        size_type
        max_nelem_for_nblk(size_type nblk)
        noexcept
          { return sizeof(storage) * (nblk - 1) / sizeof(value_type);  }

        allocator_type alloc;
        size_type nblk;
        value_type data[0];

        storage(void xdtor(...), const allocator_type& xalloc, size_type xnblk)
        noexcept
          : alloc(xalloc), nblk(xnblk)
          {
            this->dtor = xdtor;
            this->nelem = 0;

#ifdef ROCKET_DEBUG
            ::std::memset(static_cast<void*>(this->data), '*', sizeof(storage) * (this->nblk - 1));
#endif
          }

        ~storage()
          {
            // Destroy all elements backwards.
            size_type off = this->nelem;
            while(off-- != 0)
              allocator_traits<allocator_type>::destroy(this->alloc, this->data + off);

#ifdef ROCKET_DEBUG
            this->nelem = static_cast<size_type>(0xBAD1BEEF);
            ::std::memset(static_cast<void*>(this->data), '~', sizeof(storage) * (this->nblk - 1));
#endif
          }

        storage(const storage&)
          = delete;

        storage&
        operator=(const storage&)
          = delete;
      };

    using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;
    using storage_allocator = typename allocator_traits<allocator_type>::template rebind_alloc<storage>;
    using storage_pointer   = typename allocator_traits<storage_allocator>::pointer;

  private:
    storage_pointer m_qstor;

  public:
    explicit constexpr
    storage_handle(const allocator_type& alloc)
    noexcept
      : allocator_base(alloc), m_qstor()
      { }

    explicit constexpr
    storage_handle(allocator_type&& alloc)
    noexcept
      : allocator_base(::std::move(alloc)), m_qstor()
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
    do_reset(storage_pointer qstor_new)
    noexcept
      {
        // Decrement the reference count with acquire-release semantics to prevent races
        // on `qstor->alloc`.
        auto qstor = ::std::exchange(this->m_qstor, qstor_new);
        if(ROCKET_EXPECT(!qstor))
          return;

        auto hdr = reinterpret_cast<const storage_header*>(noadl::unfancy(qstor));
        if(ROCKET_EXPECT(!hdr->nref.decrement()))
          return;

        // This indirect call is paramount for incomplete type support.
        reinterpret_cast<void (*)(storage_pointer)>(hdr->dtor)(qstor);
      }

    ROCKET_NOINLINE static
    void
    do_destroy_storage(storage_pointer qstor)
    noexcept
      {
        auto nblk = qstor->nblk;
        storage_allocator st_alloc(qstor->alloc);
        noadl::destroy_at(noadl::unfancy(qstor));
        allocator_traits<storage_allocator>::deallocate(st_alloc, qstor, nblk);
      }

  public:
    constexpr
    const allocator_type&
    as_allocator()
    const noexcept
      { return static_cast<const allocator_base&>(*this);  }

    allocator_type&
    as_allocator()
    noexcept
      { return static_cast<allocator_base&>(*this);  }

    ROCKET_PURE_FUNCTION
    bool
    unique()
    const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return false;
        return qstor->nref.unique();
      }

    long
    use_count()
    const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return qstor->nref.get();
      }

    ROCKET_PURE_FUNCTION
    size_type
    capacity()
    const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return storage::max_nelem_for_nblk(qstor->nblk);
      }

    size_type
    max_size()
    const noexcept
      {
        storage_allocator st_alloc(this->as_allocator());
        auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
        return storage::max_nelem_for_nblk(max_nblk / 2);
      }

    size_type
    check_size_add(size_type base, size_type add)
    const
      {
        auto nmax = this->max_size();
        ROCKET_ASSERT(base <= nmax);
        if(nmax - base < add)
          noadl::sprintf_and_throw<length_error>("cow_vector: max size exceeded (`%lld` + `%lld` > `%lld`)",
                                                 static_cast<long long>(base), static_cast<long long>(add),
                                                 static_cast<long long>(nmax));
        return base + add;
      }

    size_type
    round_up_capacity(size_type res_arg)
    const
      {
        auto cap = this->check_size_add(0, res_arg);
        auto nblk = storage::min_nblk_for_nelem(cap);
        return storage::max_nelem_for_nblk(nblk);
      }

    ROCKET_PURE_FUNCTION
    const value_type*
    data()
    const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return reinterpret_cast<value_type*>(-1);
        return qstor->data;
      }

    value_type*
    mut_data_opt()
    noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor || !qstor->nref.unique())
          return nullptr;
        return qstor->data;
      }

    ROCKET_PURE_FUNCTION
    size_type
    size()
    const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return reinterpret_cast<storage_header*>(noadl::unfancy(qstor))->nelem;
      }

    template<typename... paramsT>
    value_type&
    emplace_back_unchecked(paramsT&&... params)
      {
        auto qstor = this->m_qstor;
        ROCKET_ASSERT_MSG(qstor, "No storage allocated");

        size_type off = qstor->nelem;
        ROCKET_ASSERT_MSG(off < this->capacity(), "No space for new elements");

        allocator_traits<allocator_type>::construct(qstor->alloc, qstor->data + off,
                                                    ::std::forward<paramsT>(params)...);
        qstor->nelem = off + 1;

        return qstor->data[off];
      }

    void
    pop_back_unchecked()
    noexcept
      {
        auto qstor = this->m_qstor;
        ROCKET_ASSERT_MSG(qstor, "No storage allocated");

        size_type off = qstor->nelem;
        ROCKET_ASSERT_MSG(off > 0, "No element to pop");

        off -= 1;
        qstor->nelem = off;
        allocator_traits<allocator_type>::destroy(qstor->alloc, qstor->data + off);
      }

    ROCKET_NOINLINE
    value_type*
    reallocate_more(const storage_handle& sth, size_type add)
      {
        // Calculate the combined length of vector (sth.size() + add).
        // The first part is copied/moved from `sth`. The second part is left uninitialized.
        auto len = sth.size();
        auto cap = this->check_size_add(len, add);

        // Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
        auto nblk = storage::min_nblk_for_nelem(cap);
        storage_allocator st_alloc(this->as_allocator());
        auto qstor = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
        noadl::construct_at(noadl::unfancy(qstor), reinterpret_cast<void (*)(...)>(this->do_destroy_storage),
                            this->as_allocator(), nblk);

        if(len) {
          try {
            // Copy/move old elements from `sth`.
            ROCKET_ASSERT(sth.m_qstor);
            storage_traits<allocator_type, storage>::dispatch_transfer(*qstor, *(sth.m_qstor));
          }
          catch(...) {
            this->do_destroy_storage(qstor);
            throw;
          }
        }

        // Set up the new storage.
        this->do_reset(qstor);
        return qstor->data + len;
      }

    void
    deallocate()
    noexcept
      { this->do_reset(nullptr);  }

    void
    share_with(const storage_handle& other)
    noexcept
      {
        auto qstor = other.m_qstor;
        if(qstor)
          reinterpret_cast<storage_header*>(noadl::unfancy(qstor))->nref.increment();
        this->do_reset(qstor);
      }

    void
    exchange_with(storage_handle& other)
    noexcept
      { ::std::swap(this->m_qstor, other.m_qstor);  }

    constexpr operator
    const storage_handle*()
    const noexcept
      { return this;  }

    operator
    storage_handle*()
    noexcept
      { return this;  }
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
    pointer m_begin;
    pointer m_cur;
    pointer m_end;

  private:
    // This constructor is called by the container.
    constexpr
    vector_iterator(valueT* begin, size_t ncur, size_t nend)
    noexcept
      : m_begin(begin), m_cur(begin + ncur), m_end(begin + nend)
      { }

  public:
    constexpr
    vector_iterator()
    noexcept
      : m_begin(), m_cur(), m_end()
      { }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_convertible<yvalueT*, valueT*>::value)>
    constexpr
    vector_iterator(const vector_iterator<vectorT, yvalueT>& other)
    noexcept
      : m_begin(other.m_begin), m_cur(other.m_cur), m_end(other.m_end)
      { }

  private:
    pointer
    do_validate(pointer cur, bool deref)
    const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "Iterator not initialized");
        ROCKET_ASSERT_MSG((this->m_begin <= cur) && (cur <= this->m_end), "Iterator out of range");
        ROCKET_ASSERT_MSG(!deref || (cur < this->m_end), "Past-the-end iterator not dereferenceable");
        return cur;
      }

  public:
    reference
    operator*()
    const noexcept
      { return *(this->do_validate(this->m_cur, true));  }

    reference
    operator[](difference_type off)
    const noexcept
      { return *(this->do_validate(this->m_cur + off, true));  }

    pointer
    operator->()
    const noexcept
      { return this->do_validate(this->m_cur, true);  }

    vector_iterator&
    operator+=(difference_type off)
    noexcept
      {
        this->m_cur = this->do_validate(this->m_cur + off, false);
        return *this;
      }

    vector_iterator
    operator+(difference_type off)
    const noexcept
      {
        auto res = *this;
        res += off;
        return res;
      }

    vector_iterator&
    operator-=(difference_type off)
    noexcept
      {
        this->m_cur = this->do_validate(this->m_cur - off, false);
        return *this;
      }

    vector_iterator
    operator-(difference_type off)
    const noexcept
      {
        auto res = *this;
        res -= off;
        return res;
      }

    friend
    vector_iterator
    operator+(difference_type off, const vector_iterator& other)
    noexcept
      { return other + off;  }

    template<typename yvalueT>
    difference_type
    operator-(const vector_iterator<vectorT, yvalueT>& other)
    const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "Iterator not initialized");
        ROCKET_ASSERT_MSG(this->m_begin == other.m_begin, "Iterator not compatible");
        return this->m_cur - other.m_cur;
      }
  };

template<typename vectorT, typename valueT>
vector_iterator<vectorT, valueT>&
operator++(vector_iterator<vectorT, valueT>& lhs)
noexcept
  { return lhs += 1;  }

template<typename vectorT, typename valueT>
vector_iterator<vectorT, valueT>
operator++(vector_iterator<vectorT, valueT>& lhs, int)
noexcept
  { return ::std::exchange(lhs, lhs + 1);  }

template<typename vectorT, typename valueT>
vector_iterator<vectorT, valueT>&
operator--(vector_iterator<vectorT, valueT>& lhs)
noexcept
  { return lhs -= 1;  }

template<typename vectorT, typename valueT>
vector_iterator<vectorT, valueT>
operator--(vector_iterator<vectorT, valueT>& lhs, int)
noexcept
  { return ::std::exchange(lhs, lhs - 1);  }

template<typename vectorT, typename xvalueT, typename yvalueT>
bool
operator==(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs - rhs == 0;  }

template<typename vectorT, typename xvalueT, typename yvalueT>
bool
operator!=(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs - rhs != 0;  }

template<typename vectorT, typename xvalueT, typename yvalueT>
bool
operator<(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs - rhs < 0;  }

template<typename vectorT, typename xvalueT, typename yvalueT>
bool
operator>(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs - rhs > 0;  }

template<typename vectorT, typename xvalueT, typename yvalueT>
bool
operator<=(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs - rhs <= 0;  }

template<typename vectorT, typename xvalueT, typename yvalueT>
bool
operator>=(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs - rhs >= 0;  }

}  // namespace details_cow_vector
