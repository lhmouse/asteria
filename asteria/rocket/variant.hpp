// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#define ROCKET_VARIANT_HPP_

#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include <cstring>  // std::memset()

namespace rocket {

template<typename... alternativesT> class variant;

#include "details/variant.tcc"

template<typename... alternativesT> class variant
  {
    static_assert(sizeof...(alternativesT) > 0, "no alternative types provided");
    static_assert(conjunction<is_nothrow_move_constructible<alternativesT>...>::value,
                  "move constructors of alternative types must not throw exceptions");

  public:
    template<typename targetT> struct index_of : details_variant::type_finder<0, targetT, alternativesT...>  { };
    template<size_t indexT> struct alternative_at : details_variant::type_getter<indexT, alternativesT...>  { };
    static constexpr size_t alternative_size = sizeof...(alternativesT);

  private:
    typename aligned_union<1, alternativesT...>::type m_stor[1];
    typename lowest_unsigned<alternative_size - 1>::type m_index;

  public:
    // 23.7.3.1, constructors
    variant() noexcept(is_nothrow_constructible<typename alternative_at<0>::type>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(m_stor));
#endif
        // Value-initialize the first alternative in place.
        noadl::construct_at(static_cast<typename alternative_at<0>::type*>(static_cast<void*>(this->m_stor)));
        this->m_index = 0;
      }

    template<typename paramT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<typename decay<paramT>::type>::value)>
        variant(paramT&& param) noexcept(is_nothrow_constructible<typename decay<paramT>::type, paramT&&>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(m_stor));
#endif
        constexpr auto index_new = index_of<typename decay<paramT>::type>::value;
        // Copy/move-initialize the alternative in place.
        noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)),
                            ::std::forward<paramT>(param));
        this->m_index = index_new;
      }

    variant(const variant& other) noexcept(conjunction<is_nothrow_copy_constructible<alternativesT>...>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(m_stor));
