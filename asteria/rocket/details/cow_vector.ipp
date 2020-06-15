// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_VECTOR_HPP_
#  error Please include <rocket/cow_vector.hpp> instead.
#endif

namespace details_cow_vector {

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
struct basic_storage : storage_header
  {
    using allocator_type   = allocT;
    using value_type       = typename allocator_type::value_type;
    using size_type        = typename allocator_traits<allocator_type>::size_type;

    static constexpr
    size_type
    min_nblk_for_nelem(size_type nelem)
    noexcept
      { return (sizeof(value_type) * nelem + sizeof(basic_storage) - 1) / sizeof(basic_storage) + 1;  }

    static constexpr
    size_type
    max_nelem_for_nblk(size_type nblk)
    noexcept
      { return sizeof(basic_storage) * (nblk - 1) / sizeof(value_type);  }

    allocator_type alloc;
    size_type nblk;
    value_type data[0];

    basic_storage(void (*xdtor)(...), const allocator_type& xalloc, size_type xnblk)
    noexcept
      : storage_header(xdtor), alloc(xalloc), nblk(xnblk)
      { this->nelem = 0;  }

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

    basic_storage&
    operator=(const basic_storage&)
      = delete;
  };

template<typename ptrT, typename allocT, typename ignoreT>
[[noreturn]] inline
void
dispatch_copy_storage(false_type, ignoreT&&, ptrT /*ptr*/, ptrT /*ptr_old*/, size_t /*off*/, size_t /*cnt*/)
  {
    // Throw an exception unconditionally, even when there is nothing to copy.
    noadl::sprintf_and_throw<domain_error>("cow_vector: `%s` not copy-constructible",
                                           typeid(typename allocT::value_type).name());
  }

template<typename ptrT, typename allocT>
inline void
dispatch_copy_storage(true_type, true_type, ptrT ptr, ptrT ptr_old, size_t off, size_t cnt)
  {
    // Optimize it using `std::memcpy()`, as the source and destination locations can't overlap.
    auto nelem = ptr->nelem;
    auto cap = basic_storage<allocT>::max_nelem_for_nblk(ptr->nblk);
    ROCKET_ASSERT(cnt <= cap - nelem);
    ::std::memcpy(ptr->data + nelem, ptr_old->data + off, cnt * sizeof(typename allocT::value_type));
    ptr->nelem = (nelem += cnt);
  }

template<typename ptrT, typename allocT>
inline
void
dispatch_copy_storage(true_type, false_type, ptrT ptr, ptrT ptr_old, size_t off, size_t cnt)
  {
    // Copy elements one by one.
    auto nelem = ptr->nelem;
    auto cap = basic_storage<allocT>::max_nelem_for_nblk(ptr->nblk);
    ROCKET_ASSERT(cnt <= cap - nelem);
    for(size_t i = off; i != off + cnt; ++i) {
      allocator_traits<allocT>::construct(ptr->alloc, ptr->data + nelem, ptr_old->data[i]);
      ptr->nelem = ++nelem;
    }
  }

template<typename ptrT, typename allocT>
inline
void
copy_storage(ptrT ptr, ptrT ptr_old, size_t off, size_t cnt)
  {
    dispatch_copy_storage<ptrT, allocT>(
       is_copy_constructible<typename allocT::value_type>(),  // copyable
       conjunction<is_trivially_copy_constructible<typename allocT::value_type>,
                   is_std_allocator<allocT>>(),  // memcpy
       ptr, ptr_old, off, cnt);
  }

template<typename ptrT, typename allocT>
inline
void
dispatch_move_storage(true_type, ptrT ptr, ptrT ptr_old, size_t off, size_t cnt)
  {
    // Optimize it using `std::memcpy()`, as the source and destination locations can't overlap.
    auto nelem = ptr->nelem;
    auto cap = basic_storage<allocT>::max_nelem_for_nblk(ptr->nblk);
    ROCKET_ASSERT(cnt <= cap - nelem);
    ::std::memcpy(ptr->data + nelem, ptr_old->data + off, cnt * sizeof(typename allocT::value_type));
    ptr->nelem = (nelem += cnt);
  }

template<typename ptrT, typename allocT>
inline
void
dispatch_move_storage(false_type, ptrT ptr, ptrT ptr_old, size_t off, size_t cnt)
  {
    // Move elements one by one.
    auto nelem = ptr->nelem;
    auto cap = basic_storage<allocT>::max_nelem_for_nblk(ptr->nblk);
    ROCKET_ASSERT(cnt <= cap - nelem);
    for(size_t i = off; i != off + cnt; ++i) {
      allocator_traits<allocT>::construct(ptr->alloc, ptr->data + nelem, ::std::move(ptr_old->data[i]));
      ptr->nelem = ++nelem;
    }
  }

template<typename ptrT, typename allocT>
inline
void
move_storage(ptrT ptr, ptrT ptr_old, size_t off, size_t cnt)
  {
    dispatch_move_storage<ptrT, allocT>(
       conjunction<is_trivially_move_constructible<typename allocT::value_type>,
                   is_std_allocator<allocT>>(),  // memcpy
       ptr, ptr_old, off, cnt);
  }

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

