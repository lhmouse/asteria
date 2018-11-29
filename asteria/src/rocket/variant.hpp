// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#define ROCKET_VARIANT_HPP_

#include <cstring> // std::memset()
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"

namespace rocket {

template<typename ...alternativesT>
  class variant;

    namespace details_variant {

    template<typename firstT, typename secondT>
      union union_pair
      {
        using first_type   = firstT;
        using second_type  = secondT;

        first_type first;
        second_type second;

        union_pair() noexcept
          {
          }
        ~union_pair()
          {
          }
      };

    // Main template definition for no type parameters.
    template<size_t minlenT, typename ...typesT>
      struct aligned_union
      {
        struct type
          {
            char bytes[minlenT];
          };
      };
    // Specialization for a size of zero and no type parameters. This has to exist to avoid arrays of zero elements.
    template<>
      struct aligned_union<0>
      {
        struct type
          {
          };
      };
    // Specialization for at least one type parameter.
    template<size_t minlenT, typename firstT, typename ...restT>
      struct aligned_union<minlenT, firstT, restT...>
      {
        struct type
          {
            union_pair<firstT, typename aligned_union<minlenT, restT...>::type> pair;
          };
      };

    template<size_t indexT, typename targetT, typename ...alternativesT>
      struct type_finder  // no value
      {
      };
    template<size_t indexT, typename targetT, typename firstT, typename ...restT>
      struct type_finder<indexT, targetT, firstT, restT...> : type_finder<indexT + 1, targetT, restT...>  // recursive
      {
      };
    template<size_t indexT, typename firstT, typename ...restT>
      struct type_finder<indexT, firstT, firstT, restT...> : integral_constant<size_t, indexT>
      {
      };

    template<size_t indexT, typename ...alternativesT>
      struct type_getter  // no type
      {
      };
    template<size_t indexT, typename firstT, typename ...restT>
      struct type_getter<indexT, firstT, restT...> : type_getter<indexT - 1, restT...>  // recursive
      {
      };
    template<typename firstT, typename ...restT>
      struct type_getter<0, firstT, restT...> : type_identity<firstT>
      {
      };

    template<typename alternativeT>
      void wrapped_copy_construct(void *tptr, const void *rptr)
      {
        noadl::construct_at(static_cast<alternativeT *>(tptr), *static_cast<const alternativeT *>(rptr));
      }
    template<typename alternativeT>
      void wrapped_move_construct(void *tptr, void *rptr)
      {
        noadl::construct_at(static_cast<alternativeT *>(tptr), ::std::move(*static_cast<alternativeT *>(rptr)));
      }
    template<typename alternativeT>
      void wrapped_copy_assign(void *tptr, const void *rptr)
      {
        *static_cast<alternativeT *>(tptr) = *static_cast<const alternativeT *>(rptr);
      }
    template<typename alternativeT>
      void wrapped_move_assign(void *tptr, void *rptr)
      {
        *static_cast<alternativeT *>(tptr) = ::std::move(*static_cast<alternativeT *>(rptr));
      }
    template<typename alternativeT>
      void wrapped_destroy(void *tptr) noexcept
      {
        noadl::destroy_at(static_cast<alternativeT *>(tptr));
      }
    template<typename alternativeT>
      void wrapped_move_construct_then_destroy(void *tptr, void *rptr)
      {
        noadl::construct_at(static_cast<alternativeT *>(tptr), ::std::move(*static_cast<alternativeT *>(rptr)));
        noadl::destroy_at(static_cast<alternativeT *>(rptr));
      }
    template<typename resultT, typename alternativeT, typename voidT, typename visitorT>
      resultT wrapped_visit(voidT *tptr, visitorT &visitor)
      {
        return ::std::forward<visitorT>(visitor)(*static_cast<alternativeT *>(tptr));
      }
    template<typename alternativeT>
      void wrapped_swap(void *tptr, void *rptr)
      {
        noadl::adl_swap(*static_cast<alternativeT *>(tptr), *static_cast<alternativeT *>(rptr));
      }

