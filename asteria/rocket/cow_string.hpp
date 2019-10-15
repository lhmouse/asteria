// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRING_HPP_
#define ROCKET_COW_STRING_HPP_

#include "compiler.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "char_traits.hpp"
#include "allocator_utilities.hpp"
#include "reference_counter.hpp"
#include <iterator>  // std::iterator_traits<>, std::random_access_iterator_tag
#include <cstring>  // std::memset()

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>,
         typename allocT = allocator<charT>> class basic_cow_string;

/* Differences from `std::basic_string`:
 * 1. All functions guarantee only basic exception safety rather than strong exception safety, hence are more efficient.
 * 2. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always return `const_reference`s.
 * 3. The copy constructor and copy assignment operator will not throw exceptions.
 * 4. The constructor taking a sole const pointer is made `explicit`.
 * 5. The assignment operator taking a character and the one taking a const pointer are not provided.
 * 6. It is possible to create strings holding non-owning references of null-terminated character arrays allocated externally.
 * 7. `data()` returns a null pointer if the string is empty.
 */

template<typename charT, typename traitsT> class basic_tinyfmt;

    namespace details_cow_string {

    template<typename charT, typename traitsT> class shallow
      {
      private:
        const charT* m_ptr;
        size_t m_len;

      public:
        explicit constexpr shallow(const charT* ptr) noexcept
          :
            m_ptr(ptr), m_len(traitsT::length(ptr))
          { }
        constexpr shallow(const charT* ptr, size_t len) noexcept
          :
            m_ptr(ptr), m_len((ROCKET_ASSERT(traitsT::eq(ptr[len], charT())), len))
          { }
        template<typename allocT> explicit shallow(const basic_cow_string<charT, traitsT, allocT>& str) noexcept
          :
            m_ptr(str.c_str()), m_len(str.length())
          { }

      public:
        constexpr const charT* c_str() const noexcept
          {
            return this->m_ptr;
          }
        constexpr size_t length() const noexcept
          {
            return this->m_len;
          }
      };

    struct storage_header
      {
        mutable reference_counter<long> nref;

        explicit storage_header() noexcept
          :
            nref()
          { }
      };

    template<typename allocT> struct basic_storage : storage_header
      {
        using allocator_type   = allocT;
        using value_type       = typename allocator_type::value_type;
        using size_type        = typename allocator_traits<allocator_type>::size_type;

        static constexpr size_type min_nblk_for_nchar(size_type nchar) noexcept
          {
            return (sizeof(value_type) * (nchar + 1) + sizeof(basic_storage) - 1) / sizeof(basic_storage) + 1;
          }
        static constexpr size_type max_nchar_for_nblk(size_type nblk) noexcept
          {
            return sizeof(basic_storage) * (nblk - 1) / sizeof(value_type) - 1;
          }

        allocator_type alloc;
        size_type nblk;
        union { value_type data[0];  };

        basic_storage(const allocator_type& xalloc, size_type xnblk) noexcept
          :
            alloc(xalloc), nblk(xnblk)
          { }
        ~basic_storage()
          { }

        basic_storage(const basic_storage&)
          = delete;
        basic_storage& operator=(const basic_storage&)
          = delete;
      };

    template<typename allocT, typename traitsT>
        class storage_handle : private allocator_wrapper_base_for<allocT>::type
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
        explicit constexpr storage_handle(const allocator_type& alloc) noexcept
          :
            allocator_base(alloc),
            m_ptr()
          { }
        explicit constexpr storage_handle(allocator_type&& alloc) noexcept
          :
            allocator_base(noadl::move(alloc)),
            m_ptr()
          { }
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
            storage_handle::do_drop_reference(ptr);
          }

        static void do_drop_reference(storage_pointer ptr) noexcept
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
        size_type capacity() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return 0;
            }
            auto cap = storage::max_nchar_for_nblk(ptr->nblk);
            ROCKET_ASSERT(cap > 0);
            return cap;
          }
        size_type max_size() const noexcept
          {
            storage_allocator st_alloc(this->as_allocator());
            auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
            return storage::max_nchar_for_nblk(max_nblk / 2);
          }
        size_type check_size_add(size_type base, size_type add) const
          {
            auto nmax = this->max_size();
            ROCKET_ASSERT(base <= nmax);
            if(nmax - base < add) {
              noadl::sprintf_and_throw<length_error>("basic_cow_string: max size exceeded (`%llu` + `%llu` > `%llu`)",
                                                     static_cast<unsigned long long>(base), static_cast<unsigned long long>(add),
                                                     static_cast<unsigned long long>(nmax));
            }
            return base + add;
          }
        size_type round_up_capacity(size_type res_arg) const
          {
            auto cap = this->check_size_add(0, res_arg);
            auto nblk = storage::min_nblk_for_nchar(cap);
            return storage::max_nchar_for_nblk(nblk);
          }
        const value_type* data() const noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return nullptr;
            }
            return ptr->data;
          }
        ROCKET_NOINLINE value_type* reallocate(const value_type* src, size_type len_one, size_type off_two,
                                                                      size_type len_two, size_type res_arg)
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
            size_type len = 0;
            if(ROCKET_UNEXPECT(len_one + len_two != 0)) {
              // Copy characters into the new block.
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
        void deallocate() noexcept
          {
            this->do_reset(storage_pointer());
          }

        void share_with(const storage_handle& other) noexcept
          {
            auto ptr = other.m_ptr;
            if(ptr) {
              // Increment the reference count.
              ptr->nref.increment();
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

        value_type* mut_data_unchecked() noexcept
          {
            auto ptr = this->m_ptr;
            if(!ptr) {
              return nullptr;
            }
            ROCKET_ASSERT(this->unique());
            return ptr->data;
          }

        void swap(storage_handle& other) noexcept
          {
            noadl::adl_swap(this->m_ptr, other.m_ptr);
          }
      };

    template<typename stringT, typename charT> class string_iterator
      {
        template<typename, typename> friend class string_iterator;
        friend stringT;

      public:
        using iterator_category  = random_access_iterator_tag;
        using value_type         = charT;
        using pointer            = value_type*;
        using reference          = value_type&;
        using difference_type    = ptrdiff_t;

        using parent_type   = stringT;

      private:
        const parent_type* m_ref;
        value_type* m_ptr;

      private:
        constexpr string_iterator(const parent_type* ref, value_type* ptr) noexcept
          :
            m_ref(ref), m_ptr(ptr)
          { }

      public:
        constexpr string_iterator() noexcept
          :
            string_iterator(nullptr, nullptr)
          { }
        template<typename ycharT, ROCKET_ENABLE_IF(is_convertible<ycharT*, charT*>::value)>
                constexpr string_iterator(const string_iterator<stringT, ycharT>& other) noexcept
          :
            string_iterator(other.m_ref, other.m_ptr)
          { }

      private:
        value_type* do_assert_valid_pointer(value_type* ptr, bool to_dereference) const noexcept
          {
            auto ref = this->m_ref;
            ROCKET_ASSERT_MSG(ref, "This iterator has not been initialized.");
            auto dist = static_cast<size_t>(ptr - ref->data());
            ROCKET_ASSERT_MSG(dist <= ref->size(), "This iterator has been invalidated.");
            ROCKET_ASSERT_MSG(!(to_dereference && (dist == ref->size())),
                              "This iterator contains a past-the-end value and cannot be dereferenced.");
            return ptr;
          }

      public:
        const parent_type* parent() const noexcept
          {
            return this->m_ref;
          }

        value_type* tell() const noexcept
          {
            auto ptr = this->do_assert_valid_pointer(this->m_ptr, false);
            return ptr;
          }
        value_type* tell_owned_by(const parent_type* ref) const noexcept
          {
            ROCKET_ASSERT_MSG(this->m_ref == ref, "This iterator does not refer to an element in the same container.");
            return this->tell();
          }
        string_iterator& seek(value_type* ptr) noexcept
          {
            this->m_ptr = this->do_assert_valid_pointer(ptr, false);
            return *this;
          }

        reference operator*() const noexcept
          {
            auto ptr = this->do_assert_valid_pointer(this->m_ptr, true);
            ROCKET_ASSERT(ptr);
            return *ptr;
          }
        pointer operator->() const noexcept
          {
            auto ptr = this->do_assert_valid_pointer(this->m_ptr, true);
            ROCKET_ASSERT(ptr);
            return ptr;
          }
        reference operator[](difference_type off) const noexcept
          {
            auto ptr = this->do_assert_valid_pointer(this->m_ptr + off, true);
            ROCKET_ASSERT(ptr);
            return *ptr;
          }
      };

    template<typename stringT, typename charT>
        inline string_iterator<stringT, charT>& operator++(string_iterator<stringT, charT>& rhs) noexcept
      {
        return rhs.seek(rhs.tell() + 1);
      }
    template<typename stringT, typename charT>
        inline string_iterator<stringT, charT>& operator--(string_iterator<stringT, charT>& rhs) noexcept
      {
        return rhs.seek(rhs.tell() - 1);
      }

    template<typename stringT, typename charT>
        inline string_iterator<stringT, charT> operator++(string_iterator<stringT, charT>& lhs, int) noexcept
      {
        auto res = lhs;
        lhs.seek(lhs.tell() + 1);
        return res;
      }
    template<typename stringT, typename charT>
        inline string_iterator<stringT, charT> operator--(string_iterator<stringT, charT>& lhs, int) noexcept
      {
        auto res = lhs;
        lhs.seek(lhs.tell() - 1);
        return res;
      }

    template<typename stringT, typename charT>
        inline string_iterator<stringT, charT>& operator+=(string_iterator<stringT, charT>& lhs,
                                                           typename string_iterator<stringT, charT>::difference_type rhs) noexcept
      {
        return lhs.seek(lhs.tell() + rhs);
      }
    template<typename stringT, typename charT>
        inline string_iterator<stringT, charT>& operator-=(string_iterator<stringT, charT>& lhs,
                                                           typename string_iterator<stringT, charT>::difference_type rhs) noexcept
      {
        return lhs.seek(lhs.tell() - rhs);
      }

    template<typename stringT, typename charT>
        inline string_iterator<stringT, charT> operator+(const string_iterator<stringT, charT>& lhs,
                                                         typename string_iterator<stringT, charT>::difference_type rhs) noexcept
      {
        auto res = lhs;
        res.seek(res.tell() + rhs);
        return res;
      }
    template<typename stringT, typename charT>
        inline string_iterator<stringT, charT> operator-(const string_iterator<stringT, charT>& lhs,
                                                         typename string_iterator<stringT, charT>::difference_type rhs) noexcept
      {
        auto res = lhs;
        res.seek(res.tell() - rhs);
        return res;
      }

    template<typename stringT, typename charT>
        inline string_iterator<stringT, charT> operator+(typename string_iterator<stringT, charT>::difference_type lhs,
                                                         const string_iterator<stringT, charT>& rhs) noexcept
      {
        auto res = rhs;
        res.seek(res.tell() + lhs);
        return res;
      }
    template<typename stringT, typename xcharT, typename ycharT>
        inline typename string_iterator<stringT, xcharT>::difference_type operator-(const string_iterator<stringT, xcharT>& lhs,
                                                                                    const string_iterator<stringT, ycharT>& rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) - rhs.tell();
      }

    template<typename stringT, typename xcharT, typename ycharT>
        inline bool operator==(const string_iterator<stringT, xcharT>& lhs,
                               const string_iterator<stringT, ycharT>& rhs) noexcept
      {
        return lhs.tell() == rhs.tell();
      }
    template<typename stringT, typename xcharT, typename ycharT>
        inline bool operator!=(const string_iterator<stringT, xcharT>& lhs,
                               const string_iterator<stringT, ycharT>& rhs) noexcept
      {
        return lhs.tell() != rhs.tell();
      }

    template<typename stringT, typename xcharT, typename ycharT>
        inline bool operator<(const string_iterator<stringT, xcharT>& lhs,
                              const string_iterator<stringT, ycharT>& rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) < rhs.tell();
      }
    template<typename stringT, typename xcharT, typename ycharT>
        inline bool operator>(const string_iterator<stringT, xcharT>& lhs,
                              const string_iterator<stringT, ycharT>& rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) > rhs.tell();
      }
    template<typename stringT, typename xcharT, typename ycharT>
        inline bool operator<=(const string_iterator<stringT, xcharT>& lhs,
                               const string_iterator<stringT, ycharT>& rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) <= rhs.tell();
      }
    template<typename stringT, typename xcharT, typename ycharT>
        inline bool operator>=(const string_iterator<stringT, xcharT>& lhs,
                               const string_iterator<stringT, ycharT>& rhs) noexcept
      {
        return lhs.tell_owned_by(rhs.parent()) >= rhs.tell();
      }

    // Implement relational operators.
    template<typename charT, typename traitsT> struct comparator
      {
        using char_type    = charT;
        using traits_type  = traitsT;
        using size_type    = size_t;

        static int inequality(const char_type* s1, size_type n1,
                              const char_type* s2, size_type n2) noexcept
          {
            if(n1 != n2) {
              return 2;
            }
            if(s1 == s2) {
              return 0;
            }
            return traits_type::compare(s1, s2, n1);
          }
        static int relation(const char_type* s1, size_type n1,
                            const char_type* s2, size_type n2) noexcept
          {
            if(n1 < n2) {
              int res = traits_type::compare(s1, s2, n1);
              if(res != 0) {
                return res;
              }
              return -1;
            }
            if(n1 > n2) {
              int res = traits_type::compare(s1, s2, n2);
              if(res != 0) {
                return res;
              }
              return +1;
            }
            return traits_type::compare(s1, s2, n1);
          }
      };

    // Replacement helpers.
    struct append_tag
      { }
    constexpr append;

    template<typename stringT, typename... paramsT>
        void tagged_append(stringT* str, append_tag, paramsT&&... params)
      {
        str->append(noadl::forward<paramsT>(params)...);
      }

    struct push_back_tag
      { }
    constexpr push_back;

    template<typename stringT, typename... paramsT>
        void tagged_append(stringT* str, push_back_tag, paramsT&&... params)
      {
        str->push_back(noadl::forward<paramsT>(params)...);
      }

    // Implement the FNV-1a hash algorithm.
    template<typename charT, typename traitsT> class basic_hasher
      {
      private:
        char32_t m_reg;

      public:
        basic_hasher() noexcept
          {
            this->do_init();
          }

      private:
        void do_init() noexcept
          {
            this->m_reg = 0x811c9dc5;
          }
        void do_append(const unsigned char* s, size_t n) noexcept
          {
            auto reg = this->m_reg;
            for(size_t i = 0; i < n; ++i) {
              reg = (reg ^ s[i]) * 0x1000193;
            }
            this->m_reg = reg;
          }
        size_t do_finish() noexcept
          {
            return this->m_reg;
          }

      public:
         basic_hasher& append(const charT* s, size_t n) noexcept
           {
             this->do_append(reinterpret_cast<const unsigned char*>(s), sizeof(charT) * n);
             return *this;
           }
         basic_hasher& append(const charT* sz) noexcept
           {
             for(auto s = sz; !traitsT::eq(*s, charT()); ++s) {
               this->do_append(reinterpret_cast<const unsigned char*>(s), sizeof(charT));
             }
             return *this;
           }
         size_t finish() noexcept
           {
             auto r = this->do_finish();
             this->do_init();
             return r;
           }
      };

    }  // namespace details_cow_string