    storage_pointer m_ptr;

  public:
    explicit constexpr
    storage_handle(const allocator_type& alloc)
    noexcept
      : allocator_base(alloc), m_ptr()
      { }

    explicit constexpr
    storage_handle(allocator_type&& alloc)
    noexcept
      : allocator_base(::std::move(alloc)), m_ptr()
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
    constexpr
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

    size_type
    capacity()
    const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return 0;

        auto cap = storage::max_nelem_for_nblk(ptr->nblk);
        ROCKET_ASSERT(cap > 0);
        return cap;
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

    const value_type*
    data()
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
    size()
    const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return 0;

        return reinterpret_cast<const storage_header*>(ptr)->nelem;
      }

    ROCKET_NOINLINE
    value_type*
    reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
      {
        if(res_arg == 0) {
          // Deallocate the block.
          this->deallocate();
          return nullptr;
        }
        auto cap = this->check_size_add(0, res_arg);

        // Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
        auto nblk = storage::min_nblk_for_nelem(cap);
        storage_allocator st_alloc(this->as_allocator());
        auto ptr = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(noadl::unfancy(ptr)), '*', sizeof(storage) * nblk);
#endif
        auto dtor = reinterpret_cast<void (*)(...)>(storage_handle::do_drop_reference);
        noadl::construct_at(noadl::unfancy(ptr), dtor, this->as_allocator(), nblk);

        // Copy or move elements into the new block.
        auto ptr_old = this->m_ptr;
        if(ROCKET_UNEXPECT(ptr_old)) {
          try {
            // Moving is only viable if the old and new allocators compare equal and the old block
            // is owned exclusively.
            if((ptr_old->alloc == ptr->alloc) && ptr_old->nref.unique()) {
              move_storage<storage_pointer, allocator_type>(ptr, ptr_old,       0, cnt_one);
              move_storage<storage_pointer, allocator_type>(ptr, ptr_old, off_two, cnt_two);
            }
            else {
              copy_storage<storage_pointer, allocator_type>(ptr, ptr_old,       0, cnt_one);
              copy_storage<storage_pointer, allocator_type>(ptr, ptr_old, off_two, cnt_two);
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
      { noadl::xswap(this->m_ptr, other.m_ptr);  }

    constexpr operator
    const storage_handle*()
    const noexcept
      { return this;  }

    operator
    storage_handle*()
    noexcept
      { return this;  }

    value_type*
    mut_data_unchecked()
    noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return nullptr;

        ROCKET_ASSERT(this->unique());
        return ptr->data;
      }

    template<typename... paramsT>
    value_type*
    emplace_back_unchecked(paramsT&&... params)
      {
        ROCKET_ASSERT(this->unique());
        ROCKET_ASSERT(this->size() < this->capacity());
        auto ptr = this->m_ptr;
        ROCKET_ASSERT(ptr);
        auto nelem = ptr->nelem;
        allocator_traits<allocator_type>::construct(ptr->alloc, ptr->data + nelem,
                                                    ::std::forward<paramsT>(params)...);
        ptr->nelem = ++nelem;
        return ptr->data + nelem - 1;
      }

    void
    pop_back_n_unchecked(size_type n)
    noexcept
      {
        ROCKET_ASSERT(this->unique());
        ROCKET_ASSERT(n <= this->size());
        if(n == 0)
          return;

        auto ptr = this->m_ptr;
        ROCKET_ASSERT(ptr);
        auto nelem = ptr->nelem;
        for(size_type i = n; i != 0; --i) {
          ptr->nelem = --nelem;
          allocator_traits<allocator_type>::destroy(ptr->alloc, ptr->data + nelem);
        }
      }
  };

template<typename vectorT, typename valueT>
class vector_iterator
  {
    template<typename, typename>
    friend
    class vector_iterator;

    friend vectorT;

  public:
    using iterator_category  = random_access_iterator_tag;
    using value_type         = typename remove_cv<valueT>::type;
    using pointer            = valueT*;
    using reference          = valueT&;
    using difference_type    = ptrdiff_t;

    using parent_type   = storage_handle<typename vectorT::allocator_type>;

  private:
    const parent_type* m_ref;
    pointer m_ptr;

  private:
    // These constructors are called by the container.
    constexpr
    vector_iterator(const parent_type* ref, pointer ptr)
    noexcept
      : m_ref(ref), m_ptr(ptr)
      { }

  public:
    constexpr
    vector_iterator()
    noexcept
      : vector_iterator(nullptr, nullptr)
      { }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_convertible<yvalueT*, valueT*>::value)>
    constexpr
    vector_iterator(const vector_iterator<vectorT, yvalueT>& other)
    noexcept
      : vector_iterator(other.m_ref, other.m_ptr)
      { }

  private:
    pointer
    do_assert_valid_pointer(pointer ptr, bool deref)
    const noexcept
      {
        auto ref = this->m_ref;
        ROCKET_ASSERT_MSG(ref, "Iterator not initialized");
        auto dist = static_cast<size_t>(ptr - ref->data());
        ROCKET_ASSERT_MSG(dist <= ref->size(), "Iterator invalidated");
        ROCKET_ASSERT_MSG(!deref || (dist < ref->size()), "Past-the-end iterator not dereferenceable");
        return ptr;
      }

  public:
    constexpr
    const parent_type*
    parent()
    const noexcept
      { return this->m_ref;  }

    pointer
    tell()
    const noexcept
      { return this->do_assert_valid_pointer(this->m_ptr, false);  }

    pointer
    tell_owned_by(const parent_type* ref)
    const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_ref == ref, "Iterator not belonging to the same container");
        return this->tell();
      }

    vector_iterator&
    seek(pointer ptr)
    noexcept
      {
        this->m_ptr = this->do_assert_valid_pointer(ptr, false);
        return *this;
      }

    reference
    operator*()
    const noexcept
      {
        auto ptr = this->do_assert_valid_pointer(this->m_ptr, true);
        ROCKET_ASSERT(ptr);
        return *ptr;
      }

    pointer
    operator->()
    const noexcept
      {
        auto ptr = this->do_assert_valid_pointer(this->m_ptr, true);
        ROCKET_ASSERT(ptr);
        return ptr;
      }

    reference
    operator[](difference_type off)
    const noexcept
      {
        auto ptr = this->do_assert_valid_pointer(this->m_ptr + off, true);
        ROCKET_ASSERT(ptr);
        return *ptr;
      }
  };

