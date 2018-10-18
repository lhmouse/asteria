// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#define ROCKET_VARIANT_HPP_

#include <type_traits> // so many...
#include <utility> // std::move(), std::forward(), std::declval(), std::swap()
#include <typeinfo>
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"

namespace rocket {

using ::std::common_type;
using ::std::is_convertible;
using ::std::decay;
using ::std::remove_cv;
using ::std::enable_if;
using ::std::is_nothrow_constructible;
using ::std::is_nothrow_assignable;
using ::std::is_nothrow_copy_constructible;
using ::std::is_nothrow_move_constructible;
using ::std::is_nothrow_copy_assignable;
using ::std::is_nothrow_move_assignable;
using ::std::is_nothrow_destructible;
using ::std::integral_constant;
using ::std::conditional;
using ::std::false_type;
using ::std::true_type;
using ::std::type_info;

template<typename ...altsT>
  class variant;

  namespace details_variant {

  template<unsigned indexT, typename ...typesT>
    struct type_getter
      {
      };
  template<typename firstT, typename ...remainingT>
    struct type_getter<0, firstT, remainingT...>
      {
        using type = firstT;
      };
  template<unsigned indexT, typename firstT, typename ...remainingT>
    struct type_getter<indexT, firstT, remainingT...>
      : type_getter<indexT - 1, remainingT...>
      {
      };

  template<unsigned indexT, typename targetT, typename ...typesT>
    struct type_finder
      {
      };
  template<unsigned indexT, typename targetT, typename firstT, typename ...remainingT>
    struct type_finder<indexT, targetT, firstT, remainingT...>
      : type_finder<indexT + 1, targetT, remainingT...>
      {
      };
  template<unsigned indexT, typename targetT, typename ...remainingT>
    struct type_finder<indexT, targetT, targetT, remainingT...>
      : integral_constant<unsigned, indexT>
      {
      };

  template<typename ...typesT>
    struct conjunction
      : true_type
      {
      };
  template<typename firstT, typename ...remainingT>
    struct conjunction<firstT, remainingT...>
      : conditional<firstT::value, conjunction<remainingT...>, false_type>::type
      {
      };

  template<typename targetT, typename ...typesT>
    struct has_type_recursive
      : false_type
      {
      };
  template<typename targetT, typename firstT, typename ...remainingT>
    struct has_type_recursive<targetT, firstT, remainingT...>
      : has_type_recursive<targetT, remainingT...>
      {
      };
  template<typename targetT, typename ...remainingT>
    struct has_type_recursive<targetT, targetT, remainingT...>
      : true_type
      {
      };
  template<typename targetT, typename ...altsT, typename ...remainingT>
    struct has_type_recursive<targetT, variant<altsT...>, remainingT...>
      : conditional<has_type_recursive<targetT, altsT...>::value, true_type, has_type_recursive<targetT, remainingT...>>::type
      {
      };
  template<typename ...altsT, typename ...remainingT>
    struct has_type_recursive<variant<altsT...>, variant<altsT...>, remainingT...>
      : true_type
      {
      };

  template<unsigned indexT, typename targetT, typename ...typesT>
    struct recursive_type_finder
      {
      };
  template<unsigned indexT, typename targetT, typename firstT, typename ...remainingT>
    struct recursive_type_finder<indexT, targetT, firstT, remainingT...>
      : conditional<has_type_recursive<targetT, firstT>::value, integral_constant<unsigned, indexT>, recursive_type_finder<indexT + 1, targetT, remainingT...>>::type
      {
      };

  template<size_t firstT, size_t ...remainingT>
    struct max_of
      : max_of<firstT, max_of<remainingT...>::value>
      {
      };
  template<size_t firstT>
    struct max_of<firstT>
      : integral_constant<size_t, firstT>
      {
      };
  template<size_t firstT, size_t secondT>
    struct max_of<firstT, secondT>
      : integral_constant<size_t, !(firstT < secondT) ? firstT : secondT>
      {
      };

