// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

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
    Value(nullptr_t = nullptr) noexcept
      {
      }
    Value(bool xval) noexcept
      :
        m_stor(G_boolean(xval))
      {
      }
    Value(signed char xval) noexcept
      :
        m_stor(G_integer(xval))
      {
      }
    Value(signed short xval) noexcept
      :
        m_stor(G_integer(xval))
      {
      }
    Value(signed xval) noexcept
      :
        m_stor(G_integer(xval))
      {
      }
    Value(signed long xval) noexcept
      :
        m_stor(G_integer(xval))
      {
      }
    Value(signed long long xval) noexcept
      :
        m_stor(G_integer(xval))
      {
      }
    Value(float xval) noexcept
      :
        m_stor(G_real(xval))
      {
      }
    Value(double xval) noexcept
      :
        m_stor(G_real(xval))
      {
      }
    Value(cow_string xval) noexcept
      :
        m_stor(G_string(::rocket::move(xval)))
      {
      }
    Value(cow_string::shallow_type xval) noexcept
      :
        m_stor(G_string(xval))
      {
      }
    template<typename OpaqueT,
             ROCKET_ENABLE_IF(::std::is_base_of<Abstract_Opaque,
                        OpaqueT>::value)> Value(rcptr<OpaqueT> xval) noexcept
      {
        // Note `xval` may be a null pointer, in which case we set `*this` to `null`.
        // The pointer itself is being moved, not the object that it points to.
        this->do_assign_opt<rcptr<OpaqueT>&&>(xval, ::std::addressof(xval));
      }
    template<typename FunctionT,  // TODO: Use a dedicated type for functions.
             ROCKET_ENABLE_IF(::std::is_base_of<Abstract_Function,
                        FunctionT>::value)> Value(rcptr<FunctionT> xval) noexcept
      {
        // Note `xval` may be a null pointer, in which case we set `*this` to `null`.
        this->do_assign_opt<rcptr<FunctionT>&&>(xval, ::std::addressof(xval));
      }
    Value(cow_vector<Value> xval) noexcept
      :
        m_stor(G_array(::rocket::move(xval)))
      {
      }
    Value(cow_hashmap<phsh_string, Value, phsh_string::hash> xval) noexcept
      :
        m_stor(G_object(::rocket::move(xval)))
      {
      }
    Value(const opt<bool>& xval) noexcept
      {
        this->do_assign_opt<G_boolean>(xval, xval);
      }
    Value(const opt<signed char>& xval) noexcept
      {
        this->do_assign_opt<G_integer>(xval, xval);
      }
    Value(const opt<signed short>& xval) noexcept
      {
        this->do_assign_opt<G_integer>(xval, xval);
      }
    Value(const opt<signed>& xval) noexcept
      {
        this->do_assign_opt<G_integer>(xval, xval);
      }
    Value(const opt<signed long>& xval) noexcept
      {
        this->do_assign_opt<G_integer>(xval, xval);
      }
    Value(const opt<signed long long>& xval) noexcept
      {
        this->do_assign_opt<G_integer>(xval, xval);
      }
    Value(const opt<float>& xval) noexcept
      {
        this->do_assign_opt<G_real>(xval, xval);
      }
    Value(const opt<double>& xval) noexcept
      {
        this->do_assign_opt<G_real>(xval, xval);
      }
    Value(const opt<cow_string>& xval) noexcept
      {
        this->do_assign_opt<const G_string&>(xval, xval);
      }
    Value(opt<cow_string>&& xval) noexcept
      {
        this->do_assign_opt<G_string&&>(xval, xval);
      }
    Value(const opt<cow_vector<Value>>& xval) noexcept
      {
        this->do_assign_opt<const G_array&>(xval, xval);
      }
    Value(opt<cow_vector<Value>>&& xval) noexcept
      {
        this->do_assign_opt<G_array&&>(xval, xval);
      }
    Value(const opt<cow_hashmap<phsh_string, Value, phsh_string::hash>>& xval) noexcept
      {
        this->do_assign_opt<const G_object&>(xval, xval);
      }
    Value(opt<cow_hashmap<phsh_string, Value, phsh_string::hash>>&& xval) noexcept
      {
        this->do_assign_opt<G_object&&>(xval, xval);
      }
    Value& operator=(nullptr_t) noexcept
      {
        this->m_stor = G_null();
        return *this;
      }
    Value& operator=(bool xval) noexcept
      {
        this->m_stor = G_boolean(xval);
        return *this;
      }
    Value& operator=(signed char xval) noexcept
      {
        this->m_stor = G_integer(xval);
        return *this;
      }
    Value& operator=(signed short xval) noexcept
      {
        this->m_stor = G_integer(xval);
        return *this;
      }
    Value& operator=(signed xval) noexcept
      {
        this->m_stor = G_integer(xval);
        return *this;
      }
    Value& operator=(signed long xval) noexcept
      {
        this->m_stor = G_integer(xval);
        return *this;
      }
    Value& operator=(signed long long xval) noexcept
      {
        this->m_stor = G_integer(xval);
        return *this;
      }
    Value& operator=(float xval) noexcept
      {
        this->m_stor = G_real(xval);
        return *this;
      }
    Value& operator=(double xval) noexcept
      {
        this->m_stor = G_real(xval);
        return *this;
      }
    Value& operator=(cow_string xval) noexcept
      {
        this->m_stor = G_string(::rocket::move(xval));
        return *this;
      }
    Value& operator=(cow_string::shallow_type xval) noexcept
      {
        this->m_stor = G_string(xval);
        return *this;
      }
    template<typename OpaqueT,
             ROCKET_ENABLE_IF(::std::is_base_of<Abstract_Opaque,
                        OpaqueT>::value)> Value& operator=(rcptr<OpaqueT> xval) noexcept
      {
        // Note `xval` may be a null pointer, in which case we set `*this` to `null`.
        // The pointer itself is being moved, not the object that it points to.
        this->do_assign_opt<rcptr<OpaqueT>&&>(xval, ::std::addressof(xval));
        return *this;
      }
    template<typename FunctionT,  // TODO: Use a dedicated type for functions.
             ROCKET_ENABLE_IF(::std::is_base_of<Abstract_Function,
                        FunctionT>::value)> Value& operator=(rcptr<FunctionT> xval) noexcept
      {
        // Note `xval` may be a null pointer, in which case we set `*this` to `null`.
        this->do_assign_opt<rcptr<FunctionT>&&>(xval, ::std::addressof(xval));
        return *this;
      }
    Value& operator=(cow_vector<Value> xval) noexcept
      {
        this->m_stor = G_array(::rocket::move(xval));
        return *this;
      }
    Value& operator=(cow_hashmap<phsh_string, Value, phsh_string::hash> xval) noexcept
      {
        this->m_stor = G_object(::rocket::move(xval));
        return *this;
      }
    Value& operator=(const opt<bool>& xval) noexcept
      {
        this->do_assign_opt<G_boolean>(xval, xval);
        return *this;
      }
    Value& operator=(const opt<signed char>& xval) noexcept
      {
        this->do_assign_opt<G_integer>(xval, xval);
        return *this;
      }
    Value& operator=(const opt<signed short>& xval) noexcept
      {
        this->do_assign_opt<G_integer>(xval, xval);
        return *this;
      }
    Value& operator=(const opt<signed>& xval) noexcept
      {
        this->do_assign_opt<G_integer>(xval, xval);
        return *this;
      }
    Value& operator=(const opt<signed long>& xval) noexcept
      {
        this->do_assign_opt<G_integer>(xval, xval);
        return *this;
      }
    Value& operator=(const opt<signed long long>& xval) noexcept
      {
        this->do_assign_opt<G_integer>(xval, xval);
        return *this;
      }
    Value& operator=(const opt<float>& xval) noexcept
      {
        this->do_assign_opt<G_real>(xval, xval);
        return *this;
      }
    Value& operator=(const opt<double>& xval) noexcept
      {
        this->do_assign_opt<G_real>(xval, xval);
        return *this;
      }
    Value& operator=(const opt<cow_string>& xval) noexcept
      {
        this->do_assign_opt<const G_string&>(xval, xval);
        return *this;
      }
    Value& operator=(opt<cow_string>&& xval) noexcept
      {
        this->do_assign_opt<G_string&&>(xval, xval);
        return *this;
      }
    Value& operator=(const opt<cow_vector<Value>>& xval) noexcept
      {
        this->do_assign_opt<const G_array&>(xval, xval);
        return *this;
      }
    Value& operator=(opt<cow_vector<Value>>&& xval) noexcept
      {
        this->do_assign_opt<G_array&&>(xval, xval);
        return *this;
      }
    Value& operator=(const opt<cow_hashmap<phsh_string, Value, phsh_string::hash>>& xval) noexcept
      {
        this->do_assign_opt<const G_object&>(xval, xval);
        return *this;
      }
    Value& operator=(opt<cow_hashmap<phsh_string, Value, phsh_string::hash>>&& xval) noexcept
      {
        this->do_assign_opt<G_object&&>(xval, xval);
        return *this;
      }

  private:
    template<typename CastT,  // how to forward the value (may be an rvalue reference type)
             typename ChkT, typename PtrT> void do_assign_opt(ChkT&& chk, PtrT&& ptr)
      {
        if(chk)
          this->m_stor = static_cast<CastT>(*ptr);
        else
          this->m_stor = G_null();
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
