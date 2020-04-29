// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#  error Please include <rocket/variant.hpp> instead.
#endif

namespace details_variant {

template<size_t indexT, typename targetT, typename... alternativesT>
struct type_finder
  // no value
  { };

template<size_t indexT, typename targetT, typename firstT, typename... restT>
struct type_finder<indexT, targetT, firstT, restT...>
  : type_finder<indexT + 1, targetT, restT...>  // recursive
  { };

template<size_t indexT, typename targetT, typename... restT>
struct type_finder<indexT, targetT, targetT, restT...>
  : integral_constant<size_t, indexT>  // found
  { };

template<size_t indexT, typename... alternativesT>
struct type_getter
  // no type
  { };

template<size_t indexT, typename firstT, typename... restT>
struct type_getter<indexT, firstT, restT...>
  : type_getter<indexT - 1, restT...>  // recursive
  { };

template<typename firstT, typename... restT>
struct type_getter<0, firstT, restT...>
  : enable_if<1, firstT>  // found
  { };

// In a `catch` block that is conditionally unreachable, direct use of `throw` is subject to compiler warnings.
// Wrapping the `throw` expression in a function could silence this warning.
[[noreturn]] ROCKET_FORCED_INLINE_FUNCTION
void
rethrow_current_exception()
  { throw;  }

template<size_t sizeT>
ROCKET_PURE_FUNCTION ROCKET_FORCED_INLINE_FUNCTION
bool
test_bit(const bool (& table)[sizeT], size_t k)
  {
    size_t sum = 0;
    for(bool b : table)
      sum += b;

    if(sum == sizeT)  // all set
      return true;
    else if(sum == 0)  // none set
      return false;
    else
      return table[k];
  }

template<typename alternativeT>
void*
wrapped_copy_construct(void* dptr, const void* sptr)
  {
    return noadl::construct_at(static_cast<alternativeT*>(dptr), *static_cast<const alternativeT*>(sptr));
  }

template<typename... alternativesT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_copy_construct(size_t k, void* dptr, const void* sptr)
  {
    // Copy it trivially if possible.
    static constexpr bool s_trivial_table[] = { is_trivially_copy_constructible<alternativesT>::value... };
    if(ROCKET_EXPECT(test_bit<>(s_trivial_table, k)))
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, alternativesT...>::type));

    // Invoke the copy constructor.
    using function_type = void* (void*, const void*);
    static constexpr function_type* s_jump_table[] = { wrapped_copy_construct<alternativesT>... };
    return s_jump_table[k](dptr, sptr);
  }

template<typename alternativeT>
void*
wrapped_move_construct(void* dptr, void* sptr)
  {
    return noadl::construct_at(static_cast<alternativeT*>(dptr), ::std::move(*static_cast<alternativeT*>(sptr)));
  }

template<typename... alternativesT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_move_construct(size_t k, void* dptr, void* sptr)
  {
    // Move it trivially if possible.
    static constexpr bool s_trivial_table[] = { is_trivially_move_constructible<alternativesT>::value... };
    if(ROCKET_EXPECT(test_bit<>(s_trivial_table, k)))
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, alternativesT...>::type));

    // Invoke the move constructor.
    using function_type = void* (void*, void*);
    static constexpr function_type* s_jump_table[] = { wrapped_move_construct<alternativesT>... };
    return s_jump_table[k](dptr, sptr);
  }

template<typename alternativeT>
void*
wrapped_copy_assign(void* dptr, const void* sptr)
  {
    return ::std::addressof(*static_cast<alternativeT*>(dptr) = *static_cast<const alternativeT*>(sptr));
  }

template<typename... alternativesT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_copy_assign(size_t k, void* dptr, const void* sptr)
  {
    // Copy it trivially if possible. Note that both areas may overlap in the case of self assignment.
    static constexpr bool s_trivial_table[] = { is_trivially_copy_assignable<alternativesT>::value... };
    if(ROCKET_EXPECT(test_bit<>(s_trivial_table, k)))
      return ::std::memmove(dptr, sptr, sizeof(typename aligned_union<1, alternativesT...>::type));

    // Invoke the copy assignment operator.
    using function_type = void* (void*, const void*);
    static constexpr function_type* s_jump_table[] = { wrapped_copy_assign<alternativesT>... };
    return s_jump_table[k](dptr, sptr);
  }