template<typename charT>
    constexpr details_cow_string::shallow<charT, char_traits<charT>> sref(const charT* ptr) noexcept
  {
    return details_cow_string::shallow<charT, char_traits<charT>>(ptr);
  }
template<typename charT>
    constexpr details_cow_string::shallow<charT, char_traits<charT>> sref(const charT* ptr, ::std::size_t len) noexcept
  {
    return details_cow_string::shallow<charT, char_traits<charT>>(ptr, len);
  }
template<typename charT, typename traitsT, typename allocT>
    constexpr details_cow_string::shallow<charT, traitsT> sref(const basic_cow_string<charT, traitsT, allocT>& str) noexcept
  {
    return details_cow_string::shallow<charT, traitsT>(str);
  }

template<typename charT, typename traitsT, typename allocT> class basic_cow_string
  {
    static_assert(!is_array<charT>::value, "`charT` must not be an array type.");
    static_assert(is_trivial<charT>::value, "`charT` must be a trivial type.");
    static_assert(is_same<typename allocT::value_type, charT>::value,
                  "`allocT::value_type` must denote the same type as `charT`.");

  public:
    // types
    using value_type      = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocT;

    using size_type        = typename allocator_traits<allocator_type>::size_type;
    using difference_type  = typename allocator_traits<allocator_type>::difference_type;
    using const_reference  = const value_type&;
    using reference        = value_type&;

    using const_iterator          = details_cow_string::string_iterator<basic_cow_string, const value_type>;
    using iterator                = details_cow_string::string_iterator<basic_cow_string, value_type>;
    using const_reverse_iterator  = ::std::reverse_iterator<const_iterator>;
    using reverse_iterator        = ::std::reverse_iterator<iterator>;

    using shallow_type   = details_cow_string::shallow<charT, traitsT>;

    static constexpr size_type npos = size_type(-1);
    static const value_type null_char[1];

    // hash support
    struct hash;

  private:
    const value_type* m_ptr;
    size_type m_len;
    details_cow_string::storage_handle<allocator_type, traits_type> m_sth;

  public:
    // 24.3.2.2, construct/copy/destroy
    constexpr basic_cow_string(shallow_type sh, const allocator_type& alloc = allocator_type()) noexcept
      :
        m_ptr(sh.c_str()), m_len(sh.length()), m_sth(alloc)
      { }
    explicit constexpr basic_cow_string(const allocator_type& alloc) noexcept
      :
        basic_cow_string(shallow_type(null_char, 0), alloc)
      { }
    constexpr basic_cow_string(clear_t = clear_t()) noexcept(is_nothrow_constructible<allocator_type>::value)
      :
        basic_cow_string(allocator_type())
      { }
    basic_cow_string(const basic_cow_string& other) noexcept
      :
        basic_cow_string(allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
      {
        this->assign(other);
      }
    basic_cow_string(const basic_cow_string& other, const allocator_type& alloc) noexcept
      :
        basic_cow_string(alloc)
      {
        this->assign(other);
      }
    basic_cow_string(basic_cow_string&& other) noexcept
      :
        basic_cow_string(noadl::move(other.m_sth.as_allocator()))
      {
        this->assign(noadl::move(other));
      }
    basic_cow_string(basic_cow_string&& other, const allocator_type& alloc) noexcept
      :
        basic_cow_string(alloc)
      {
        this->assign(noadl::move(other));
      }
    basic_cow_string(const basic_cow_string& other, size_type pos, size_type n = npos,
                     const allocator_type& alloc = allocator_type())
      :
        basic_cow_string(alloc)
      {
        this->assign(other, pos, n);
      }
    basic_cow_string(const value_type* s, size_type n, const allocator_type& alloc = allocator_type())
      :
        basic_cow_string(alloc)
      {
        this->assign(s, n);
      }
    explicit basic_cow_string(const value_type* s, const allocator_type& alloc = allocator_type())
      :
        basic_cow_string(alloc)
      {
        this->assign(s);
      }
    basic_cow_string(size_type n, value_type ch, const allocator_type& alloc = allocator_type())
      :
        basic_cow_string(alloc)
      {
        this->assign(n, ch);
      }
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)>
        basic_cow_string(inputT first, inputT last, const allocator_type& alloc = allocator_type())
      :
        basic_cow_string(alloc)
      {
        this->assign(noadl::move(first), noadl::move(last));
      }
    basic_cow_string(initializer_list<value_type> init, const allocator_type& alloc = allocator_type())
      :
        basic_cow_string(alloc)
      {
        this->assign(init);
      }
    basic_cow_string& operator=(clear_t) noexcept
      {
        this->clear();
        return *this;
      }
    basic_cow_string& operator=(shallow_type sh) noexcept
      {
        this->assign(sh);
        return *this;
      }
    basic_cow_string& operator=(const basic_cow_string& other) noexcept
      {
        noadl::propagate_allocator_on_copy(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->assign(other);
        return *this;
      }
    basic_cow_string& operator=(basic_cow_string&& other) noexcept
      {
        noadl::propagate_allocator_on_move(this->m_sth.as_allocator(), noadl::move(other.m_sth.as_allocator()));
        this->assign(noadl::move(other));
        return *this;
      }
    basic_cow_string& operator=(initializer_list<value_type> init)
      {
        this->assign(init);
        return *this;
      }

  private:
    // Reallocate the storage to `res_arg` characters, not including the null terminator.
    value_type* do_reallocate(size_type len_one, size_type off_two, size_type len_two, size_type res_arg)
      {
        ROCKET_ASSERT(len_one <= off_two);
        ROCKET_ASSERT(off_two <= this->m_len);
        ROCKET_ASSERT(len_two <= this->m_len - off_two);
        auto ptr = this->m_sth.reallocate(this->m_ptr, len_one, off_two, len_two, res_arg);
        if(!ptr) {
          // The storage has been deallocated.
          this->m_ptr = null_char;
          this->m_len = 0;
          return nullptr;
        }
        ROCKET_ASSERT(this->m_sth.unique());
        this->m_ptr = ptr;
        this->m_len = len_one + len_two;
        return ptr;
      }
    // Add a null terminator at `ptr[len]` then set `len` there.
    void do_set_length(size_type len) noexcept
      {
        ROCKET_ASSERT(len <= this->m_sth.capacity());
        auto ptr = this->m_sth.mut_data_unchecked();
        if(ptr) {
          ROCKET_ASSERT(ptr == this->m_ptr);
          traits_type::assign(ptr[len], value_type());
        }
        this->m_len = len;
      }
    // Clear contents. Deallocate the storage if it is shared at all.
    void do_clear() noexcept
      {
        if(!this->unique()) {
          this->m_sth.deallocate();
          this->m_ptr = null_char;
          this->m_len = 0;
          return;
        }
        this->do_set_length(0);
      }
    // Reallocate more storage as needed, without shrinking.
    void do_reserve_more(size_type cap_add)
      {
        auto len = this->size();
        auto cap = this->m_sth.check_size_add(len, cap_add);
        if(!this->unique() || ROCKET_UNEXPECT(this->capacity() < cap)) {
#ifndef ROCKET_DEBUG
          // Reserve more space for non-debug builds.
          cap = noadl::max(cap, len + len / 2 + 31);
#endif
          this->do_reallocate(0, 0, len, cap | 1);
        }
        ROCKET_ASSERT(this->capacity() >= cap);
      }

    [[noreturn]] ROCKET_NOINLINE void do_throw_subscript_of_range(size_type pos) const
      {
        noadl::sprintf_and_throw<out_of_range>("basic_cow_string: subscript out of range (`%lld` > `%llu`)",
                                               static_cast<unsigned long long>(pos),
                                               static_cast<unsigned long long>(this->size()));
      }

    // This function works the same way as `substr()`.
    // Ensure `tpos` is in `[0, size()]` and return `min(tn, size() - tpos)`.
    ROCKET_PURE_FUNCTION size_type do_clamp_substr(size_type tpos, size_type tn) const
      {
        auto tlen = this->size();
        if(tpos > tlen) {
          this->do_throw_subscript_of_range(tpos);
        }
        return noadl::min(tlen - tpos, tn);
      }

    template<typename... paramsT>
        value_type* do_replace_no_bound_check(size_type tpos, size_type tn, paramsT&&... params)
      {
        auto len_old = this->size();
        ROCKET_ASSERT(tpos <= len_old);
        details_cow_string::tagged_append(this, noadl::forward<paramsT>(params)...);
        auto len_add = this->size() - len_old;
        auto len_sfx = len_old - (tpos + tn);
        this->do_reserve_more(len_sfx);
        auto ptr = this->m_sth.mut_data_unchecked();
        traits_type::copy(ptr + len_old + len_add, ptr + tpos + tn, len_sfx);
        traits_type::move(ptr + tpos, ptr + len_old, len_add + len_sfx);
        this->do_set_length(len_old + len_add - tn);
        return ptr + tpos;
      }
    value_type* do_erase_no_bound_check(size_type tpos, size_type tn)
      {
        auto len_old = this->size();
        ROCKET_ASSERT(tpos <= len_old);
        ROCKET_ASSERT(tn <= len_old - tpos);
        if(!this->unique()) {
          auto ptr = this->do_reallocate(tpos, tpos + tn, len_old - (tpos + tn), len_old);
          return ptr + tpos;
        }
        auto ptr = this->m_sth.mut_data_unchecked();
        traits_type::move(ptr + tpos, ptr + tpos + tn, len_old - (tpos + tn));
        this->do_set_length(len_old - tn);
        return ptr + tpos;
      }

    // These are generic implementations for `{{,r}find,find_{first,last}{,_not}_of}()` functions.
    template<typename predT>
        size_type do_xfind_if(size_type first, size_type last, difference_type step, predT pred) const
      {
        auto cur = first;
        for(;;) {
          auto ptr = this->data() + cur;
          if(pred(ptr)) {
            ROCKET_ASSERT(cur != npos);
            break;
          }
          if(cur == last) {
            cur = npos;
            break;
          }
          cur += static_cast<size_type>(step);
        }
        return cur;
      }
    template<typename predT>
        size_type do_find_forwards_if(size_type from, size_type n, predT pred) const
      {
        auto len = this->size();
        if(len < n) {
          return npos;
        }
        auto rlen = len - n;
        if(from > rlen) {
          return npos;
        }
        return this->do_xfind_if(from, rlen, +1, noadl::move(pred));
      }
    template<typename predT>
        size_type do_find_backwards_if(size_type to, size_type n, predT pred) const
      {
        auto len = this->size();
        if(len < n) {
          return npos;
        }
        auto rlen = len - n;
        if(to > rlen) {
          return this->do_xfind_if(rlen, 0, -1, noadl::move(pred));
        }
        return this->do_xfind_if(to, 0, -1, noadl::move(pred));
      }

  public:
    // 24.3.2.3, iterators
    const_iterator begin() const noexcept
      {
        return const_iterator(this, this->data());
      }
    const_iterator end() const noexcept
      {
        return const_iterator(this, this->data() + this->size());
      }
    const_reverse_iterator rbegin() const noexcept
      {
        return const_reverse_iterator(this->end());
      }
    const_reverse_iterator rend() const noexcept
      {
        return const_reverse_iterator(this->begin());
      }

    const_iterator cbegin() const noexcept
      {
        return this->begin();
      }
    const_iterator cend() const noexcept
      {
        return this->end();
      }
    const_reverse_iterator crbegin() const noexcept
      {
        return this->rbegin();
      }
    const_reverse_iterator crend() const noexcept
      {
        return this->rend();
      }

    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator mut_begin()
      {
        return iterator(this, this->mut_data());
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    iterator mut_end()
      {
        return iterator(this, this->mut_data() + this->size());
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    reverse_iterator mut_rbegin()
      {
        return reverse_iterator(this->mut_end());
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    reverse_iterator mut_rend()
      {
        return reverse_iterator(this->mut_begin());
      }

    // 24.3.2.4, capacity
    bool empty() const noexcept
      {
        return this->m_len == 0;
      }
    size_type size() const noexcept
      {
        return this->m_len;
      }
    size_type length() const noexcept
      {
        return this->m_len;
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
    // N.B. The return type is a non-standard extension.
    basic_cow_string& resize(size_type n, value_type ch = value_type())
      {
        auto len_old = this->size();
        if(len_old < n) {
          this->append(n - len_old, ch);
        }
        else if(len_old > n) {
          this->pop_back(len_old - n);
        }
        ROCKET_ASSERT(this->size() == n);
        return *this;
      }
    size_type capacity() const noexcept
      {
        return this->m_sth.capacity();
      }
    // N.B. The return type is a non-standard extension.
    basic_cow_string& reserve(size_type res_arg)
      {
        auto len = this->size();
        auto cap_new = this->m_sth.round_up_capacity(noadl::max(len, res_arg));
        // If the storage is shared with other strings, force rellocation to prevent copy-on-write upon modification.
        if(this->unique() && (this->capacity() >= cap_new)) {
          return *this;
        }
        this->do_reallocate(0, 0, len, cap_new);
        ROCKET_ASSERT(this->capacity() >= res_arg);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    basic_cow_string& shrink_to_fit()
      {
        auto len = this->size();
        auto cap_min = this->m_sth.round_up_capacity(len);
        // Don't increase memory usage.
        if(!this->unique() || (this->capacity() <= cap_min)) {
          return *this;
        }
        this->do_reallocate(0, 0, len, len);
        ROCKET_ASSERT(this->capacity() <= cap_min);
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    basic_cow_string& clear() noexcept
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

    // 24.3.2.5, element access
    const_reference at(size_type pos) const
      {
        auto len = this->size();
        if(pos >= len) {
          this->do_throw_subscript_of_range(pos);
        }
        return this->data()[pos];
      }
    const_reference operator[](size_type pos) const noexcept
      {
        auto len = this->size();
        // Reading from the character at `size()` is permitted.
        ROCKET_ASSERT(pos <= len);
        return this->c_str()[pos];
      }
    const_reference front() const noexcept
      {
        auto len = this->size();
        ROCKET_ASSERT(len > 0);
        return this->data()[0];
      }
    const_reference back() const noexcept
      {
        auto len = this->size();
        ROCKET_ASSERT(len > 0);
        return this->data()[len - 1];
      }

    // There is no `at()` overload that returns a non-const reference. This is the consequent overload which does that.
    // N.B. This is a non-standard extension.
    reference mut(size_type pos)
      {
        auto len = this->size();
        if(pos >= len) {
          this->do_throw_subscript_of_range(pos);
        }
        return this->mut_data()[pos];
      }
    // N.B. This is a non-standard extension.
    reference mut_front()
      {
        auto len = this->size();
        ROCKET_ASSERT(len > 0);
        return this->mut_data()[0];
      }
    // N.B. This is a non-standard extension.
    reference mut_back()
      {
        auto len = this->size();
        ROCKET_ASSERT(len > 0);
        return this->mut_data()[len - 1];
      }

    // 24.3.2.6, modifiers
    basic_cow_string& operator+=(const basic_cow_string& other)
      {
        return this->append(other);
      }
    basic_cow_string& operator+=(const value_type* s)
      {
        return this->append(s);
      }
    basic_cow_string& operator+=(value_type ch)
      {
        return this->push_back(ch);
      }
    basic_cow_string& operator+=(initializer_list<value_type> init)
      {
        return this->append(init);
      }

    // N.B. This is a non-standard extension.
    basic_cow_string& operator<<(const basic_cow_string& other)
      {
        return this->append(other);
      }
    // N.B. This is a non-standard extension.
    basic_cow_string& operator<<(const value_type* s)
      {
        return this->append(s);
      }
    // N.B. This is a non-standard extension.
    basic_cow_string& operator<<(value_type ch)
      {
        return this->push_back(ch);
      }

    basic_cow_string& append(const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        return this->append(other.data() + pos, other.do_clamp_substr(pos, n));
      }
    basic_cow_string& append(const value_type* s, size_type n)
      {
        if(n == 0) {
          return *this;
        }
        auto len_old = this->size();
        // Check for overlapped strings before `do_reserve_more()`.
        auto srpos = static_cast<uintptr_t>(s - this->data());
        this->do_reserve_more(n);
        auto ptr = this->m_sth.mut_data_unchecked();
        if(srpos < len_old) {
          traits_type::move(ptr + len_old, ptr + srpos, n);
          this->do_set_length(len_old + n);
          return *this;
        }
        traits_type::copy(ptr + len_old, s, n);
        this->do_set_length(len_old + n);
        return *this;
      }
    basic_cow_string& append(const value_type* s)
      {
        return this->append(s, traits_type::length(s));
      }
    basic_cow_string& append(size_type n, value_type ch)
      {
        if(n == 0) {
          return *this;
        }
        auto len_old = this->size();
        this->do_reserve_more(n);
        auto ptr = this->m_sth.mut_data_unchecked();
        traits_type::assign(ptr + len_old, n, ch);
        this->do_set_length(len_old + n);
        return *this;
      }
    basic_cow_string& append(initializer_list<value_type> init)
      {
        return this->append(init.begin(), init.size());
      }
    // N.B. This is a non-standard extension.
    basic_cow_string& append(const value_type* first, const value_type* last)
      {
        ROCKET_ASSERT(first <= last);
        return this->append(first, static_cast<size_type>(last - first));
      }
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category),
                              ROCKET_DISABLE_IF(is_convertible<inputT, const value_type*>::value)>
        basic_cow_string& append(inputT first, inputT last)
      {
        if(first == last) {
          return *this;
        }
        basic_cow_string other(this->m_sth.as_allocator());
        other.reserve(this->size() + noadl::estimate_distance(first, last));
        other.append(this->data(), this->size());
        noadl::ranged_do_while(noadl::move(first), noadl::move(last), [&](const inputT& it) { other.push_back(*it);  });
        this->assign(noadl::move(other));
        return *this;
      }
    // N.B. The return type is a non-standard extension.
    basic_cow_string& push_back(value_type ch)
      {
        auto len_old = this->size();
        this->do_reserve_more(1);
        auto ptr = this->m_sth.mut_data_unchecked();
        traits_type::assign(ptr[len_old], ch);
        this->do_set_length(len_old + 1);
        return *this;
      }

    basic_cow_string& erase(size_type tpos = 0, size_type tn = npos)
      {
        this->do_erase_no_bound_check(tpos, this->do_clamp_substr(tpos, tn));
        return *this;
      }
    // N.B. This function may throw `std::bad_alloc`.
    iterator erase(const_iterator tfirst, const_iterator tlast)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        auto ptr = this->do_erase_no_bound_check(tpos, tn);
        return iterator(this, ptr);
      }
    // N.B. This function may throw `std::bad_alloc`.
    iterator erase(const_iterator tfirst)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto ptr = this->do_erase_no_bound_check(tpos, 1);
        return iterator(this, ptr);
      }
    // N.B. This function may throw `std::bad_alloc`.
    // N.B. The return type and parameter are non-standard extensions.
    basic_cow_string& pop_back(size_type n = 1)
      {
        if(n == 0) {
          return *this;
        }
        auto len_old = this->size();
        ROCKET_ASSERT(n <= len_old);
        if(!this->unique()) {
          this->do_reallocate(0, 0, len_old - n, len_old);
          return *this;
        }
        this->do_set_length(len_old - n);
        return *this;
      }

    basic_cow_string& assign(shallow_type sh) noexcept
      {
        this->m_sth.deallocate();
        this->m_ptr = sh.c_str();
        this->m_len = sh.length();
        return *this;
      }
    basic_cow_string& assign(const basic_cow_string& other) noexcept
      {
        this->m_sth.share_with(other.m_sth);
        this->m_ptr = other.m_ptr;
        this->m_len = other.m_len;
        return *this;
      }
    basic_cow_string& assign(basic_cow_string&& other) noexcept
      {
        this->m_sth.share_with(noadl::move(other.m_sth));
        this->m_ptr = ::std::exchange(other.m_ptr, null_char);
        this->m_len = ::std::exchange(other.m_len, size_type(0));
        return *this;
      }
    basic_cow_string& assign(const basic_cow_string& other, size_type pos, size_type n = npos)
      {
        this->do_replace_no_bound_check(0, this->size(), details_cow_string::append, other, pos, n);
        return *this;
      }
    basic_cow_string& assign(const value_type* s, size_type n)
      {
        this->do_replace_no_bound_check(0, this->size(), details_cow_string::append, s, n);
        return *this;
      }
    basic_cow_string& assign(const value_type* s)
      {
        this->do_replace_no_bound_check(0, this->size(), details_cow_string::append, s);
        return *this;
      }
    basic_cow_string& assign(size_type n, value_type ch)
      {
        this->clear();
        this->append(n, ch);
        return *this;
      }
    basic_cow_string& assign(initializer_list<value_type> init)
      {
        this->clear();
        this->append(init);
        return *this;
      }
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)>
        basic_cow_string& assign(inputT first, inputT last)
      {
        this->do_replace_no_bound_check(0, this->size(), details_cow_string::append, noadl::move(first), noadl::move(last));
        return *this;
      }

    basic_cow_string& insert(size_type tpos, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), details_cow_string::append, other, pos, n);
        return *this;
      }
    basic_cow_string& insert(size_type tpos, const value_type* s, size_type n)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), details_cow_string::append, s, n);
        return *this;
      }
    basic_cow_string& insert(size_type tpos, const value_type* s)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), details_cow_string::append, s);
        return *this;
      }
    basic_cow_string& insert(size_type tpos, size_type n, value_type ch)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), details_cow_string::append, n, ch);
        return *this;
      }
    // N.B. This is a non-standard extension.
    basic_cow_string& insert(size_type tpos, initializer_list<value_type> init)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, 0), details_cow_string::append, init);
        return *this;
      }
    // N.B. This is a non-standard extension.
    iterator insert(const_iterator tins, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, other, pos, n);
        return iterator(this, ptr);
      }
    // N.B. This is a non-standard extension.
    iterator insert(const_iterator tins, const value_type* s, size_type n)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, s, n);
        return iterator(this, ptr);
      }
    // N.B. This is a non-standard extension.
    iterator insert(const_iterator tins, const value_type* s)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, s);
        return iterator(this, ptr);
      }
    iterator insert(const_iterator tins, size_type n, value_type ch)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, n, ch);
        return iterator(this, ptr);
      }
    iterator insert(const_iterator tins, initializer_list<value_type> init)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append, init);
        return iterator(this, ptr);
      }
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)>
        iterator insert(const_iterator tins, inputT first, inputT last)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::append,
                                                   noadl::move(first), noadl::move(last));
        return iterator(this, ptr);
      }
    iterator insert(const_iterator tins, value_type ch)
      {
        auto tpos = static_cast<size_type>(tins.tell_owned_by(this) - this->data());
        auto ptr = this->do_replace_no_bound_check(tpos, 0, details_cow_string::push_back, ch);
        return iterator(this, ptr);
      }

    basic_cow_string& replace(size_type tpos, size_type tn, const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn), details_cow_string::append, other, pos, n);
        return *this;
      }
    basic_cow_string& replace(size_type tpos, size_type tn, const value_type* s, size_type n)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn), details_cow_string::append, s, n);
        return *this;
      }
    basic_cow_string& replace(size_type tpos, size_type tn, const value_type* s)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn), details_cow_string::append, s);
        return *this;
      }
    basic_cow_string& replace(size_type tpos, size_type tn, size_type n, value_type ch)
      {
        this->do_replace_no_bound_check(tpos, this->do_clamp_substr(tpos, tn), details_cow_string::append, n, ch);
        return *this;
      }
    // N.B. The last two parameters are non-standard extensions.
    basic_cow_string& replace(const_iterator tfirst, const_iterator tlast,
                              const basic_cow_string& other, size_type pos = 0, size_type n = npos)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, other, pos, n);
        return *this;
      }
    basic_cow_string& replace(const_iterator tfirst, const_iterator tlast, const value_type* s, size_type n)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, s, n);
        return *this;
      }
    basic_cow_string& replace(const_iterator tfirst, const_iterator tlast, const value_type* s)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, s);
        return *this;
      }
    basic_cow_string& replace(const_iterator tfirst, const_iterator tlast, size_type n, value_type ch)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, n, ch);
        return *this;
      }
    basic_cow_string& replace(const_iterator tfirst, const_iterator tlast, initializer_list<value_type> init)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, init);
        return *this;
      }
    template<typename inputT, ROCKET_ENABLE_IF_HAS_TYPE(iterator_traits<inputT>::iterator_category)>
        basic_cow_string& replace(const_iterator tfirst, const_iterator tlast, inputT first, inputT last)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::append, noadl::move(first), noadl::move(last));
        return *this;
      }
    // N.B. This is a non-standard extension.
    basic_cow_string& replace(const_iterator tfirst, const_iterator tlast, value_type ch)
      {
        auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this) - this->data());
        auto tn = static_cast<size_type>(tlast.tell_owned_by(this) - tfirst.tell());
        this->do_replace_no_bound_check(tpos, tn, details_cow_string::push_back, ch);
        return *this;
      }

    size_type copy(value_type* s, size_type tn, size_type tpos = 0) const
      {
        auto rlen = this->do_clamp_substr(tpos, tn);
        traits_type::copy(s, this->data() + tpos, rlen);
        return rlen;
      }

    void swap(basic_cow_string& other) noexcept
      {
        noadl::propagate_allocator_on_swap(this->m_sth.as_allocator(), other.m_sth.as_allocator());
        this->m_sth.swap(other.m_sth);
        noadl::adl_swap(this->m_ptr, other.m_ptr);
        noadl::adl_swap(this->m_len, other.m_len);
      }

    // 24.3.2.7, string operations
    const value_type* data() const noexcept
      {
        return this->m_ptr;
      }
    const value_type* c_str() const noexcept
      {
        return this->m_ptr;
      }

    // Get a pointer to mutable data. This function may throw `std::bad_alloc`.
    // N.B. This is a non-standard extension.
    value_type* mut_data()
      {
        if(!this->unique()) {
          this->do_reallocate(0, 0, this->size(), this->size() | 1);
        }
        return this->m_sth.mut_data_unchecked();
      }

    // N.B. The return type differs from `std::basic_string`.
    const allocator_type& get_allocator() const noexcept
      {
        return this->m_sth.as_allocator();
      }
    allocator_type& get_allocator() noexcept
      {
        return this->m_sth.as_allocator();
      }

    size_type find(const basic_cow_string& other, size_type from = 0) const noexcept
      {
        return this->find(other.data(), from, other.size());
      }
    // N.B. This is a non-standard extension.
    size_type find(const basic_cow_string& other, size_type from, size_type pos, size_type n = npos) const
      {
        return this->find(other.data() + pos, from, other.do_clamp_substr(pos, n));
      }
    size_type find(const value_type* s, size_type from, size_type n) const noexcept
      {
        return this->do_find_forwards_if(from, n, [&](const value_type* ts) { return traits_type::compare(ts, s, n) == 0;  });
      }
    size_type find(const value_type* s, size_type from = 0) const noexcept
      {
        return this->find(s, from, traits_type::length(s));
      }
    size_type find(value_type ch, size_type from = 0) const noexcept
      {
        // This can be optimized.
        if(from >= this->size()) {
          return npos;
        }
        auto ptr = traits_type::find(this->data() + from, this->size() - from, ch);
        if(!ptr) {
          return npos;
        }
        auto tpos = static_cast<size_type>(ptr - this->data());
        ROCKET_ASSERT(tpos < npos);
        return tpos;
      }

    // N.B. This is a non-standard extension.
    size_type rfind(const basic_cow_string& other, size_type to = npos) const noexcept
      {
        return this->rfind(other.data(), to, other.size());
      }
    size_type rfind(const basic_cow_string& other, size_type to, size_type pos, size_type n = npos) const
      {
        return this->rfind(other.data() + pos, to, other.do_clamp_substr(pos, n));
      }
    size_type rfind(const value_type* s, size_type to, size_type n) const noexcept
      {
        return this->do_find_backwards_if(to, n, [&](const value_type* ts) { return traits_type::compare(ts, s, n) == 0;  });
      }
    size_type rfind(const value_type* s, size_type to = npos) const noexcept
      {
        return this->rfind(s, to, traits_type::length(s));
      }
    size_type rfind(value_type ch, size_type to = npos) const noexcept
      {
        return this->do_find_backwards_if(to, 1, [&](const value_type* ts) { return traits_type::eq(*ts, ch);  });
      }

    // N.B. This is a non-standard extension.
    size_type find_first_of(const basic_cow_string& other, size_type from = 0) const noexcept
      {
        return this->find_first_of(other.data(), from, other.size());
      }
    size_type find_first_of(const basic_cow_string& other, size_type from, size_type pos, size_type n = npos) const
      {
        return this->find_first_of(other.data() + pos, from, other.do_clamp_substr(pos, n));
      }
    size_type find_first_of(const value_type* s, size_type from, size_type n) const noexcept
      {
        return this->do_find_forwards_if(from, 1, [&](const value_type* ts) { return traits_type::find(s, n, *ts) != nullptr;  });
      }
    size_type find_first_of(const value_type* s, size_type from = 0) const noexcept
      {
        return this->find_first_of(s, from, traits_type::length(s));
      }
    size_type find_first_of(value_type ch, size_type from = 0) const noexcept
      {
        return this->find(ch, from);
      }

    // N.B. This is a non-standard extension.
    size_type find_last_of(const basic_cow_string& other, size_type to = npos) const noexcept
      {
        return this->find_last_of(other.data(), to, other.size());
      }
    size_type find_last_of(const basic_cow_string& other, size_type to, size_type pos, size_type n = npos) const
      {
        return this->find_last_of(other.data() + pos, to, other.do_clamp_substr(pos, n));
      }
    size_type find_last_of(const value_type* s, size_type to, size_type n) const noexcept
      {
        return this->do_find_backwards_if(to, 1, [&](const value_type* ts) { return traits_type::find(s, n, *ts) != nullptr;  });
      }
    size_type find_last_of(const value_type* s, size_type to = npos) const noexcept
      {
        return this->find_last_of(s, to, traits_type::length(s));
      }
    size_type find_last_of(value_type ch, size_type to = npos) const noexcept
      {
        return this->rfind(ch, to);
      }

    // N.B. This is a non-standard extension.
    size_type find_first_not_of(const basic_cow_string& other, size_type from = 0) const noexcept
      {
        return this->find_first_not_of(other.data(), from, other.size());
      }
    size_type find_first_not_of(const basic_cow_string& other, size_type from, size_type pos, size_type n = npos) const
      {
        return this->find_first_not_of(other.data() + pos, from, other.do_clamp_substr(pos, n));
      }
    size_type find_first_not_of(const value_type* s, size_type from, size_type n) const noexcept
      {
        return this->do_find_forwards_if(from, 1, [&](const value_type* ts) { return traits_type::find(s, n, *ts) == nullptr;  });
      }
    size_type find_first_not_of(const value_type* s, size_type from = 0) const noexcept
      {
        return this->find_first_not_of(s, from, traits_type::length(s));
      }
    size_type find_first_not_of(value_type ch, size_type from = 0) const noexcept
      {
        return this->do_find_forwards_if(from, 1, [&](const value_type* ts) { return !traits_type::eq(*ts, ch);  });
      }

    // N.B. This is a non-standard extension.
    size_type find_last_not_of(const basic_cow_string& other, size_type to = npos) const noexcept
      {
        return this->find_last_not_of(other.data(), to, other.size());
      }
    size_type find_last_not_of(const basic_cow_string& other, size_type to, size_type pos, size_type n = npos) const
      {
        return this->find_last_not_of(other.data() + pos, to, other.do_clamp_substr(pos, n));
      }
    size_type find_last_not_of(const value_type* s, size_type to, size_type n) const noexcept
      {
        return this->do_find_backwards_if(to, 1, [&](const value_type* ts) { return traits_type::find(s, n, *ts) == nullptr;  });
      }
    size_type find_last_not_of(const value_type* s, size_type to = npos) const noexcept
      {
        return this->find_last_not_of(s, to, traits_type::length(s));
      }
    size_type find_last_not_of(value_type ch, size_type to = npos) const noexcept
      {
        return this->do_find_backwards_if(to, 1, [&](const value_type* ts) { return !traits_type::eq(*ts, ch);  });
      }

    // N.B. This is a non-standard extension.
    template<typename predT> size_type find_first_if(predT pred, size_type from = 0) const
      {
        return this->do_find_forwards_if(from, 1, [&](const char* p) { return pred(*p);  });
      }
    // N.B. This is a non-standard extension.
    template<typename predT> size_type find_last_if(predT pred, size_type to = npos) const
      {
        return this->do_find_backwards_if(to, 1, [&](const char* p) { return pred(*p);  });
      }

    basic_cow_string substr(size_type pos = 0, size_type n = npos) const
      {
        if((pos == 0) && (n >= this->size())) {
          // Utilize reference counting.
          return basic_cow_string(*this, this->m_sth.as_allocator());
        }
        return basic_cow_string(*this, pos, n, this->m_sth.as_allocator());
      }

    int compare(const basic_cow_string& other) const noexcept
      {
        return this->compare(other.data(), other.size());
      }
    // N.B. This is a non-standard extension.
    int compare(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->compare(other.data() + pos, other.do_clamp_substr(pos, n));
      }
    // N.B. This is a non-standard extension.
    int compare(const value_type* s, size_type n) const noexcept
      {
        return details_cow_string::comparator<charT, traitsT>::relation(this->data(), this->size(), s, n);
      }
    int compare(const value_type* s) const noexcept
      {
        return this->compare(s, traits_type::length(s));
      }
    // N.B. The last two parameters are non-standard extensions.
    int compare(size_type tpos, size_type tn, const basic_cow_string& other, size_type pos = 0, size_type n = npos) const
      {
        return this->compare(tpos, tn, other.data() + pos, other.do_clamp_substr(pos, n));
      }
    int compare(size_type tpos, size_type tn, const value_type* s, size_type n) const
      {
        return details_cow_string::comparator<charT, traitsT>::relation(this->data() + tpos, this->do_clamp_substr(tpos, tn), s, n);
      }
    int compare(size_type tpos, size_type tn, const value_type* s) const
      {
        return this->compare(tpos, tn, s, traits_type::length(s));
      }
    // N.B. This is a non-standard extension.
    int compare(size_type tpos, const basic_cow_string& other, size_type pos = 0, size_type n = npos) const
      {
        return this->compare(tpos, npos, other, pos, n);
      }
    // N.B. This is a non-standard extension.
    int compare(size_type tpos, const value_type* s, size_type n) const
      {
        return this->compare(tpos, npos, s, n);
      }
    // N.B. This is a non-standard extension.
    int compare(size_type tpos, const value_type* s) const
      {
        return this->compare(tpos, npos, s);
      }

    // N.B. These are extensions but might be standardized in C++20.
    bool starts_with(const basic_cow_string& other) const noexcept
      {
        return this->starts_with(other.data(), other.size());
      }
    // N.B. This is a non-standard extension.
    bool starts_with(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->starts_with(other.data() + pos, other.do_clamp_substr(pos, n));
      }
    bool starts_with(const value_type* s, size_type n) const noexcept
      {
        return (n <= this->size()) && (traits_type::compare(this->data(), s, n) == 0);
      }
    bool starts_with(const value_type* s) const noexcept
      {
        return this->starts_with(s, traits_type::length(s));
      }
    bool starts_with(value_type ch) const noexcept
      {
        return (1 <= this->size()) && traits_type::eq(this->front(), ch);
      }

    bool ends_with(const basic_cow_string& other) const noexcept
      {
        return this->ends_with(other.data(), other.size());
      }
    // N.B. This is a non-standard extension.
    bool ends_with(const basic_cow_string& other, size_type pos, size_type n = npos) const
      {
        return this->ends_with(other.data() + pos, other.do_clamp_substr(pos, n));
      }
    bool ends_with(const value_type* s, size_type n) const noexcept
      {
        return (n <= this->size()) && (traits_type::compare(this->data() + this->size() - n, s, n) == 0);
      }
    bool ends_with(const value_type* s) const noexcept
      {
        return this->ends_with(s, traits_type::length(s));
      }
    bool ends_with(value_type ch) const noexcept
      {
        return (1 <= this->size()) && traits_type::eq(this->back(), ch);
      }
  };

