// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_HPP_
#  error Please include <rocket/variant.hpp> instead.
#endif

namespace details_variant {

// Get the index of `T` in `S`.
template<size_t N, typename T, typename... S>
struct type_finder
  { };

template<size_t N, typename T, typename... S>
struct type_finder<N, T, T, S...>
  : integral_constant<size_t, N>  // found
  { };

template<size_t N, typename T, typename M, typename... S>
struct type_finder<N, T, M, S...>
  : type_finder<N + 1, T, S...>
  { };

// Get the `N`-th type in `S`.
template<size_t N, typename... S>
struct type_getter
  { };

template<typename M, typename... S>
struct type_getter<0, M, S...>
  : enable_if<true, M>  // found
  { };

template<size_t N, typename M, typename... S>
struct type_getter<N, M, S...>
  : type_getter<N - 1, S...>
  { };

// Get the maximum argument.
template<size_t... N>
struct max_size;

template<size_t M>
struct max_size<M>
  : integral_constant<size_t, M>
  { };

template<size_t M, size_t N, size_t... S>
struct max_size<M, N, S...>
  : max_size<(N < M) ? M : N, S...>
  { };

// Pack bits as integers.
template<size_t N, typename C, uint32_t... S>
struct packed_words;

template<uint32_t... M, uint32_t... S>
struct packed_words<0, integer_sequence<uint32_t, M...>, S...>
  {
    uint32_t m_words[sizeof...(M)] = { M... };
  };

template<size_t N, uint32_t... M, uint32_t a0, uint32_t a1, uint32_t a2,
         uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7,
         uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3, uint32_t b4,
         uint32_t b5, uint32_t b6, uint32_t b7, uint32_t c0, uint32_t c1,
         uint32_t c2, uint32_t c3, uint32_t c4, uint32_t c5, uint32_t c6,
         uint32_t c7, uint32_t d0, uint32_t d1, uint32_t d2, uint32_t d3,
         uint32_t d4, uint32_t d5, uint32_t d6, uint32_t d7, uint32_t... S>
struct packed_words<N, integer_sequence<uint32_t, M...>,
         a0,a1,a2,a3,a4,a5,a6,a7, b0,b1,b2,b3,b4,b5,b6,b7,
         c0,c1,c2,c3,c4,c5,c6,c7, d0,d1,d2,d3,d4,d5,d6,d7, S...>
  : packed_words<
         N - 1, integer_sequence<uint32_t, M...,
         (a0|a1<<1|a2<<2|a3<<3|a4<<4|a5<<5|a6<<6|a7<<7|b0<<8|b1<<9|b2<<10|
          b3<<11|b4<<12|b5<<13|b6<<14|b7<<15|c0<<16|c1<<17|c2<<18|c3<<19|
          c4<<20|c5<<21|c6<<22|c7<<23|d0<<24|d1<<25|d2<<26|d3<<27|d4<<28|
          d5<<29|d6<<30|d7<<31)>, S...>
  { };

// These are jump tables.
template<bool... bitsT>
class const_bitset
  : private packed_words<(sizeof...(bitsT) + 31) / 32,
        integer_sequence<uint32_t>, bitsT..., 0,0,0,0,0,0,0,
              0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0>
  {
  public:
    constexpr
    const_bitset() noexcept
      = default;

    constexpr bool
    operator[](size_t k) const noexcept
      { return this->m_words[k / 32] & UINT32_C(1) << k % 32;  }
  };

template<typename targetT, targetT*... ptrsT>
class const_func_table
  {
  private:
    targetT* m_ptrs[sizeof...(ptrsT)] = { ptrsT... };

  public:
    constexpr
    const_func_table() noexcept
      = default;

