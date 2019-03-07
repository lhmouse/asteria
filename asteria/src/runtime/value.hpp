// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VALUE_HPP_
#define ASTERIA_RUNTIME_VALUE_HPP_

#include "../fwd.hpp"
#include "abstract_opaque.hpp"
#include "abstract_function.hpp"
#include "../rocket/preprocessor_utilities.h"
#include "../rocket/variant.hpp"

namespace Asteria {

class Value
  {
  public:
    enum Compare : std::uint8_t
      {
        compare_unordered  = 0,
        compare_less       = 1,
        compare_equal      = 2,
        compare_greater    = 3,
      };

    using Variant = rocket::variant<
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
    static_assert(rocket::is_nothrow_copy_constructible<Variant>::value, "???");

  public:
    template<typename TypeT> ROCKET_PURE_FUNCTION static constexpr Value_Type get_type() noexcept
      {
        return static_cast<Value_Type>(Variant::index_of<TypeT>::value);
      }
    template<typename TypeT> ROCKET_PURE_FUNCTION static const char * get_type_name() noexcept
      {
        return Value::get_type_name(get_type<TypeT>());
      }

    // The objects returned by these functions are allocated statically and exist throughout the program.
    ROCKET_PURE_FUNCTION static const char * get_type_name(Value_Type etype) noexcept;
    ROCKET_PURE_FUNCTION static const Value & get_null() noexcept;

  private:
    Variant m_stor;

  public:
    Value() noexcept
      : m_stor()  // Initialize to `null`.
      {
      }
    template<typename AltT, ROCKET_ENABLE_IF(std::is_constructible<Variant, AltT &&>::value)> Value(AltT &&alt)
      : m_stor(std::forward<AltT>(alt))
      {
      }
    template<typename AltT, ROCKET_ENABLE_IF(std::is_assignable<Variant, AltT &&>::value)> Value & operator=(AltT &&alt)
      {
        this->m_stor = std::forward<AltT>(alt);
        return *this;
      }

  public:
    Value_Type type() const noexcept
      {
        return static_cast<Value_Type>(this->m_stor.index());
      }
    template<typename AltT> const AltT * opt() const noexcept
      {
        return this->m_stor.get<AltT>();
      }
    template<typename AltT> const AltT & check() const
      {
        return this->m_stor.as<AltT>();
      }

    template<typename AltT> AltT * opt() noexcept
      {
        return this->m_stor.get<AltT>();
      }
    template<typename AltT> AltT & check()
      {
        return this->m_stor.as<AltT>();
      }

    bool test() const noexcept;
    Compare compare(const Value &other) const noexcept;
    void print(std::ostream &os, std::size_t indent_increment = 2, std::size_t indent_next = 0) const;

    bool unique() const noexcept;
    long use_count() const noexcept;
    void enumerate_variables(const Abstract_Variable_Callback &callback) const;
  };

inline std::ostream & operator<<(std::ostream &os, const Value &value)
  {
    value.print(os);
    return os;
  }

}  // namespace Asteria

#endif
