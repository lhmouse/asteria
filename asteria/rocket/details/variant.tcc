// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#  error Please include <rocket/variant.hpp> instead.
#endif

namespace details_variant {

template<size_t indexT, typename targetT, typename... alternativesT>
    struct type_finder  // no value
  { };

template<size_t indexT, typename targetT, typename firstT, typename... restT>
    struct type_finder<indexT, targetT, firstT, restT...> : type_finder<indexT + 1, targetT, restT...>  // recursive
  { };

template<size_t indexT, typename targetT, typename... restT>
    struct type_finder<indexT, targetT, targetT, restT...> : integral_constant<size_t, indexT>  // found
  { };

template<size_t indexT, typename... alternativesT>
    struct type_getter  // no type
  { };

template<size_t indexT, typename firstT, typename... restT>
    struct type_getter<indexT, firstT, restT...> : type_getter<indexT - 1, restT...>  // recursive
  { };

template<typename firstT, typename... restT>
    struct type_getter<0, firstT, restT...> : enable_if<1, firstT>  // found
  { };

// In a `catch` block that is conditionally unreachable, direct use of `throw` is possibly subject to compiler warnings.
// Wrapping the `throw` expression in a function could silence this warning.
[[noreturn]] inline void rethrow_current_exception()
  { throw;  }

constexpr bool all_of(const bool* table, size_t count) noexcept
  { return (count == 0) || (table[0] && all_of(table + 1, count - 1));  }

constexpr bool any_of(const bool* table, size_t count) noexcept
  { return (count != 0) && (table[0] || any_of(table + 1, count - 1));  }

constexpr bool test_bit(const bool* table, size_t count, size_t index) noexcept
  { return all_of(table, count) || (any_of(table, count) && table[index]);  }

template<typename alternativeT> void wrapped_copy_construct(void* dptr, const void* sptr)
  { noadl::construct_at(static_cast<alternativeT*>(dptr), *static_cast<const alternativeT*>(sptr));  }

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
  { noadl::construct_at(static_cast<alternativeT*>(dptr), ::std::move(*static_cast<alternativeT*>(sptr)));  }

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
  { *static_cast<alternativeT*>(dptr) = *static_cast<const alternativeT*>(sptr);  }

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
  { *static_cast<alternativeT*>(dptr) = ::std::move(*static_cast<alternativeT*>(sptr));  }

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
  { noadl::destroy_at(static_cast<alternativeT*>(dptr));  }

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
    noadl::construct_at(static_cast<alternativeT*>(dptr), ::std::move(*static_cast<alternativeT*>(sptr)));
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
  { noadl::xswap(*static_cast<alternativeT*>(dptr), *static_cast<alternativeT*>(sptr));  }

template<typename... alternativesT> void dispatch_swap(size_t rindex, void* dptr, void* sptr)
  {
    static constexpr bool s_trivial_table[] = { conjunction<is_trivially_move_constructible<alternativesT>...,
                                                            is_trivially_move_assignable<alternativesT>...,
                                                            is_trivially_destructible<alternativesT>...>::value };
    if(ROCKET_EXPECT(test_bit(s_trivial_table, sizeof...(alternativesT), rindex))) {
      // Swap them trivially.
      using storage = typename aligned_union<1, alternativesT...>::type;
      noadl::xswap(*static_cast<storage*>(sptr), *static_cast<storage*>(dptr));
      return;
    }
    // Invoke the `swap()` function via ADL.
    static constexpr void (*const s_jump_table[])(void*, void*) = { wrapped_adl_swap<alternativesT>... };
    s_jump_table[rindex](dptr, sptr);
  }

template<typename alternativeT, typename voidT, typename visitorT> void wrapped_visit(voidT* sptr, visitorT&& visitor)
  { return ::std::forward<visitorT>(visitor)(*static_cast<alternativeT*>(sptr));  }

}  // namespace details_variant
