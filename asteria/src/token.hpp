// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TOKEN_HPP_
#define ASTERIA_TOKEN_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"
#include "parser_result.hpp"

namespace Asteria {

class Token
  {
  public:
    enum Keyword : Uint8
      {
        keyword_var       =  0,
        keyword_const     =  1,
        keyword_func      =  2,
        keyword_if        =  3,
        keyword_else      =  4,
        keyword_switch    =  5,
        keyword_case      =  6,
        keyword_default   =  7,
        keyword_do        =  8,
        keyword_while     =  9,
        keyword_for       = 10,
        keyword_each      = 11,
        keyword_try       = 12,
        keyword_catch     = 13,
        keyword_defer     = 14,
        keyword_break     = 15,
        keyword_continue  = 16,
        keyword_throw     = 17,
        keyword_return    = 18,
        keyword_null      = 19,
        keyword_true      = 20,
        keyword_false     = 21,
        keyword_nan       = 22,
        keyword_infinity  = 23,
        keyword_this      = 24,
        keyword_unset     = 25,
        keyword_export    = 26,
        keyword_import    = 27,
      };
    enum Punctuator : Uint8
      {
        punctuator_add         =  0,  // +
        punctuator_add_eq      =  1,  // +=
        punctuator_sub         =  2,  // -
        punctuator_sub_eq      =  3,  // -=
        punctuator_mul         =  4,  // *
        punctuator_mul_eq      =  5,  // *=
        punctuator_div         =  6,  // /
        punctuator_div_eq      =  7,  // /=
        punctuator_mod         =  8,  // %
        punctuator_mod_eq      =  9,  // %=
        punctuator_inc         = 10,  // ++
        punctuator_dec         = 11,  // --
        punctuator_sll         = 12,  // <<<
        punctuator_sll_eq      = 13,  // <<<=
        punctuator_srl         = 14,  // >>>
        punctuator_srl_eq      = 15,  // >>>=
        punctuator_sla         = 16,  // <<
        punctuator_sla_eq      = 17,  // <<=
        punctuator_sra         = 18,  // >>
        punctuator_sra_eq      = 19,  // >>=
        punctuator_andb        = 20,  // &
        punctuator_andb_eq     = 21,  // &=
        punctuator_andl        = 22,  // &&
        punctuator_andl_eq     = 23,  // &&=
        punctuator_orb         = 24,  // |
        punctuator_orb_eq      = 25,  // |=
        punctuator_orl         = 26,  // ||
        punctuator_orl_eq      = 27,  // ||=
        punctuator_xorb        = 28,  // ^
        punctuator_xorb_eq     = 29,  // ^=
        punctuator_notb        = 30,  // ~
        punctuator_notl        = 31,  // !
        punctuator_cmp_eq      = 32,  // ==
        punctuator_cmp_ne      = 33,  // !=
        punctuator_cmp_lt      = 34,  // <
        punctuator_cmp_gt      = 35,  // >
        punctuator_cmp_lte     = 36,  // <=
        punctuator_cmp_gte     = 37,  // >=
        punctuator_dot         = 38,  // .
        punctuator_quest       = 39,  // ?
        punctuator_quest_eq    = 40,  // ?=
        punctuator_assign      = 41,  // =
        punctuator_parenth_op  = 42,  // (
        punctuator_parenth_cl  = 43,  // )
        punctuator_bracket_op  = 44,  // [
        punctuator_bracket_cl  = 45,  // ]
        punctuator_brace_op    = 46,  // {
        punctuator_brace_cl    = 47,  // }
        punctuator_comma       = 48,  // ,
        punctuator_colon       = 49,  // :
        punctuator_semicol     = 50,  // ;
      };

    struct S_keyword
      {
        Keyword keyword;
      };
    struct S_punctuator
      {
        Punctuator punct;
      };
    struct S_identifier
      {
        String name;
      };
    struct S_integer_literal
      {
        Uint64 value;
      };
    struct S_real_literal
      {
        Xfloat value;
      };
    struct S_string_literal
      {
        String value;
      };

    enum Index : Uint8
      {
        index_keyword          = 0,
        index_punctuator       = 1,
        index_identifier       = 2,
        index_integer_literal  = 3,
        index_real_literal     = 4,
        index_string_literal   = 5,
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_keyword          // 0,
        , S_punctuator       // 1,
        , S_identifier       // 2,
        , S_integer_literal  // 3,
        , S_real_literal     // 4,
        , S_string_literal   // 5,
      )>;

  public:
    static const char * get_keyword(Keyword keyword) noexcept;
    static const char * get_punctuator(Punctuator punct) noexcept;

  private:
    String m_file;
    Uint64 m_line;
    Size m_offset;
    Size m_length;
    Variant m_stor;

  public:
    // This constructor does not accept lvalues.
    template<typename AltT, typename std::enable_if<(Variant::index_of<AltT>::value || true)>::type * = nullptr>
      Token(const String &file, Uint64 line, Size offset, Size length, AltT &&alt)
        : m_file(file), m_line(line), m_offset(offset), m_length(length), m_stor(std::forward<AltT>(alt))
        {
        }
    ~Token();

  public:
    const String & get_file() const noexcept
      {
        return this->m_file;
      }
    Uint64 get_line() const noexcept
      {
        return this->m_line;
      }
    Size get_offset() const noexcept
      {
        return this->m_offset;
      }
    Size get_length() const noexcept
      {
        return this->m_length;
      }

    Index index() const noexcept
      {
        return Index(this->m_stor.index());
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

    void dump(std::ostream &os) const;
  };

inline std::ostream & operator<<(std::ostream &os, const Token &token)
  {
    token.dump(os);
    return os;
  }

}

#endif