    template<typename... argsT>
    constexpr typename ::std::result_of<targetT*(argsT&&...)>::type
    operator()(size_t k, argsT&&... args) const
      noexcept(noexcept(::std::declval<targetT*>()(args...)))
      { return this->m_ptrs[k](::std::forward<argsT>(args)...);  }
  };

// In a `catch` block that is conditionally unreachable, direct use
// of `throw` is subject to compiler warnings. Wrapping the `throw`
// expression in a function silences this warning.
[[noreturn]] ROCKET_ALWAYS_INLINE void
rethrow_current_exception()
  {
    throw;
  }

// These are copy/move helpers.
template<typename altT>
void
wrapped_copy_construct(void* dptr, const void* sptr)
  {
    auto dp = static_cast<altT*>(dptr);
    auto sp = static_cast<const altT*>(sptr);
    noadl::construct(dp, *sp);
  }

template<typename altT>
void
wrapped_move_construct(void* dptr, void* sptr)
  {
    auto dp = static_cast<altT*>(dptr);
    auto sp = static_cast<altT*>(sptr);
    noadl::construct(dp, ::std::move(*sp));
  }

template<typename altT>
void
wrapped_copy_assign(void* dptr, const void* sptr)
  {
    auto dp = static_cast<altT*>(dptr);
    auto sp = static_cast<const altT*>(sptr);
    *dp = *sp;
  }

template<typename altT>
void
wrapped_move_assign(void* dptr, void* sptr)
  {
    auto dp = static_cast<altT*>(dptr);
    auto sp = static_cast<altT*>(sptr);
    *dp = ::std::move(*sp);
  }

template<typename altT>
void
wrapped_destroy(void* sptr)
  {
    auto sp = static_cast<altT*>(sptr);
    noadl::destroy(sp);
  }

template<typename altT>
void
wrapped_move_then_destroy(void* dptr, void* sptr)
  {
    auto dp = static_cast<altT*>(dptr);
    auto sp = static_cast<altT*>(sptr);
    noadl::construct(dp, ::std::move(*sp));
    noadl::destroy(sp);
  }

template<typename altT>
void
wrapped_xswap(void* dptr, void* sptr)
  {
    auto dp = static_cast<altT*>(dptr);
    auto sp = static_cast<altT*>(sptr);
    noadl::xswap(*dp, *sp);
  }

template<typename xaltT, typename xvoidT, typename visitorT>
void
wrapped_visit(xvoidT* sptr, visitorT&& visitor)
  {
    auto sp = static_cast<xaltT*>(sptr);
    ::std::forward<visitorT>(visitor)(*sp);
  }

constexpr size_t nbytes_eager_copy = 8 * sizeof(void*);

template<typename... altsT>
ROCKET_ALWAYS_INLINE void
dispatch_copy_construct(size_t k, void* dptr, const void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_copy_constructible<altsT>::value...>();

    static constexpr auto t_nbytes =
        max_size<(sizeof(altsT)
                  * !is_empty<altsT>::value
                  * is_trivially_copy_constructible<altsT>::value)...>::value;

    static constexpr auto nt_funcs =
        const_func_table<void (void*, const void*),
                         wrapped_copy_construct<altsT>...>();

    // Perform an unconditional bytewise copy for constructors.
    if(t_nbytes - 1 < nbytes_eager_copy)
      ::std::memcpy(dptr, sptr, t_nbytes);

    // Call the constructor, only if the type is non-trivial.
    if(!trivial[k])
      return nt_funcs(k, dptr, sptr);

    // Perform a bytewise copy for trivial types.
    if(t_nbytes > nbytes_eager_copy)
      ::std::memcpy(dptr, sptr, t_nbytes);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE void
dispatch_move_construct(size_t k, void* dptr, void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_move_constructible<altsT>::value...>();

    static constexpr auto t_nbytes =
        max_size<(sizeof(altsT)
                  * !is_empty<altsT>::value
                  * is_trivially_move_constructible<altsT>::value)...>::value;

    static constexpr auto nt_funcs =
        const_func_table<void (void*, void*),
                         wrapped_move_construct<altsT>...>();

    // Perform an unconditional bytewise copy for constructors.
    if(t_nbytes - 1 < nbytes_eager_copy)
      ::std::memcpy(dptr, sptr, t_nbytes);

    // Call the constructor, only if the type is non-trivial.
    if(!trivial[k])
      return nt_funcs(k, dptr, sptr);

    // Perform a bytewise copy for trivial types.
    if(t_nbytes > nbytes_eager_copy)
      ::std::memcpy(dptr, sptr, t_nbytes);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE void
dispatch_move_then_destroy(size_t k, void* dptr, void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_copyable<altsT>::value...>();

    static constexpr auto t_nbytes =
        max_size<(sizeof(altsT)
                  * !is_empty<altsT>::value
                  * is_trivially_copyable<altsT>::value)...>::value;

    static constexpr auto nt_funcs =
        const_func_table<void (void*, void*),
                         wrapped_move_then_destroy<altsT>...>();

    // Perform an unconditional bytewise copy for constructors.
    if(t_nbytes - 1 < nbytes_eager_copy)
      ::std::memcpy(dptr, sptr, t_nbytes);

    // Call the constructor, only if the type is non-trivial.
    if(!trivial[k])
      return nt_funcs(k, dptr, sptr);

    // Perform a bytewise copy for trivial types.
    if(t_nbytes > nbytes_eager_copy)
      ::std::memcpy(dptr, sptr, t_nbytes);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE void
dispatch_destroy(size_t k, void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_destructible<altsT>::value...>();

    static constexpr auto nt_funcs =
        const_func_table<void (void*),
                         wrapped_destroy<altsT>...>();

    // Call the destructor, only if the type is non-trivial.
    if(!trivial[k])
      return nt_funcs(k, sptr);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE void
dispatch_copy_assign(size_t k, void* dptr, const void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_copy_assignable<altsT>::value...>();

    static constexpr auto t_nbytes =
        max_size<(sizeof(altsT)
                  * !is_empty<altsT>::value
                  * is_trivially_copy_assignable<altsT>::value)...>::value;

    static constexpr auto nt_funcs =
        const_func_table<void (void*, const void*),
                         wrapped_copy_assign<altsT>...>();

    // Don't bother writing overlapped regions.
    if(dptr == sptr)
      return;

    // If the type is non-trivial, call the assignment operator.
    if(!trivial[k])
      return nt_funcs(k, dptr, sptr);

    // Perform a bytewise copy for trivial types.
    if(t_nbytes != 0)
      ::std::memcpy(dptr, sptr, t_nbytes);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE void
dispatch_move_assign(size_t k, void* dptr, void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_move_assignable<altsT>::value...>();

    static constexpr auto t_nbytes =
        max_size<(sizeof(altsT)
                  * !is_empty<altsT>::value
                  * is_trivially_move_assignable<altsT>::value)...>::value;

    static constexpr auto nt_funcs =
        const_func_table<void (void*, void*),
                         wrapped_move_assign<altsT>...>();

    // Don't bother writing overlapped regions.
    if(dptr == sptr)
      return;

    // If the type is non-trivial, call the assignment operator.
    if(!trivial[k])
      return nt_funcs(k, dptr, sptr);

    // Perform a bytewise copy for trivial types.
    if(t_nbytes != 0)
      ::std::memcpy(dptr, sptr, t_nbytes);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE void
dispatch_xswap(size_t k, void* dptr, void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_move_assignable<altsT>::value...>();

    static constexpr auto t_nbytes =
        max_size<(sizeof(altsT)
                  * !is_empty<altsT>::value
                  * is_trivially_move_assignable<altsT>::value)...>::value;

    static constexpr auto nt_funcs =
        const_func_table<void (void*, void*),
                         wrapped_xswap<altsT>...>();

    // Declare the storage type for all trivial alternatives.
    using t_storage = typename aligned_union<0,
              typename conditional<is_trivially_move_assignable<altsT>::value,
                                   altsT, char>::type...>::type;

    // Don't bother writing overlapped regions.
    if(dptr == sptr)
      return;

    // If the type is non-trivial, call the swap function.
    if(!trivial[k])
      return nt_funcs(k, dptr, sptr);

    // Perform a bytewise swap for trivial types.
    if(t_nbytes != 0)
      wrapped_xswap<t_storage>(dptr, sptr);
  }

}  // namespace details_variant
