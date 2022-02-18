// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#define ROCKET_VARIANT_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include "throw.hpp"
#include <cstring>  // std::memset()

namespace rocket {

template<typename... altsT>
class variant;

#include "details/variant.ipp"

template<typename... altsT>
class variant
  {
    static_assert(sizeof...(altsT) > 0, "no alternative types provided");
    static_assert(conjunction<is_nothrow_move_constructible<altsT>...>::value,
                  "move constructors of alternative types must not throw exceptions");

  public:
    template<typename targetT>
    struct index_of
      : details_variant::type_finder<0, targetT, altsT...>
      { };

    template<size_t indexT>
    struct alternative_at
      : details_variant::type_getter<indexT, altsT...>
      { };

    static constexpr size_t alternative_size = sizeof...(altsT);

  private:
    using my_storage = typename aligned_union<0, altsT...>::type;
    static constexpr size_t my_align = alignof(my_storage);

    union {
      my_storage m_stor[1];

      // This is used for constexpr initialization.
      typename alternative_at<0>::type m_init_stor;
    };

    union {
      typename lowest_unsigned<alternative_size - 1>::type m_index;

      // Like above, this eliminates padding bytes for constexpr initialization.
      typename conditional<(my_align < 4),
            conditional<(my_align == 1),  uint8_t, uint16_t>,  // 1, 2
            conditional<(my_align == 4), uint32_t, uint64_t>   // 4, 8
          >::type::type m_init_index;
    };

  private:
    template<size_t indexT>
    ROCKET_PURE const typename alternative_at<indexT>::type*
    do_cast_storage() const noexcept
      { return (const typename alternative_at<indexT>::type*) this->m_stor;  }

    template<size_t indexT>
    ROCKET_PURE typename alternative_at<indexT>::type*
    do_cast_storage() noexcept
      { return (typename alternative_at<indexT>::type*) this->m_stor;  }

  public:
    // 23.7.3.1, constructors
    constexpr
    variant() noexcept(is_nothrow_constructible<typename alternative_at<0>::type>::value)
      : m_init_stor(), m_init_index()
      { }

    template<typename paramT,
    ROCKET_ENABLE_IF_HAS_VALUE(index_of<typename decay<paramT>::type>::value)>
    variant(paramT&& param)
      noexcept(is_nothrow_constructible<typename decay<paramT>::type, paramT&&>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(m_stor));
#endif
        constexpr auto index_new = index_of<typename decay<paramT>::type>::value;
        // Copy/move-initialize the alternative in place.
        noadl::construct(this->do_cast_storage<index_new>(),
            ::std::forward<paramT>(param));
        this->m_index = index_new;
      }

    variant(const variant& other)
      noexcept(conjunction<is_nothrow_copy_constructible<altsT>...>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(m_stor));
#endif
        auto index_new = other.m_index;
        // Copy-construct the active alternative in place.
        details_variant::dispatch_copy_construct<altsT...>(
                      index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
      }

