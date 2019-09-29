// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#define ROCKET_VARIANT_HPP_

#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include <cstring>  // std::memset()

namespace rocket {

template<typename... alternativesT> class variant;

    namespace details_variant {

    template<size_t indexT, typename targetT,
             typename... alternativesT> struct type_finder  // no value
      {
      };
    template<size_t indexT, typename targetT,
             typename firstT, typename... restT> struct type_finder<indexT, targetT,
                                                                    firstT, restT...> : type_finder<indexT + 1, targetT, restT...>  // recursive
      {
      };
    template<size_t indexT, typename targetT,
             typename... restT> struct type_finder<indexT, targetT,
                                                   targetT, restT...> : integral_constant<size_t, indexT>  // found
      {
      };

    template<size_t indexT,
             typename... alternativesT> struct type_getter  // no type
      {
      };
    template<size_t indexT,
             typename firstT, typename... restT> struct type_getter<indexT,
                                                                    firstT, restT...> : type_getter<indexT - 1, restT...>  // recursive
      {
      };
    template<typename firstT,
             typename... restT> struct type_getter<0,
                                                   firstT, restT...> : enable_if<1, firstT>  // found
      {
      };

    // In a `catch` block that is conditionally unreachable, direct use of `throw` is possibly subject to compiler warnings.
    // Wrapping the `throw` expression in a function could silence this warning.
    [[noreturn]] inline void rethrow_current_exception()
      {
        throw;
      }

    constexpr bool all_of(const bool* table, size_t count) noexcept
      {
        return (count == 0) || (table[0] && all_of(table + 1, count - 1));
      }
    constexpr bool any_of(const bool* table, size_t count) noexcept
      {
        return (count != 0) && (table[0] || any_of(table + 1, count - 1));
      }
    constexpr bool test_bit(const bool* table, size_t count, size_t index) noexcept
      {
        return all_of(table, count) || (any_of(table, count) && table[index]);
      }

    template<typename alternativeT> void wrapped_copy_construct(void* dptr, const void* sptr)
      {
        noadl::construct_at(static_cast<alternativeT*>(dptr), *static_cast<const alternativeT*>(sptr));
      }
    template<typename... alternativesT> void dispatch_copy_construct(size_t rindex, void* dptr, const void* sptr)
      {
        static constexpr bool s_trivial_table[] = { is_trivially_copy_constructible<alternativesT>::value... };
        if(ROCKET_EXPECT(test_bit(s_trivial_table, sizeof...(alternativesT), rindex))) {
          // Copy it trivially.
          using storage = typename aligned_union<1, alternativesT...>::type;
          ::std::memcpy(dptr, sptr, sizeof(storage));
          return;
        }
        // Invoke the copy constructor.
        static constexpr void (*const s_jump_table[])(void*, const void*) = { wrapped_copy_construct<alternativesT>... };
        s_jump_table[rindex](dptr, sptr);
      }

    template<typename alternativeT> void wrapped_move_construct(void* dptr, void* sptr)
      {
        noadl::construct_at(static_cast<alternativeT*>(dptr), noadl::move(*static_cast<alternativeT*>(sptr)));
      }
    template<typename... alternativesT> void dispatch_move_construct(size_t rindex, void* dptr, void* sptr)
      {
        static constexpr bool s_trivial_table[] = { is_trivially_move_constructible<alternativesT>::value... };
        if(ROCKET_EXPECT(test_bit(s_trivial_table, sizeof...(alternativesT), rindex))) {
          // Move it trivially.
          using storage = typename aligned_union<1, alternativesT...>::type;
          ::std::memcpy(dptr, sptr, sizeof(storage));
          return;
        }
        // Invoke the move constructor.
        static constexpr void (*const s_jump_table[])(void*, void*) = { wrapped_move_construct<alternativesT>... };
        s_jump_table[rindex](dptr, sptr);
      }

