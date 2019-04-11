// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VALUE_HPP_
#define ASTERIA_RUNTIME_VALUE_HPP_

#include "../fwd.hpp"
#include "abstract_opaque.hpp"
#include "abstract_function.hpp"

namespace Asteria {

class Value
  {
  public:
    enum Compare : std::uint8_t
      {
        compare_unordered  = 0,  // 0000
        compare_equal      = 1,  // 0001
        compare_less       = 8,  // 1000
        compare_greater    = 9,  // 1001
      };

    using Xvariant = Variant<
      ROCKET_CDR(
        , D_null      // 0,
        , D_boolean   // 1,
        , D_integer   // 2,
        , D_real      // 3,
        , D_string    // 4,
        , D_opaque    // 5,
        , D_function  // 6,
        , D_array     // 7,
        , D_object    // 8,
      )>;
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "???");

  private:
    template<typename XvalueT> static inline ROCKET_RETURN_ENABLE_IF(Compare, std::is_integral<XvalueT>::value) do_compare_3way(XvalueT lhs, XvalueT rhs) noexcept;
    static inline Compare do_compare_3way(D_real lhs, D_real rhs) noexcept;

    static inline std::ostream& do_auto_indent(std::ostream& os, std::size_t indent_increment, std::size_t indent_next);

  public:
    // The objects returned by these functions are allocated statically and exist throughout the program.
    ROCKET_PURE_FUNCTION static const char* get_type_name(Dtype etype) noexcept;
    ROCKET_PURE_FUNCTION static const Value& get_null() noexcept;

  private:
    Xvariant m_stor;

  public:
    Value() noexcept
      : m_stor()  // Initialize to `null`.
      {
      }
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Xvariant::index_of<typename std::decay<AltT>::type>::value)> Value(AltT&& alt) noexcept
      : m_stor(rocket::forward<AltT>(alt))
      {
      }
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Xvariant::index_of<typename std::decay<AltT>::type>::value)> Value& operator=(AltT&& alt) noexcept
      {
        this->m_stor = rocket::forward<AltT>(alt);
        return *this;
      }

  public:
    Dtype dtype() const noexcept
      {
        return static_cast<Dtype>(this->m_stor.index());
      }
    template<typename AltT> const AltT* opt() const noexcept
      {
        return this->m_stor.get<AltT>();
      }
    template<typename AltT> const AltT& check() const
      {
        return this->m_stor.as<AltT>();
      }

    template<typename AltT> AltT* opt() noexcept
      {
        return this->m_stor.get<AltT>();
      }
    template<typename AltT> AltT& check()
      {
        return this->m_stor.as<AltT>();
      }

    void swap(Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
      }

    bool is_convertible_to_real() const noexcept
      {
        return rocket::is_any_of(this->dtype(), { dtype_integer, dtype_real });
      }
    D_real convert_to_real() const;

    bool test() const noexcept;
    Compare compare(const Value& other) const noexcept;
    void print(std::ostream& os, bool quote_strings = false) const;
    void dump(std::ostream& os, std::size_t indent_increment = 2, std::size_t indent_next = 0) const;

    bool unique() const noexcept;
    long use_count() const noexcept;
    void enumerate_variables(const Abstract_Variable_Callback& callback) const;
  };

inline void swap(Value& lhs, Value& rhs) noexcept
  {
    lhs.swap(rhs);
  }

inline std::ostream& operator<<(std::ostream& os, const Value& value)
  {
    value.dump(os);
    return os;
  }

}  // namespace Asteria

#endif
