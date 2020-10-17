// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#  error Please include <rocket/variant.hpp> instead.
#endif

namespace details_variant {

template<size_t indexT, typename targetT, typename... altsT>
struct type_finder
  { };

template<size_t indexT, typename targetT, typename firstT, typename... restT>
struct type_finder<indexT, targetT, firstT, restT...>
  : type_finder<indexT + 1, targetT, restT...>  // recursive
  { };

template<size_t indexT, typename targetT, typename... restT>
struct type_finder<indexT, targetT, targetT, restT...>
  : integral_constant<size_t, indexT>  // found
  { };

template<size_t indexT, typename... altsT>
struct type_getter
  { };

template<size_t indexT, typename firstT, typename... restT>
struct type_getter<indexT, firstT, restT...>
  : type_getter<indexT - 1, restT...>  // recursive
  { };

template<typename firstT, typename... restT>
struct type_getter<0, firstT, restT...>
  : identity<firstT>  // found
  { };

// In a `catch` block that is conditionally unreachable, direct use of `throw` is
// subject to compiler warnings. Wrapping the `throw` expression in a function could
// silence this warning.
[[noreturn]] ROCKET_FORCED_INLINE_FUNCTION
void
rethrow_current_exception()
  { throw;  }

template<uint32_t... M>
struct u32seq
  { };

// Prepend `M` to `N...` to create a new sequence.
template<uint32_t M, typename S>
struct prepend_u32seq;

template<uint32_t M, uint32_t... N>
struct prepend_u32seq<M, u32seq<N...>>
  : identity<u32seq<M, N...>>
  { };

// Pack bools as bytes.
template<typename S>
struct pack_bool
  : identity<u32seq<>>
  { };

template<uint32_t A>
struct pack_bool<u32seq<A>>
  : identity<u32seq<(A)>>
  { };

template<uint32_t A, uint32_t B>
struct pack_bool<u32seq<A, B>>
  : identity<u32seq<(A | B << 1)>>
  { };

template<uint32_t A, uint32_t B, uint32_t C>
struct pack_bool<u32seq<A, B, C>>
  : identity<u32seq<(A | B << 1 | C << 2)>>
  { };

template<uint32_t A, uint32_t B, uint32_t C, uint32_t D>
struct pack_bool<u32seq<A, B, C, D>>
  : identity<u32seq<(A | B << 1 | C << 2 | D << 3)>>
  { };

template<uint32_t A, uint32_t B, uint32_t C, uint32_t D, uint32_t E>
struct pack_bool<u32seq<A, B, C, D, E>>
  : identity<u32seq<(A | B << 1 | C << 2 | D << 3 | E << 4)>>
  { };

template<uint32_t A, uint32_t B, uint32_t C, uint32_t D, uint32_t E, uint32_t F>
struct pack_bool<u32seq<A, B, C, D, E, F>>
  : identity<u32seq<(A | B << 1 | C << 2 | D << 3 | E << 4 | F << 5)>>
  { };

template<uint32_t A, uint32_t B, uint32_t C, uint32_t D, uint32_t E, uint32_t F, uint32_t G>
struct pack_bool<u32seq<A, B, C, D, E, F, G>>
  : identity<u32seq<(A | B << 1 | C << 2 | D << 3 | E << 4 | F << 5 | G << 6)>>
  { };

template<uint32_t A, uint32_t B, uint32_t C, uint32_t D, uint32_t E, uint32_t F, uint32_t G,
         uint32_t H, uint32_t... N>
struct pack_bool<u32seq<A, B, C, D, E, F, G, H, N...>>
  : prepend_u32seq<(A | B << 1 | C << 2 | D << 3 | E << 4 | F << 5 | G << 6 | H << 7),
                   typename pack_bool<u32seq<N...>>::type>
  { };

// Pack bytes as 32-bit integers in little endian.
template<typename S>
struct pack_byte
  : identity<u32seq<>>
  { };

template<uint32_t A>
struct pack_byte<u32seq<A>>
  : identity<u32seq<(A)>>
  { };

template<uint32_t A, uint32_t B>
struct pack_byte<u32seq<A, B>>
  : identity<u32seq<(A | B << 8)>>
  { };

template<uint32_t A, uint32_t B, uint32_t C>
struct pack_byte<u32seq<A, B, C>>
  : identity<u32seq<(A | B << 8 | C << 16)>>
  { };

template<uint32_t A, uint32_t B, uint32_t C, uint32_t D, uint32_t... N>
struct pack_byte<u32seq<A, B, C, D, N...>>
  : prepend_u32seq<(A | B << 8 | C << 16 | D << 24),
                   typename pack_byte<u32seq<N...>>::type>
  { };

// Store packed integers into an array.
template<bool... B>
class const_bitset
  {
  private:
    uint32_t m_words[(sizeof...(B) + 31) / 32];

    template<uint32_t... M>
    explicit constexpr
    const_bitset(u32seq<M...>)
      noexcept
      : m_words{ M... }
      { }

  public:
    constexpr
    const_bitset()
      noexcept
      : const_bitset(typename pack_byte<typename pack_bool<u32seq<B...>>::type>::type())
      { }

