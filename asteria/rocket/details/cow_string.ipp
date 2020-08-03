// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRING_HPP_
#  error Please include <rocket/cow_string.hpp> instead.
#endif

namespace details_cow_string {

struct storage_header
  {
    mutable reference_counter<long> nref;

    explicit
    storage_header()
    noexcept
      : nref()
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
    min_nblk_for_nchar(size_type nchar)
    noexcept
      { return (sizeof(value_type) * (nchar + 1) + sizeof(basic_storage) - 1) / sizeof(basic_storage) + 1;  }

    static constexpr
    size_type
    max_nchar_for_nblk(size_type nblk)
    noexcept
      { return sizeof(basic_storage) * (nblk - 1) / sizeof(value_type) - 1;  }

    allocator_type alloc;
    size_type nblk;
    value_type data[0];

    basic_storage(const allocator_type& xalloc, size_type xnblk)
    noexcept
      : alloc(xalloc), nblk(xnblk)
      { }

    ~basic_storage()
      { }

    basic_storage(const basic_storage&)
      = delete;

    basic_storage&
    operator=(const basic_storage&)
      = delete;
  };

template<typename allocT, typename traitsT>
class storage_handle
  : private allocator_wrapper_base_for<allocT>::type
  {
  public:
    using allocator_type   = allocT;
    using traits_type      = traitsT;
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

        storage_handle::do_drop_reference(ptr);
      }

    static
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

        auto cap = storage::max_nchar_for_nblk(ptr->nblk);
        ROCKET_ASSERT(cap > 0);
        return cap;
      }

    size_type
    max_size()
    const noexcept
      {
        storage_allocator st_alloc(this->as_allocator());
        auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
        return storage::max_nchar_for_nblk(max_nblk / 2);
      }

    size_type
    check_size_add(size_type base, size_type add)
    const
      {
        auto nmax = this->max_size();
        ROCKET_ASSERT(base <= nmax);
        if(nmax - base < add)
          noadl::sprintf_and_throw<length_error>("cow_string: max size exceeded (`%lld` + `%lld` > `%lld`)",
                                                 static_cast<long long>(base), static_cast<long long>(add),
                                                 static_cast<long long>(nmax));
        return base + add;
      }

    size_type
    round_up_capacity(size_type res_arg)
    const
      {
        auto cap = this->check_size_add(0, res_arg);
        auto nblk = storage::min_nblk_for_nchar(cap);
        return storage::max_nchar_for_nblk(nblk);
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

    ROCKET_NOINLINE
    value_type*
    reallocate(const value_type* src, size_type len_one, size_type off_two, size_type len_two, size_type res_arg)
      {
        if(res_arg == 0) {
          // Deallocate the block.
          this->deallocate();
          return nullptr;
        }
        auto cap = this->check_size_add(0, res_arg);

        // Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
        auto nblk = storage::min_nblk_for_nchar(cap);
        storage_allocator st_alloc(this->as_allocator());
        auto ptr = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(noadl::unfancy(ptr)), '*', sizeof(storage) * nblk);
#endif
        noadl::construct_at(noadl::unfancy(ptr), this->as_allocator(), nblk);

        // Copy characters into the new block.
        size_type len = 0;
        if(len_one | len_two) {
          // The two blocks will not overlap.
          ROCKET_ASSERT(len_one <= cap - len);
          traits_type::copy(ptr->data + len, src, len_one);
          len += len_one;

          ROCKET_ASSERT(len_two <= cap - len);
          traits_type::copy(ptr->data + len, src + off_two, len_two);
          len += len_two;
        }
        // Add a null character.
        traits_type::assign(ptr->data[len], value_type());

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
          ptr->nref.increment();
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
  };

template<typename stringT, typename charT>
class string_iterator
  {
    template<typename, typename>
    friend
    class string_iterator;

    friend stringT;

  public:
    using iterator_category  = random_access_iterator_tag;
    using value_type         = typename remove_cv<charT>::type;
    using pointer            = charT*;
    using reference          = charT&;
    using difference_type    = ptrdiff_t;

    using parent_type   = stringT;

  private:
    const parent_type* m_ref;
    pointer m_ptr;

  private:
    // These constructors are called by the container.
    constexpr
    string_iterator(const parent_type* ref, pointer ptr)
    noexcept
      : m_ref(ref), m_ptr(ptr)
      { }

  public:
    constexpr
    string_iterator()
    noexcept
      : string_iterator(nullptr, nullptr)
      { }

    template<typename ycharT,
    ROCKET_ENABLE_IF(is_convertible<ycharT*, charT*>::value)>
    constexpr
    string_iterator(const string_iterator<stringT, ycharT>& other)
    noexcept
      : string_iterator(other.m_ref, other.m_ptr)
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
        ROCKET_ASSERT_MSG(!deref || (dist < ref->size()),
                          "Past-the-end iterator not dereferenceable");
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
        ROCKET_ASSERT_MSG(this->m_ref == ref, "Dangling iterator");
        return this->tell();
      }

    string_iterator&
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

template<typename stringT, typename charT>
inline
string_iterator<stringT, charT>&
operator++(string_iterator<stringT, charT>& rhs)
noexcept
  { return rhs.seek(rhs.tell() + 1);  }

template<typename stringT, typename charT>
inline
string_iterator<stringT, charT>&
operator--(string_iterator<stringT, charT>& rhs)
noexcept
  { return rhs.seek(rhs.tell() - 1);  }

template<typename stringT, typename charT>
inline
string_iterator<stringT, charT>
operator++(string_iterator<stringT, charT>& lhs, int)
noexcept
  {
    auto res = lhs;
    lhs.seek(lhs.tell() + 1);
    return res;
  }

