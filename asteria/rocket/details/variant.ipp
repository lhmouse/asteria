// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#  error Please include <rocket/variant.hpp> instead.
#endif

namespace details_variant {

template<size_t indexT, typename targetT, typename... alternsT>
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

template<size_t indexT, typename... alternsT>
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

template<typename alternT>
void*
wrapped_copy_construct(void* dptr, const void* sptr)
  {
    return noadl::construct_at(static_cast<alternT*>(dptr), *static_cast<const alternT*>(sptr));
  }

template<typename... alternsT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_copy_construct(size_t k, void* dptr, const void* sptr)
  {
    using function_type = void* (void*, const void*);
    static constexpr function_type* table[] = { (is_trivially_copy_constructible<alternsT>::value
                                                   ? nullptr
                                                   : wrapped_copy_construct<alternsT>)... };
    if(auto qfunc = table[k])
      return qfunc(dptr, sptr);
    else
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, alternsT...>::type));
  }

template<typename alternT>
void*
wrapped_move_construct(void* dptr, void* sptr)
  {
    return noadl::construct_at(static_cast<alternT*>(dptr), ::std::move(*static_cast<alternT*>(sptr)));
  }

template<typename... alternsT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_move_construct(size_t k, void* dptr, void* sptr)
  {
    using function_type = void* (void*, void*);
    static constexpr function_type* table[] = { (is_trivially_move_constructible<alternsT>::value
                                                   ? nullptr
                                                   : wrapped_move_construct<alternsT>)... };
    if(auto qfunc = table[k])
      return qfunc(dptr, sptr);
    else
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, alternsT...>::type));
  }

template<typename alternT>
void*
wrapped_copy_assign(void* dptr, const void* sptr)
  {
    return ::std::addressof(*static_cast<alternT*>(dptr) = *static_cast<const alternT*>(sptr));
  }

template<typename... alternsT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_copy_assign(size_t k, void* dptr, const void* sptr)
  {
    using function_type = void* (void*, const void*);
    static constexpr function_type* table[] = { (is_trivially_copy_assignable<alternsT>::value
                                                   ? nullptr
                                                   : wrapped_copy_assign<alternsT>)... };
    if(auto qfunc = table[k])
      return qfunc(dptr, sptr);
    else
      return ::std::memmove(dptr, sptr, sizeof(typename aligned_union<1, alternsT...>::type));
  }

template<typename alternT>
void*
wrapped_move_assign(void* dptr, void* sptr)
  {
    return ::std::addressof(*static_cast<alternT*>(dptr) = ::std::move(*static_cast<alternT*>(sptr)));
  }

template<typename... alternsT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_move_assign(size_t k, void* dptr, void* sptr)
  {
    using function_type = void* (void*, void*);
    static constexpr function_type* table[] = { (is_trivially_move_assignable<alternsT>::value
                                                   ? nullptr
                                                   : wrapped_move_assign<alternsT>)... };
    if(auto qfunc = table[k])
      return qfunc(dptr, sptr);
    else
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, alternsT...>::type));
  }

template<typename alternT>
void
wrapped_destroy(void* dptr)
  {
    return noadl::destroy_at(static_cast<alternT*>(dptr));
  }

template<typename... alternsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_destroy(size_t k, void* dptr)
  {
    using function_type = void (void*);
    static constexpr function_type* table[] = { (is_trivially_destructible<alternsT>::value
                                                   ? nullptr
                                                   : wrapped_destroy<alternsT>)... };
    if(auto qfunc = table[k])
      return qfunc(dptr);
    else
      return;  // do nothing
  }

template<typename alternT>
void*
wrapped_move_then_destroy(void* dptr, void* sptr)
  {
    auto p = noadl::construct_at(static_cast<alternT*>(dptr), ::std::move(*static_cast<alternT*>(sptr)));
    noadl::destroy_at(static_cast<alternT*>(sptr));
    return p;
  }

template<typename... alternsT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_move_then_destroy(size_t k, void* dptr, void* sptr)
  {
    using function_type = void* (void*, void*);
    static constexpr function_type* table[] = { (is_trivially_move_constructible<alternsT>::value &&
                                                 is_trivially_destructible<alternsT>::value
                                                   ? nullptr
                                                   : wrapped_move_then_destroy<alternsT>)... };
    if(auto qfunc = table[k])
      return qfunc(dptr, sptr);
    else
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, alternsT...>::type));
  }

template<typename alternT>
void
wrapped_xswap(void* dptr, void* sptr)
  {
    return noadl::xswap(*static_cast<alternT*>(dptr), *static_cast<alternT*>(sptr));
  }

template<typename... alternsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_swap(size_t k, void* dptr, void* sptr)
  {
    using function_type = void (void*, void*);
    static constexpr function_type* table[] = { (is_trivially_move_constructible<alternsT>::value &&
                                                 is_trivially_move_assignable<alternsT>::value &&
                                                 is_trivially_destructible<alternsT>::value
                                                   ? nullptr
                                                   : wrapped_xswap<alternsT>)... };
    if(auto qfunc = table[k])
      return qfunc(dptr, sptr);
    else
      return wrapped_xswap<typename aligned_union<1, alternsT...>::type>(dptr, sptr);
  }

template<typename alternT, typename voidT, typename visitorT>
void
wrapped_visit(voidT* sptr, visitorT&& visitor)
  {
    ::std::forward<visitorT>(visitor)(*static_cast<alternT*>(sptr));
  }

}  // namespace details_variant