    constexpr
    bool
    operator[](size_t k)
      const noexcept
      { return this->m_words[k / 32] & (uint32_t(1) << k % 32);  }
  };

template<typename targetT, targetT*... ptrsT>
class ptr_table
  {
  private:
    targetT* m_ptrs[sizeof...(ptrsT)] = { ptrsT... };

  public:
    constexpr
    ptr_table()
      noexcept
      { }

    constexpr
    targetT*
    operator[](size_t k)
      const noexcept
      { return this->m_ptrs[k];  }
  };

template<typename altT>
void*
wrapped_copy_construct(void* dptr, const void* sptr)
  {
    return noadl::construct_at(static_cast<altT*>(dptr), *static_cast<const altT*>(sptr));
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_copy_construct(size_t k, void* dptr, const void* sptr)
  {
    static constexpr const_bitset<is_trivially_copy_constructible<altsT>::value...> trivial;
    static constexpr ptr_table<void* (void*, const void*), wrapped_copy_construct<altsT>...> ptrs;

    if(ROCKET_EXPECT(trivial[k]))
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, altsT...>::type));
    else
      return ptrs[k](dptr, sptr);
  }

template<typename altT>
void*
wrapped_move_construct(void* dptr, void* sptr)
  {
    return noadl::construct_at(static_cast<altT*>(dptr), ::std::move(*static_cast<altT*>(sptr)));
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_move_construct(size_t k, void* dptr, void* sptr)
  {
    static constexpr const_bitset<is_trivially_move_constructible<altsT>::value...> trivial;
    static constexpr ptr_table<void* (void*, void*), wrapped_move_construct<altsT>...> ptrs;

    if(ROCKET_EXPECT(trivial[k]))
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, altsT...>::type));
    else
      return ptrs[k](dptr, sptr);
  }

template<typename altT>
void*
wrapped_copy_assign(void* dptr, const void* sptr)
  {
    return ::std::addressof(*static_cast<altT*>(dptr) = *static_cast<const altT*>(sptr));
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_copy_assign(size_t k, void* dptr, const void* sptr)
  {
    static constexpr const_bitset<is_trivially_copy_assignable<altsT>::value...> trivial;
    static constexpr ptr_table<void* (void*, const void*), wrapped_copy_assign<altsT>...> ptrs;

    if(ROCKET_EXPECT(trivial[k]))
      return ::std::memmove(dptr, sptr, sizeof(typename aligned_union<1, altsT...>::type));
    else
      return ptrs[k](dptr, sptr);
  }

template<typename altT>
void*
wrapped_move_assign(void* dptr, void* sptr)
  {
    return ::std::addressof(*static_cast<altT*>(dptr) = ::std::move(*static_cast<altT*>(sptr)));
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_move_assign(size_t k, void* dptr, void* sptr)
  {
    static constexpr const_bitset<is_trivially_move_assignable<altsT>::value...> trivial;
    static constexpr ptr_table<void* (void*, void*), wrapped_move_assign<altsT>...> ptrs;

    if(ROCKET_EXPECT(trivial[k]))
      return ::std::memmove(dptr, sptr, sizeof(typename aligned_union<1, altsT...>::type));
    else
      return ptrs[k](dptr, sptr);
  }

template<typename altT>
void
wrapped_destroy(void* dptr)
  {
    return noadl::destroy_at(static_cast<altT*>(dptr));
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_destroy(size_t k, void* dptr)
  {
    static constexpr const_bitset<is_trivially_destructible<altsT>::value...> trivial;
    static constexpr ptr_table<void (void*), wrapped_destroy<altsT>...> ptrs;

    if(ROCKET_EXPECT(trivial[k]))
      return;
    else
      return ptrs[k](dptr);
  }

template<typename altT>
void*
wrapped_move_then_destroy(void* dptr, void* sptr)
  {
    auto p = noadl::construct_at(static_cast<altT*>(dptr), ::std::move(*static_cast<altT*>(sptr)));
    noadl::destroy_at(static_cast<altT*>(sptr));
    return p;
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void*
dispatch_move_then_destroy(size_t k, void* dptr, void* sptr)
  {
    static constexpr const_bitset<is_trivially_copyable<altsT>::value...> trivial;
    static constexpr ptr_table<void* (void*, void*), wrapped_move_then_destroy<altsT>...> ptrs;

    if(ROCKET_EXPECT(trivial[k]))
      return ::std::memcpy(dptr, sptr, sizeof(typename aligned_union<1, altsT...>::type));
    else
      return ptrs[k](dptr, sptr);
  }

template<typename altT>
void
wrapped_xswap(void* dptr, void* sptr)
  {
    return noadl::xswap(*static_cast<altT*>(dptr), *static_cast<altT*>(sptr));
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_swap(size_t k, void* dptr, void* sptr)
  {
    static constexpr const_bitset<is_trivially_copyable<altsT>::value...> trivial;
    static constexpr ptr_table<void (void*, void*), wrapped_xswap<altsT>...> ptrs;

    if(ROCKET_EXPECT(trivial[k]))
      return wrapped_xswap<typename aligned_union<1, altsT...>::type>(dptr, sptr);
    else
      return ptrs[k](dptr, sptr);
  }

template<typename altT, typename voidT, typename visitorT>
void
wrapped_visit(voidT* sptr, visitorT&& visitor)
  {
    ::std::forward<visitorT>(visitor)(*static_cast<altT*>(sptr));
  }

}  // namespace details_variant