  template<typename ...altsT>
    struct basic_storage
      {
        // The `+ 0` parts are necessary to work around a bug in GCC 4.8.
        alignas(max_of<alignof(altsT)...>::value + 0) char bytes[max_of<sizeof(altsT)...>::value + 0];
      };

    namespace details_is_nothrow_swappable {

    using ::std::swap;

    template<typename typeT>
      struct is_nothrow_swappable
        : integral_constant<bool, noexcept(swap(::std::declval<typeT &>(), ::std::declval<typeT &>()))>
        {
        };

    }

  template<typename typeT>
    struct is_nothrow_swappable
      : details_is_nothrow_swappable::is_nothrow_swappable<typeT>
      {
      };

  template<typename ...altsT>
    struct visit_helper
      {
        template<typename voidT, typename visitorT, typename ...paramsT>
          void operator()(voidT * /*stor*/, unsigned /*index*/, visitorT &&/*visitor*/, paramsT &&.../*params*/) const
            {
              ROCKET_ASSERT_MSG(false, "The type index provided was out of range.");
            }
      };
  template<typename firstT, typename ...remainingT>
    struct visit_helper<firstT, remainingT...>
      {
        template<typename voidT, typename visitorT, typename ...paramsT>
          void operator()(voidT *stor, unsigned index, visitorT &&visitor, paramsT &&...params) const
            {
              if(index == 0) {
                ::std::forward<visitorT>(visitor)(reinterpret_cast<firstT *>(stor), ::std::forward<paramsT>(params)...);
              } else {
                visit_helper<remainingT...>()(stor, index - 1, ::std::forward<visitorT>(visitor), ::std::forward<paramsT>(params)...);
              }
            }
      };

  struct visitor_copy_construct
    {
      template<typename altT>
        void operator()(altT *ptr, const void *src) const
          {
            noadl::construct_at(ptr, *(static_cast<const altT *>(src)));
          }
    };
  struct visitor_move_construct
    {
      template<typename altT>
        void operator()(altT *ptr, void *src) const
          {
            noadl::construct_at(ptr, ::std::move(*(static_cast<altT *>(src))));
          }
    };
  struct visitor_copy_assign
    {
      template<typename altT>
        void operator()(altT *ptr, const void *src) const
          {
            *ptr = *(static_cast<const altT *>(src));
          }
    };
  struct visitor_move_assign
    {
      template<typename altT>
        void operator()(altT *ptr, void *src) const
          {
            *ptr = ::std::move(*(static_cast<altT *>(src)));
          }
    };
  struct visitor_destroy
    {
      template<typename altT>
        void operator()(altT *ptr) const
          {
            noadl::destroy_at(ptr);
          }
    };
  struct visitor_get_type_info
    {
      template<typename altT>
        void operator()(const altT *ptr, const type_info **ti) const
          {
            *ti = &(typeid(*ptr));
          }
    };
  struct visitor_wrapper
    {
      template<typename altT, typename nextT>
        void operator()(altT *ptr, nextT &&next) const
          {
            ::std::forward<nextT>(next)(*ptr);
          }
    };
  struct visitor_swap
    {
      template<typename altT, typename sourceT>
        void operator()(altT *ptr, sourceT *src) const
          {
            noadl::adl_swap(*ptr, *(static_cast<altT *>(src)));
          }
    };

  // This function silences the warning about `throw` statements inside a possible `noexcept` function.
  [[noreturn]] inline void rethrow_current_exception()
    {
      throw;
    }

  }

template<typename ...altsT>
  class variant
    {
    public:
      template<unsigned indexT>
        struct at
          : details_variant::type_getter<indexT, altsT...>
          {
          };
      template<typename altT>
        struct index_of
          : details_variant::type_finder<0, altT, altsT...>
          {
          };

      template<typename ...addT>
        struct prepend
          : common_type<variant<addT..., altsT...>>
          {
          };
      template<typename ...addT>
        struct append
          : common_type<variant<altsT..., addT...>>
          {
          };

    private:
      unsigned m_turnout : 1;
      unsigned m_index : 15;
      details_variant::basic_storage<altsT...> m_buffers[2];