    template<typename typeT>
      struct trivial_copy_construct :
#ifdef ROCKET_HAS_TRIVIALITY_TRAITS
        ::std::is_trivially_copy_constructible<typeT>
#else
        ::std::has_trivial_copy_constructor<typeT>
#endif
      {
      };
    template<typename typeT>
      struct trivial_move_construct :
#ifdef ROCKET_HAS_TRIVIALITY_TRAITS
        ::std::is_trivially_move_constructible<typeT>
#else
        ::std::has_trivial_copy_constructor<typeT>
#endif
      {
      };
    template<typename typeT>
      struct trivial_copy_assign :
#ifdef ROCKET_HAS_TRIVIALITY_TRAITS
        ::std::is_trivially_copy_assignable<typeT>
#else
        ::std::has_trivial_copy_assign<typeT>
#endif
      {
      };
    template<typename typeT>
      struct trivial_move_assign :
#ifdef ROCKET_HAS_TRIVIALITY_TRAITS
        ::std::is_trivially_move_assignable<typeT>
#else
        ::std::has_trivial_copy_assign<typeT>
#endif
      {
      };

    }

template<typename ...alternativesT>
  class variant
  {
    static_assert(sizeof...(alternativesT) > 0, "At least one alternative type must be provided.");
    static_assert(conjunction<is_nothrow_move_constructible<alternativesT>...>::value, "No move constructors of alternative types are allowed to throw exceptions.");

  public:
    template<typename targetT>
      struct index_of : details_variant::type_finder<0, targetT, alternativesT...>
      {
      };

    template<size_t indexT>
      struct type_at : details_variant::type_getter<indexT, alternativesT...>
      {
      };

    static constexpr size_t type_size = sizeof...(alternativesT);

  private:
    struct storage
      {
        typename details_variant::aligned_union<1, alternativesT...>::type bytes;

        template<typename otherT>
          operator const otherT * () const noexcept
          {
            return static_cast<const otherT *>(static_cast<const void *>(this));
          }
        template<typename otherT>
          operator otherT * () noexcept
          {
            return static_cast<otherT *>(static_cast<void *>(this));
          }
      };

    static constexpr bool do_check_all_of(const bool *bptr, size_t count = type_size) noexcept
      {
        return (count == 0) || (bptr[0] && variant::do_check_all_of(bptr + 1, count - 1));
      }
    static constexpr bool do_check_any_of(const bool *bptr, size_t count = type_size) noexcept
      {
        return (count != 0) && (bptr[0] || variant::do_check_all_of(bptr + 1, count - 1));
      }
    static constexpr bool do_check_fast_call(size_t rindex, const bool *bptr) noexcept
      {
        return variant::do_check_all_of(bptr) || (variant::do_check_any_of(bptr) && ROCKET_EXPECT(bptr[rindex]));
      }

    template<typename resultT>
      static constexpr typename add_const<resultT>::type & do_lookup(resultT *const (&table)[type_size], size_t rindex) noexcept
      {
        return *(table[rindex]);
      }