template<typename charT, typename traitsT, typename allocT>
    const charT basic_cow_string<charT, traitsT, allocT>::null_char[1] = { 0 };

template<typename charT, typename traitsT, typename allocT>
    struct basic_cow_string<charT, traitsT, allocT>::hash
  {
    using result_type    = size_t;
    using argument_type  = basic_cow_string;

    constexpr result_type operator()(const argument_type& str) const noexcept
      {
        return details_cow_string::basic_hasher<charT, traitsT>().append(str.data(), str.size()).finish();
      }
    constexpr result_type operator()(const charT* s) const noexcept
      {
        return details_cow_string::basic_hasher<charT, traitsT>().append(s).finish();
      }
  };

extern template class basic_cow_string<char>;
extern template class basic_cow_string<wchar_t>;
extern template class basic_cow_string<char16_t>;
extern template class basic_cow_string<char32_t>;

using cow_string     = basic_cow_string<char>;
using cow_wstring    = basic_cow_string<wchar_t>;
using cow_u16string  = basic_cow_string<char16_t>;
using cow_u32string  = basic_cow_string<char32_t>;

template<typename charT, typename traitsT, typename allocT>
    inline basic_cow_string<charT, traitsT, allocT> operator+(const basic_cow_string<charT, traitsT, allocT>& lhs,
                                                              const basic_cow_string<charT, traitsT, allocT>& rhs)
  {
    auto res = lhs;
    res.append(rhs);
    return res;
  }
