// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_XPRUNIT_HPP_
#define ASTERIA_COMPILER_XPRUNIT_HPP_

#include "../fwd.hpp"
#include "../value.hpp"
#include "../source_location.hpp"

namespace Asteria {

class Xprunit
  {
  public:
    struct S_literal
      {
        Value val;
      };
    struct S_named_reference
      {
        phsh_string name;
      };
    struct S_closure_function
      {
        Source_Location sloc;
        cow_vector<phsh_string> params;
        cow_vector<Statement> body;
      };
    struct S_branch
      {
        cow_vector<Xprunit> branch_true;
        cow_vector<Xprunit> branch_false;
        bool assign;
      };
    struct S_function_call
      {
        Source_Location sloc;
        cow_vector<bool> args_by_refs;
      };
    struct S_member_access
      {
        phsh_string name;
      };
    struct S_operator_rpn
      {
        Xop xop;
        bool assign;  // This parameter is ignored for `++`, `--`, `[]` and `=`.
      };
    struct S_unnamed_array
      {
        uint32_t nelems;
      };
    struct S_unnamed_object
      {
        cow_vector<phsh_string> keys;
      };
    struct S_coalescence
      {
        cow_vector<Xprunit> branch_null;
        bool assign;
      };
    struct S_operator_fma
      {
        bool assign;
      };

    enum Index : uint8_t
      {
        index_literal           =  0,
        index_named_reference   =  1,
        index_closure_function  =  2,
        index_branch            =  3,
        index_function_call     =  4,
        index_member_access     =  5,
        index_operator_rpn      =  6,
        index_unnamed_array     =  7,
        index_unnamed_object    =  8,
        index_coalescence       =  9,
        index_operator_fma      = 10,
      };
    using Xvariant = variant<
      ROCKET_CDR(
        , S_literal           //  0,
        , S_named_reference   //  1,
        , S_closure_function  //  2,
        , S_branch            //  3,
        , S_function_call     //  4,
        , S_member_access     //  5,
        , S_operator_rpn      //  6,
        , S_unnamed_array     //  7,
        , S_unnamed_object    //  8,
        , S_coalescence       //  9,
        , S_operator_fma      // 10,
      )>;
    static_assert(std::is_nothrow_copy_assignable<Xvariant>::value, "???");

  private:
    Xvariant m_stor;

  public:
    template<typename XunitT, ASTERIA_SFINAE_CONSTRUCT(Xvariant, XunitT&&)> Xprunit(XunitT&& unit) noexcept
      : m_stor(rocket::forward<XunitT>(unit))
      {
      }
    template<typename XunitT, ASTERIA_SFINAE_ASSIGN(Xvariant, XunitT&&)> Xprunit& operator=(XunitT&& unit) noexcept
      {
        this->m_stor = rocket::forward<XunitT>(unit);
        return *this;
      }

  public:
    Index index() const noexcept
      {
        return static_cast<Index>(this->m_stor.index());
      }

    void swap(Xprunit& other) noexcept
      {
        this->m_stor.swap(other.m_stor);
      }

    cow_vector<AIR_Node>& generate_code(cow_vector<AIR_Node>& code,
                                        const Compiler_Options& opts, TCO_Aware tco_aware, const Analytic_Context& ctx) const;
  };

inline void swap(Xprunit& lhs, Xprunit& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
