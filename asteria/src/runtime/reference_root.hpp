// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_ROOT_HPP_
#define ASTERIA_RUNTIME_REFERENCE_ROOT_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "variable.hpp"

namespace Asteria {

class Reference_Root
  {
  public:
    struct S_null
      {
      };
    struct S_constant
      {
        Value source;
      };
    struct S_temporary
      {
        Value value;
      };
    struct S_variable
      {
        Rcptr<Variable> var_opt;
      };
    struct S_tail_call
      {
        Source_Location sloc;
        Cow_String func;
        Rcptr<Abstract_Function> target;
        Cow_Vector<Reference> args;  // The last element is the `this` reference.
      };

    enum Index : std::uint8_t
      {
        index_null       = 0,
        index_constant   = 1,
        index_temporary  = 2,
        index_variable   = 3,
        index_tail_call  = 4,
      };
    using Xvariant = Variant<
      ROCKET_CDR(
        , S_null       // 0,
        , S_constant   // 1,
        , S_temporary  // 2,
        , S_variable   // 3,
        , S_tail_call  // 4,
      )>;
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "???");

  private:
    Xvariant m_stor;

  public:
    Reference_Root() noexcept
      : m_stor()  // Initialize to a null reference.
      {
      }
    template<typename XrootT, ASTERIA_SFINAE_CONSTRUCT(Xvariant, XrootT&&)> Reference_Root(XrootT&& root) noexcept
      : m_stor(rocket::forward<XrootT>(root))
      {
      }
    template<typename XrootT, ASTERIA_SFINAE_ASSIGN(Xvariant, XrootT&&)> Reference_Root& operator=(XrootT&& root) noexcept
      {
        this->m_stor = rocket::forward<XrootT>(root);
        return *this;
      }

  public:
    Index index() const noexcept
      {
        return static_cast<Index>(this->m_stor.index());
      }
    bool is_constant() const noexcept
      {
        return rocket::is_any_of(this->index(), { index_null, index_constant });
      }
    bool is_temporary() const noexcept
      {
        return rocket::is_any_of(this->index(), { index_null, index_temporary });
      }
    bool is_variable() const noexcept
      {
        return this->index() == index_variable;
      }
    bool is_tail_call() const noexcept
      {
        return this->index() == index_tail_call;
      }

    void swap(Reference_Root& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
      }

    const Value& dereference_const() const;
    Value& dereference_mutable() const;
    Rcptr<Abstract_Function> unpack_tail_call_opt(Cow_Bivector<Source_Location, Cow_String>& backtrace, Reference& self, Cow_Vector<Reference>& args);

    void enumerate_variables(const Abstract_Variable_Callback& callback) const;
  };

inline void swap(Reference_Root& lhs, Reference_Root& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
