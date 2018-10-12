// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_HPP_
#define ASTERIA_VALUE_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"
#include "rocket/refcounted_ptr.hpp"
#include "shared_opaque_wrapper.hpp"
#include "shared_function_wrapper.hpp"

namespace Asteria {

class Value
  {
  public:
    enum Compare : Uint8
      {
        compare_unordered  = 0,
        compare_less       = 1,
        compare_equal      = 2,
        compare_greater    = 3,
      };

    enum Type : Uint8
      {
        type_null      = 0,
        type_boolean   = 1,
        type_integer   = 2,
        type_real      = 3,
        type_string    = 4,
        type_opaque    = 5,
        type_function  = 6,
        type_array     = 7,
        type_object    = 8,
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

  public:
    static const char * get_type_name(Type type) noexcept;

  private:
    Variant m_stor;

  public:
    Value() noexcept
      : m_stor()  // Initialize to `null`.
      {
      }
    template<typename AltT, typename std::enable_if<std::is_constructible<Variant, AltT &&>::value>::type * = nullptr>
      Value(AltT &&alt)
        : m_stor(std::forward<AltT>(alt))
        {
        }
    template<typename AltT, typename std::enable_if<std::is_constructible<Variant, AltT &&>::value>::type * = nullptr>
      Value & operator=(AltT &&alt)
        {
          this->m_stor = std::forward<AltT>(alt);
          return *this;
        }
    ~Value();

    Value(const Value &) noexcept;
    Value & operator=(const Value &) noexcept;
    Value(Value &&) noexcept;
    Value & operator=(Value &&) noexcept;

  public:
    Type type() const noexcept
      {
        return Type(this->m_stor.index());
      }
    template<typename AltT>
      const AltT * opt() const noexcept
        {
          return this->m_stor.get<AltT>();
        }
    template<typename AltT>
      const AltT & check() const
        {
          return this->m_stor.as<AltT>();
        }

    template<typename AltT>
      AltT * opt() noexcept
        {
          return this->m_stor.get<AltT>();
        }
    template<typename AltT>
      AltT & check()
        {
          return this->m_stor.as<AltT>();
        }

    bool test() const noexcept;
    Compare compare(const Value &other) const noexcept;
    void dump(std::ostream &os, Size indent_increment = 2, Size indent_next = 0) const;

    void collect_variables(bool (*callback)(void *, const rocket::refcounted_ptr<Variable> &), void *param) const;
  };

inline std::ostream & operator<<(std::ostream &os, const Value &value)
  {
    value.dump(os);
    return os;
  }

}

#endif