#endif
        auto index_new = other.m_index;
        // Copy-construct the active alternative in place.
        details_variant::dispatch_copy_construct<alternativesT...>(index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
      }

    variant(variant&& other) noexcept
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(m_stor));
#endif
        auto index_new = other.m_index;
        // Move-construct the active alternative in place.
        details_variant::dispatch_move_construct<alternativesT...>(index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
      }

    // 23.7.3.3, assignment
    template<typename paramT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<paramT>::value)>
        variant& operator=(const paramT& param) noexcept(conjunction<is_nothrow_copy_assignable<paramT>,
                                                                     is_nothrow_copy_constructible<paramT>>::value)
      {
        auto index_old = this->m_index;
        constexpr auto index_new = index_of<paramT>::value;
        if(index_old == index_new) {
          // Copy-assign the alternative in place.
          *static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)) = param;
        }
        else if(is_nothrow_copy_constructible<paramT>::value) {
          // Destroy the old alternative.
          details_variant::dispatch_destroy<alternativesT...>(index_old, this->m_stor);
          // Copy-construct the alternative in place.
          noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)), param);
          this->m_index = index_new;
        }
        else {
          // Make a backup.
          typename aligned_union<1, alternativesT...>::type backup[1];
          details_variant::dispatch_move_construct_then_destroy<alternativesT...>(index_old, backup, this->m_stor);
          try {
            // Copy-construct the alternative in place.
            noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)), param);
            this->m_index = index_new;
          }
          catch(...) {
            // Move the backup back in case of exceptions.
            details_variant::dispatch_move_construct_then_destroy<alternativesT...>(index_old, this->m_stor, backup);
            details_variant::rethrow_current_exception();
          }
          details_variant::dispatch_destroy<alternativesT...>(index_old, backup);
        }
        return *this;
      }

    // N.B. This assignment operator only accepts rvalues hence no backup is needed.
    template<typename paramT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<paramT>::value)>
        variant& operator=(paramT&& param) noexcept(is_nothrow_move_assignable<paramT>::value)
      {
        auto index_old = this->m_index;
        constexpr auto index_new = index_of<paramT>::value;
        if(index_old == index_new) {
          // Move-assign the alternative in place.
          *static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)) = ::std::move(param);
        }
        else {
          // Destroy the old alternative.
          details_variant::dispatch_destroy<alternativesT...>(index_old, this->m_stor);
          // Move-construct the alternative in place.
          noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)),
                              ::std::move(param));
          this->m_index = index_new;
        }
        return *this;
      }

    variant& operator=(const variant& other) noexcept(conjunction<is_nothrow_copy_assignable<alternativesT>...,
                                                                  is_nothrow_copy_constructible<alternativesT>...>::value)
      {
        auto index_old = this->m_index;
        auto index_new = other.m_index;
        if(index_old == index_new) {
          // Copy-assign the alternative in place.
          details_variant::dispatch_copy_assign<alternativesT...>(index_new, this->m_stor, other.m_stor);
        }
        else if(conjunction<is_nothrow_copy_constructible<alternativesT>...>::value) {
          // Destroy the old alternative.
          details_variant::dispatch_destroy<alternativesT...>(index_old, this->m_stor);
          // Copy-construct the alternative in place.
          details_variant::dispatch_copy_construct<alternativesT...>(index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
        }
        else {
          // Make a backup.
          typename aligned_union<1, alternativesT...>::type backup[1];
          details_variant::dispatch_move_construct_then_destroy<alternativesT...>(index_old, backup, this->m_stor);
          try {
            // Copy-construct the alternative in place.
            details_variant::dispatch_copy_construct<alternativesT...>(index_new, this->m_stor, other.m_stor);
            this->m_index = index_new;
          }
          catch(...) {
            // Move the backup back in case of exceptions.
            details_variant::dispatch_move_construct_then_destroy<alternativesT...>(index_old, this->m_stor, backup);
            details_variant::rethrow_current_exception();
          }
          details_variant::dispatch_destroy<alternativesT...>(index_old, backup);
        }
        return *this;
      }

    variant& operator=(variant&& other) noexcept(conjunction<is_nothrow_move_assignable<alternativesT>...>::value)
      {
        auto index_old = this->m_index;
        auto index_new = other.m_index;
        if(index_old == index_new) {
          // Move-assign the alternative in place.
          details_variant::dispatch_move_assign<alternativesT...>(index_new, this->m_stor, other.m_stor);
        }
        else {
          // Move-construct the alternative in place.
          details_variant::dispatch_destroy<alternativesT...>(index_old, this->m_stor);
          details_variant::dispatch_move_construct<alternativesT...>(index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
        }
        return *this;
      }

    // 23.7.3.2, destructor
    ~variant()
      {
        auto index_old = this->m_index;
        // Destroy the active alternative in place.
        details_variant::dispatch_destroy<alternativesT...>(index_old, this->m_stor);
#ifdef ROCKET_DEBUG
        this->m_index = static_cast<decltype(m_index)>(0xBAD1BEEF);
        ::std::memset(this->m_stor, '@', sizeof(m_stor));
#endif
      }

  private:
    [[noreturn]] ROCKET_NOINLINE void do_throw_index_mismatch(size_t yindex, const type_info& ytype) const
      {
        noadl::sprintf_and_throw<invalid_argument>("variant: index mismatch (requesting `%d` [`%s`], got `%d` [`%s`]).",
                                                   static_cast<int>(yindex), ytype.name(),
                                                   static_cast<int>(this->index()), this->type().name());
      }

  public:
    // 23.7.3.5, value status
    size_t index() const noexcept
      {
        ROCKET_ASSERT(this->m_index < alternative_size);
        return this->m_index;
      }

    const type_info& type() const noexcept
      {
        static const type_info* const s_table[] = { ::std::addressof(typeid(alternativesT))... };
        return *(s_table[this->m_index]);
      }

    // accessors
    template<size_t indexT> const typename alternative_at<indexT>::type* get() const noexcept
      {
        if(this->m_index != indexT)
          return nullptr;
        return static_cast<const typename alternative_at<indexT>::type*>(static_cast<const void*>(this->m_stor));
      }

    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)> const targetT* get() const noexcept
      {
        return this->get<index_of<targetT>::value>();
      }

    template<size_t indexT> typename alternative_at<indexT>::type* get() noexcept
      {
        if(this->m_index != indexT)
          return nullptr;
        return static_cast<typename alternative_at<indexT>::type*>(static_cast<void*>(this->m_stor));
      }

    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)> targetT* get() noexcept
      {
        return this->get<index_of<targetT>::value>();
      }

    template<size_t indexT> const typename alternative_at<indexT>::type& as() const
      {
        auto ptr = this->get<indexT>();
        if(!ptr)
          this->do_throw_index_mismatch(indexT, typeid(typename alternative_at<indexT>::type));
        return *ptr;
      }

    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)> const targetT& as() const
      {
        return this->as<index_of<targetT>::value>();
      }

    template<size_t indexT> typename alternative_at<indexT>::type& as()
      {
        auto ptr = this->get<indexT>();
        if(!ptr)
          this->do_throw_index_mismatch(indexT, typeid(typename alternative_at<indexT>::type));
        return *ptr;
      }

    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)> targetT& as()
      {
        return this->as<index_of<targetT>::value>();
      }

    template<typename visitorT> void visit(visitorT&& visitor) const
      {
        static constexpr void (*const s_jump_table[])(const void*, visitorT&&) =
          { details_variant::wrapped_visit<const alternativesT>... };
        s_jump_table[this->m_index](this->m_stor, ::std::forward<visitorT>(visitor));
      }

    template<typename visitorT> void visit(visitorT&& visitor)
      {
        static constexpr void (*const s_jump_table[])(void*, visitorT&&) =
          { details_variant::wrapped_visit<alternativesT>... };
        s_jump_table[this->m_index](this->m_stor, ::std::forward<visitorT>(visitor));
      }

    // 23.7.3.4, modifiers
    template<size_t indexT, typename... paramsT>
        typename alternative_at<indexT>::type& emplace(paramsT&&... params)
                           noexcept(is_nothrow_constructible<typename alternative_at<indexT>::type, paramsT&&...>::value)
      {
        auto index_old = this->m_index;
        constexpr auto index_new = indexT;
        if(is_nothrow_constructible<typename alternative_at<index_new>::type, paramsT&&...>::value) {
          // Destroy the old alternative.
          details_variant::dispatch_destroy<alternativesT...>(index_old, this->m_stor);
          // Construct the alternative in place.
          noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)),
                              ::std::forward<paramsT>(params)...);
          this->m_index = index_new;
        }
        else {
          // Make a backup.
          typename aligned_union<1, alternativesT...>::type backup[1];
          details_variant::dispatch_move_construct_then_destroy<alternativesT...>(index_old, backup, this->m_stor);
          try {
            // Construct the alternative in place.
            noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)),
                                ::std::forward<paramsT>(params)...);
            this->m_index = index_new;
          }
          catch(...) {
            // Move the backup back in case of exceptions.
            details_variant::dispatch_move_construct_then_destroy<alternativesT...>(index_old, this->m_stor, backup);
            details_variant::rethrow_current_exception();
          }
          details_variant::dispatch_destroy<alternativesT...>(index_old, backup);
        }
        return *static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor));
      }

    template<typename targetT, typename... paramsT>
        targetT& emplace(paramsT&&... params) noexcept(is_nothrow_constructible<targetT, paramsT&&...>::value)
      {
        return this->emplace<index_of<targetT>::value>(::std::forward<paramsT>(params)...);
      }

    // 23.7.3.6, swap
    variant& swap(variant& other) noexcept(conjunction<is_nothrow_swappable<alternativesT>...>::value)
      {
        auto index_old = this->m_index;
        auto index_new = other.m_index;
        if(index_old == index_new) {
          // Swap both alternatives in place.
          details_variant::dispatch_swap<alternativesT...>(index_old, this->m_stor, other.m_stor);
        }
        else {
          // Swap active alternatives using an indeterminate buffer.
          typename aligned_union<1, alternativesT...>::type backup[1];
          details_variant::dispatch_move_construct_then_destroy<alternativesT...>(index_old, backup, this->m_stor);
          // Move-construct the other alternative in place.
          details_variant::dispatch_move_construct_then_destroy<alternativesT...>(index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
          // Move the backup into `other`.
          details_variant::dispatch_move_construct_then_destroy<alternativesT...>(index_old, other.m_stor, backup);
          other.m_index = index_old;
        }
        return *this;
      }
  };

template<typename... alternativesT>
    inline void swap(variant<alternativesT...>& lhs,
                     variant<alternativesT...>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

}  // namespace rocket

#endif
