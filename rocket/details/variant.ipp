// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_VARIANT_
#  error Please include <rocket/variant.hpp> instead.
#endif
namespace details_variant {

// Get the index of `T` in `S`.
template<size_t N, typename T, typename... S>
struct type_finder
  {
  };

template<size_t N, typename T, typename... S>
struct type_finder<N, T, T, S...>
  :
    integral_constant<size_t, N>  // found
  {
  };

template<size_t N, typename T, typename M, typename... S>
struct type_finder<N, T, M, S...>
  :
    type_finder<N + 1, T, S...>
  {
  };

// Get the `N`-th type in `S`.
template<size_t N, typename... S>
struct type_getter
  {
  };

template<typename M, typename... S>
struct type_getter<0, M, S...>
  :
    enable_if<true, M>  // found
  {
  };

template<size_t N, typename M, typename... S>
struct type_getter<N, M, S...>
  :
    type_getter<N - 1, S...>
  {
  };

// Define types for static lookup tables.
template<bool... bitsT>
class const_bitset
  {
  private:
    uint32_t m_popcnt = 0;
    uint32_t m_words[(sizeof...(bitsT) + 31) / 32] = { };

  public:
    constexpr
    const_bitset() noexcept
      {
        constexpr bool bits[] = { bitsT... };
        for(size_t k = 0;  k != sizeof...(bitsT);  ++k)
          if(bits[k]) {
            this->m_popcnt ++;
            this->m_words[k / 32] |= (1U << k % 32);
          }
      }

    constexpr
    bool
    operator[](size_t k) const noexcept
      {
        if(this->m_popcnt == 0)
          return false;
        else if(this->m_popcnt == sizeof...(bitsT))
          return true;
        else
          return this->m_words[k / 32] & (1U << k % 32);
      }
  };

template<typename targetT, targetT*... ptrsT>
class const_func_table
  {
  private:
    targetT* m_ptrs[sizeof...(ptrsT)] = { ptrsT... };

  public:
    constexpr
    const_func_table() noexcept = default;

    template<typename... argsT>
    constexpr
#ifdef __cpp_lib_is_invocable
    typename ::std::invoke_result<targetT*, argsT&&...>::type
#else
    typename ::std::result_of<targetT* (argsT&&...)>::type
#endif
    operator()(size_t k, argsT&&... args) const
      {
        return this->m_ptrs[k] (::std::forward<argsT>(args)...);
      }
  };

// In a `catch` block that is conditionally unreachable, direct use
// of `throw` is subject to compiler warnings. Wrapping the `throw`
// expression in a function silences this warning.
[[noreturn]] ROCKET_ALWAYS_INLINE
void
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
#ifdef __cpp_lib_invoke
    ::std::invoke(::std::forward<visitorT>(visitor), *sp);
#else
    ::std::ref(visitor) (*sp);
#endif
  }

constexpr size_t nbytes_eager_copy = 32;  // sizeof(__m128) * 2