    variant(variant&& other) noexcept
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(m_stor));
#endif
        auto index_new = other.m_index;
        if(is_trivial<typename alternative_at<0>::type>::value) {
          // Destroy the moved value so we don't have to invoke its destructor
          // later, which is an indirect call.
          details_variant::dispatch_move_then_destroy<altsT...>(
                      index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
          other.m_index = 0;
        }
        else {
          // Move-construct the active alternative in place.
          details_variant::dispatch_move_construct<altsT...>(
                      index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
        }
      }

    // 23.7.3.3, assignment
    template<typename paramT,
    ROCKET_ENABLE_IF_HAS_VALUE(index_of<paramT>::value)>
    variant&
    operator=(const paramT& param)
      noexcept(conjunction<is_nothrow_copy_assignable<paramT>,
                           is_nothrow_copy_constructible<paramT>>::value)
      {
        auto index_old = this->m_index;
        constexpr auto index_new = index_of<paramT>::value;
        if(index_old == index_new) {
          // Copy-assign the alternative in place.
          this->do_cast_storage<index_new>()[0] = param;
          ROCKET_ASSERT(this->m_index == index_new);
        }
        else if(is_nothrow_copy_constructible<paramT>::value) {
          // Destroy the old alternative.
          details_variant::dispatch_destroy<altsT...>(index_old, this->m_stor);
          // Copy-construct the alternative in place.
          noadl::construct(this->do_cast_storage<index_new>(), param);
          this->m_index = index_new;
        }
        else {
          // Make a backup.
          typename aligned_union<0, altsT...>::type backup[1];
          details_variant::dispatch_move_then_destroy<altsT...>(
                    index_old, backup, this->m_stor);
          try {
            // Copy-construct the alternative in place.
            noadl::construct(this->do_cast_storage<index_new>(), param);
          }
          catch(...) {
            // Move the backup back in case of exceptions.
            details_variant::dispatch_move_then_destroy<altsT...>(
                    index_old, this->m_stor, backup);
            details_variant::rethrow_current_exception();
          }
          details_variant::dispatch_destroy<altsT...>(index_old, backup);
          this->m_index = index_new;
        }
        return *this;
      }

    // N.B. This assignment operator only accepts rvalues hence no backup is needed.
    template<typename paramT,
    ROCKET_ENABLE_IF_HAS_VALUE(index_of<paramT>::value)>
    variant&
    operator=(paramT&& param) noexcept(is_nothrow_move_assignable<paramT>::value)
      {
        auto index_old = this->m_index;
        constexpr auto index_new = index_of<paramT>::value;
        if(index_old == index_new) {
          // Move-assign the alternative in place.
          this->do_cast_storage<index_new>()[0] = ::std::move(param);
          ROCKET_ASSERT(this->m_index == index_new);
        }
        else {
          // Destroy the old alternative.
          details_variant::dispatch_destroy<altsT...>(index_old, this->m_stor);
          // Move-construct the alternative in place.
          noadl::construct(this->do_cast_storage<index_new>(), ::std::move(param));
          this->m_index = index_new;
        }
        return *this;
      }

    variant&
    operator=(const variant& other)
      noexcept(conjunction<is_nothrow_copy_assignable<altsT>...,
                           is_nothrow_copy_constructible<altsT>...>::value)
      {
        auto index_old = this->m_index;
        auto index_new = other.m_index;
        if(index_old == index_new) {
          // Copy-assign the alternative in place.
          details_variant::dispatch_copy_assign<altsT...>(
                    index_new, this->m_stor, other.m_stor);
          ROCKET_ASSERT(this->m_index == index_new);
        }
        else if(conjunction<is_nothrow_copy_constructible<altsT>...>::value) {
          // Destroy the old alternative.
          details_variant::dispatch_destroy<altsT...>(
                    index_old, this->m_stor);
          // Copy-construct the alternative in place.
          details_variant::dispatch_copy_construct<altsT...>(
                    index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
        }
        else {
          // Make a backup.
          typename aligned_union<0, altsT...>::type backup[1];
          details_variant::dispatch_move_then_destroy<altsT...>(
                    index_old, backup, this->m_stor);
          try {
            // Copy-construct the alternative in place.
            details_variant::dispatch_copy_construct<altsT...>(
                    index_new, this->m_stor, other.m_stor);
          }
          catch(...) {
            // Move the backup back in case of exceptions.
            details_variant::dispatch_move_then_destroy<altsT...>(
                    index_old, this->m_stor, backup);
            details_variant::rethrow_current_exception();
          }
          details_variant::dispatch_destroy<altsT...>(index_old, backup);
          this->m_index = index_new;
        }
        return *this;
      }

    variant&
    operator=(variant&& other)
      noexcept(conjunction<is_nothrow_move_assignable<altsT>...>::value)
      {
        auto index_old = this->m_index;
        auto index_new = other.m_index;
        if(index_old == index_new) {
          // Move-assign the alternative in place.
          details_variant::dispatch_move_assign<altsT...>(
                    index_new, this->m_stor, other.m_stor);
          ROCKET_ASSERT(this->m_index == index_new);
        }
        else if(is_trivial<typename alternative_at<0>::type>::value) {
          // Destroy the moved value so we don't have to call its destructor
          // later, which is indirect.
          details_variant::dispatch_destroy<altsT...>(
                    index_old, this->m_stor);
          details_variant::dispatch_move_then_destroy<altsT...>(
                    index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
          other.m_index = 0;
        }
        else {
          // Move-construct the alternative in place.
          details_variant::dispatch_destroy<altsT...>(
                    index_old, this->m_stor);
          details_variant::dispatch_move_construct<altsT...>(
                    index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
        }
        return *this;
      }

    // 23.7.3.6, swap
    variant&
    swap(variant& other)
      noexcept(conjunction<is_nothrow_swappable<altsT>...>::value)
      {
        auto index_old = this->m_index;
        auto index_new = other.m_index;
        if(index_old == index_new) {
          // Swap both alternatives in place.
          details_variant::dispatch_xswap<altsT...>(
                    index_old, this->m_stor, other.m_stor);
        }
        else {
          // Swap active alternatives using an indeterminate buffer.
          typename aligned_union<0, altsT...>::type backup[1];
          details_variant::dispatch_move_then_destroy<altsT...>(
                    index_old, backup, this->m_stor);

          // Move-construct the other alternative in place.
          details_variant::dispatch_move_then_destroy<altsT...>(
                    index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
          // Move the backup into `other`.
          details_variant::dispatch_move_then_destroy<altsT...>(
                    index_old, other.m_stor, backup);
          other.m_index = index_old;
        }
        return *this;
      }

    // 23.7.3.2, destructor
    ~variant()
      {
        auto index_old = this->m_index;
        // Destroy the active alternative in place.
        details_variant::dispatch_destroy<altsT...>(
                    index_old, this->m_stor);
#ifdef ROCKET_DEBUG
        this->m_index = static_cast<decltype(m_index)>(0xBAD1BEEF);
        ::std::memset(this->m_stor, '@', sizeof(m_stor));
#endif
      }

  private:
    [[noreturn]] ROCKET_NEVER_INLINE void
    do_throw_index_mismatch(size_t yindex, const type_info& ytype) const
      {
        noadl::sprintf_and_throw<invalid_argument>(
              "variant: index mismatch (expecting `%d` [`%s`], got `%d` [`%s`]).",
              static_cast<int>(yindex), ytype.name(),
              static_cast<int>(this->index()), this->type().name());
      }

  public:
    // 23.7.3.5, value status
    constexpr size_t
    index() const noexcept
      { return ROCKET_ASSERT(this->m_index < alternative_size), this->m_index;  }

    const type_info&
    type() const noexcept
      {
        static const type_info* const table[] = { &(typeid(altsT))... };
        return *(table[this->m_index]);
      }

    // accessors
    template<size_t indexT>
    const typename alternative_at<indexT>::type*
    get() const noexcept
      {
        if(this->m_index != indexT)
          return nullptr;
        return this->do_cast_storage<indexT>();
      }

    template<typename targetT,
    ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)>
    const targetT*
    get() const noexcept
      {
        return this->get<index_of<targetT>::value>();
      }

    template<size_t indexT>
    typename alternative_at<indexT>::type*
    get() noexcept
      {
        if(this->m_index != indexT)
          return nullptr;
        return this->do_cast_storage<indexT>();
      }

    template<typename targetT,
    ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)>
    targetT*
    get() noexcept
      {
        return this->get<index_of<targetT>::value>();
      }

    template<size_t indexT>
    const typename alternative_at<indexT>::type&
    as() const
      {
        auto ptr = this->get<indexT>();
        if(!ptr)
          this->do_throw_index_mismatch(indexT,
                       typeid(typename alternative_at<indexT>::type));
        return *ptr;
      }

    template<typename targetT,
    ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)>
    const targetT&
    as() const
      {
        return this->as<index_of<targetT>::value>();
      }

    template<size_t indexT>
    typename alternative_at<indexT>::type&
    as()
      {
        auto ptr = this->get<indexT>();
        if(!ptr)
          this->do_throw_index_mismatch(indexT,
                       typeid(typename alternative_at<indexT>::type));
        return *ptr;
      }

    template<typename targetT,
    ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)>
    targetT&
    as()
      {
        return this->as<index_of<targetT>::value>();
      }

    template<typename visitorT>
    void
    visit(visitorT&& visitor) const
      {
        static constexpr auto nt_funcs =
             details_variant::const_func_table<void (const void*, visitorT&&),
                         details_variant::wrapped_visit<const altsT>...>();

        table(this->m_index, this->m_stor, ::std::forward<visitorT>(visitor));
      }

    template<typename visitorT>
    void
    visit(visitorT&& visitor)
      {
        static constexpr auto nt_funcs =
             details_variant::const_func_table<void (void*, visitorT&&),
                         details_variant::wrapped_visit<altsT>...>();

        table(this->m_index, this->m_stor, ::std::forward<visitorT>(visitor));
      }

    // 23.7.3.4, modifiers
    template<size_t indexT, typename... paramsT>
    typename alternative_at<indexT>::type&
    emplace(paramsT&&... params)
      noexcept(is_nothrow_constructible<typename alternative_at<indexT>::type,
                                        paramsT&&...>::value)
      {
        auto index_old = this->m_index;
        constexpr auto index_new = indexT;
        if(is_nothrow_constructible<typename alternative_at<index_new>::type,
                                    paramsT&&...>::value) {
          // Destroy the old alternative.
          details_variant::dispatch_destroy<altsT...>(
                          index_old, this->m_stor);
          // Construct the alternative in place.
          noadl::construct(this->do_cast_storage<index_new>(),
                              ::std::forward<paramsT>(params)...);
          this->m_index = index_new;
        }
        else {
          // Make a backup.
          typename aligned_union<0, altsT...>::type backup[1];
          details_variant::dispatch_move_then_destroy<altsT...>(
                    index_old, backup, this->m_stor);
          try {
            // Construct the alternative in place.
            noadl::construct(this->do_cast_storage<index_new>(),
                                ::std::forward<paramsT>(params)...);
          }
          catch(...) {
            // Move the backup back in case of exceptions.
            details_variant::dispatch_move_then_destroy<altsT...>(
                    index_old, this->m_stor, backup);
            details_variant::rethrow_current_exception();
          }
          details_variant::dispatch_destroy<altsT...>(index_old, backup);
          this->m_index = index_new;
        }
        return this->do_cast_storage<index_new>()[0];
      }

    template<typename targetT, typename... paramsT>
    targetT&
    emplace(paramsT&&... params)
      noexcept(is_nothrow_constructible<targetT, paramsT&&...>::value)
      {
        return this->emplace<index_of<targetT>::value>(
                         ::std::forward<paramsT>(params)...);
      }
  };

#if __cpp_inline_variables + 0 < 201606  // < c++17
template<typename... altsT>
const size_t variant<altsT...>::alternative_size;
#endif

template<typename... altsT>
inline void
swap(variant<altsT...>& lhs, variant<altsT...>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