    static void do_dispatch_copy_construct(size_t rindex, void *tptr, const void *rptr)
      {
        static constexpr bool s_fast_call[] = { details_variant::trivial_copy_construct<alternativesT>::value... };
        if(variant::do_check_fast_call(rindex, s_fast_call)) {
          ::std::memcpy(tptr, rptr, sizeof(storage));
          return;
        }
        static constexpr void (*const s_table[])(void *, const void *) = { &details_variant::wrapped_copy_construct<alternativesT>... };
        return variant::do_lookup(s_table, rindex)(tptr, rptr);
      }
    static void do_dispatch_move_construct(size_t rindex, void *tptr, void *rptr)
      {
        static constexpr bool s_fast_call[] = { details_variant::trivial_move_construct<alternativesT>::value... };
        if(variant::do_check_fast_call(rindex, s_fast_call)) {
          ::std::memcpy(tptr, rptr, sizeof(storage));
          return;
        }
        static constexpr void (*const s_table[])(void *, void *) = { &details_variant::wrapped_move_construct<alternativesT>... };
        return variant::do_lookup(s_table, rindex)(tptr, rptr);
      }
    static void do_dispatch_copy_assign(size_t rindex, void *tptr, const void *rptr)
      {
        static constexpr bool s_fast_call[] = { details_variant::trivial_copy_assign<alternativesT>::value... };
        if(variant::do_check_fast_call(rindex, s_fast_call)) {
          ::std::memmove(tptr, rptr, sizeof(storage));  // They may overlap in case of self assignment.
          return;
        }
        static constexpr void (*const s_table[])(void *, const void *) = { &details_variant::wrapped_copy_assign<alternativesT>... };
        return variant::do_lookup(s_table, rindex)(tptr, rptr);
      }
    static void do_dispatch_move_assign(size_t rindex, void *tptr, void *rptr)
      {
        static constexpr bool s_fast_call[] = { details_variant::trivial_move_assign<alternativesT>::value... };
        if(variant::do_check_fast_call(rindex, s_fast_call)) {
          ::std::memmove(tptr, rptr, sizeof(storage));  // They may overlap in case of self assignment.
          return;
        }
        static constexpr void (*const s_table[])(void *, void *) = { &details_variant::wrapped_move_assign<alternativesT>... };
        return variant::do_lookup(s_table, rindex)(tptr, rptr);
      }
    static void do_dispatch_destroy(size_t rindex, void *tptr)
      {
        static constexpr bool s_fast_call[] = { is_trivially_destructible<alternativesT>::value... };
        if(variant::do_check_fast_call(rindex, s_fast_call)) {
          // There is nothing to do.
          return;
        }
        static constexpr void (*const s_table[])(void *) = { &details_variant::wrapped_destroy<alternativesT>... };
        return variant::do_lookup(s_table, rindex)(tptr);
      }
    static void do_dispatch_move_construct_then_destroy(size_t rindex, void *tptr, void *rptr)
      {
        static constexpr bool s_fast_call[] = { conjunction<details_variant::trivial_move_construct<alternativesT>, is_trivially_destructible<alternativesT>>::value... };
        if(variant::do_check_fast_call(rindex, s_fast_call)) {
          ::std::memcpy(tptr, rptr, sizeof(storage));
          return;
        }
        static constexpr void (*const s_table[])(void *, void *) = { &details_variant::wrapped_move_construct_then_destroy<alternativesT>... };
        return variant::do_lookup(s_table, rindex)(tptr, rptr);
      }

  private:
    typename lowest_unsigned<type_size - 1>::type m_index;
    storage m_stor;

  public:
    // 23.7.3.1, constructors
    variant() noexcept(is_nothrow_constructible<typename type_at<0>::type>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void *>(&(this->m_stor)), '*', sizeof(this->m_stor));
#endif
        constexpr auto index_new = size_t(0);
        // Value-initialize the first alternative in place.
        noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor));
        this->m_index = index_new;
      }
    template<typename paramT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<typename decay<paramT>::type>::value)>
      variant(paramT &&param) noexcept(is_nothrow_constructible<typename decay<paramT>::type, paramT &&>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void *>(&(this->m_stor)), '*', sizeof(this->m_stor));
#endif
        constexpr auto index_new = index_of<typename decay<paramT>::type>::value;
        // Copy/move-initialize the alternative in place.
        noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor), ::std::forward<paramT>(param));
        this->m_index = index_new;
      }
    variant(const variant &other) noexcept(conjunction<is_nothrow_copy_constructible<alternativesT>...>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void *>(&(this->m_stor)), '*', sizeof(this->m_stor));