    private:
      const void * do_get_front_buffer() const noexcept
        {
          const auto turnout = this->m_turnout;
          return this->m_buffers + turnout;
        }
      void * do_get_front_buffer() noexcept
        {
          const auto turnout = this->m_turnout;
          return this->m_buffers + turnout;
        }
      const void * do_get_back_buffer() const noexcept
        {
          const auto turnout = this->m_turnout;
          return this->m_buffers + (turnout ^ 1);
        }
      void * do_get_back_buffer() noexcept
        {
          const auto turnout = this->m_turnout;
          return this->m_buffers + (turnout ^ 1);
        }

      void do_set_up_new_buffer(unsigned index_new) noexcept
        {
          const auto turnout_old = this->m_turnout;
          this->m_turnout = (turnout_old ^ 1) & 0x0001;
          const auto index_old = this->m_index;
          this->m_index = index_new & 0x7FFF;
          // Destroy the old buffer and poison its contents.
          details_variant::visit_helper<altsT...>()(this->m_buffers + turnout_old, index_old,
                                                    details_variant::visitor_destroy());
#ifdef ROCKET_DEBUG
          ::std::memset(this->m_buffers + turnout_old, '@', sizeof(this->m_buffers[0]));
#endif
        }

    public:
      variant() noexcept(is_nothrow_constructible<typename details_variant::type_getter<0, altsT...>::type>::value)
        : m_turnout(0)
        {
          // Value-initialize the first alternative.
          constexpr auto eindex = unsigned(0);
          using etype = typename details_variant::type_getter<eindex, altsT...>::type;
          // Default-construct the first alternative in-place.
          const auto ptr = static_cast<etype *>(this->do_get_front_buffer());
          noadl::construct_at(ptr);
          this->m_index = 0;
        }
      template<typename altT, typename enable_if<details_variant::has_type_recursive<typename decay<altT>::type, altsT...>::value>::type * = nullptr>
        variant(altT &&alt)
          : m_turnout(0)
          {
            // This overload enables construction using an alternative of nested variants.
            constexpr auto eindex = details_variant::recursive_type_finder<0, typename decay<altT>::type, altsT...>::value;
            using etype = typename details_variant::type_getter<eindex, altsT...>::type;
            // Construct the alternative in-place.
            const auto ptr = static_cast<etype *>(this->do_get_front_buffer());
            noadl::construct_at(ptr, ::std::forward<altT>(alt));
            this->m_index = eindex;
          }
      variant(const variant &other) noexcept(details_variant::conjunction<is_nothrow_copy_constructible<altsT>...>::value)
        : m_turnout(0)
        {
          // Copy-construct the active alternative from `other`.
          details_variant::visit_helper<altsT...>()(this->do_get_front_buffer(), other.m_index,
                                                    details_variant::visitor_copy_construct(), other.do_get_front_buffer());
          this->m_index = other.m_index;
        }
      variant(variant &&other) noexcept(details_variant::conjunction<is_nothrow_move_constructible<altsT>...>::value)
        : m_turnout(0)
        {
          // Move-construct the active alternative from `other`.
          details_variant::visit_helper<altsT...>()(this->do_get_front_buffer(), other.m_index,
                                                    details_variant::visitor_move_construct(), other.do_get_front_buffer());
          this->m_index = other.m_index;
        }
      template<typename altT, typename enable_if<details_variant::has_type_recursive<typename decay<altT>::type, altsT...>::value>::type * = nullptr>
        variant & operator=(altT &&alt)
          {
            // This overload, unlike `set()`, enables assignment using an alternative of nested variants.
            constexpr auto eindex = details_variant::recursive_type_finder<0, typename decay<altT>::type, altsT...>::value;
            using etype = typename details_variant::type_getter<eindex, altsT...>::type;
            if(this->m_index == eindex) {
              // Assign the active alternative using perfect forwarding.
              const auto ptr = static_cast<etype *>(this->do_get_front_buffer());
              *ptr = ::std::forward<altT>(alt);
              return *this;
            }
            // Construct the active alternative using perfect forwarding, then destroy the old alternative.
            const auto ptr = static_cast<etype *>(this->do_get_back_buffer());
            noadl::construct_at(ptr, ::std::forward<altT>(alt));
            this->do_set_up_new_buffer(eindex);
            return *this;
          }
      variant & operator=(const variant &other) noexcept(details_variant::conjunction<is_nothrow_copy_assignable<altsT>..., is_nothrow_copy_constructible<altsT>...>::value)
        {
          if(this->m_index == other.m_index) {
            // Copy-assign the active alternative from `other`
            details_variant::visit_helper<altsT...>()(this->do_get_front_buffer(), other.m_index,
                                                      details_variant::visitor_copy_assign(), other.do_get_front_buffer());
            return *this;
          }
          // Copy-construct the active alternative from `other`, then destroy the old alternative.
          details_variant::visit_helper<altsT...>()(this->do_get_back_buffer(), other.m_index,
                                                    details_variant::visitor_copy_construct(), other.do_get_front_buffer());
          this->do_set_up_new_buffer(other.m_index);
          return *this;
        }
      variant & operator=(variant &&other) noexcept(details_variant::conjunction<is_nothrow_move_assignable<altsT>..., is_nothrow_move_constructible<altsT>...>::value)
        {
          if(this->m_index == other.m_index) {
            // Move-assign the active alternative from `other`
            details_variant::visit_helper<altsT...>()(this->do_get_front_buffer(), other.m_index,
                                                      details_variant::visitor_move_assign(), other.do_get_front_buffer());
            return *this;
          }
          // Move-construct the active alternative from `other`, then destroy the old alternative.
          details_variant::visit_helper<altsT...>()(this->do_get_back_buffer(), other.m_index,
                                                    details_variant::visitor_move_construct(), other.do_get_front_buffer());
          this->do_set_up_new_buffer(other.m_index);
          return *this;
        }
      ~variant()
        {
          // Destroy the active alternative and poison all contents.
          details_variant::visit_helper<altsT...>()(this->do_get_front_buffer(), this->m_index,
                                                    details_variant::visitor_destroy());
#ifdef ROCKET_DEBUG
          this->m_index = 0x6EAD;
          ::std::memset(m_buffers, '#', sizeof(m_buffers));
#endif
        }

