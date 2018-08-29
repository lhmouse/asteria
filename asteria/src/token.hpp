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
    enum Keyword : std::uint8_t
      {
        keyword_var       =  0,
        keyword_const     =  1,
        keyword_function  =  2,
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
    enum Punctuator : std::uint8_t
      {
        punctuator_add         =  0, // +
        punctuator_add_eq      =  1, // +=
        punctuator_sub         =  2, // -
        punctuator_sub_eq      =  3, // -=
        punctuator_mul         =  4, // *
        punctuator_mul_eq      =  5, // *=
        punctuator_div         =  6, // /
        punctuator_div_eq      =  7, // /=
        punctuator_mod         =  8, // %
        punctuator_mod_eq      =  9, // %=
        punctuator_inc         = 10, // ++
        punctuator_dec         = 11, // --
        punctuator_sll         = 12, // <<<
        punctuator_sll_eq      = 13, // <<<=
        punctuator_srl         = 14, // >>>
        punctuator_srl_eq      = 15, // >>>=
        punctuator_sla         = 16, // <<
        punctuator_sla_eq      = 17, // <<=
        punctuator_sra         = 18, // >>
        punctuator_sra_eq      = 19, // >>=
        punctuator_andb        = 20, // &
        punctuator_andb_eq     = 21, // &=
        punctuator_andl        = 22, // &&
        punctuator_andl_eq     = 23, // &&=
        punctuator_orb         = 24, // |
        punctuator_orb_eq      = 25, // |=
        punctuator_orl         = 26, // ||
        punctuator_orl_eq      = 27, // ||=
        punctuator_xorb        = 28, // ^
        punctuator_xorb_eq     = 29, // ^=
        punctuator_notb        = 30, // ~
        punctuator_notl        = 31, // !
        punctuator_cmp_eq      = 32, // ==
        punctuator_cmp_ne      = 33, // !=
        punctuator_cmp_lt      = 34, // <
        punctuator_cmp_gt      = 35, // >
        punctuator_cmp_lte     = 36, // <=
        punctuator_cmp_gte     = 37, // >=
        punctuator_dot         = 38, // .
        punctuator_condition   = 39, // ?
        punctuator_assign      = 40, // =
        punctuator_parenth_op  = 41, // (
        punctuator_parenth_cl  = 42, // )
        punctuator_bracket_op  = 43, // [
        punctuator_bracket_cl  = 44, // ]
        punctuator_brace_op    = 45, // {
        punctuator_brace_cl    = 46, // }
        punctuator_comma       = 47, // ,
        punctuator_colon       = 48, // :
        punctuator_semicolon   = 49, // ;
      };

    enum Index : std::uint8_t
      {
        index_keyword          = 0,
        index_punctuator       = 1,
        index_identifier       = 2,
        index_integer_literal  = 3,
        index_double_literal   = 4,
        index_string_literal   = 5,
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
        String id;
      };
    struct S_integer_literal
      {
        Unsigned value;
      };
    struct S_double_literal
      {
        Double value;
      };
    struct S_string_literal
      {
        String value;
      };
    using Variant = rocket::variant<
      ROCKET_CDR(
        , S_keyword          // 0
        , S_punctuator       // 1
        , S_identifier       // 2
        , S_integer_literal  // 3
        , S_double_literal   // 4
        , S_string_literal   // 5
      )>;

  private:
    Unsigned m_line;
    Unsigned m_column;
    std::size_t m_length;
    Variant m_stor;

  public:
    template<typename AltT, typename std::enable_if<std::is_constructible<Variant, AltT &&>::value>::type * = nullptr>
      Token(Unsigned line, Unsigned column, std::size_t length, AltT &&alt)
        : m_line(line), m_column(column), m_length(length), m_stor(std::forward<AltT>(alt))
        {
        }
    ~Token();

    Token(Token &&) noexcept;
    Token & operator=(Token &&) noexcept;

  public:
    Unsigned get_line() const noexcept
      {
        return m_line;
      }
    Unsigned get_column() const noexcept
      {
        return m_column;
      }
    std::size_t get_length() const noexcept
      {
        return m_length;
      }
    Index index() const noexcept
      {
        return static_cast<Index>(m_stor.index());
      }
    template<typename AltT>
      const AltT * opt() const noexcept
        {
          return m_stor.get<AltT>();
        }
    template<typename AltT>
      const AltT & check() const
        {
          return m_stor.as<AltT>();
        }
  };

extern Parser_result tokenize_line_no_comment_incremental(Vector<Token> &tokens_out, Unsigned line, const String &str);

}

#endif