    template<typename alternativeT> void wrapped_copy_assign(void* dptr, const void* sptr)
      {
        *static_cast<alternativeT*>(dptr) = *static_cast<const alternativeT*>(sptr);
      }
    template<typename... alternativesT> void dispatch_copy_assign(size_t rindex, void* dptr, const void* sptr)
      {
        static constexpr bool s_trivial_table[] = { is_trivially_copy_assignable<alternativesT>::value... };
        if(ROCKET_EXPECT(test_bit(s_trivial_table, sizeof...(alternativesT), rindex))) {
          // Copy it trivially. Note that both areas may overlap in the case of self assignment.
          using storage = typename aligned_union<1, alternativesT...>::type;
          ::std::memmove(dptr, sptr, sizeof(storage));
          return;
        }
        // Invoke the copy assignment operator.
        static constexpr void (*const s_jump_table[])(void*, const void*) = { wrapped_copy_assign<alternativesT>... };
        s_jump_table[rindex](dptr, sptr);
      }

    template<typename alternativeT> void wrapped_move_assign(void* dptr, void* sptr)
      {
        *static_cast<alternativeT*>(dptr) = noadl::move(*static_cast<alternativeT*>(sptr));
      }
    template<typename... alternativesT> void dispatch_move_assign(size_t rindex, void* dptr, void* sptr)
      {
        static constexpr bool s_trivial_table[] = { is_trivially_move_assignable<alternativesT>::value... };
        if(ROCKET_EXPECT(test_bit(s_trivial_table, sizeof...(alternativesT), rindex))) {
          // Move it trivially.
          using storage = typename aligned_union<1, alternativesT...>::type;
          ::std::memcpy(dptr, sptr, sizeof(storage));
          return;
        }
        // Invoke the move assignment operator.
        static constexpr void (*const s_jump_table[])(void*, void*) = { wrapped_move_assign<alternativesT>... };
        s_jump_table[rindex](dptr, sptr);
      }

    template<typename alternativeT> void wrapped_destroy(void* dptr) noexcept
      {
        noadl::destroy_at(static_cast<alternativeT*>(dptr));
      }
    template<typename... alternativesT> void dispatch_destroy(size_t rindex, void* dptr)
      {
        static constexpr bool s_trivial_table[] = { is_trivially_destructible<alternativesT>::value... };
        if(ROCKET_EXPECT(test_bit(s_trivial_table, sizeof...(alternativesT), rindex))) {
          // Destroy it trivially. There is nothing to do.
          return;
        }
        // Invoke the destructor.
        static constexpr void (*const s_jump_table[])(void*) = { wrapped_destroy<alternativesT>... };
        s_jump_table[rindex](dptr);
      }

    template<typename alternativeT> void wrapped_move_construct_then_destroy(void* dptr, void* sptr)
      {
        noadl::construct_at(static_cast<alternativeT*>(dptr), noadl::move(*static_cast<alternativeT*>(sptr)));
        noadl::destroy_at(static_cast<alternativeT*>(sptr));
      }
    template<typename... alternativesT> void dispatch_move_construct_then_destroy(size_t rindex, void* dptr, void* sptr)
      {
        static constexpr bool s_trivial_table[] = { conjunction<is_trivially_move_constructible<alternativesT>,
                                                                is_trivially_destructible<alternativesT>>::value... };
        if(ROCKET_EXPECT(test_bit(s_trivial_table, sizeof...(alternativesT), rindex))) {
          // Move it trivially. Destruction is no-op.
          using storage = typename aligned_union<1, alternativesT...>::type;
          ::std::memcpy(dptr, sptr, sizeof(storage));
          return;
        }
        // Invoke the move constructor followed by the destructor.
        static constexpr void (*const s_jump_table[])(void*, void*) = { wrapped_move_construct_then_destroy<alternativesT>... };
        s_jump_table[rindex](dptr, sptr);
      }

