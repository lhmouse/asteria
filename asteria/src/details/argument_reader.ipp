// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ARGUMENT_READER_HPP_
#  error Please include <asteria/runtime/argument_reader.hpp> instead.
#endif

namespace asteria {
namespace details_argument_reader {

// This struct records all parameters of an overload.
// It can be copied elsewhere and back; any further operations will
// resume from that point.
struct State
  {
    cow_string params;
    uint32_t nparams = 0;
    bool ended = false;
    bool matched = false;
  };

// This ensures `Global_Context` is always passed as a non-const lvalue.
template<typename XValT>
constexpr typename ::std::remove_reference<XValT>::type&&
move(XValT&& xval) noexcept
  { return static_cast<typename ::std::remove_reference<XValT>::type&&>(xval);  }

constexpr Global_Context&
move(Global_Context& global) noexcept
  { return global;  }

// The void return type needs special treatment.
template<typename RetT, typename SeqT, typename FuncT, typename TupleT>
struct Applier;

template<typename RetT, size_t... N, typename FuncT, typename... ArgsT>
struct Applier<RetT, ::std::index_sequence<N...>, FuncT, ::std::tuple<ArgsT...>>
  {
    static Reference&
    do_apply(Reference& self, FuncT& func, ::std::tuple<ArgsT...>& args)
      {
        auto val = func(::std::get<N>(args)...);
        return self.set_temporary(::std::move(val));
      }
  };

template<size_t... N, typename FuncT, typename... ArgsT>
struct Applier<void, ::std::index_sequence<N...>, FuncT, ::std::tuple<ArgsT...>>
  {
    static Reference&
    do_apply(Reference& self, FuncT& func, ::std::tuple<ArgsT...>& args)
      {
        func(::std::get<N>(args)...);
        return self.set_void();
      }
  };

template<typename FuncT, typename... ArgsT>
inline Reference&
apply_and_set_result(Reference& self, FuncT&& func, ::std::tuple<ArgsT...>&& args)
  {
    return Applier<decltype(func(::std::declval<ArgsT>()...)),
                   ::std::index_sequence_for<ArgsT...>,
                   FuncT, ::std::tuple<ArgsT...>>::do_apply(self, func, args);
  }

}  // namespace details_argument_reader
}  // namespace asteria