template<typename alternativeT>
void*
wrapped_move_assign(void* dptr, void* sptr)
  {
    return ::std::addressof(*static_cast<alternativeT*>(dptr) = ::std::move(*static_cast<alternativeT*>(sptr)));
  }

template<typename... alternativesT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_move_assign(size_t k, void* dptr, void* sptr)
  {
    // Move it trivially if possible.
    static constexpr bool s_trivial_table[] = { is_trivially_move_assignable<alternativesT>::value... };
    if(ROCKET_EXPECT(test_bit<>(s_trivial_table, k)))
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, alternativesT...>::type));

    // Invoke the move assignment operator.
    using function_type = void* (void*, void*);
    static constexpr function_type* s_jump_table[] = { wrapped_move_assign<alternativesT>... };
    return s_jump_table[k](dptr, sptr);
  }

template<typename alternativeT>
void
wrapped_destroy(void* dptr)
  {
    return noadl::destroy_at(static_cast<alternativeT*>(dptr));
  }

template<typename... alternativesT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_destroy(size_t k, void* dptr)
  {
    // Destroy it trivially if possible.
    static constexpr bool s_trivial_table[] = { is_trivially_destructible<alternativesT>::value... };
    if(ROCKET_EXPECT(test_bit<>(s_trivial_table, k)))
      return;

    // Invoke the destructor.
    using function_type = void (void*);
    static constexpr function_type* s_jump_table[] = { wrapped_destroy<alternativesT>... };
    return s_jump_table[k](dptr);
  }

template<typename alternativeT>
void*
wrapped_move_then_destroy(void* dptr, void* sptr)
  {
    auto p = noadl::construct_at(static_cast<alternativeT*>(dptr), ::std::move(*static_cast<alternativeT*>(sptr)));
    noadl::destroy_at(static_cast<alternativeT*>(sptr));
    return p;
  }

template<typename... alternativesT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_move_then_destroy(size_t k, void* dptr, void* sptr)
  {
    // Move it trivially if possible.
    static constexpr bool s_trivial_table[] = { conjunction<is_trivially_move_constructible<alternativesT>,
                                                            is_trivially_destructible<alternativesT>>::value... };
    if(ROCKET_EXPECT(test_bit<>(s_trivial_table, k)))
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, alternativesT...>::type));

    // Invoke the move constructor followed by the destructor.
    using function_type = void* (void*, void*);
    static constexpr function_type* s_jump_table[] = { wrapped_move_then_destroy<alternativesT>... };
    return s_jump_table[k](dptr, sptr);
  }

template<typename alternativeT>
void
wrapped_xswap(void* dptr, void* sptr)
  {
    return noadl::xswap(*static_cast<alternativeT*>(dptr), *static_cast<alternativeT*>(sptr));
  }

template<typename... alternativesT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_swap(size_t k, void* dptr, void* sptr)
  {
    // Swap them trivially if possible.
    static constexpr bool s_trivial_table[] = { conjunction<is_trivially_move_constructible<alternativesT>...,
                                                            is_trivially_move_assignable<alternativesT>...,
                                                            is_trivially_destructible<alternativesT>...>::value };
    if(ROCKET_EXPECT(test_bit<>(s_trivial_table, k)))
      return wrapped_xswap<typename aligned_union<1, alternativesT...>::type>(dptr, sptr);

    // Invoke the `swap()` function via ADL.
    using function_type = void (void*, void*);
    static constexpr function_type* s_jump_table[] = { wrapped_xswap<alternativesT>... };
    return s_jump_table[k](dptr, sptr);
  }

template<typename alternativeT, typename voidT, typename visitorT>
void
wrapped_visit(voidT* sptr, visitorT&& visitor)
  {
    ::std::forward<visitorT>(visitor)(*static_cast<alternativeT*>(sptr));
  }

}  // namespace details_variant
