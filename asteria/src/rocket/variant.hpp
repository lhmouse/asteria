// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#define ROCKET_VARIANT_HPP_

#include <type_traits> // so many...
#include <utility> // std::move(), std::forward(), std::declval(), std::swap()
#include <typeinfo> // std::type_info
#include <cstring> // std::memset()
#include <cstddef> // std::size_t
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"

namespace rocket {

using ::std::common_type;
using ::std::decay;
using ::std::is_nothrow_constructible;
using ::std::is_nothrow_copy_constructible;
using ::std::is_nothrow_move_constructible;
using ::std::is_nothrow_copy_assignable;
using ::std::is_nothrow_move_assignable;
using ::std::is_trivially_destructible;
using ::std::integral_constant;
using ::std::enable_if;
using ::std::type_info;
using ::std::size_t;

template<typename ...alternativesT>
  class variant;

    namespace details_variant {

    template<typename firstT, typename secondT>
      union union_pair
      {
        using first_type   = firstT;
        using second_type  = secondT;

        firstT first;
        secondT second;

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
      struct type_getter<0, firstT, restT...> : enable_if<true, firstT>
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
    template<typename alternativeT, typename voidT, typename visitorT>
      void wrapped_visit(voidT *tptr, visitorT &visitor)
      {
        ::std::forward<visitorT>(visitor)(*static_cast<alternativeT *>(tptr));
      }
    template<typename alternativeT>
      void wrapped_swap(void *tptr, void *rptr)
      {
        noadl::adl_swap(*static_cast<alternativeT *>(tptr), *static_cast<alternativeT *>(rptr));
      }

    template<typename typeT>
      struct trivial_copy_construct :
#if ROCKET_TRIVIAL_TYPE_TRAITS
        ::std::is_trivially_copy_constructible<typeT>
#else
        ::std::has_trivial_copy_constructor<typeT>
#endif
      {
      };
    template<typename typeT>
      struct trivial_move_construct :
#if ROCKET_TRIVIAL_TYPE_TRAITS
        ::std::is_trivially_move_constructible<typeT>
#else
        ::std::has_trivial_copy_constructor<typeT>
#endif
      {
      };
    template<typename typeT>
      struct trivial_copy_assign :
#if ROCKET_TRIVIAL_TYPE_TRAITS
        ::std::is_trivially_copy_assignable<typeT>
#else
        ::std::has_trivial_copy_assign<typeT>
#endif
      {
      };
    template<typename typeT>
      struct trivial_move_assign :
#if ROCKET_TRIVIAL_TYPE_TRAITS
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

    static inline void do_dispatch_copy_construct(size_t rindex, void *tptr, const void *rptr)
      {
        if(conjunction<details_variant::trivial_copy_construct<alternativesT>...>::value) {
          ::std::memcpy(tptr, rptr, sizeof(storage));
          return;
        }
        static constexpr void (*const s_table[])(void *, const void *) = { &details_variant::wrapped_copy_construct<alternativesT>... };
        (*(s_table[rindex]))(tptr, rptr);
      }
    static inline void do_dispatch_move_construct(size_t rindex, void *tptr, void *rptr)
      {
        if(conjunction<details_variant::trivial_move_construct<alternativesT>...>::value) {
          ::std::memcpy(tptr, rptr, sizeof(storage));
          return;
        }
        static constexpr void (*const s_table[])(void *, void *) = { &details_variant::wrapped_move_construct<alternativesT>... };
        (*(s_table[rindex]))(tptr, rptr);
      }
    static inline void do_dispatch_copy_assign(size_t rindex, void *tptr, const void *rptr)
      {
        if(conjunction<details_variant::trivial_copy_assign<alternativesT>...>::value) {
          ::std::memmove(tptr, rptr, sizeof(storage));  // They may overlap in case of self assignment.
          return;
        }
        static constexpr void (*const s_table[])(void *, const void *) = { &details_variant::wrapped_copy_assign<alternativesT>... };
        (*(s_table[rindex]))(tptr, rptr);
      }
    static inline void do_dispatch_move_assign(size_t rindex, void *tptr, void *rptr)
      {
        if(conjunction<details_variant::trivial_move_assign<alternativesT>...>::value) {
          ::std::memcpy(tptr, rptr, sizeof(storage));
          return;
        }
        static constexpr void (*const s_table[])(void *, void *) = { &details_variant::wrapped_move_assign<alternativesT>... };
        (*(s_table[rindex]))(tptr, rptr);
      }
    static inline void do_dispatch_destroy(size_t rindex, void *tptr)
      {
        if(conjunction<is_trivially_destructible<alternativesT>...>::value) {
          // There is nothing to do.
          return;
        }
        static constexpr void (*const s_table[])(void *) = { &details_variant::wrapped_destroy<alternativesT>... };
        (*(s_table[rindex]))(tptr);
      }

