// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

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
  : identity<M>  // found
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

// Store packed integers into an array.
template<bool... B>
class const_bitset
  {
  private:
    uint32_t m_words[(sizeof...(B) + 31) / 32];

  public:
    constexpr
    const_bitset()
      noexcept
      : m_words{ }
      {
        constexpr uint32_t ext_bits[] = { B... };
        for(size_t k = 0;  k != sizeof...(B);  ++k)
          this->m_words[k / 32] |= ext_bits[k] << k % 32;
      }

    constexpr
    bool
    operator[](size_t k)
      const noexcept
      { return this->m_words[k / 32] & UINT32_C(1) << k % 32;  }
  };

template<typename targetT, targetT*... ptrsT>
class const_func_table
  {
  private:
    targetT* m_ptrs[sizeof...(ptrsT)];

  public:
    constexpr
    const_func_table()
      noexcept
      : m_ptrs{ ptrsT... }
      { }

    template<typename... argsT>
    constexpr
    typename ::std::result_of<targetT*(argsT&&...)>::type
    operator()(size_t k, argsT&&... args)
      const
      { return this->m_ptrs[k](::std::forward<argsT>(args)...);  }
  };

// In a `catch` block that is conditionally unreachable, direct use
// of `throw` is subject to compiler warnings. Wrapping the `throw`
// expression in a function silences this warning.
[[noreturn]] ROCKET_FORCED_INLINE_FUNCTION
void
rethrow_current_exception()
  { throw;  }

// These are copy/move helpers.
template<typename altT>
void
wrapped_copy_construct(void* dptr, const void* sptr)
  {
    auto dp = static_cast<altT*>(dptr);
    auto sp = static_cast<const altT*>(sptr);
    noadl::construct_at(dp, *sp);
  }

template<typename altT>
void
wrapped_move_construct(void* dptr, void* sptr)
  {
    auto dp = static_cast<altT*>(dptr);
    auto sp = static_cast<altT*>(sptr);
    noadl::construct_at(dp, ::std::move(*sp));
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
    noadl::destroy_at(sp);
  }

template<typename altT>
void
wrapped_move_then_destroy(void* dptr, void* sptr)
  {
    auto dp = static_cast<altT*>(dptr);
    auto sp = static_cast<altT*>(sptr);
    noadl::construct_at(dp, ::std::move(*sp));
    noadl::destroy_at(sp);
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

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_copy_construct(size_t k, void* dptr, const void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_copy_constructible<altsT>::value...>();

    if(ROCKET_EXPECT(trivial[k])) {
      // Get the maximum possible size of trivial storage.
      static constexpr auto nbytes =
          max_size<(sizeof(altsT) *
              !is_empty<altsT>::value *
              is_trivially_copy_constructible<altsT>::value)...>::value;

      if(nbytes != 0)
        ::std::memcpy(dptr, sptr, nbytes);
    }
    else {
      // Use a jump table for non-trivial types.
      static constexpr auto funcs =
          const_func_table<void (void*, const void*),
                         wrapped_copy_construct<altsT>...>();

      funcs(k, dptr, sptr);
    }
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_move_construct(size_t k, void* dptr, void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_move_constructible<altsT>::value...>();

    if(ROCKET_EXPECT(trivial[k])) {
      // Get the maximum possible size of trivial storage.
      static constexpr auto nbytes =
          max_size<(sizeof(altsT) *
              !is_empty<altsT>::value *
              is_trivially_move_constructible<altsT>::value)...>::value;

      if(nbytes != 0)
        ::std::memcpy(dptr, sptr, nbytes);
    }
    else {
      // Use a jump table for non-trivial types.
      static constexpr auto funcs =
          const_func_table<void (void*, void*),
                         wrapped_move_construct<altsT>...>();

      funcs(k, dptr, sptr);
    }
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_move_then_destroy(size_t k, void* dptr, void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_copyable<altsT>::value...>();

    if(ROCKET_EXPECT(trivial[k])) {
      // Get the maximum possible size of trivial storage.
      static constexpr auto nbytes =
          max_size<(sizeof(altsT) *
              !is_empty<altsT>::value *
              is_trivially_copyable<altsT>::value)...>::value;

      if(nbytes != 0)
        ::std::memcpy(dptr, sptr, nbytes);
    }
    else {
      // Use a jump table for non-trivial types.
      static constexpr auto funcs =
          const_func_table<void (void*, void*),
                         wrapped_move_then_destroy<altsT>...>();

      funcs(k, dptr, sptr);
    }
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_destroy(size_t k, void* sptr)
  {
    static constexpr auto trivial =
        const_bitset<is_trivially_destructible<altsT>::value...>();

    if(ROCKET_EXPECT(trivial[k]))
      return;

    // Use a jump table for non-trivial types.
    static constexpr auto funcs =
        const_func_table<void (void*),
                         wrapped_destroy<altsT>...>();

    funcs(k, sptr);
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_copy_assign(size_t k, void* dptr, const void* sptr)
  {
    if(ROCKET_UNEXPECT(dptr == sptr))
      return;

    static constexpr auto trivial =
        const_bitset<is_trivially_copy_assignable<altsT>::value...>();

    if(ROCKET_EXPECT(trivial[k])) {
      // Get the maximum possible size of trivial storage.
      static constexpr auto nbytes =
          max_size<(sizeof(altsT) *
              !is_empty<altsT>::value *
              is_trivially_copy_assignable<altsT>::value)...>::value;

      ::std::memcpy(dptr, sptr, nbytes);
    }
    else {
      // Use a jump table for non-trivial types.
      static constexpr auto funcs =
          const_func_table<void (void*, const void*),
                         wrapped_copy_assign<altsT>...>();

      funcs(k, dptr, sptr);
    }
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_move_assign(size_t k, void* dptr, void* sptr)
  {
    if(ROCKET_UNEXPECT(dptr == sptr))
      return;

    static constexpr auto trivial =
        const_bitset<is_trivially_move_assignable<altsT>::value...>();

    if(ROCKET_EXPECT(trivial[k])) {
      // Get the maximum possible size of trivial storage.
      static constexpr auto nbytes =
          max_size<(sizeof(altsT) *
              !is_empty<altsT>::value *
              is_trivially_move_assignable<altsT>::value)...>::value;

      ::std::memcpy(dptr, sptr, nbytes);
    }
    else {
      // Use a jump table for non-trivial types.
      static constexpr auto funcs =
          const_func_table<void (void*, void*),
                         wrapped_move_assign<altsT>...>();

      funcs(k, dptr, sptr);
    }
  }

template<typename... altsT>
ROCKET_FORCED_INLINE_FUNCTION
void
dispatch_xswap(size_t k, void* dptr, void* sptr)
  {
    if(ROCKET_UNEXPECT(dptr == sptr))
      return;

    static constexpr auto trivial =
        const_bitset<is_trivially_move_assignable<altsT>::value...>();

    if(ROCKET_EXPECT(trivial[k])) {
      // Get the maximum possible size of trivial storage.
      static constexpr auto nbytes =
          max_size<(sizeof(altsT) *
              !is_empty<altsT>::value *
              is_trivially_move_assignable<altsT>::value)...>::value;

      if(nbytes != 0)
        wrapped_xswap<typename aligned_union<nbytes,
                                 char>::type>(dptr, sptr);
    }
    else {
      // Use a jump table for non-trivial types.
      static constexpr auto funcs =
          const_func_table<void (void*, void*),
                           wrapped_xswap<altsT>...>();

      funcs(k, dptr, sptr);
    }
  }

}  // namespace details_variant
