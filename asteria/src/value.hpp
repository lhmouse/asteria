// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VALUE_HPP_
#define ASTERIA_VALUE_HPP_

#include "fwd.hpp"
#include "abstract_opaque.hpp"
#include "abstract_function.hpp"

namespace Asteria {

class Value
  {
  public:
    using Xvariant = variant<
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
    static_assert(::std::is_nothrow_copy_assignable<Xvariant>::value, "");

  private:
    Xvariant m_stor;

  public:
    Value() noexcept
      :
        m_stor()  // Initialize to `null`.
      {
      }
    ASTERIA_VARIANT_CONSTRUCTOR(Value, Xvariant, XValT, xval)
      :
        m_stor(::rocket::forward<XValT>(xval))
      {
      }
    ASTERIA_VARIANT_ASSIGNMENT(Value, Xvariant, XValT, xval)
      {
        this->m_stor = ::rocket::forward<XValT>(xval);
        return *this;
      }
    // conversion from pointers
    template<typename XValT> Value(const XValT* opt)
      {
        this->do_check_assign<const XValT&>(opt, opt);
      }
    template<typename XValT> Value& operator=(const XValT* opt)
      {
        return this->do_check_assign<const XValT&>(opt, opt);
      }
    // conversion from optional values
    template<typename XValT> Value(const opt<XValT>& opt)
      {
        this->do_check_assign<const XValT&>(opt, opt);
      }
    template<typename XValT> Value& operator=(const opt<XValT>& opt)
      {
        return this->do_check_assign<const XValT&>(opt, opt);
      }
    template<typename XValT> Value(opt<XValT>&& opt) noexcept
      {
        this->do_check_assign<XValT&&>(opt, opt);
      }
    template<typename XValT> Value& operator=(opt<XValT>&& opt) noexcept
      {
        return this->do_check_assign<XValT&&>(opt, opt);
      }
    // conversion from `G_function` and `G_opaque`
    template<typename XValT> Value(const rcptr<XValT>& ptr)
      {
        this->do_check_assign<const rcptr<XValT>&>(ptr, ::std::addressof(ptr));
      }
    template<typename XValT> Value& operator=(const rcptr<XValT>& ptr)
      {
        return this->do_check_assign<const rcptr<XValT>&>(ptr, ::std::addressof(ptr));
      }
    template<typename XValT> Value(rcptr<XValT>&& ptr)
      {
        this->do_check_assign<rcptr<XValT>&&>(ptr, ::std::addressof(ptr));
      }
    template<typename XValT> Value& operator=(rcptr<XValT>&& ptr)
      {
        return this->do_check_assign<rcptr<XValT>&&>(ptr, ::std::addressof(ptr));
      }

  private:
    template<typename XValT,  // non-deducible
             typename ChkT, typename PtrT> Value& do_check_assign(ChkT& chk, PtrT&& ptr)
      {
        if(chk)
          this->m_stor = ::rocket::forward<XValT>(*ptr);
        else
          this->m_stor = nullptr;
        return *this;
      }

  public:
    Gtype gtype() const noexcept
      {
        return static_cast<Gtype>(this->m_stor.index());
      }
    const char* what_gtype() const noexcept
      {
        return describe_gtype(static_cast<Gtype>(this->m_stor.index()));
      }

    bool is_null() const noexcept
      {
        return this->gtype() == gtype_null;
      }

    bool is_boolean() const noexcept
      {
        return this->gtype() == gtype_boolean;
      }
    G_boolean as_boolean() const
      {
        return this->m_stor.as<gtype_boolean>();
      }
    G_boolean& open_boolean()
      {
        return this->m_stor.as<gtype_boolean>();
      }

    bool is_integer() const noexcept
      {
        return this->gtype() == gtype_integer;
      }
    G_integer as_integer() const
      {
        return this->m_stor.as<gtype_integer>();
      }
    G_integer& open_integer()
      {
        return this->m_stor.as<gtype_integer>();
      }

    bool is_real() const noexcept
      {
        return this->gtype() == gtype_real;
      }
    G_real as_real() const
      {
        return this->m_stor.as<gtype_real>();
      }
    G_real& open_real()
      {
        return this->m_stor.as<gtype_real>();
      }

    bool is_string() const noexcept
      {
        return this->gtype() == gtype_string;
      }
    const G_string& as_string() const
      {
        return this->m_stor.as<gtype_string>();
      }
    G_string& open_string()
      {
        return this->m_stor.as<gtype_string>();
      }

    bool is_function() const noexcept
      {
        return this->gtype() == gtype_function;
      }
    const G_function& as_function() const
      {
        return this->m_stor.as<gtype_function>();
      }
    G_function& open_function()
      {
        return this->m_stor.as<gtype_function>();
      }

    bool is_opaque() const noexcept
      {
        return this->gtype() == gtype_opaque;
      }
    const G_opaque& as_opaque() const
      {
        return this->m_stor.as<gtype_opaque>();
      }
    G_opaque& open_opaque()
      {
        auto& altr = this->m_stor.as<gtype_opaque>();
        if(ROCKET_UNEXPECT(altr.use_count() > 1)) {
          // Copy the opaque object as needed.
          G_opaque qown;
          auto qnew = altr->clone_opt(qown);
          ROCKET_ASSERT(qnew == qown.get());
          // Replace the current instance.
          if(qown)
            altr = ::rocket::move(qown);
        }
        return altr;
      }

    bool is_array() const noexcept
      {
        return this->gtype() == gtype_array;
      }
    const G_array& as_array() const
      {
        return this->m_stor.as<gtype_array>();
      }
    G_array& open_array()
      {
        return this->m_stor.as<gtype_array>();
      }

    bool is_object() const noexcept
      {
        return this->gtype() == gtype_object;
      }
    const G_object& as_object() const
      {
        return this->m_stor.as<gtype_object>();
      }
    G_object& open_object()
      {
        return this->m_stor.as<gtype_object>();
      }

    Value& swap(Value& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
        return *this;
      }

    bool is_convertible_to_real() const noexcept
      {
        if(this->gtype() == gtype_integer) {
          return true;
        }
        return this->gtype() == gtype_real;
      }
    G_real convert_to_real() const
      {
        if(this->gtype() == gtype_integer) {
          return G_real(this->m_stor.as<gtype_integer>());
        }
        return this->m_stor.as<gtype_real>();
      }
    G_real& mutate_into_real()
      {
        if(this->gtype() == gtype_integer) {
          return this->m_stor.emplace<gtype_real>(G_real(this->m_stor.as<gtype_integer>()));
        }
        return this->m_stor.as<gtype_real>();
      }

    ROCKET_PURE_FUNCTION bool test() const noexcept;
    ROCKET_PURE_FUNCTION Compare compare(const Value& other) const noexcept;

    ROCKET_PURE_FUNCTION bool unique() const noexcept;
    ROCKET_PURE_FUNCTION long use_count() const noexcept;
    ROCKET_PURE_FUNCTION long gcref_split() const noexcept;

    tinyfmt& print(tinyfmt& fmt, bool escape = false) const;
    tinyfmt& dump(tinyfmt& fmt, size_t indent = 2, size_t hanging = 0) const;
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

inline void swap(Value& lhs, Value& rhs) noexcept
  {
    lhs.swap(rhs);
  }

inline tinyfmt& operator<<(tinyfmt& fmt, const Value& value)
  {
    return value.dump(fmt);
  }

}  // namespace Asteria

#endif