    template<typename alternativeT> void wrapped_adl_swap(void* dptr, void* sptr)
      {
        noadl::adl_swap(*static_cast<alternativeT*>(dptr), *static_cast<alternativeT*>(sptr));
      }
    template<typename... alternativesT> void dispatch_swap(size_t rindex, void* dptr, void* sptr)
      {
        static constexpr bool s_trivial_table[] = { conjunction<is_trivially_move_constructible<alternativesT>...,
                                                                is_trivially_move_assignable<alternativesT>...,
                                                                is_trivially_destructible<alternativesT>...>::value };
        if(ROCKET_EXPECT(test_bit(s_trivial_table, sizeof...(alternativesT), rindex))) {
          // Swap them trivially.
          using storage = typename aligned_union<1, alternativesT...>::type;
          ::std::swap(*static_cast<storage*>(sptr), *static_cast<storage*>(dptr));
          return;
        }
        // Invoke the `swap()` function via ADL.
        static constexpr void (*const s_jump_table[])(void*, void*) = { wrapped_adl_swap<alternativesT>... };
        s_jump_table[rindex](dptr, sptr);
      }

    template<typename alternativeT, typename voidT, typename visitorT> void wrapped_visit(voidT* sptr, visitorT&& visitor)
      {
        return noadl::forward<visitorT>(visitor)(*static_cast<alternativeT*>(sptr));
      }

    }  // namespace details_variant

template<typename... alternativesT> class variant
  {
    static_assert(sizeof...(alternativesT) > 0, "At least one alternative must be provided.");
    static_assert(conjunction<is_nothrow_move_constructible<alternativesT>...>::value, "No move constructors of alternatives are allowed to throw exceptions.");

  public:
    template<typename targetT> struct index_of : details_variant::type_finder<0, targetT, alternativesT...>
      {
      };
    template<size_t indexT> struct alternative_at : details_variant::type_getter<indexT, alternativesT...>
      {
      };
    static constexpr size_t alternative_size = sizeof...(alternativesT);

  private:
    typename aligned_union<1, alternativesT...>::type m_stor[1];
    typename lowest_unsigned<alternative_size - 1>::type m_index;

  public:
    // 23.7.3.1, constructors
    variant() noexcept(is_nothrow_constructible<typename alternative_at<0>::type>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(this->m_stor));
#endif
        constexpr auto index_new = size_t(0);
        // Value-initialize the first alternative in place.
        noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)));
        this->m_index = index_new;
      }
    template<typename paramT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<typename decay<paramT>::type>::value)>
            variant(paramT&& param) noexcept(is_nothrow_constructible<typename decay<paramT>::type, paramT&&>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(this->m_stor));
#endif
        constexpr auto index_new = index_of<typename decay<paramT>::type>::value;
        // Copy/move-initialize the alternative in place.
        noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)), noadl::forward<paramT>(param));
        this->m_index = index_new;
      }
    variant(const variant& other) noexcept(conjunction<is_nothrow_copy_constructible<alternativesT>...>::value)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(this->m_stor));
#endif
        auto index_new = other.m_index;
        // Copy-construct the active alternative in place.
        details_variant::dispatch_copy_construct<alternativesT...>(index_new, this->m_stor, other.m_stor);
        this->m_index = index_new;
      }
    variant(variant&& other) noexcept
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_stor, '*', sizeof(this->m_stor));
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
          *static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)) = noadl::move(param);
        }
        else {
          // Destroy the old alternative.
          details_variant::dispatch_destroy<alternativesT...>(index_old, this->m_stor);
          // Move-construct the alternative in place.
          noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)), noadl::move(param));
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
        this->m_index = static_cast<decltype(this->m_index)>(0xBAD1BEEF);
        ::std::memset(this->m_stor, '@', sizeof(this->m_stor));