    public:
      unsigned index() const noexcept
        {
          ROCKET_ASSERT(this->m_index < sizeof...(altsT));
          return this->m_index;
        }
      const type_info & type() const noexcept
        {
          auto ti = static_cast<const type_info *>(nullptr);
          details_variant::visit_helper<const altsT...>()(this->do_get_front_buffer(), this->m_index,
                                                          details_variant::visitor_get_type_info(), &ti);
          ROCKET_ASSERT(ti);
          return *ti;
        }
      template<typename altT>
        const altT * get() const noexcept
          {
            constexpr auto eindex = details_variant::type_finder<0, altT, altsT...>::value;
            using etype = typename details_variant::type_getter<eindex, altsT...>::type;
            if(this->m_index != eindex) {
              return nullptr;
            }
            return static_cast<const etype *>(this->do_get_front_buffer());
          }
      template<typename altT>
        altT * get() noexcept
          {
            constexpr auto eindex = details_variant::type_finder<0, altT, altsT...>::value;
            using etype = typename details_variant::type_getter<eindex, altsT...>::type;
            if(this->m_index != eindex) {
              return nullptr;
            }
            return static_cast<etype *>(this->do_get_front_buffer());
          }
      template<typename altT>
        const altT & as() const
          {
            const auto ptr = this->get<altT>();
            if(!ptr) {
              noadl::throw_invalid_argument("variant: The index requested is `%d` (`%s`), but the index currently active is `%d` (`%s`).",
                                            static_cast<int>(index_of<altT>::value), typeid(altT).name(), static_cast<int>(this->index()), this->type().name());
            }
            return *ptr;
          }
      template<typename altT>
        altT & as()
          {
            const auto ptr = this->get<altT>();
            if(!ptr) {
              noadl::throw_invalid_argument("variant: The index requested is `%d` (`%s`), but the index currently active is `%d` (`%s`).",
                                            static_cast<int>(index_of<altT>::value), typeid(altT).name(), static_cast<int>(this->index()), this->type().name());
            }
            return *ptr;
          }
      template<typename altT, typename ...paramsT>
        altT & emplace(paramsT &&...params) noexcept(is_nothrow_constructible<altT, paramsT &&...>::value)
          {
            // This overload, unlike `operator=()`, does not accept an alternative of nested variants.
            constexpr auto eindex = details_variant::type_finder<0, typename remove_cv<altT>::type, altsT...>::value;
            using etype = typename details_variant::type_getter<eindex, altsT...>::type;
            // Construct the active alternative using perfect forwarding, then destroy the old alternative.
            const auto ptr = static_cast<etype *>(this->do_get_back_buffer());
            noadl::construct_at(ptr, ::std::forward<paramsT>(params)...);
            this->do_set_up_new_buffer(eindex);
            return *ptr;
          }
      template<typename altT>
        altT & set(altT &&alt) noexcept(details_variant::conjunction<is_nothrow_move_assignable<altsT>..., is_nothrow_move_constructible<altsT>...>::value)
          {
            // This overload, unlike `operator=()`, does not accept an alternative of nested variants.
            constexpr auto eindex = details_variant::type_finder<0, typename decay<altT>::type, altsT...>::value;
            using etype = typename details_variant::type_getter<eindex, altsT...>::type;
            if(this->m_index == eindex) {
              // Assign the active alternative using perfect forwarding.
              const auto ptr = static_cast<etype *>(this->do_get_front_buffer());
              *ptr = ::std::forward<altT>(alt);
              return *ptr;
            }
            // Construct the active alternative using perfect forwarding, then destroy the old alternative.
            const auto ptr = static_cast<etype *>(this->do_get_back_buffer());
            noadl::construct_at(ptr, ::std::forward<altT>(alt));
            this->do_set_up_new_buffer(eindex);
            return *ptr;
          }

