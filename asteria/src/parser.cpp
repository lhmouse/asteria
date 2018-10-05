// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "parser.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "statement.hpp"
#include "block.hpp"
#include "xpnode.hpp"
#include "expression.hpp"
#include "utilities.hpp"
#include <algorithm>

namespace Asteria {

Parser::~Parser()
  {
  }

namespace {

  inline Parser_error do_make_parser_error(const Token_stream &tstrm_io, Parser_error::Code code)
    {
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return Parser_error(0, 0, 0, code);
      }
      return Parser_error(qtok->get_line(), qtok->get_offset(), qtok->get_length(), code);
    }

  inline void do_tell_source_location(String &file_out, Uint32 &line_out, const Token_stream &tstrm_io)
    {
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return;
      }
      file_out = qtok->get_file();
      line_out = qtok->get_line();
    }

  bool do_match_keyword(Token_stream &tstrm_io, Token::Keyword keyword)
    {
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_keyword>();
      if(!qalt) {
        return false;
      }
      if(qalt->keyword != keyword) {
        return false;
      }
      tstrm_io.shift();
      return true;
    }

  bool do_match_punctuator(Token_stream &tstrm_io, Token::Punctuator punct)
    {
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_punctuator>();
      if(!qalt) {
        return false;
      }
      if(qalt->punct != punct) {
        return false;
      }
      tstrm_io.shift();
      return true;
    }

  bool do_accept_identifier(String &name_out, Token_stream &tstrm_io)
    {
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_identifier>();
      if(!qalt) {
        return false;
      }
      name_out = qalt->name;
      tstrm_io.shift();
      return true;
    }

  bool do_accept_string_literal(String &value_out, Token_stream &tstrm_io)
    {
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_string_literal>();
      if(!qalt) {
        return false;
      }
      value_out = qalt->value;
      tstrm_io.shift();
      return true;
    }

  bool do_accept_keyword_as_identifier(String &name_out, Token_stream &tstrm_io)
    {
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_keyword>();
      if(!qalt) {
        return false;
      }
      name_out = String::shallow(Token::get_keyword(qalt->keyword));
      tstrm_io.shift();
      return true;
    }

  bool do_accept_prefix_operator(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // prefix-operator ::=
      //   "+" | "-" | "~" | "!" | "++" | "--" | "unset"
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return false;
      }
      Xpnode::Xop xop;
      switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword: {
          const auto &alt = qtok->check<Token::S_keyword>();
          switch(rocket::weaken_enum(alt.keyword)) {
            case Token::keyword_unset: {
              xop = Xpnode::xop_prefix_unset;
              tstrm_io.shift();
              break;
            }
            default: {
              return false;
            }
          }
          break;
        }
        case Token::index_punctuator: {
          const auto &alt = qtok->check<Token::S_punctuator>();
          switch(rocket::weaken_enum(alt.punct)) {
            case Token::punctuator_add: {
              xop = Xpnode::xop_prefix_pos;
              tstrm_io.shift();
              break;
            }
            case Token::punctuator_sub: {
              xop = Xpnode::xop_prefix_neg;
              tstrm_io.shift();
              break;
            }
            case Token::punctuator_notb: {
              xop = Xpnode::xop_prefix_notb;
              tstrm_io.shift();
              break;
            }
            case Token::punctuator_notl: {
              xop = Xpnode::xop_prefix_notl;
              tstrm_io.shift();
              break;
            }
            case Token::punctuator_inc: {
              xop = Xpnode::xop_prefix_inc;
              tstrm_io.shift();
              break;
            }
            case Token::punctuator_dec: {
              xop = Xpnode::xop_prefix_dec;
              tstrm_io.shift();
              break;
            }
            default: {
              return false;
            }
          }
          break;
        }
        default: {
          return false;
        }
      }
      Xpnode::S_operator_rpn node_c = { xop, false };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_postfix_operator(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // postfix-operator ::=
      //   "++" | "--"
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_punctuator>();
      if(!qalt) {
        return false;
      }
      Xpnode::Xop xop;
      switch(rocket::weaken_enum(qalt->punct)) {
        case Token::punctuator_inc: {
          xop = Xpnode::xop_postfix_inc;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_dec: {
          xop = Xpnode::xop_postfix_dec;
          tstrm_io.shift();
          break;
        }
        default: {
          return false;
        }
      }
      Xpnode::S_operator_rpn node_c = { xop, false };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_literal(Value &value_out, Token_stream &tstrm_io)
    {
      // literal ::=
      //   null-literal | boolean-literal | string-literal | noescape-string-literal |
      //   numeric-literal | nan-literal | infinity-literal
      // null-literal ::=
      //   "null"
      // boolean-litearl ::=
      //   "false" | "true"
      // nan-literal ::=
      //   "nan"
      // infinity-literal ::=
      //   "infinity"
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return false;
      }
      switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword: {
          const auto &alt = qtok->check<Token::S_keyword>();
          switch(rocket::weaken_enum(alt.keyword)) {
            case Token::keyword_null: {
              value_out = nullptr;
              tstrm_io.shift();
              break;
            }
            case Token::keyword_false: {
              value_out = D_boolean(false);
              tstrm_io.shift();
              break;
            }
            case Token::keyword_true: {
              value_out = D_boolean(true);
              tstrm_io.shift();
              break;
            }
            case Token::keyword_nan: {
              value_out = std::numeric_limits<D_real>::quiet_NaN();
              tstrm_io.shift();
              break;
            }
            case Token::keyword_infinity: {
              value_out = std::numeric_limits<D_real>::infinity();
              tstrm_io.shift();
              break;
            }
            default: {
              return false;
            }
          }
          break;
        }
        case Token::index_integer_literal: {
          const auto &alt = qtok->check<Token::S_integer_literal>();
          value_out = D_integer(alt.value);
          tstrm_io.shift();
          break;
        }
        case Token::index_real_literal: {
          const auto &alt = qtok->check<Token::S_real_literal>();
          value_out = D_real(alt.value);
          tstrm_io.shift();
          break;
        }
        case Token::index_string_literal: {
          const auto &alt = qtok->check<Token::S_string_literal>();
          value_out = D_string(alt.value);
          tstrm_io.shift();
          break;
        }
        default: {
          return false;
        }
      }
      return true;
    }

  extern bool do_accept_statement_as_block(Vector<Statement> &stmts_out, Token_stream &tstrm_io);
  extern bool do_accept_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io);
  extern bool do_accept_expression(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io);

  bool do_accept_block_statement_list(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // block ::=
      //   "{" statement-list-opt "}"
      // statement-list-opt ::=
      //   statement-list | ""
      // statement-list ::=
      //   statement statement-list-opt
      if(do_match_punctuator(tstrm_io, Token::punctuator_brace_op) == false) {
        return false;
      }
      for(;;) {
        bool stmt_got = do_accept_statement(stmts_out, tstrm_io);
        if(stmt_got == false) {
          break;
        }
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_brace_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_brace_or_statement_expected);
      }
      return true;
    }

  bool do_accept_block_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      Vector<Statement> stmts;
      if(do_accept_block_statement_list(stmts, tstrm_io) == false) {
        return false;
      }
      Statement::S_block stmt_c = { std::move(stmts) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_identifier_list(Vector<String> &names_out, Token_stream &tstrm_io)
    {
      // identifier-list-opt ::=
      //   identifier-list | ""
      // identifier-list ::=
      //   identifier ( "," identifier-list | "" )
      String name;
      if(do_accept_identifier(name, tstrm_io) == false) {
        return false;
      }
      for(;;) {
        names_out.emplace_back(std::move(name));
        if(do_match_punctuator(tstrm_io, Token::punctuator_comma) == false) {
          break;
        }
        if(do_accept_identifier(name, tstrm_io) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_identifier_expected);
        }
      }
      return true;
    }

  bool do_accept_named_reference(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      String name;
      if(do_accept_identifier(name, tstrm_io) == false) {
        return false;
      }
      Xpnode::S_named_reference node_c = { std::move(name) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_literal(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      Value value;
      if(do_accept_literal(value, tstrm_io) == false) {
        return false;
      }
      Xpnode::S_literal node_c = { std::move(value) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_this(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      if(do_match_keyword(tstrm_io, Token::keyword_this) == false) {
        return false;
      }
      Xpnode::S_named_reference node_c = { String::shallow("__this") };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_closure_function(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // Copy these parameters before reading from the stream which is destructive.
      String file;
      Uint32 line = 0;
      do_tell_source_location(file, line, tstrm_io);
      // closure-function ::=
      //   "func" "(" identifier-list-opt ")" statement
      if(do_match_keyword(tstrm_io, Token::keyword_func) == false) {
        return false;
      }
      Vector<String> params;
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_open_parenthesis_expected);
      }
      if(do_accept_identifier_list(params, tstrm_io) == false) {
        // This is optional.
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_parenthesis_expected);
      }
      Vector<Statement> body;
      if(do_accept_statement_as_block(body, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_statement_expected);
      }
      Xpnode::S_closure_function node_c = { std::move(file), line, std::move(params), std::move(body) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_unnamed_array(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // unnamed-array ::=
      //   "[" array-element-list-opt "]"
      // array-element-list-opt ::=
      //   array-element-list | ""
      // array-element-list ::=
      //   expression ( comma-or-semicolon array-element-list-opt | "" )
      // comma-or-semicolon ::=
      //   "," | ";"
      if(do_match_punctuator(tstrm_io, Token::punctuator_bracket_op) == false) {
        return false;
      }
      Size elem_cnt = 0;
      for(;;) {
        if(do_accept_expression(nodes_out, tstrm_io) == false) {
          break;
        }
        ++elem_cnt;
        bool has_next = do_match_punctuator(tstrm_io, Token::punctuator_comma) ||
                        do_match_punctuator(tstrm_io, Token::punctuator_semicol);
        if(has_next == false) {
          break;
        }
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_bracket_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_bracket_or_expression_expected);
      }
      Xpnode::S_unnamed_array node_c = { elem_cnt };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_unnamed_object(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // unnamed-object ::=
      //   "{" key-mapped-list-opt "}"
      // key-mapped-list-opt ::=
      //   key-mapped-list | ""
      // key-mapped-list ::=
      //   ( string-literal | identifier ) "=" expression ( comma-or-semicolon key-mapped-list-opt | "" )
      if(do_match_punctuator(tstrm_io, Token::punctuator_brace_op) == false) {
        return false;
      }
      Vector<String> keys;
      for(;;) {
        const auto duplicate_key_error = do_make_parser_error(tstrm_io, Parser_error::code_duplicate_object_key);
        String key;
        bool key_got = do_accept_string_literal(key, tstrm_io) ||
                       do_accept_identifier(key, tstrm_io) ||
                       do_accept_keyword_as_identifier(key, tstrm_io);
        if(key_got == false) {
          break;
        }
        if(do_match_punctuator(tstrm_io, Token::punctuator_assign) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_equals_sign_expected);
        }
        if(std::find(keys.begin(), keys.end(), key) != keys.end()) {
          throw duplicate_key_error;
        }
        if(do_accept_expression(nodes_out, tstrm_io) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
        }
        keys.emplace_back(std::move(key));
        bool has_next = do_match_punctuator(tstrm_io, Token::punctuator_comma) ||
                        do_match_punctuator(tstrm_io, Token::punctuator_semicol);
        if(has_next == false) {
          break;
        }
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_brace_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_brace_or_object_key_expected);
      }
      Xpnode::S_unnamed_object node_c = { std::move(keys) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_nested_expression(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // nested-expression ::=
      //   "(" expression ")"
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_op) == false) {
        return false;
      }
      if(do_accept_expression(nodes_out, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_parenthesis_expected);
      }
      return true;
    }

  bool do_accept_primary_expression(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // primary-expression ::=
      //   identifier | literal | "this" | closure-function | unnamed-array | unnamed-object | nested-expression
      return do_accept_named_reference(nodes_out, tstrm_io) ||
             do_accept_literal(nodes_out, tstrm_io) ||
             do_accept_this(nodes_out, tstrm_io) ||
             do_accept_closure_function(nodes_out, tstrm_io) ||
             do_accept_unnamed_object(nodes_out, tstrm_io) ||
             do_accept_unnamed_array(nodes_out, tstrm_io) ||
             do_accept_nested_expression(nodes_out, tstrm_io);
    }

  bool do_accept_postfix_function_call(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // Copy these parameters before reading from the stream which is destructive.
      String file;
      Uint32 line = 0;
      do_tell_source_location(file, line, tstrm_io);
      // postfix-function-call ::=
      //   "(" argument-list-opt ")"
      // argument-list-opt ::=
      //   argument-list | ""
      // argument-list ::=
      //   expression ( "," argument-list | "" )
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_op) == false) {
        return false;
      }
      Size arg_cnt = 0;
      if(do_accept_expression(nodes_out, tstrm_io) != false) {
        for(;;) {
          ++arg_cnt;
          if(do_match_punctuator(tstrm_io, Token::punctuator_comma) == false) {
            break;
          }
          if(do_accept_expression(nodes_out, tstrm_io) == false) {
            throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
          }
        }
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_parenthesis_or_argument_expected);
      }
      Xpnode::S_function_call node_c = { std::move(file), line, arg_cnt };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_postfix_subscript(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // postfix-subscript ::=
      //   "[" expression "]"
      if(do_match_punctuator(tstrm_io, Token::punctuator_bracket_op) == false) {
        return false;
      }
      if(do_accept_expression(nodes_out, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_bracket_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_bracket_expected);
      }
      Xpnode::S_subscript node_c = { String() };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_postfix_member_access(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // postfix-member-access ::=
      //   "." identifier
      if(do_match_punctuator(tstrm_io, Token::punctuator_dot) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_identifier_expected);
      }
      Xpnode::S_subscript node_c = { std::move(name) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_infix_element(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // infix-element ::=
      //   ( prefix-operator-list primary-expression | primary_expression ) postfix-operator-list-opt
      // prefix-operator-list-opt ::=
      //   prefix-operator-list | ""
      // prefix-operator-list ::=
      //   prefix-operator prefix-operator-list-opt
      // postfix-operator-list-opt ::=
      //   postfix-operator-list | ""
      // postfix-operator-list ::=
      //   postfix-operator | postfix-function-call | postfix-subscript | postfix-member-access
      Vector<Xpnode> prefixes;
      if(do_accept_prefix_operator(prefixes, tstrm_io) == false) {
        if(do_accept_primary_expression(nodes_out, tstrm_io) == false) {
          return false;
        }
      } else {
        for(;;) {
          bool prefix_got = do_accept_prefix_operator(prefixes, tstrm_io);
          if(prefix_got == false) {
            break;
          }
        }
        if(do_accept_primary_expression(nodes_out, tstrm_io) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
        }
      }
      for(;;) {
        bool postfix_got = do_accept_postfix_operator(nodes_out, tstrm_io) ||
                           do_accept_postfix_function_call(nodes_out, tstrm_io) ||
                           do_accept_postfix_subscript(nodes_out, tstrm_io) ||
                           do_accept_postfix_member_access(nodes_out, tstrm_io);
        if(postfix_got == false) {
          break;
        }
      }
      while(prefixes.empty() == false) {
        nodes_out.emplace_back(std::move(prefixes.mut_back()));
        prefixes.pop_back();
      }
      return true;
    }

  class Infix_element_base
    {
    public:
      enum Precedence : Uint8
        {
          precedence_multiplicative  =  1,
          precedence_additive        =  2,
          precedence_shift           =  3,
          precedence_relational      =  4,
          precedence_equality        =  5,
          precedence_bitwise_and     =  6,
          precedence_bitwise_xor     =  7,
          precedence_bitwise_or      =  8,
          precedence_logical_and     =  9,
          precedence_logical_or      = 10,
          precedence_coalescence     = 11,
          precedence_assignment      = 12,
        };

    public:
      virtual ~Infix_element_base()
        {
        }

    public:
      virtual Precedence precedence() const noexcept = 0;
      virtual void extract(Vector<Xpnode> &nodes_out) = 0;
      virtual void append(Infix_element_base &&elem) = 0;
    };

  class Infix_head : public Infix_element_base
    {
    private:
      Vector<Xpnode> m_nodes;

    public:
      explicit Infix_head(Vector<Xpnode> &&nodes)
        : m_nodes(std::move(nodes))
        {
        }

    public:
      Precedence precedence() const noexcept override
        {
          return std::numeric_limits<Precedence>::max();
        }
      void extract(Vector<Xpnode> &nodes_out) override
        {
          nodes_out.append(std::make_move_iterator(this->m_nodes.mut_begin()), std::make_move_iterator(this->m_nodes.mut_end()));
        }
      void append(Infix_element_base &&elem) override
        {
          elem.extract(this->m_nodes);
        }
    };

  bool do_accept_infix_head(std::unique_ptr<Infix_element_base> &elem_out, Token_stream &tstrm_io)
    {
      Vector<Xpnode> nodes;
      if(do_accept_infix_element(nodes, tstrm_io) == false) {
        return false;
      }
      elem_out.reset(new Infix_head(std::move(nodes)));
      return true;
    }

  class Infix_selection : public Infix_element_base
    {
    public:
      enum Sop
        {
          sop_quest   = 0,  // ? :
          sop_and     = 1,  // &&
          sop_or      = 2,  // ||
          sop_coales  = 3,  // ??
        };

    private:
      Sop m_sop;
      bool m_assign;
      Vector<Xpnode> m_branch_true;
      Vector<Xpnode> m_branch_false;

    public:
      Infix_selection(Sop sop, bool assign, Vector<Xpnode> &&branch_true, Vector<Xpnode> &&branch_false)
        : m_sop(sop), m_assign(assign), m_branch_true(std::move(branch_true)), m_branch_false(std::move(branch_false))
        {
        }

    public:
      Precedence precedence() const noexcept override
        {
          if(this->m_assign) {
            return precedence_assignment;
          }
          switch(this->m_sop) {
            case sop_quest: {
              return precedence_assignment;
            }
            case sop_and: {
              return precedence_logical_and;
            }
            case sop_or: {
              return precedence_logical_or;
            }
            case sop_coales: {
              return precedence_coalescence;
            }
            default: {
              ASTERIA_TERMINATE("Invalid infix selection `", this->m_sop, "` has been encountered.");
            }
          }
        }
      void extract(Vector<Xpnode> &nodes_out) override
        {
          if(this->m_sop == sop_coales) {
            Xpnode::S_coalescence node_c = { this->m_assign, std::move(this->m_branch_false) };
            nodes_out.emplace_back(std::move(node_c));
            return;
          }
          Xpnode::S_branch node_c = { this->m_assign, std::move(this->m_branch_true), std::move(this->m_branch_false) };
          nodes_out.emplace_back(std::move(node_c));
        }
      void append(Infix_element_base &&elem) override
        {
          elem.extract(this->m_branch_false);
        }
    };

  bool do_accept_infix_selection_quest(std::unique_ptr<Infix_element_base> &elem_out, Token_stream &tstrm_io)
    {
      bool assign = false;
      if(do_match_punctuator(tstrm_io, Token::punctuator_quest) == false) {
        if(do_match_punctuator(tstrm_io, Token::punctuator_quest_eq) == false) {
          return false;
        }
        assign = true;
      }
      Vector<Xpnode> branch_true;
      if(do_accept_expression(branch_true, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_colon) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_colon_expected);
      }
      Vector<Xpnode> branch_false;
      if(do_accept_infix_element(branch_false, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      elem_out.reset(new Infix_selection(Infix_selection::sop_quest, assign, std::move(branch_true), std::move(branch_false)));
      return true;
    }

  bool do_accept_infix_selection_and(std::unique_ptr<Infix_element_base> &elem_out, Token_stream &tstrm_io)
    {
      bool assign = false;
      if(do_match_punctuator(tstrm_io, Token::punctuator_andl) == false) {
        if(do_match_punctuator(tstrm_io, Token::punctuator_andl_eq) == false) {
          return false;
        }
        assign = true;
      }
      Vector<Xpnode> branch_true;
      if(do_accept_infix_element(branch_true, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      elem_out.reset(new Infix_selection(Infix_selection::sop_and, assign, std::move(branch_true), Vector<Xpnode>()));
      return true;
    }

  bool do_accept_infix_selection_or(std::unique_ptr<Infix_element_base> &elem_out, Token_stream &tstrm_io)
    {
      bool assign = false;
      if(do_match_punctuator(tstrm_io, Token::punctuator_orl) == false) {
        if(do_match_punctuator(tstrm_io, Token::punctuator_orl_eq) == false) {
          return false;
        }
        assign = true;
      }
      Vector<Xpnode> branch_false;
      if(do_accept_infix_element(branch_false, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      elem_out.reset(new Infix_selection(Infix_selection::sop_or, assign, Vector<Xpnode>(), std::move(branch_false)));
      return true;
    }

  bool do_accept_infix_selection_coales(std::unique_ptr<Infix_element_base> &elem_out, Token_stream &tstrm_io)
    {
      bool assign = false;
      if(do_match_punctuator(tstrm_io, Token::punctuator_coales) == false) {
        if(do_match_punctuator(tstrm_io, Token::punctuator_coales_eq) == false) {
          return false;
        }
        assign = true;
      }
      Vector<Xpnode> branch_null;
      if(do_accept_infix_element(branch_null, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      elem_out.reset(new Infix_selection(Infix_selection::sop_or, assign, Vector<Xpnode>(), std::move(branch_null)));
      return true;
    }

  class Infix_operator : public Infix_element_base
    {
    private:
      Xpnode::Xop m_xop;
      bool m_assign;
      Vector<Xpnode> m_rhs;

    public:
      Infix_operator(Xpnode::Xop xop, bool assign, Vector<Xpnode> &&rhs)
        : m_xop(xop), m_assign(assign), m_rhs(std::move(rhs))
        {
        }

    public:
      Precedence precedence() const noexcept override
        {
          if(this->m_assign) {
            return precedence_assignment;
          }
          switch(rocket::weaken_enum(this->m_xop)) {
            case Xpnode::xop_infix_mul:
            case Xpnode::xop_infix_div:
            case Xpnode::xop_infix_mod: {
              return precedence_multiplicative;
            }
            case Xpnode::xop_infix_add:
            case Xpnode::xop_infix_sub: {
              return precedence_additive;
            }
            case Xpnode::xop_infix_sla:
            case Xpnode::xop_infix_sra:
            case Xpnode::xop_infix_sll:
            case Xpnode::xop_infix_srl: {
              return precedence_shift;
            }
            case Xpnode::xop_infix_cmp_lt:
            case Xpnode::xop_infix_cmp_lte:
            case Xpnode::xop_infix_cmp_gt:
            case Xpnode::xop_infix_cmp_gte: {
              return precedence_relational;
            }
            case Xpnode::xop_infix_cmp_eq:
            case Xpnode::xop_infix_cmp_ne:
            case Xpnode::xop_infix_cmp_3way: {
              return precedence_equality;
            }
            case Xpnode::xop_infix_andb: {
              return precedence_bitwise_and;
            }
            case Xpnode::xop_infix_xorb: {
              return precedence_bitwise_xor;
            }
            case Xpnode::xop_infix_orb: {
              return precedence_bitwise_or;
            }
            case Xpnode::xop_infix_assign: {
              return precedence_assignment;
            }
            default: {
              ASTERIA_TERMINATE("Invalid infix operator `", this->m_xop, "` has been encountered.");
            }
          }
        }
      void extract(Vector<Xpnode> &nodes_out) override
        {
          nodes_out.append(std::make_move_iterator(this->m_rhs.mut_begin()), std::make_move_iterator(this->m_rhs.mut_end()));
          // Don't forget the operator!
          Xpnode::S_operator_rpn node_c = { this->m_xop, this->m_assign };
          nodes_out.emplace_back(std::move(node_c));
        }
      void append(Infix_element_base &&elem) override
        {
          elem.extract(this->m_rhs);
        }
    };

  bool do_accept_infix_operator(std::unique_ptr<Infix_element_base> &elem_out, Token_stream &tstrm_io)
    {
      // infix-operator ::=
      //   ( "+"  | "-"  | "*"  | "/"  | "%"  | "<<"  | ">>"  | "<<<"  | ">>>"  | "&"  | "|"  | "^"  |
      //     "+=" | "-=" | "*=" | "/=" | "%=" | "<<=" | ">>=" | "<<<=" | ">>>=" | "&=" | "|=" | "^=" |
      //     "="  | "==" | "!=" | "<"  | ">"  | "<="  | ">="  | "<=>"  ) infix-element
      const auto qtok = tstrm_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_punctuator>();
      if(!qalt) {
        return false;
      }
      bool assign = false;
      Xpnode::Xop xop;
      switch(rocket::weaken_enum(qalt->punct)) {
        case Token::punctuator_add_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_add:
          xop = Xpnode::xop_infix_add;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_sub_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_sub:
          xop = Xpnode::xop_infix_sub;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_mul_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_mul:
          xop = Xpnode::xop_infix_mul;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_div_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_div:
          xop = Xpnode::xop_infix_div;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_mod_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_mod:
          xop = Xpnode::xop_infix_mod;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_sla_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_sla:
          xop = Xpnode::xop_infix_sla;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_sra_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_sra:
          xop = Xpnode::xop_infix_sra;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_sll_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_sll:
          xop = Xpnode::xop_infix_sll;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_srl_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_srl:
          xop = Xpnode::xop_infix_srl;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_andb_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_andb:
          xop = Xpnode::xop_infix_andb;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_orb_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_orb:
          xop = Xpnode::xop_infix_orb;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_xorb_eq: {
          assign = true;
          // Fallthrough.
        case Token::punctuator_xorb:
          xop = Xpnode::xop_infix_xorb;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_assign: {
          xop = Xpnode::xop_infix_assign;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_cmp_eq: {
          xop = Xpnode::xop_infix_cmp_eq;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_cmp_ne: {
          xop = Xpnode::xop_infix_cmp_ne;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_cmp_lt: {
          xop = Xpnode::xop_infix_cmp_lt;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_cmp_gt: {
          xop = Xpnode::xop_infix_cmp_gt;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_cmp_lte: {
          xop = Xpnode::xop_infix_cmp_lte;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_cmp_gte: {
          xop = Xpnode::xop_infix_cmp_gte;
          tstrm_io.shift();
          break;
        }
        case Token::punctuator_spaceship: {
          xop = Xpnode::xop_infix_cmp_3way;
          tstrm_io.shift();
          break;
        }
        default: {
          return false;
        }
      }
      Vector<Xpnode> rhs;
      if(do_accept_infix_element(rhs, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      elem_out.reset(new Infix_operator(xop, assign, std::move(rhs)));
      return true;
    }

  bool do_accept_expression(Vector<Xpnode> &nodes_out, Token_stream &tstrm_io)
    {
      // expression ::=
      //   infix-element infix-operator-list-opt
      // infix-operator-list-opt ::=
      //   infix-operator-list | ""
      // infix-operator-list ::=
      //   ( infix-selection | infix-operator ) infix-operator-list-opt
      // infix-selection ::=
      //   ( "?"  expression ":" | "&&"  | "||"  | "??"  |
      //     "?=" expression ":" | "&&=" | "||=" | "??=" ) infix-element
      std::unique_ptr<Infix_element_base> elem;
      if(do_accept_infix_head(elem, tstrm_io) == false) {
        return false;
      }
      Vector<std::unique_ptr<Infix_element_base>> stack;
      stack.emplace_back(std::move(elem));
      for(;;) {
        bool elem_got = do_accept_infix_selection_quest(elem, tstrm_io) ||
                        do_accept_infix_selection_and(elem, tstrm_io) ||
                        do_accept_infix_selection_or(elem, tstrm_io) ||
                        do_accept_infix_selection_coales(elem, tstrm_io) ||
                        do_accept_infix_operator(elem, tstrm_io);
        if(elem_got == false) {
          break;
        }
        const auto prec = elem->precedence();
        // Assignment operations have the lowest precedence and group from right to left.
        if(prec != elem->precedence_assignment) {
          while((stack.size() >= 2) && (stack.back()->precedence() <= prec)) {
            stack.at(stack.size() - 2)->append(std::move(*(stack.back())));
            stack.pop_back();
          }
        }
        stack.emplace_back(std::move(elem));
      }
      while(stack.size() >= 2) {
        stack.at(stack.size() - 2)->append(std::move(*(stack.back())));
        stack.pop_back();
      }
      stack.front()->extract(nodes_out);
      return true;
    }

  bool do_accept_variable_definition(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // variable-definition ::=
      //   "var" identifier equal-initailizer-opt ";"
      // equal-initializer-opt ::=
      //   equal-initializer | ""
      // equal-initializer ::=
      //   "=" expression
      if(do_match_keyword(tstrm_io, Token::keyword_var) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_identifier_expected);
      }
      Vector<Xpnode> init;
      if(do_match_punctuator(tstrm_io, Token::punctuator_assign) != false) {
        // The initializer is optional.
        if(do_accept_expression(init, tstrm_io) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
        }
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_semicolon_expected);
      }
      Statement::S_var_def stmt_c = { std::move(name), false, std::move(init) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_immutable_variable_definition(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // immutable-variable-definition ::=
      //   "const" identifier equal-initailizer ";"
      // equal-initializer ::=
      //   "=" expression
      if(do_match_keyword(tstrm_io, Token::keyword_const) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_identifier_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_assign) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_equals_sign_expected);
      }
      Vector<Xpnode> init;
      if(do_accept_expression(init, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_semicolon_expected);
      }
      Statement::S_var_def stmt_c = { std::move(name), true, std::move(init) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_function_definition(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // Copy these parameters before reading from the stream which is destructive.
      String file;
      Uint32 line = 0;
      do_tell_source_location(file, line, tstrm_io);
      // function-definition ::=
      //   "func" identifier "(" identifier-list-opt ")" statement
      if(do_match_keyword(tstrm_io, Token::keyword_func) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_identifier_expected);
      }
      Vector<String> params;
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_open_parenthesis_expected);
      }
      if(do_accept_identifier_list(params, tstrm_io) == false) {
        // This is optional.
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_parenthesis_expected);
      }
      Vector<Statement> body;
      if(do_accept_statement_as_block(body, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_statement_expected);
      }
      Statement::S_func_def stmt_c = { std::move(file), line, std::move(name), std::move(params), std::move(body) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_expression_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // expression-statement ::=
      //   expression ";"
      Vector<Xpnode> expr;
      if(do_accept_expression(expr, tstrm_io) == false) {
        return false;
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_semicolon_expected);
      }
      Statement::S_expr stmt_c = { std::move(expr) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_if_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // if-statement ::=
      //   "if" "(" expression ")" statement ( "else" statement | "" )
      if(do_match_keyword(tstrm_io, Token::keyword_if) == false) {
        return false;
      }
      Vector<Xpnode> cond;
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_open_parenthesis_expected);
      }
      if(do_accept_expression(cond, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_parenthesis_expected);
      }
      Vector<Statement> branch_true;
      if(do_accept_statement_as_block(branch_true, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_statement_expected);
      }
      Vector<Statement> branch_false;
      if(do_match_keyword(tstrm_io, Token::keyword_else) != false) {
        // The `else` branch is optional.
        if(do_accept_statement_as_block(branch_false, tstrm_io) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_statement_expected);
        }
      }
      Statement::S_if stmt_c = { std::move(cond), std::move(branch_true), std::move(branch_false) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_switch_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // switch-statement ::=
      //   "switch" "(" expression ")" switch-block
      // switch-block ::=
      //   "{" swtich-clause-list-opt "}"
      // switch-clause-list-opt ::=
      //   switch-clause-list | ""
      // switch-clause-list ::=
      //   ( "case" expression | "default" ) ":" statement-list-opt switch-clause-list-opt
      if(do_match_keyword(tstrm_io, Token::keyword_switch) == false) {
        return false;
      }
      Vector<Xpnode> ctrl;
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_open_parenthesis_expected);
      }
      if(do_accept_expression(ctrl, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_parenthesis_expected);
      }
      Bivector<Expression, Block> clauses;
      if(do_match_punctuator(tstrm_io, Token::punctuator_brace_op) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_open_brace_expected);
      }
      for(;;) {
        Vector<Xpnode> cond;
        if(do_match_keyword(tstrm_io, Token::keyword_default) == false) {
          if(do_match_keyword(tstrm_io, Token::keyword_case) == false) {
            break;
          }
          if(do_accept_expression(cond, tstrm_io) == false) {
            throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
          }
        }
        if(do_match_punctuator(tstrm_io, Token::punctuator_colon) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_colon_expected);
        }
        Vector<Statement> stmts;
        for(;;) {
          bool stmt_got = do_accept_statement(stmts, tstrm_io);
          if(stmt_got == false) {
            break;
          }
        }
        clauses.emplace_back(std::move(cond), std::move(stmts));
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_brace_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_brace_or_switch_clause_expected);
      }
      Statement::S_switch stmt_c = { std::move(ctrl), std::move(clauses) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_do_while_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // do-while-statement ::=
      //   "do" statement "while" "(" expression ")" ";"
      if(do_match_keyword(tstrm_io, Token::keyword_do) == false) {
        return false;
      }
      Vector<Statement> body;
      if(do_accept_statement_as_block(body, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_statement_expected);
      }
      if(do_match_keyword(tstrm_io, Token::keyword_while) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_keyword_while_expected);
      }
      Vector<Xpnode> cond;
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_open_parenthesis_expected);
      }
      if(do_accept_expression(cond, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_parenthesis_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_semicolon_expected);
      }
      Statement::S_do_while stmt_c = { std::move(body), std::move(cond) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_while_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // while-statement ::=
      //   "while" "(" expression ")" statement
      if(do_match_keyword(tstrm_io, Token::keyword_while) == false) {
        return false;
      }
      Vector<Xpnode> cond;
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_open_parenthesis_expected);
      }
      if(do_accept_expression(cond, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_parenthesis_expected);
      }
      Vector<Statement> body;
      if(do_accept_statement_as_block(body, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_statement_expected);
      }
      Statement::S_while stmt_c = { std::move(cond), std::move(body) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_for_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // for-statement ::=
      //   "for" "(" ( for-statement-range | for-statement-triplet ) ")" statement
      // for-statement-range ::=
      //   "each" identifier ( "," identifier | "") ":" expression
      // for-statement-triplet ::=
      //   ( null-statement | variable-definition | expression-statement ) expression-opt ";" expression-opt
      if(do_match_keyword(tstrm_io, Token::keyword_for) == false) {
        return false;
      }
      // This for-statement is ranged if and only if `key_name` is non-empty, where `step` is used as the range initializer.
      String key_name;
      String mapped_name;
      Vector<Statement> init;
      Vector<Xpnode> cond;
      Vector<Xpnode> step;
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_open_parenthesis_expected);
      }
      if(do_match_keyword(tstrm_io, Token::keyword_each) != false) {
        if(do_accept_identifier(key_name, tstrm_io) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_identifier_expected);
        }
        if(do_match_punctuator(tstrm_io, Token::punctuator_comma) != false) {
          // The mapped reference is optional.
          if(do_accept_identifier(mapped_name, tstrm_io) == false) {
            throw do_make_parser_error(tstrm_io, Parser_error::code_identifier_expected);
          }
        }
        if(do_match_punctuator(tstrm_io, Token::punctuator_colon) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_colon_expected);
        }
        if(do_accept_expression(step, tstrm_io) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
        }
      } else {
        bool init_got = do_accept_variable_definition(init, tstrm_io) ||
                        do_match_punctuator(tstrm_io, Token::punctuator_semicol) ||
                        do_accept_expression_statement(init, tstrm_io);
        if(init_got == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_for_statement_initializer_expected);
        }
        if(do_accept_expression(cond, tstrm_io) == false) {
          // This is optional.
        }
        if(do_match_punctuator(tstrm_io, Token::punctuator_semicol) == false) {
          throw do_make_parser_error(tstrm_io, Parser_error::code_semicolon_expected);
        }
        if(do_accept_expression(step, tstrm_io) == false) {
          // This is optional.
        }
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_parenthesis_expected);
      }
      Vector<Statement> body;
      if(do_accept_statement_as_block(body, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_statement_expected);
      }
      if(key_name.empty()) {
        Statement::S_for stmt_c = { std::move(init), std::move(cond), std::move(step), std::move(body) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }
      Statement::S_for_each stmt_c = { std::move(key_name), std::move(mapped_name), std::move(step), std::move(body) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_break_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // break-statement ::=
      //   "break" ( "switch" | "while" | "for" ) ";"
      if(do_match_keyword(tstrm_io, Token::keyword_break) == false) {
        return false;
      }
      Statement::Target target = Statement::target_unspec;
      if(do_match_keyword(tstrm_io, Token::keyword_switch) != false) {
        target = Statement::target_switch;
        goto z;
      }
      if(do_match_keyword(tstrm_io, Token::keyword_while) != false) {
        target = Statement::target_while;
        goto z;
      }
      if(do_match_keyword(tstrm_io, Token::keyword_for) != false) {
        target = Statement::target_for;
        goto z;
      }
    z:
      if(do_match_punctuator(tstrm_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_semicolon_expected);
      }
      Statement::S_break stmt_c = { target };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_continue_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // continue-statement ::=
      //   "continue" ( "while" | "for" ) ";"
      if(do_match_keyword(tstrm_io, Token::keyword_continue) == false) {
        return false;
      }
      Statement::Target target = Statement::target_unspec;
      if(do_match_keyword(tstrm_io, Token::keyword_while) != false) {
        target = Statement::target_while;
        goto z;
      }
      if(do_match_keyword(tstrm_io, Token::keyword_for) != false) {
        target = Statement::target_for;
        goto z;
      }
    z:
      if(do_match_punctuator(tstrm_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_semicolon_expected);
      }
      Statement::S_continue stmt_c = { target };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_throw_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // throw-statement ::=
      //   "throw" expression ";"
      if(do_match_keyword(tstrm_io, Token::keyword_throw) == false) {
        return false;
      }
      Vector<Xpnode> expr;
      if(do_accept_expression(expr, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_expression_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_semicolon_expected);
      }
      Statement::S_throw stmt_c = { std::move(expr) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_return_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // return-statement ::=
      //   "return" expression-opt ";"
      if(do_match_keyword(tstrm_io, Token::keyword_return) == false) {
        return false;
      }
      Vector<Xpnode> expr;
      if(do_accept_expression(expr, tstrm_io) == false) {
        // This is optional.
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_semicolon_expected);
      }
      Statement::S_return stmt_c = { std::move(expr) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_try_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // try-statement ::=
      //   "try" statement "catch" "(" identifier ")" statement
      if(do_match_keyword(tstrm_io, Token::keyword_try) == false) {
        return false;
      }
      Vector<Statement> body_try;
      if(do_accept_statement_as_block(body_try, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_statement_expected);
      }
      if(do_match_keyword(tstrm_io, Token::keyword_catch) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_keyword_catch_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_open_parenthesis_expected);
      }
      String except_name;
      if(do_accept_identifier(except_name, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_identifier_expected);
      }
      if(do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_close_parenthesis_expected);
      }
      Vector<Statement> body_catch;
      if(do_accept_statement_as_block(body_catch, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_statement_expected);
      }
      Statement::S_try stmt_c = { std::move(body_try), std::move(except_name), std::move(body_catch) };
      stmts_out.emplace_back(std::move(stmt_c));
      return true;
    }

  bool do_accept_nonblock_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      ASTERIA_DEBUG_LOG("Looking for a nonblock statement: ", tstrm_io.empty() ? String::shallow("<no token>") : ASTERIA_FORMAT_STRING(*(tstrm_io.peek_opt())));
      // nonblock-statement ::=
      //   null-statement |
      //   variable-definition | immutable-variable-definition | function-definition |
      //   expression-statement |
      //   if-statement | switch-statement |
      //   do-while-statement | while-statement | for-statement |
      //   break-statement | continue-statement | throw-statement | return-statement |
      //   try-statement
      return do_match_punctuator(tstrm_io, Token::punctuator_semicol) ||
             do_accept_variable_definition(stmts_out, tstrm_io) ||
             do_accept_immutable_variable_definition(stmts_out, tstrm_io) ||
             do_accept_function_definition(stmts_out, tstrm_io) ||
             do_accept_expression_statement(stmts_out, tstrm_io) ||
             do_accept_if_statement(stmts_out, tstrm_io) ||
             do_accept_switch_statement(stmts_out, tstrm_io) ||
             do_accept_do_while_statement(stmts_out, tstrm_io) ||
             do_accept_while_statement(stmts_out, tstrm_io) ||
             do_accept_for_statement(stmts_out, tstrm_io) ||
             do_accept_break_statement(stmts_out, tstrm_io) ||
             do_accept_continue_statement(stmts_out, tstrm_io) ||
             do_accept_throw_statement(stmts_out, tstrm_io) ||
             do_accept_return_statement(stmts_out, tstrm_io) ||
             do_accept_try_statement(stmts_out, tstrm_io);
    }

  bool do_accept_statement_as_block(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // statement ::=
      //   block-statement | nonblock-statement
      return do_accept_block_statement_list(stmts_out, tstrm_io) ||
             do_accept_nonblock_statement(stmts_out, tstrm_io);
    }

  bool do_accept_statement(Vector<Statement> &stmts_out, Token_stream &tstrm_io)
    {
      // statement ::=
      //   block-statement | nonblock-statement
      return do_accept_block_statement(stmts_out, tstrm_io) ||
             do_accept_nonblock_statement(stmts_out, tstrm_io);
    }

}