#endif
      }

  private:
    [[noreturn]] ROCKET_NOINLINE void do_throw_index_mismatch(size_t yindex, const type_info& ytype) const
      {
        noadl::sprintf_and_throw<invalid_argument>("variant: The index requested is `%ld` (`%s`), but the index currently active is `%ld` (`%s`).",
                                                   static_cast<long>(yindex), ytype.name(), static_cast<long>(this->index()), this->type().name());
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
        if(this->m_index != indexT) {
          return nullptr;
        }
        return static_cast<const typename alternative_at<indexT>::type*>(static_cast<const void*>(this->m_stor));
      }
    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)> const targetT* get() const noexcept
      {
        return this->get<index_of<targetT>::value>();
      }
    template<size_t indexT> typename alternative_at<indexT>::type* get() noexcept
      {
        if(this->m_index != indexT) {
          return nullptr;
        }
        return static_cast<typename alternative_at<indexT>::type*>(static_cast<void*>(this->m_stor));
      }
    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)> targetT* get() noexcept
      {
        return this->get<index_of<targetT>::value>();
      }

    template<size_t indexT> const typename alternative_at<indexT>::type& as() const
      {
        auto ptr = this->get<indexT>();
        if(!ptr) {
          this->do_throw_index_mismatch(indexT, typeid(typename alternative_at<indexT>::type));
        }
        return *ptr;
      }
    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)> const targetT& as() const
      {
        return this->as<index_of<targetT>::value>();
      }
    template<size_t indexT> typename alternative_at<indexT>::type& as()
      {
        auto ptr = this->get<indexT>();
        if(!ptr) {
          this->do_throw_index_mismatch(indexT, typeid(typename alternative_at<indexT>::type));
        }
        return *ptr;
      }
    template<typename targetT, ROCKET_ENABLE_IF_HAS_VALUE(index_of<targetT>::value)> targetT& as()
      {
        return this->as<index_of<targetT>::value>();
      }

    template<typename visitorT> void visit(visitorT&& visitor) const
      {
        static constexpr void (*const s_jump_table[])(const void*, visitorT&&) = { details_variant::wrapped_visit<const alternativesT>... };
        s_jump_table[this->m_index](this->m_stor, noadl::forward<visitorT>(visitor));
      }
    template<typename visitorT> void visit(visitorT&& visitor)
      {
        static constexpr void (*const s_jump_table[])(void*, visitorT&&) = { details_variant::wrapped_visit<alternativesT>... };
        s_jump_table[this->m_index](this->m_stor, noadl::forward<visitorT>(visitor));
      }

    // 23.7.3.4, modifiers
    template<size_t indexT, typename... paramsT>
            typename alternative_at<indexT>::type& emplace(paramsT&&... params) noexcept(is_nothrow_constructible<typename alternative_at<indexT>::type, paramsT&&...>::value)
      {
        auto index_old = this->m_index;
        constexpr auto index_new = indexT;
        if(is_nothrow_constructible<typename alternative_at<index_new>::type, paramsT&&...>::value) {
          // Destroy the old alternative.
          details_variant::dispatch_destroy<alternativesT...>(index_old, this->m_stor);
          // Construct the alternative in place.
          noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)), noadl::forward<paramsT>(params)...);
          this->m_index = index_new;
        }
        else {
          // Make a backup.
          typename aligned_union<1, alternativesT...>::type backup[1];
          details_variant::dispatch_move_construct_then_destroy<alternativesT...>(index_old, backup, this->m_stor);
          try {
            // Construct the alternative in place.
            noadl::construct_at(static_cast<typename alternative_at<index_new>::type*>(static_cast<void*>(this->m_stor)), noadl::forward<paramsT>(params)...);
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
        return this->emplace<index_of<targetT>::value>(noadl::forward<paramsT>(params)...);
      }

    // 23.7.3.6, swap
    void swap(variant& other) noexcept(conjunction<is_nothrow_swappable<alternativesT>...>::value)
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
      }
  };

#if !(defined(__cpp_inline_variables) && (__cpp_inline_variables >= 201606))
template<typename... alternativesT> constexpr size_t variant<alternativesT...>::alternative_size;
#endif

template<typename... alternativesT> void swap(variant<alternativesT...>& lhs, variant<alternativesT...>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    return lhs.swap(rhs);
  }

}  // namespace rocket

#endif