template<typename vectorT, typename valueT>
inline
vector_iterator<vectorT, valueT>&
operator++(vector_iterator<vectorT, valueT>& rhs)
noexcept
  { return rhs.seek(rhs.tell() + 1);  }

template<typename vectorT, typename valueT>
inline
vector_iterator<vectorT, valueT>&
operator--(vector_iterator<vectorT, valueT>& rhs)
noexcept
  { return rhs.seek(rhs.tell() - 1);  }

template<typename vectorT, typename valueT>
inline
vector_iterator<vectorT, valueT>
operator++(vector_iterator<vectorT, valueT>& lhs, int)
noexcept
  {
    auto res = lhs;
    lhs.seek(lhs.tell() + 1);
    return res;
  }

template<typename vectorT, typename valueT>
inline
vector_iterator<vectorT, valueT>
operator--(vector_iterator<vectorT, valueT>& lhs, int)
noexcept
  {
    auto res = lhs;
    lhs.seek(lhs.tell() - 1);
    return res;
  }

template<typename vectorT, typename valueT>
inline
vector_iterator<vectorT, valueT>&
operator+=(vector_iterator<vectorT, valueT>& lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs)
noexcept
  { return lhs.seek(lhs.tell() + rhs);  }

template<typename vectorT, typename valueT>
inline
vector_iterator<vectorT, valueT>&
operator-=(vector_iterator<vectorT, valueT>& lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs)
noexcept
  { return lhs.seek(lhs.tell() - rhs);  }