template<typename stringT, typename charT>
inline
string_iterator<stringT, charT>
operator--(string_iterator<stringT, charT>& lhs, int)
noexcept
  {
    auto res = lhs;
    lhs.seek(lhs.tell() - 1);
    return res;
  }

template<typename stringT, typename charT>
inline
string_iterator<stringT, charT>&
operator+=(string_iterator<stringT, charT>& lhs, typename string_iterator<stringT, charT>::difference_type rhs)
noexcept
  { return lhs.seek(lhs.tell() + rhs);  }

template<typename stringT, typename charT>
inline
string_iterator<stringT, charT>&
operator-=(string_iterator<stringT, charT>& lhs, typename string_iterator<stringT, charT>::difference_type rhs)
noexcept
  { return lhs.seek(lhs.tell() - rhs);  }

template<typename stringT, typename charT>
inline
string_iterator<stringT, charT>
operator+(const string_iterator<stringT, charT>& lhs, typename string_iterator<stringT, charT>::difference_type rhs)
noexcept
  {
    auto res = lhs;
    res.seek(res.tell() + rhs);
    return res;
  }

template<typename stringT, typename charT>
inline
string_iterator<stringT, charT>
operator-(const string_iterator<stringT, charT>& lhs, typename string_iterator<stringT, charT>::difference_type rhs)
noexcept
  {
    auto res = lhs;
    res.seek(res.tell() - rhs);
    return res;
  }

template<typename stringT, typename charT>
inline
string_iterator<stringT, charT>
operator+(typename string_iterator<stringT, charT>::difference_type lhs, const string_iterator<stringT, charT>& rhs)
noexcept
  {
    auto res = rhs;
    res.seek(res.tell() + lhs);
    return res;
  }

template<typename stringT, typename xcharT, typename ycharT>
inline
typename string_iterator<stringT, xcharT>::difference_type
operator-(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  {
    return lhs.tell_owned_by(rhs.parent()) - rhs.tell();
  }

template<typename stringT, typename xcharT, typename ycharT>
inline
bool
operator==(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs.tell() == rhs.tell();  }

template<typename stringT, typename xcharT, typename ycharT>
inline
bool
operator!=(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs.tell() != rhs.tell();  }

template<typename stringT, typename xcharT, typename ycharT>
inline
bool
operator<(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs.tell_owned_by(rhs.parent()) < rhs.tell();  }

template<typename stringT, typename xcharT, typename ycharT>
inline
bool
operator>(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs.tell_owned_by(rhs.parent()) > rhs.tell();  }

template<typename stringT, typename xcharT, typename ycharT>
inline
bool
operator<=(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs.tell_owned_by(rhs.parent()) <= rhs.tell();  }

template<typename stringT, typename xcharT, typename ycharT>
inline
bool
operator>=(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs.tell_owned_by(rhs.parent()) >= rhs.tell();  }

// Implement relational operators.
template<typename charT, typename traitsT>
struct comparator
  {
    using char_type    = charT;
    using traits_type  = traitsT;
    using size_type    = size_t;

    static
    int
    inequality(const char_type* s1, size_type n1, const char_type* s2, size_type n2)
    noexcept
      {
        if(n1 != n2)
          return 2;

        if(s1 == s2)
          return 0;

        return traits_type::compare(s1, s2, n1);
      }

    static
    int
    relation(const char_type* s1, size_type n1, const char_type* s2, size_type n2)
    noexcept
      {
        if(n1 < n2)
          return (traits_type::compare(s1, s2, n1) > 0) ? +1 : -1;

        if(n1 > n2)
          return (traits_type::compare(s1, s2, n1) < 0) ? -1 : +1;

        return traits_type::compare(s1, s2, n1);
      }
  };

// Replacement helpers.
struct append_tag
  { }
  constexpr append;

template<typename stringT, typename... paramsT>
inline
void
tagged_append(stringT* str, append_tag, paramsT&&... params)
  { str->append(::std::forward<paramsT>(params)...);  }

struct push_back_tag
  { }
  constexpr push_back;

template<typename stringT, typename... paramsT>
inline
void
tagged_append(stringT* str, push_back_tag, paramsT&&... params)
  { str->push_back(::std::forward<paramsT>(params)...);  }

// Implement the FNV-1a hash algorithm.
template<typename charT, typename traitsT>
class basic_hasher
  {
  private:
    static constexpr char32_t xoffset = 0x811C9DC5;
    static constexpr char32_t xprime = 0x1000193;

    char32_t m_reg = xoffset;

  public:
    constexpr
    basic_hasher&
    append(const charT& c)
    noexcept
      {
        char32_t word = static_cast<char32_t>(c);
        char32_t reg = this->m_reg;

        for(size_t k = 0;  k < sizeof(c);  ++k)
          reg = (reg ^ ((word >> k * 8) & 0xFF)) * xprime;

        this->m_reg = reg;
        return *this;
      }

    constexpr
    basic_hasher&
    append(const charT* s, size_t n)
      {
        for(auto sp = s;  sp != s + n;  ++sp)
          this->append(*sp);
        return *this;
      }

    constexpr
    basic_hasher&
    append(const charT* s)
      {
        for(auto sp = s;  !traitsT::eq(*sp, charT());  ++sp)
          this->append(*sp);
        return *this;
      }

    constexpr
    size_t
    finish()
    noexcept
      {
        char32_t reg = this->m_reg;
        this->m_reg = xoffset;
        return reg;
      }
  };

}  // namespace details_cow_string