template<typename charT, typename traitsT, typename allocT>
    inline basic_cow_string<charT, traitsT, allocT>&& operator+(basic_cow_string<charT, traitsT, allocT>&& lhs,
                                                                basic_cow_string<charT, traitsT, allocT>&& rhs)
  {
    auto ntotal = lhs.size() + rhs.size();
    if(ROCKET_EXPECT((ntotal <= lhs.capacity()) || (ntotal > rhs.capacity())))
      return noadl::move(lhs.append(rhs));
    else
      return noadl::move(rhs.insert(0, lhs));
  }

template<typename charT, typename traitsT, typename allocT>
    inline basic_cow_string<charT, traitsT, allocT> operator+(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs)
  {
    auto res = lhs;
    res.append(rhs);
    return res;
  }
template<typename charT, typename traitsT, typename allocT>
    inline basic_cow_string<charT, traitsT, allocT> operator+(const basic_cow_string<charT, traitsT, allocT>& lhs, charT rhs)
  {
    auto res = lhs;
    res.push_back(rhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
    inline basic_cow_string<charT, traitsT, allocT> operator+(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
  {
    auto res = rhs;
    res.insert(0, lhs);
    return res;
  }
template<typename charT, typename traitsT, typename allocT>
    inline basic_cow_string<charT, traitsT, allocT> operator+(charT lhs, const basic_cow_string<charT, traitsT, allocT>& rhs)
  {
    auto res = rhs;
    res.insert(0, 1, lhs);
    return res;
  }

template<typename charT, typename traitsT, typename allocT>
    inline basic_cow_string<charT, traitsT, allocT>&& operator+(basic_cow_string<charT, traitsT, allocT>&& lhs, const charT* rhs)
  {
    return noadl::move(lhs.append(rhs));
  }
template<typename charT, typename traitsT, typename allocT>
    inline basic_cow_string<charT, traitsT, allocT>&& operator+(basic_cow_string<charT, traitsT, allocT>&& lhs, charT rhs)
  {
    return noadl::move(lhs.append(1, rhs));
  }

template<typename charT, typename traitsT, typename allocT>
    inline basic_cow_string<charT, traitsT, allocT>&& operator+(const charT* lhs, basic_cow_string<charT, traitsT, allocT>&& rhs)
  {
    // Prepend `lhs` to `rhs`.
    return noadl::move(rhs.insert(0, lhs));
  }
template<typename charT, typename traitsT, typename allocT>
    inline basic_cow_string<charT, traitsT, allocT>&& operator+(charT lhs, basic_cow_string<charT, traitsT, allocT>&& rhs)
  {
    // Prepend `lhs` to `rhs`.
    return noadl::move(rhs.insert(0, 1, lhs));
  }

template<typename charT, typename traitsT, typename allocT>
    inline bool operator==(const basic_cow_string<charT, traitsT, allocT>& lhs,
                           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::inequality(lhs.data(), lhs.size(), rhs.data(), rhs.size()) == 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator!=(const basic_cow_string<charT, traitsT, allocT>& lhs,
                           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::inequality(lhs.data(), lhs.size(), rhs.data(), rhs.size()) != 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator<(const basic_cow_string<charT, traitsT, allocT>& lhs,
                          const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) < 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator>(const basic_cow_string<charT, traitsT, allocT>& lhs,
                          const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) > 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator<=(const basic_cow_string<charT, traitsT, allocT>& lhs,
                           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) <= 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator>=(const basic_cow_string<charT, traitsT, allocT>& lhs,
                           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs.data(), rhs.size()) >= 0;
  }

template<typename charT, typename traitsT, typename allocT>
    inline bool operator==(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::inequality(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) == 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator!=(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::inequality(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) != 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator<(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) < 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator>(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) > 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator<=(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) <= 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator>=(const basic_cow_string<charT, traitsT, allocT>& lhs, const charT* rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs, traitsT::length(rhs)) >= 0;
  }

template<typename charT, typename traitsT, typename allocT>
    inline bool operator==(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::inequality(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) == 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator!=(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::inequality(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) != 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator<(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) < 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator>(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) > 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator<=(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) <= 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator>=(const charT* lhs, const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs, traitsT::length(lhs), rhs.data(), rhs.size()) >= 0;
  }

template<typename charT, typename traitsT, typename allocT>
    inline bool operator==(const basic_cow_string<charT, traitsT, allocT>& lhs,
                           details_cow_string::shallow<charT, traitsT> rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::inequality(lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) == 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator!=(const basic_cow_string<charT, traitsT, allocT>& lhs,
                           details_cow_string::shallow<charT, traitsT> rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::inequality(lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) != 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator<(const basic_cow_string<charT, traitsT, allocT>& lhs,
                          details_cow_string::shallow<charT, traitsT> rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) < 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator>(const basic_cow_string<charT, traitsT, allocT>& lhs,
                          details_cow_string::shallow<charT, traitsT> rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) > 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator<=(const basic_cow_string<charT, traitsT, allocT>& lhs,
                           details_cow_string::shallow<charT, traitsT> rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) <= 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator>=(const basic_cow_string<charT, traitsT, allocT>& lhs,
                           details_cow_string::shallow<charT, traitsT> rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.data(), lhs.size(), rhs.c_str(), rhs.length()) >= 0;
  }

template<typename charT, typename traitsT, typename allocT>
    inline bool operator==(details_cow_string::shallow<charT, traitsT> lhs,
                           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::inequality(lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) == 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator!=(details_cow_string::shallow<charT, traitsT> lhs,
                           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::inequality(lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) != 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator<(details_cow_string::shallow<charT, traitsT> lhs,
                          const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) < 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator>(details_cow_string::shallow<charT, traitsT> lhs,
                          const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) > 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator<=(details_cow_string::shallow<charT, traitsT> lhs,
                           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) <= 0;
  }
template<typename charT, typename traitsT, typename allocT>
    inline bool operator>=(details_cow_string::shallow<charT, traitsT> lhs,
                           const basic_cow_string<charT, traitsT, allocT>& rhs) noexcept
  {
    return details_cow_string::comparator<charT, traitsT>::relation(lhs.c_str(), lhs.length(), rhs.data(), rhs.size()) >= 0;
  }

template<typename charT, typename traitsT, typename allocT>
    inline void swap(basic_cow_string<charT, traitsT, allocT>& lhs,
                     basic_cow_string<charT, traitsT, allocT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    return lhs.swap(rhs);
  }

template<typename charT, typename traitsT, typename allocT>
    inline basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt,
                                                     const basic_cow_string<charT, traitsT, allocT>& str)
  {
    return fmt.put(str.c_str(), str.size());
  }

}  // namespace rocket

#endif