template<typename vectorT, typename valueT>
inline
vector_iterator<vectorT, valueT>
operator+(const vector_iterator<vectorT, valueT>& lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs)
noexcept
  {
    auto res = lhs;
    res.seek(res.tell() + rhs);
    return res;
  }

template<typename vectorT, typename valueT>
inline
vector_iterator<vectorT, valueT>
operator-(const vector_iterator<vectorT, valueT>& lhs, typename vector_iterator<vectorT, valueT>::difference_type rhs)
noexcept
  {
    auto res = lhs;
    res.seek(res.tell() - rhs);
    return res;
  }

template<typename vectorT, typename valueT>
inline
vector_iterator<vectorT, valueT>
operator+(typename vector_iterator<vectorT, valueT>::difference_type lhs, const vector_iterator<vectorT, valueT>& rhs)
noexcept
  {
    auto res = rhs;
    res.seek(res.tell() + lhs);
    return res;
  }

template<typename vectorT, typename xvalueT, typename yvalueT>
inline
typename vector_iterator<vectorT, xvalueT>::difference_type
operator-(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  {
    return lhs.tell_owned_by(rhs.parent()) - rhs.tell();
  }

template<typename vectorT, typename xvalueT, typename yvalueT>
inline
bool
operator==(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs.tell() == rhs.tell();  }

template<typename vectorT, typename xvalueT, typename yvalueT>
inline
bool
operator!=(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs.tell() != rhs.tell();  }

template<typename vectorT, typename xvalueT, typename yvalueT>
inline
bool
operator<(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs.tell_owned_by(rhs.parent()) < rhs.tell();  }

template<typename vectorT, typename xvalueT, typename yvalueT>
inline
bool
operator>(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs.tell_owned_by(rhs.parent()) > rhs.tell();  }

template<typename vectorT, typename xvalueT, typename yvalueT>
inline
bool
operator<=(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs.tell_owned_by(rhs.parent()) <= rhs.tell();  }

template<typename vectorT, typename xvalueT, typename yvalueT>
inline
bool
operator>=(const vector_iterator<vectorT, xvalueT>& lhs, const vector_iterator<vectorT, yvalueT>& rhs)
noexcept
  { return lhs.tell_owned_by(rhs.parent()) >= rhs.tell();  }

// Insertion helpers.
struct append_tag
  { }
  constexpr append;

template<typename vectorT, typename... paramsT>
inline
void
tagged_append(vectorT* vec, append_tag, paramsT&&... params)
  { vec->append(::std::forward<paramsT>(params)...);  }

struct emplace_back_tag
  { }
  constexpr emplace_back;

template<typename vectorT, typename... paramsT>
inline
void
tagged_append(vectorT* vec, emplace_back_tag, paramsT&&... params)
  { vec->emplace_back(::std::forward<paramsT>(params)...);  }

struct push_back_tag
  { }
  constexpr push_back;

template<typename vectorT, typename... paramsT>
inline
void
tagged_append(vectorT* vec, push_back_tag, paramsT&&... params)
  { vec->push_back(::std::forward<paramsT>(params)...);  }

}  // namespace details_cow_vector