  private:
    unsigned short m_index;
    storage m_stor;

  public:
    // 23.7.3.1, constructors
    variant() noexcept(is_nothrow_constructible<typename type_at<0>::type>::value)
      {
        constexpr auto index_new = size_t(0);
        // Value-initialize the first alternative in place.
        noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor));
        this->m_index = index_new;
      }
    template<typename paramT, typename enable_if<(index_of<typename decay<paramT>::type>::value || true)>::type * = nullptr>
      variant(paramT &&param) noexcept(is_nothrow_constructible<typename decay<paramT>::type, paramT &&>::value)
      {
        constexpr auto index_new = index_of<typename decay<paramT>::type>::value;
        // Copy/move-initialize the alternative in place.
        noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor), ::std::forward<paramT>(param));
        this->m_index = index_new;
      }
    variant(const variant &other) noexcept(conjunction<is_nothrow_copy_constructible<alternativesT>...>::value)
      {
        const auto index_new = other.m_index;
        // Copy-construct the active alternative in place.
        variant::do_dispatch_copy_construct(index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
      }
    variant(variant &&other) noexcept
      {
        const auto index_new = other.m_index;
        // Move-construct the active alternative in place.
        variant::do_dispatch_move_construct(index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
      }
    // 23.7.3.3, assignment
    template<typename paramT, typename enable_if<(index_of<paramT>::value || true)>::type * = nullptr>
      variant & operator=(const paramT &param) noexcept(conjunction<is_nothrow_copy_assignable<alternativesT>...>::value)
      {
        const auto index_old = this->m_index;
        constexpr auto index_new = index_of<typename decay<paramT>::type>::value;
        if(index_old == index_new) {
          // Copy-assign the alternative in place.
          *static_cast<typename type_at<index_new>::type *>(this->m_stor) = param;
          return *this;
        }
        // Make a backup.
        storage backup;
        variant::do_dispatch_move_construct(index_old, backup, this->m_stor);
        // Destroy the old alternative.
        variant::do_dispatch_destroy(index_old, this->m_stor);
        try {
          // Copy-construct the alternative in place.
          noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor), param);
          this->m_index = index_new;
          variant::do_dispatch_destroy(index_old, backup);
        } catch(...) {
          // Move the backup back in case of exceptions.
          variant::do_dispatch_move_construct(index_old, this->m_stor, backup);
          variant::do_dispatch_destroy(index_old, backup);
          // In a `catch` block that is conditionally unreachable, direct use of `throw` is possibly subject to compiler warnings.
          // Wrapping the `throw` expression in a lambda could silence this warning.
          []{ throw; }();
        }
        return *this;
      }
    // N.B. This assignment operator only accepts rvalues hence no backup is needed.
    template<typename paramT, typename enable_if<(index_of<paramT>::value || true)>::type * = nullptr>
      variant & operator=(paramT &&param) noexcept(conjunction<is_nothrow_move_assignable<alternativesT>...>::value)
      {
        const auto index_old = this->m_index;
        constexpr auto index_new = index_of<typename decay<paramT>::type>::value;
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
    variant & operator=(const variant &other) noexcept(conjunction<is_nothrow_copy_assignable<alternativesT>...>::value)
      {
        const auto index_old = this->m_index;
        const auto index_new = other.m_index;
        if(index_old == index_new) {
          // Copy-assign the alternative in place.
          variant::do_dispatch_copy_assign(index_new, this->m_stor, other.m_stor);
          return *this;
        }
        // Make a backup.
        storage backup;
        variant::do_dispatch_move_construct(index_old, backup, this->m_stor);
        variant::do_dispatch_destroy(index_old, this->m_stor);
        try {
          // Copy-construct the alternative in place.
          variant::do_dispatch_copy_construct(index_new, this->m_stor, other.m_stor);
          this->m_index = index_new;
          variant::do_dispatch_destroy(index_old, backup);
        } catch(...) {
          // Move the backup back in case of exceptions.
          variant::do_dispatch_move_construct(index_old, this->m_stor, backup);
          variant::do_dispatch_destroy(index_old, backup);
          // In a `catch` block that is conditionally unreachable, direct use of `throw` is possibly subject to compiler warnings.
          // Wrapping the `throw` expression in a lambda could silence this warning.
          []{ throw; }();
        }
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
        ::std::memset(&(this->m_stor), '#', sizeof(this->m_stor));
        this->m_index = 0xA596;
#endif
      }

  public:
    // 23.7.3.5, value status
    size_t index() const noexcept
      {
        return this->m_index;
      }
    const type_info & type() const noexcept
      {
        static constexpr const type_info *const s_table[] = { &typeid(alternativesT)... };
        return *(s_table[this->m_index]);
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
    template<typename targetT, typename enable_if<(index_of<targetT>::value || true)>::type * = nullptr>
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
    template<typename targetT, typename enable_if<(index_of<targetT>::value || true)>::type * = nullptr>
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
    template<typename targetT, typename enable_if<(index_of<targetT>::value || true)>::type * = nullptr>
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
    template<typename targetT, typename enable_if<(index_of<targetT>::value || true)>::type * = nullptr>
      targetT & as()
      {
        return this->as<index_of<targetT>::value>();
      }

    template<typename visitorT>
      void visit(visitorT &&visitor) const
      {
        static constexpr void (*const s_table[])(const void *, visitorT &) = { &details_variant::wrapped_visit<alternativesT, const void, visitorT>... };
        return (*(s_table[this->m_index]))(this->m_stor, visitor);
      }
    template<typename visitorT>
      void visit(visitorT &&visitor)
      {
        static constexpr void (*const s_table[])(void *, visitorT &) = { &details_variant::wrapped_visit<alternativesT, void, visitorT>... };
        return (*(s_table[this->m_index]))(this->m_stor, visitor);
      }

    // 23.7.3.4, modifiers
    template<size_t indexT, typename ...paramsT>
      typename type_at<indexT>::type & emplace(paramsT &&...params) noexcept(is_nothrow_constructible<typename type_at<indexT>::type, paramsT &&...>::value)
      {
        const auto index_old = this->m_index;
        constexpr auto index_new = indexT;
        // Make a backup.
        storage backup;
        variant::do_dispatch_move_construct(index_old, backup, this->m_stor);
        variant::do_dispatch_destroy(index_old, this->m_stor);
        try {
          // Construct the alternative in place.
          noadl::construct_at(static_cast<typename type_at<index_new>::type *>(this->m_stor), ::std::forward<paramsT>(params)...);
          this->m_index = index_new;
          variant::do_dispatch_destroy(index_old, backup);
        } catch(...) {
          // Move the backup back in case of exceptions.
          variant::do_dispatch_move_construct(index_old, this->m_stor, backup);
          variant::do_dispatch_destroy(index_old, backup);
          // In a `catch` block that is conditionally unreachable, direct use of `throw` is possibly subject to compiler warnings.
          // Wrapping the `throw` expression in a lambda could silence this warning.
          []{ throw; }();
        }
        return *static_cast<typename type_at<index_new>::type *>(this->m_stor);
      }
    template<typename targetT, typename ...paramsT, typename enable_if<(index_of<targetT>::value || true)>::type * = nullptr>
      targetT & emplace(paramsT &&...params) noexcept(is_nothrow_constructible<targetT, paramsT &&...>::value)
      {
        return this->emplace<index_of<typename decay<targetT>::type>::value>(::std::forward<paramsT>(params)...);
      }

    // 23.7.3.6, swap
    void swap(variant &other) noexcept(conjunction<is_nothrow_swappable<alternativesT>...>::value)
      {
        const auto index_old = this->m_index;
        const auto index_new = other.m_index;
        if(index_old == index_new) {
          // Swap both alternatives in place.
          static constexpr void (*const s_table[])(void *, void *) = { &details_variant::wrapped_swap<alternativesT>... };
          (*(s_table[index_old]))(this->m_stor, other.m_stor);
          return;
        }
        // Make a backup.
        storage backup;
        variant::do_dispatch_move_construct(index_old, backup, this->m_stor);
        variant::do_dispatch_destroy(index_old, this->m_stor);
        // Move-construct the other alternative in place.
        variant::do_dispatch_move_construct(index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
        variant::do_dispatch_destroy(index_new, other.m_stor);
        // Move the backup into `other`.
        variant::do_dispatch_move_construct(index_old, other.m_stor, backup);
        other.m_index = index_old;
        variant::do_dispatch_destroy(index_old, backup);
      }
  };

template<typename ...alternativesT>
  void swap(variant<alternativesT...> &lhs, variant<alternativesT...> &other) noexcept(conjunction<is_nothrow_swappable<alternativesT>...>::value)
  {
    lhs.swap(other);
  }

}

#endif