      template<typename visitorT>
        void visit(visitorT &&visitor) const
          {
            details_variant::visit_helper<altsT...>()(this->do_get_front_buffer(), this->m_index,
                                                      details_variant::visitor_wrapper(), ::std::forward<visitorT>(visitor));
          }
      template<typename visitorT>
        void visit(visitorT &&visitor)
          {
            details_variant::visit_helper<altsT...>()(this->do_get_front_buffer(), this->m_index,
                                                      details_variant::visitor_wrapper(), ::std::forward<visitorT>(visitor));
          }

      void swap(variant &other) noexcept(details_variant::conjunction<details_variant::is_nothrow_swappable<altsT>..., is_nothrow_move_constructible<altsT>...>::value)
        {
          if(this->m_index == other.m_index) {
            // Swap the active alternatives.
            details_variant::visit_helper<altsT...>()(this->do_get_front_buffer(), other.m_index,
                                                      details_variant::visitor_swap(), other.do_get_front_buffer());
            return;
          }
          // Move-construct the active alternative in `*this` from `other`.
          details_variant::visit_helper<altsT...>()(this->do_get_back_buffer(), other.m_index,
                                                    details_variant::visitor_move_construct(), other.do_get_front_buffer());
          try {
            // Move-construct the active alternative in `other` from `*this`.
            details_variant::visit_helper<altsT...>()(other.do_get_back_buffer(), this->m_index,
                                                      details_variant::visitor_move_construct(), this->do_get_front_buffer());
          } catch(...) {
            // In case of an exception, the second object will not have been constructed.
            // Destroy the first object that has just been constructed, then rethrow the exception.
            details_variant::visit_helper<altsT...>()(this->do_get_back_buffer(), other.m_index,
                                                      details_variant::visitor_destroy());
            details_variant::rethrow_current_exception();
          }
          // Destroy both alternatives.
          const auto this_index = this->m_index;
          this->do_set_up_new_buffer(other.m_index);
          other.do_set_up_new_buffer(this_index);
        }
    };

template<typename ...altsT>
  void swap(variant<altsT...> &lhs, variant<altsT...> &other) noexcept(noexcept(lhs.swap(other)))
    {
      lhs.swap(other);
    }

}

#endif