template<typename... altsT>
ROCKET_ALWAYS_INLINE
void
dispatch_copy_construct(size_t k, void* dptr, const void* sptr)
  {
    static constexpr size_t max_size =
        noadl::max(sizeof(altsT)
                   * !is_empty<altsT>::value
                   * is_trivially_copy_constructible<altsT>::value...);

    static constexpr const_bitset<
        is_trivially_copy_constructible<altsT>::value...> trivial;

    static constexpr const_func_table<
        void (void*, const void*),
        wrapped_copy_construct<altsT>...> nt_funcs;

    // This can be optimized if the type is trivial.
    if((max_size > nbytes_eager_copy) || ROCKET_UNEXPECT(!trivial[k]))
      nt_funcs(k, dptr, sptr);
    else
      ::std::memcpy(dptr, sptr, max_size);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE
void
dispatch_move_construct(size_t k, void* dptr, void* sptr)
  {
    static constexpr size_t max_size =
        noadl::max(sizeof(altsT)
                   * !is_empty<altsT>::value
                   * is_trivially_move_constructible<altsT>::value...);

    static constexpr const_bitset<
        is_trivially_move_constructible<altsT>::value...> trivial;

    static constexpr const_func_table<
        void (void*, void*),
        wrapped_move_construct<altsT>...> nt_funcs;

    // This can be optimized if the type is trivial.
    if((max_size > nbytes_eager_copy) || ROCKET_UNEXPECT(!trivial[k]))
      nt_funcs(k, dptr, sptr);
    else
      ::std::memcpy(dptr, sptr, max_size);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE
void
dispatch_move_then_destroy(size_t k, void* dptr, void* sptr)
  {
    static constexpr size_t max_size =
        noadl::max(sizeof(altsT)
                   * !is_empty<altsT>::value
                   * is_trivially_copyable<altsT>::value...);

    static constexpr const_bitset<
        is_trivially_copyable<altsT>::value...> trivial;

    static constexpr const_func_table<
        void (void*, void*),
        wrapped_move_then_destroy<altsT>...> nt_funcs;

    // This can be optimized if the type is trivial.
    if((max_size > nbytes_eager_copy) || ROCKET_UNEXPECT(!trivial[k]))
      nt_funcs(k, dptr, sptr);
    else
      ::std::memcpy(dptr, sptr, max_size);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE
void
dispatch_destroy(size_t k, void* sptr)
  {
    static constexpr const_bitset<
        is_trivially_destructible<altsT>::value...> trivial;

    static constexpr const_func_table<
        void (void*),
        wrapped_destroy<altsT>...> nt_funcs;

    // This can be optimized if the type is trivial.
    if(ROCKET_UNEXPECT(!trivial[k]))
      nt_funcs(k, sptr);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE
void
dispatch_copy_assign(size_t k, void* dptr, const void* sptr)
  {
    static constexpr size_t max_size =
        noadl::max(sizeof(altsT)
                   * !is_empty<altsT>::value
                   * is_trivially_copy_assignable<altsT>::value...);

    static constexpr const_bitset<
        is_trivially_copy_assignable<altsT>::value...> trivial;

    static constexpr const_func_table<
        void (void*, const void*),
        wrapped_copy_assign<altsT>...> nt_funcs;

    // Don't bother writing overlapped regions.
    if(dptr == sptr)
      return;

    // This can be optimized if the type is trivial.
    if((max_size > nbytes_eager_copy) || ROCKET_UNEXPECT(!trivial[k]))
      nt_funcs(k, dptr, sptr);
    else
      ::std::memcpy(dptr, sptr, max_size);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE
void
dispatch_move_assign(size_t k, void* dptr, void* sptr)
  {
    static constexpr size_t max_size =
        noadl::max(sizeof(altsT)
                   * !is_empty<altsT>::value
                   * is_trivially_move_assignable<altsT>::value...);

    static constexpr const_bitset<
        is_trivially_move_assignable<altsT>::value...> trivial;

    static constexpr const_func_table<
        void (void*, void*),
        wrapped_move_assign<altsT>...> nt_funcs;

    // Don't bother writing overlapped regions.
    if(dptr == sptr)
      return;

    // This can be optimized if the type is trivial.
    if((max_size > nbytes_eager_copy) || ROCKET_UNEXPECT(!trivial[k]))
      nt_funcs(k, dptr, sptr);
    else
      ::std::memcpy(dptr, sptr, max_size);
  }

template<typename... altsT>
ROCKET_ALWAYS_INLINE
void
dispatch_xswap(size_t k, void* dptr, void* sptr)
  {
    static constexpr size_t max_size =
        noadl::max(sizeof(altsT)
                   * !is_empty<altsT>::value
                   * is_trivially_move_assignable<altsT>::value...);

    static constexpr const_bitset<
        is_trivially_move_assignable<altsT>::value...> trivial;

    static constexpr const_func_table<
        void (void*, void*),
        wrapped_xswap<altsT>...> nt_funcs;

    // Don't bother writing overlapped regions.
    if(dptr == sptr)
      return;

    // This can be optimized if the type is trivial.
    if((max_size > nbytes_eager_copy) || ROCKET_UNEXPECT(!trivial[k]))
      nt_funcs(k, dptr, sptr);
    else
      wrapped_xswap<typename aligned_union<0,
          typename conditional<is_trivially_move_assignable<altsT>::value,
                               altsT, char>::type...>
                               ::type>(dptr, sptr);
  }

}  // namespace details_variant
