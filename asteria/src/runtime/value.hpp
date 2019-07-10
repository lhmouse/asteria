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
        , G_null      // 0,
        , G_boolean   // 1,
        , G_integer   // 2,
        , G_real      // 3,
        , G_string    // 4,
        , G_opaque    // 5,
        , G_function  // 6,
        , G_array     // 7,
        , G_object    // 8,
      )>;
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "???");

  public:
    // The objects returned by these functions are allocated statically and exist throughout the program.
    ROCKET_PURE_FUNCTION static const char* get_gtype_name(Gtype gtype) noexcept;
    ROCKET_PURE_FUNCTION static const Value& get_null() noexcept;

  private:
    Xvariant m_stor;

  public:
    Value() noexcept
      : m_stor()  // Initialize to `null`.
      {
      }
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Xvariant::index_of<typename std::decay<AltT>::type>::value)> Value(AltT&& altr) noexcept
      : m_stor(rocket::forward<AltT>(altr))
      {
      }
    template<typename AltT, ROCKET_ENABLE_IF_HAS_VALUE(Xvariant::index_of<typename std::decay<AltT>::type>::value)> Value& operator=(AltT&& altr) noexcept
      {
        this->m_stor = rocket::forward<AltT>(altr);
        return *this;
      }

  public:
    Gtype gtype() const noexcept
      {
        return static_cast<Gtype>(this->m_stor.index());
      }
    const char* gtype_name() const noexcept
      {
        return Value::get_gtype_name(static_cast<Gtype>(this->m_stor.index()));
      }

    bool is_null() const noexcept
      {
        return this->m_stor.index() == gtype_null;
      }

    bool is_boolean() const noexcept
      {
        return this->m_stor.index() == gtype_boolean;
      }
    G_boolean as_boolean() const
      {
        return this->m_stor.as<gtype_boolean>();
      }
    G_boolean& mut_boolean()
      {
        return this->m_stor.as<gtype_boolean>();
      }

    bool is_integer() const noexcept
      {
        return this->m_stor.index() == gtype_integer;
      }
    G_integer as_integer() const
      {
        return this->m_stor.as<gtype_integer>();
      }
    G_integer& mut_integer()
      {
        return this->m_stor.as<gtype_integer>();
      }

    bool is_real() const noexcept
      {
        return this->m_stor.index() == gtype_real;
      }
    G_real as_real() const
      {
        return this->m_stor.as<gtype_real>();
      }
    G_real& mut_real()
      {
        return this->m_stor.as<gtype_real>();
      }

    bool is_string() const noexcept
      {
        return this->m_stor.index() == gtype_string;
      }
    const G_string& as_string() const
      {
        return this->m_stor.as<gtype_string>();
      }
    G_string& mut_string()
      {
        return this->m_stor.as<gtype_string>();
      }

    bool is_function() const noexcept
      {
        return this->m_stor.index() == gtype_function;
      }
    const G_function& as_function() const
      {
        return this->m_stor.as<gtype_function>();
      }
    G_function& mut_function()
      {
        return this->m_stor.as<gtype_function>();
      }

    bool is_opaque() const noexcept
      {
        return this->m_stor.index() == gtype_opaque;
      }
    const G_opaque& as_opaque() const
      {
        return this->m_stor.as<gtype_opaque>();
      }
    G_opaque& mut_opaque()
      {
        return this->m_stor.as<gtype_opaque>();
      }

    bool is_array() const noexcept
      {
        return this->m_stor.index() == gtype_array;
      }
    const G_array& as_array() const
      {
        return this->m_stor.as<gtype_array>();
      }
    G_array& mut_array()
      {
        return this->m_stor.as<gtype_array>();
      }

    bool is_object() const noexcept
      {
        return this->m_stor.index() == gtype_object;
      }
    const G_object& as_object() const
      {
        return this->m_stor.as<gtype_object>();
      }
    G_object& mut_object()
      {
        return this->m_stor.as<gtype_object>();
      }

    void swap(Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
      }

    bool is_convertible_to_real() const noexcept
      {
        if(this->m_stor.index() == gtype_integer) {
          return true;
        }
        return this->m_stor.index() == gtype_real;
      }
    G_real convert_to_real() const
      {
        if(this->m_stor.index() == gtype_integer) {
          return G_real(this->m_stor.as<gtype_integer>());
        }
        return this->m_stor.as<gtype_real>();
      }
    G_real& mutate_into_real()
      {
        if(this->m_stor.index() == gtype_integer) {
          return this->m_stor.emplace<gtype_real>(G_real(this->m_stor.as<gtype_integer>()));
        }
        return this->m_stor.as<gtype_real>();
      }

    ROCKET_PURE_FUNCTION bool test() const noexcept;
    ROCKET_PURE_FUNCTION Compare compare(const Value& other) const noexcept;

    ROCKET_PURE_FUNCTION bool unique() const noexcept;
    ROCKET_PURE_FUNCTION long use_count() const noexcept;
    ROCKET_PURE_FUNCTION long gcref_split() const noexcept;

    std::ostream& print(std::ostream& os, bool escape = false) const;
    std::ostream& dump(std::ostream& os, std::size_t indent = 2, std::size_t hanging = 0) const;
    void enumerate_variables(const Abstract_Variable_Callback& callback) const;
  };

inline void swap(Value& lhs, Value& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

inline std::ostream& operator<<(std::ostream& os, const Value& value)
  {
    return value.dump(os);
  }

}  // namespace Asteria

#endif