Parser_error Parser::get_parser_error() const noexcept
  {
    switch(this->state()) {
      case state_empty: {
        return Parser_error(0, 0, 0, Parser_error::code_no_data_loaded);
      }
      case state_error: {
        return this->m_stor.as<Parser_error>();
      }
      case state_success: {
        return Parser_error(0, 0, 0, Parser_error::code_success);
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }

bool Parser::empty() const noexcept
  {
    switch(this->state()) {
      case state_empty: {
        return true;
      }
      case state_error: {
        return true;
      }
      case state_success: {
        return this->m_stor.as<Vector<Statement>>().empty();
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }

bool Parser::load(Token_stream &tstrm_io)
  try {
    // This has to be done before anything else because of possibility of exceptions.
    this->m_stor.set(nullptr);
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Parse the document recursively.
    ///////////////////////////////////////////////////////////////////////////
    Vector<Statement> stmts;
    while(tstrm_io.empty() == false) {
      // document ::=
      //   statement-list-opt
      if(do_accept_statement(stmts, tstrm_io) == false) {
        throw do_make_parser_error(tstrm_io, Parser_error::code_statement_expected);
      }
    }
    // Accept the result.
    this->m_stor.set(std::move(stmts));
    return true;
  } catch(Parser_error &err) {  // Don't play this at home.
    ASTERIA_DEBUG_LOG("Parser error: line = ", err.line(), ", offset = ", err.offset(), ", length = ", err.length(),
                      ", code = ", err.code(), " (", Parser_error::get_code_description(err.code()), ")");
    this->m_stor.set(std::move(err));
    return false;
  }

void Parser::clear() noexcept
  {
    this->m_stor.set(nullptr);
  }

Block Parser::extract_document()
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
      case state_success: {
        auto stmts = std::move(this->m_stor.as<Vector<Statement>>());
        this->m_stor.set(nullptr);
        return std::move(stmts);
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }

}