#endif
        const auto index_new = other.m_index;
        // Copy-construct the active alternative in place.
        variant::do_dispatch_copy_construct(index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
      }
    variant(variant &&other) noexcept
      {
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void *>(&(this->m_stor)), '*', sizeof(this->m_stor));
#endif
        const auto index_new = other.m_index;
        // Move-construct the active alternative in place.
        variant::do_dispatch_move_construct(index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
      }
    // 23.7.3.3, assignment
    template<typename paramT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<paramT>::value)>
      variant & operator=(const paramT &param) noexcept(is_nothrow_copy_assignable<paramT>::value && is_nothrow_copy_constructible<paramT>::value)
      {
        const auto index_old = this->m_index;
        constexpr auto index_new = index_of<paramT>::value;
        if(index_old == index_new) {
          // Copy-assign the alternative in place.
          *static_cast<typename type_at<index_new>::type *>(this->m_stor) = param;
          return *this;
        }
        if(is_nothrow_copy_constructible<paramT>::value) {
          // Destroy the old alternative.
          variant::do_dispatch_destroy(index_old, this->m_stor);
          // Copy-construct the alternative in place.
          noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor), param);
          this->m_index = index_new;
          return *this;
        }
        // Make a backup.
        storage backup;
        variant::do_dispatch_move_construct_then_destroy(index_old, backup, this->m_stor);
        try {
          // Copy-construct the alternative in place.
          noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor), param);
          this->m_index = index_new;
        } catch(...) {
          // Move the backup back in case of exceptions.
          variant::do_dispatch_move_construct_then_destroy(index_old, this->m_stor, backup);
          // In a `catch` block that is conditionally unreachable, direct use of `throw` is possibly subject to compiler warnings.
          // Wrapping the `throw` expression in a lambda could silence this warning.
          []{ throw; }();
        }
        variant::do_dispatch_destroy(index_old, backup);
        return *this;
      }
    // N.B. This assignment operator only accepts rvalues hence no backup is needed.
    template<typename paramT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<paramT>::value)>
      variant & operator=(paramT &&param) noexcept(is_nothrow_move_assignable<paramT>::value)
      {
        const auto index_old = this->m_index;
        constexpr auto index_new = index_of<paramT>::value;
        if(index_old == index_new) {
          // Move-assign the alternative in place.
          *static_cast<typename type_at<index_new>::type *>(this->m_stor) = ::std::move(param);
          return *this;
        }
        // Destroy the old alternative.
        variant::do_dispatch_destroy(index_old, this->m_stor);
        // Move-construct the alternative in place.
        noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor), ::std::move(param));
        this->m_index = index_new;
        return *this;
      }
    variant & operator=(const variant &other) noexcept(conjunction<is_nothrow_copy_assignable<alternativesT>..., is_nothrow_copy_constructible<alternativesT>...>::value)
      {
        const auto index_old = this->m_index;
        const auto index_new = other.m_index;
        if(index_old == index_new) {
          // Copy-assign the alternative in place.
          variant::do_dispatch_copy_assign(index_new, this->m_stor, other.m_stor);
          return *this;
        }
        if(conjunction<is_nothrow_copy_constructible<alternativesT>...>::value) {
          // Destroy the old alternative.
          variant::do_dispatch_destroy(index_old, this->m_stor);
          // Copy-construct the alternative in place.
          variant::do_dispatch_copy_construct(index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
          return *this;
        }
        // Make a backup.
        storage backup;
        variant::do_dispatch_move_construct_then_destroy(index_old, backup, this->m_stor);
        try {
          // Copy-construct the alternative in place.
          variant::do_dispatch_copy_construct(index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
        } catch(...) {
          // Move the backup back in case of exceptions.
          variant::do_dispatch_move_construct_then_destroy(index_old, this->m_stor, backup);
          // In a `catch` block that is conditionally unreachable, direct use of `throw` is possibly subject to compiler warnings.
          // Wrapping the `throw` expression in a lambda could silence this warning.
          []{ throw; }();
        }
        variant::do_dispatch_destroy(index_old, backup);
        return *this;
      }
    variant & operator=(variant &&other) noexcept(conjunction<is_nothrow_move_assignable<alternativesT>...>::value)
      {
        const auto index_old = this->m_index;
        const auto index_new = other.m_index;
        if(index_old == index_new) {
          // Move-assign the alternative in place.
          variant::do_dispatch_move_assign(index_new, this->m_stor, other.m_stor);
          return *this;
        }
        // Move-construct the alternative in place.
        variant::do_dispatch_destroy(index_old, this->m_stor);
        variant::do_dispatch_move_construct(index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
        return *this;
      }
    // 23.7.3.2, destructor
    ~variant()
      {
        const auto index_old = this->m_index;
        // Destroy the active alternative in place.
        variant::do_dispatch_destroy(index_old, this->m_stor);
#ifdef ROCKET_DEBUG
        this->m_index = static_cast<decltype(this->m_index)>(0xBAD1BEEF);
        ::std::memset(static_cast<void *>(&(this->m_stor)), '@', sizeof(this->m_stor));
#endif
      }

  public:
    // 23.7.3.5, value status
    size_t index() const noexcept
      {
        ROCKET_ASSERT(this->m_index < type_size);
        return this->m_index;
      }
    const type_info & type() const noexcept
      {
        static constexpr const type_info *const s_table[] = { &typeid(alternativesT)... };
        return variant::do_lookup(s_table, this->m_index);
      }

    // accessors
    template<size_t indexT>
      const typename type_at<indexT>::type * get() const noexcept
      {
        if(this->m_index != indexT) {
          return nullptr;
        }
        return static_cast<const typename type_at<indexT>::type *>(this->m_stor);
      }
    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)>
      const targetT * get() const noexcept
      {
        return this->get<index_of<targetT>::value>();
      }
    template<size_t indexT>
      typename type_at<indexT>::type * get() noexcept
      {
        if(this->m_index != indexT) {
          return nullptr;
        }
        return static_cast<typename type_at<indexT>::type *>(this->m_stor);
      }
    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)>
      targetT * get() noexcept
      {
        return this->get<index_of<targetT>::value>();
      }

    template<size_t indexT>
      const typename type_at<indexT>::type & as() const
      {
        const auto ptr = this->get<indexT>();
        if(!ptr) {
          noadl::throw_invalid_argument("variant: The index requested is `%d` (`%s`), but the index currently active is `%d` (`%s`).",
                                        static_cast<int>(indexT), typeid(typename type_at<indexT>::type).name(), static_cast<int>(this->index()), this->type().name());
        }
        return *ptr;
      }
    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)>
      const targetT & as() const
      {
        return this->as<index_of<targetT>::value>();
      }
    template<size_t indexT>
      typename type_at<indexT>::type & as()
      {
        const auto ptr = this->get<indexT>();
        if(!ptr) {
          noadl::throw_invalid_argument("variant: The index requested is `%d` (`%s`), but the index currently active is `%d` (`%s`).",
                                        static_cast<int>(indexT), typeid(typename type_at<indexT>::type).name(), static_cast<int>(this->index()), this->type().name());
        }
        return *ptr;
      }
    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)>
      targetT & as()
      {
        return this->as<index_of<targetT>::value>();
      }

    template<typename visitorT>
      typename ::std::common_type<decltype(::std::declval<visitorT &&>()(::std::declval<const alternativesT &>()))...>::type visit(visitorT &&visitor) const
      {
        using result_type = typename ::std::common_type<decltype(::std::declval<visitorT &&>()(::std::declval<const alternativesT &>()))...>::type;
        static constexpr result_type (*const s_table[])(const void *, visitorT &) = { &details_variant::wrapped_visit<result_type, alternativesT, const void, visitorT>... };
        return variant::do_lookup(s_table, this->m_index)(this->m_stor, visitor);
      }
    template<typename visitorT>
      typename ::std::common_type<decltype(::std::declval<visitorT &&>()(::std::declval<alternativesT &>()))...>::type visit(visitorT &&visitor)
      {
        using result_type = typename ::std::common_type<decltype(::std::declval<visitorT &&>()(::std::declval<alternativesT &>()))...>::type;
        static constexpr result_type (*const s_table[])(void *, visitorT &) = { &details_variant::wrapped_visit<result_type, alternativesT, void, visitorT>... };
        return variant::do_lookup(s_table, this->m_index)(this->m_stor, visitor);
      }

    // 23.7.3.4, modifiers
    template<size_t indexT, typename ...paramsT>
      typename type_at<indexT>::type & emplace(paramsT &&...params) noexcept(is_nothrow_constructible<typename type_at<indexT>::type, paramsT &&...>::value)
      {
        const auto index_old = this->m_index;
        constexpr auto index_new = indexT;
        if(is_nothrow_constructible<typename type_at<indexT>::type, paramsT &&...>::value) {
          // Destroy the old alternative.
          variant::do_dispatch_destroy(index_old, this->m_stor);
          // Construct the alternative in place.
          noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor), ::std::forward<paramsT>(params)...);
          this->m_index = index_new;
          return *this;
        }
        // Make a backup.
        storage backup;
        variant::do_dispatch_move_construct_then_destroy(index_old, backup, this->m_stor);
        try {
          // Construct the alternative in place.
          noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor), ::std::forward<paramsT>(params)...);
          this->m_index = index_new;
        } catch(...) {
          // Move the backup back in case of exceptions.
          variant::do_dispatch_move_construct_then_destroy(index_old, this->m_stor, backup);
          // In a `catch` block that is conditionally unreachable, direct use of `throw` is possibly subject to compiler warnings.
          // Wrapping the `throw` expression in a lambda could silence this warning.
          []{ throw; }();
        }
        variant::do_dispatch_destroy(index_old, backup);
        return *static_cast<typename type_at<index_new>::type *>(this->m_stor);
      }
    template<typename targetT, typename ...paramsT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)>
      targetT & emplace(paramsT &&...params) noexcept(is_nothrow_constructible<targetT, paramsT &&...>::value)
      {
        return this->emplace<index_of<targetT>::value>(::std::forward<paramsT>(params)...);
      }

    // 23.7.3.6, swap
    void swap(variant &other) noexcept(conjunction<is_nothrow_swappable<alternativesT>...>::value)
      {
        const auto index_old = this->m_index;
        const auto index_new = other.m_index;
        if(index_old == index_new) {
          // Swap both alternatives in place.
          static constexpr void (*const s_table[])(void *, void *) = { &details_variant::wrapped_swap<alternativesT>... };
          return variant::do_lookup(s_table, index_old)(this->m_stor, other.m_stor);
        }
        // Swap active alternatives using an indeterminate buffer.
        storage backup;
        variant::do_dispatch_move_construct_then_destroy(index_old, backup, this->m_stor);
        // Move-construct the other alternative in place.
        variant::do_dispatch_move_construct_then_destroy(index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
        // Move the backup into `other`.
        variant::do_dispatch_move_construct_then_destroy(index_old, other.m_stor, backup);
        other.m_index = index_old;
      }
  };

#ifndef __cpp_inline_variables
template<typename ...alternativesT>
  constexpr size_t variant<alternativesT...>::type_size;
#endif

template<typename ...alternativesT>
  void swap(variant<alternativesT...> &lhs, variant<alternativesT...> &other) noexcept(conjunction<is_nothrow_swappable<alternativesT>...>::value)
  {
    lhs.swap(other);
  }

}

#endif
