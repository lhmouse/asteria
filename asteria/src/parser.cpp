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

namespace Asteria {

Parser::~Parser()
  {
  }

namespace {

  inline Parser_result do_make_parser_result(const Token_stream &toks_io, Parser_result::Error error)
    {
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return Parser_result(0, 0, 0, error);
      }
      return Parser_result(qtok->get_line(), qtok->get_offset(), qtok->get_length(), error);
    }

  bool do_match_keyword(Token_stream &toks_io, Token::Keyword keyword)
    {
      const auto qtok = toks_io.peek_opt();
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
      toks_io.shift();
      return true;
    }

  bool do_match_punctuator(Token_stream &toks_io, Token::Punctuator punct)
    {
      const auto qtok = toks_io.peek_opt();
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
      toks_io.shift();
      return true;
    }

  bool do_accept_identifier(String &name_out, Token_stream &toks_io)
    {
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_identifier>();
      if(!qalt) {
        return false;
      }
      name_out = qalt->name;
      toks_io.shift();
      return true;
    }

  bool do_accept_string_literal(String &value_out, Token_stream &toks_io)
    {
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_string_literal>();
      if(!qalt) {
        return false;
      }
      value_out = qalt->value;
      toks_io.shift();
      return true;
    }

  bool do_accept_export_directive(Statement &stmt_out, Token_stream &toks_io)
    {
      // export-directive ::=
      //   "export" identifier ";"
      if(do_match_keyword(toks_io, Token::keyword_export) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_export stmt_c = { std::move(name) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_import_directive(Statement &stmt_out, Token_stream &toks_io)
    {
      // import-directive ::=
      //   "import" string-literal ";"
      if(do_match_keyword(toks_io, Token::keyword_import) == false) {
        return false;
      }
      String path;
      if(do_accept_string_literal(path, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_string_literal_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_import stmt_c = { std::move(path) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  template<typename SinkT>
    extern bool do_accept_statement(SinkT &sink_out, Token_stream &toks_io);

  bool do_accept_block(Block &block_out, Token_stream &toks_io)
    {
      // block ::=
      //   "{" statement-list-opt "}"
      // statement-list-opt ::=
      //   statement-list | ""
      // statement-list ::=
      //   statement statement-list-opt
      if(do_match_punctuator(toks_io, Token::punctuator_brace_op) == false) {
        return false;
      }
      Vector<Statement> stmts;
      for(;;) {
        Statement stmt;
        if(do_accept_statement(stmt, toks_io) == false) {
          break;
        }
        stmts.emplace_back(std::move(stmt));
      }
      if(do_match_punctuator(toks_io, Token::punctuator_brace_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_brace_or_statement_expected);
      }
      block_out = std::move(stmts);
      return true;
    }
  bool do_accept_block(Statement &stmt_out, Token_stream &toks_io)
    {
      Block block;
      if(do_accept_block(block, toks_io) == false) {
        return false;
      }
      Statement::S_block stmt_c = { std::move(block) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_null_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // null-statement ::=
      //   ";"
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        return false;
      }
      Statement::S_null stmt_c = { };
      stmt_out = std::move(stmt_c);
      return true;
    }

  extern bool do_accept_expression(Expression &expr_out, Token_stream &toks_io);

  void do_tell_source_location(String &file_out, Uint64 &line_out, const Token_stream &toks_io)
    {
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return;
      }
      file_out = qtok->get_file();
      line_out = qtok->get_line();
    }

  bool do_accept_identifier_list(Vector<String> &names_out, Token_stream &toks_io)
    {
      // identifier-list-opt ::=
      //   identifier-list | ""
      // identifier-list ::=
      //   identifier ( "," identifier-list | "" )
      const auto size_init = names_out.size();
      for(;;) {
        String name;
        if(do_accept_identifier(name, toks_io) == false) {
          if(names_out.size() == size_init) {
            return false;
          }
          throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
        }
        names_out.emplace_back(std::move(name));
        if(do_match_punctuator(toks_io, Token::punctuator_comma) == false) {
          break;
        }
      }
      return true;
    }

  bool do_accept_named_reference(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        return false;
      }
      Xpnode::S_named_reference node_c = { std::move(name) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_literal(Value &value_out, Token_stream &toks_io)
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
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return false;
      }
      switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword: {
          const auto &alt = qtok->check<Token::S_keyword>();
          switch(rocket::weaken_enum(alt.keyword)) {
            case Token::keyword_null: {
              value_out = D_null();
              break;
            }
            case Token::keyword_false: {
              value_out = D_boolean(false);
              break;
            }
            case Token::keyword_true: {
              value_out = D_boolean(true);
              break;
            }
            case Token::keyword_nan: {
              value_out = std::numeric_limits<D_real>::quiet_NaN();
              break;
            }
            case Token::keyword_infinity: {
              value_out = std::numeric_limits<D_real>::infinity();
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
          break;
        }
        case Token::index_real_literal: {
          const auto &alt = qtok->check<Token::S_real_literal>();
          value_out = D_real(alt.value);
          break;
        }
        case Token::index_string_literal: {
          const auto &alt = qtok->check<Token::S_string_literal>();
          value_out = D_string(alt.value);
          break;
        }
        default: {
          return false;
        }
      }
      toks_io.shift();
      return true;
    }
  bool do_accept_literal(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      Value value;
      if(do_accept_literal(value, toks_io) == false) {
        return false;
      }
      Xpnode::S_literal node_c = { std::move(value) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_this(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      if(do_match_keyword(toks_io, Token::keyword_this) == false) {
        return false;
      }
      Xpnode::S_named_reference node_c = { String::shallow("__this") };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_closure_function(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      // Copy these parameters before reading from the stream which is destructive.
      String file;
      Uint64 line = 0;
      do_tell_source_location(file, line, toks_io);
      // closure-function ::=
      //   "func" "(" identifier-list-opt ")" statement
      if(do_match_keyword(toks_io, Token::keyword_func) == false) {
        return false;
      }
      Vector<String> params;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_accept_identifier_list(params, toks_io) == false) {
        // This is optional.
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block body;
      if(do_accept_statement(body, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      Xpnode::S_closure_function node_c = { std::move(file), line, std::move(params), std::move(body) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_unnamed_array(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      // unnamed-array ::=
      //   "[" array-element-list-opt "]"
      // array-element-list-opt ::=
      //   array-element-list | ""
      // array-element-list ::=
      //   expression ( comma-or-semicolon array-element-list-opt | "" )
      // comma-or-semicolon ::=
      //   "," | ";"
      if(do_match_punctuator(toks_io, Token::punctuator_bracket_op) == false) {
        return false;
      }
      Vector<Expression> elems;
      for(;;) {
        Expression init;
        if(do_accept_expression(init, toks_io) == false) {
          break;
        }
        elems.emplace_back(std::move(init));
        if((do_match_punctuator(toks_io, Token::punctuator_comma) == false) && (do_match_punctuator(toks_io, Token::punctuator_semicol) == false)) {
          break;
        }
      }
      if(do_match_punctuator(toks_io, Token::punctuator_bracket_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_bracket_or_expression_expected);
      }
      Xpnode::S_unnamed_array node_c = { std::move(elems) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_unnamed_object(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      // unnamed-object ::=
      //   "{" key-mapped-list-opt "}"
      // key-mapped-list-opt ::=
      //   key-mapped-list | ""
      // key-mapped-list ::=
      //   ( string-literal | identifier ) "=" expression ( comma-or-semicolon key-mapped-list-opt | "" )
      // nested-expression ::=
      //   "(" expression ")"
      if(do_match_punctuator(toks_io, Token::punctuator_brace_op) == false) {
        return false;
      }
      Dictionary<Expression> pairs;
      for(;;) {
        const auto duplicate_key_result = do_make_parser_result(toks_io, Parser_result::error_duplicate_object_key);
        String name;
        if((do_accept_string_literal(name, toks_io) == false) && (do_accept_identifier(name, toks_io) == false)) {
          break;
        }
        if(do_match_punctuator(toks_io, Token::punctuator_assign) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_equals_sign_expected);
        }
        Expression init;
        if(do_accept_expression(init, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
        }
        if(pairs.try_emplace(std::move(name), std::move(init)).second == false) {
          throw duplicate_key_result;
        }
        if((do_match_punctuator(toks_io, Token::punctuator_comma) == false) && (do_match_punctuator(toks_io, Token::punctuator_semicol) == false)) {
          break;
        }
      }
      if(do_match_punctuator(toks_io, Token::punctuator_brace_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_brace_or_object_key_expected);
      }
      Xpnode::S_unnamed_object node_c = { std::move(pairs) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_nested_expression(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      // nested-expression ::=
      //   "(" expression ")"
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        return false;
      }
      Expression expr;
      if(do_accept_expression(expr, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Xpnode::S_subexpression node_c = { std::move(expr) };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_primary_expression(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      // primary-expression ::=
      //   identifier | literal | "this" | closure-function | unnamed-array | unnamed-object | nested-expression
      return do_accept_named_reference(nodes_out, toks_io) ||
             do_accept_literal(nodes_out, toks_io) ||
             do_accept_this(nodes_out, toks_io) ||
             do_accept_closure_function(nodes_out, toks_io) ||
             do_accept_unnamed_object(nodes_out, toks_io) ||
             do_accept_unnamed_array(nodes_out, toks_io) ||
             do_accept_nested_expression(nodes_out, toks_io);
    }

  bool do_accept_prefix_operator(Xpnode::Xop &xop_out, Token_stream &toks_io)
    {
      // prefix-operator ::=
      //   "+" | "-" | "~" | "!" | "++" | "--" | "unset"
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return false;
      }
      switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword: {
          const auto &alt = qtok->check<Token::S_keyword>();
          switch(rocket::weaken_enum(alt.keyword)) {
            case Token::keyword_unset: {
              xop_out = Xpnode::xop_prefix_unset;
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
              xop_out = Xpnode::xop_prefix_pos;
              break;
            }
            case Token::punctuator_sub: {
              xop_out = Xpnode::xop_prefix_neg;
              break;
            }
            case Token::punctuator_notb: {
              xop_out = Xpnode::xop_prefix_notb;
              break;
            }
            case Token::punctuator_notl: {
              xop_out = Xpnode::xop_prefix_notl;
              break;
            }
            case Token::punctuator_inc: {
              xop_out = Xpnode::xop_prefix_inc;
              break;
            }
            case Token::punctuator_dec: {
              xop_out = Xpnode::xop_prefix_dec;
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
      toks_io.shift();
      return true;
    }
  bool do_accept_prefix_operator(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      Xpnode::Xop xop;
      if(do_accept_prefix_operator(xop, toks_io) == false) {
        return false;
      }
      Xpnode::S_operator_rpn node_c = { xop, false };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_postfix_operator(Xpnode::Xop &xop_out, Token_stream &toks_io)
    {
      // postfix-operator ::=
      //   "++" | "--"
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_punctuator>();
      if(!qalt) {
        return false;
      }
      switch(rocket::weaken_enum(qalt->punct)) {
        case Token::punctuator_inc: {
          xop_out = Xpnode::xop_postfix_inc;
          break;
        }
        case Token::punctuator_dec: {
          xop_out = Xpnode::xop_postfix_dec;
          break;
        }
        default: {
          return false;
        }
      }
      toks_io.shift();
      return true;
    }
  bool do_accept_postfix_operator(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      Xpnode::Xop xop;
      if(do_accept_postfix_operator(xop, toks_io) == false) {
        return false;
      }
      Xpnode::S_operator_rpn node_c = { xop, false };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_postfix_function_call(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      // Copy these parameters before reading from the stream which is destructive.
      String file;
      Uint64 line = 0;
      do_tell_source_location(file, line, toks_io);
      // postfix-function-call ::=
      //   "(" argument-list-opt ")"
      // argument-list-opt ::=
      //   argument-list | ""
      // argument-list ::=
      //   expression ( "," argument-list | "" )
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        return false;
      }
      Vector<Expression> args;
      for(;;) {
        Expression arg;
        if(do_accept_expression(arg, toks_io) == false) {
          if(args.empty()) {
            break;
          }
          throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
        }
        args.emplace_back(std::move(arg));
        if(do_match_punctuator(toks_io, Token::punctuator_comma) == false) {
          break;
        }
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_or_argument_expected);
      }
      for(Size i = 0; i < args.size(); ++i) {
        Xpnode::S_subexpression node_c = { std::move(args.mut(i)) };
        nodes_out.emplace_back(std::move(node_c));
      }
      Xpnode::S_function_call node_c = { std::move(file), line, args.size() };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_postfix_subscript(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      // postfix-subscript ::=
      //   "[" expression "]"
      if(do_match_punctuator(toks_io, Token::punctuator_bracket_op) == false) {
        return false;
      }
      Expression expr;
      if(do_accept_expression(expr, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_bracket_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_bracket_expected);
      }
      Xpnode::S_subexpression node_i = { std::move(expr) };
      nodes_out.emplace_back(std::move(node_i));
      Xpnode::S_operator_rpn node_c = { Xpnode::xop_postfix_at, false };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_postfix_member_access(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
    {
      // postfix-member-access ::=
      //   "." identifier
      if(do_match_punctuator(toks_io, Token::punctuator_dot) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      Xpnode::S_literal node_i = { D_string(std::move(name)) };
      nodes_out.emplace_back(std::move(node_i));
      Xpnode::S_operator_rpn node_c = { Xpnode::xop_postfix_at, false };
      nodes_out.emplace_back(std::move(node_c));
      return true;
    }

  bool do_accept_infix_element(Vector<Xpnode> &nodes_out, Token_stream &toks_io)
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
      for(;;) {
        bool prefix_got = do_accept_prefix_operator(prefixes, toks_io);
        if(prefix_got == false) {
          break;
        }
      }
      if(do_accept_primary_expression(nodes_out, toks_io) == false) {
        if(prefixes.empty()) {
          return false;
        }
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      for(;;) {
        bool postfix_got = do_accept_postfix_operator(nodes_out, toks_io) ||
                           do_accept_postfix_function_call(nodes_out, toks_io) ||
                           do_accept_postfix_subscript(nodes_out, toks_io) ||
                           do_accept_postfix_member_access(nodes_out, toks_io);
        if(postfix_got == false) {
          break;
        }
      }
      nodes_out.append(std::make_move_iterator(prefixes.mut_rbegin()), std::make_move_iterator(prefixes.mut_rend()));
      return true;
    }

  bool do_accept_expression(Expression &expr_out, Token_stream &toks_io)
    {
      // TODO
      Vector<Xpnode> nodes;
      if(do_accept_infix_element(nodes, toks_io) == false) {
        return false;
      }
      expr_out = std::move(nodes);
      return true;
    }

  bool do_accept_variable_definition(Statement &stmt_out, Token_stream &toks_io)
    {
      // variable-definition ::=
      //   "var" identifier equal-initailizer-opt ";"
      // equal-initializer-opt ::=
      //   equal-initializer | ""
      // equal-initializer ::=
      //   "=" expression
      if(do_match_keyword(toks_io, Token::keyword_var) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      Expression init;
      if(do_match_punctuator(toks_io, Token::punctuator_assign)) {
        if(do_accept_expression(init, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
        }
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_var_def stmt_c = { std::move(name), false, std::move(init) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_immutable_variable_definition(Statement &stmt_out, Token_stream &toks_io)
    {
      // immutable-variable-definition ::=
      //   "const" identifier equal-initailizer ";"
      // equal-initializer ::=
      //   "=" expression
      if(do_match_keyword(toks_io, Token::keyword_const) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_assign) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_equals_sign_expected);
      }
      Expression init;
      if(do_accept_expression(init, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_var_def stmt_c = { std::move(name), true, std::move(init) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_function_definition(Statement &stmt_out, Token_stream &toks_io)
    {
      // Copy these parameters before reading from the stream which is destructive.
      String file;
      Uint64 line = 0;
      do_tell_source_location(file, line, toks_io);
      // function-definition ::=
      //   "func" identifier "(" identifier-list-opt ")" statement
      if(do_match_keyword(toks_io, Token::keyword_func) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      Vector<String> params;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_accept_identifier_list(params, toks_io) == false) {
        // This is optional.
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block body;
      if(do_accept_statement(body, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      Statement::S_func_def stmt_c = { std::move(file), line, std::move(name), std::move(params), std::move(body) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_expression_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // expression-statement ::=
      //   expression ";"
      Expression expr;
      if(do_accept_expression(expr, toks_io) == false) {
        return false;
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_expr stmt_c = { std::move(expr) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_if_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // if-statement ::=
      //   "if" "(" expression ")" statement ( "else" statement | "" )
      if(do_match_keyword(toks_io, Token::keyword_if) == false) {
        return false;
      }
      Expression cond;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_accept_expression(cond, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block branch_true;
      if(do_accept_statement(branch_true, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      Block branch_false;
      if(do_match_keyword(toks_io, Token::keyword_else)) {
        if(do_accept_statement(branch_false, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
        }
      }
      Statement::S_if stmt_c = { std::move(cond), std::move(branch_true), std::move(branch_false) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_switch_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // switch-statement ::=
      //   "switch" "(" expression ")" switch-block
      // switch-block ::=
      //   "{" swtich-clause-list-opt "}"
      // switch-clause-list-opt ::=
      //   switch-clause-list | ""
      // switch-clause-list ::=
      //   ( "case" expression | "default" ) ":" statement-list-opt switch-clause-list-opt
      if(do_match_keyword(toks_io, Token::keyword_switch) == false) {
        return false;
      }
      Expression ctrl;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_accept_expression(ctrl, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Bivector<Expression, Block> clauses;
      if(do_match_punctuator(toks_io, Token::punctuator_brace_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_brace_expected);
      }
      for(;;) {
        Expression cond;
        if(do_match_keyword(toks_io, Token::keyword_default) == false) {
          if(do_match_keyword(toks_io, Token::keyword_case) == false) {
            break;
          }
          if(do_accept_expression(cond, toks_io) == false) {
            throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
          }
        }
        if(do_match_punctuator(toks_io, Token::punctuator_colon) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_colon_expected);
        }
        Vector<Statement> stmts;
        for(;;) {
          Statement stmt;
          if(do_accept_statement(stmt, toks_io) == false) {
            break;
          }
          stmts.emplace_back(std::move(stmt));
        }
        clauses.emplace_back(std::move(cond), std::move(stmts));
      }
      if(do_match_punctuator(toks_io, Token::punctuator_brace_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_brace_or_switch_clause_expected);
      }
      Statement::S_switch stmt_c = { std::move(ctrl), std::move(clauses) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_do_while_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // do-while-statement ::=
      //   "do" statement "while" "(" expression ")" ";"
      if(do_match_keyword(toks_io, Token::keyword_do) == false) {
        return false;
      }
      Block body;
      if(do_accept_statement(body, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      if(do_match_keyword(toks_io, Token::keyword_while) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_keyword_while_expected);
      }
      Expression cond;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_accept_expression(cond, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_while stmt_c = { true, std::move(cond), std::move(body) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_while_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // while-statement ::=
      //   "while" "(" expression ")" statement
      if(do_match_keyword(toks_io, Token::keyword_while) == false) {
        return false;
      }
      Expression cond;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_accept_expression(cond, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block body;
      if(do_accept_statement(body, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      Statement::S_while stmt_c = { false, std::move(cond), std::move(body) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_for_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // for-statement ::=
      //   "for" "(" ( for-statement-range | for-statement-triplet ) ")" statement
      // for-statement-range ::=
      //   "each" identifier "," identifier ":" expression
      // for-statement-triplet ::=
      //   ( null-statement | variable-definition | expression-statement ) expression-opt ";" expression-opt
      if(do_match_keyword(toks_io, Token::keyword_for) == false) {
        return false;
      }
      // This for-statement is ranged if and only if `key_name` is non-empty, where `step` is used as the range initializer.
      String key_name;
      String mapped_name;
      Statement init_stmt;
      Expression cond;
      Expression step;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_match_keyword(toks_io, Token::keyword_each)) {
        if(do_accept_identifier(key_name, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
        }
        if(do_match_punctuator(toks_io, Token::punctuator_comma) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_comma_expected);
        }
        if(do_accept_identifier(mapped_name, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
        }
        if(do_match_punctuator(toks_io, Token::punctuator_colon) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_colon_expected);
        }
        if(do_accept_expression(step, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
        }
      } else {
        bool init_got = do_accept_variable_definition(init_stmt, toks_io) ||
                        do_accept_null_statement(init_stmt, toks_io) ||
                        do_accept_expression_statement(init_stmt, toks_io);
        if(init_got == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_for_statement_initializer_expected);
        }
        if(do_accept_expression(cond, toks_io) == false) {
          // This is optional.
        }
        if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
        }
        if(do_accept_expression(step, toks_io) == false) {
          // This is optional.
        }
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block body;
      if(do_accept_statement(body, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      if(key_name.empty()) {
        Vector<Statement> init;
        init.emplace_back(std::move(init_stmt));
        Statement::S_for stmt_c = { std::move(init), std::move(cond), std::move(step), std::move(body) };
        stmt_out = std::move(stmt_c);
      } else {
        Statement::S_for_each stmt_c = { std::move(key_name), std::move(mapped_name), std::move(step), std::move(body) };
        stmt_out = std::move(stmt_c);
      }
      return true;
    }

  bool do_accept_break_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // break-statement ::=
      //   "break" ( "switch" | "while" | "for" ) ";"
      if(do_match_keyword(toks_io, Token::keyword_break) == false) {
        return false;
      }
      Statement::Target target = Statement::target_unspec;
      if(do_match_keyword(toks_io, Token::keyword_switch)) {
        target = Statement::target_switch;
        goto z;
      }
      if(do_match_keyword(toks_io, Token::keyword_while)) {
        target = Statement::target_while;
        goto z;
      }
      if(do_match_keyword(toks_io, Token::keyword_for)) {
        target = Statement::target_for;
        goto z;
      }
    z:
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_break stmt_c = { target };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_continue_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // continue-statement ::=
      //   "continue" ( "while" | "for" ) ";"
      if(do_match_keyword(toks_io, Token::keyword_continue) == false) {
        return false;
      }
      Statement::Target target = Statement::target_unspec;
      if(do_match_keyword(toks_io, Token::keyword_while)) {
        target = Statement::target_while;
        goto z;
      }
      if(do_match_keyword(toks_io, Token::keyword_for)) {
        target = Statement::target_for;
        goto z;
      }
    z:
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_continue stmt_c = { target };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_throw_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // throw-statement ::=
      //   "throw" expression ";"
      if(do_match_keyword(toks_io, Token::keyword_throw) == false) {
        return false;
      }
      Expression expr;
      if(do_accept_expression(expr, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_throw stmt_c = { std::move(expr) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_return_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // return-statement ::=
      //   "return" expression-opt ";"
      if(do_match_keyword(toks_io, Token::keyword_return) == false) {
        return false;
      }
      Expression expr;
      if(do_accept_expression(expr, toks_io) == false) {
        // This is optional.
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_return stmt_c = { std::move(expr) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_try_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // try-statement ::=
      //   "try" statement "catch" "(" identifier ")" statement
      if(do_match_keyword(toks_io, Token::keyword_try) == false) {
        return false;
      }
      Block body_try;
      if(do_accept_statement(body_try, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      if(do_match_keyword(toks_io, Token::keyword_catch) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_keyword_catch_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      String except_name;
      if(do_accept_identifier(except_name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block body_catch;
      if(do_accept_statement(body_catch, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      Statement::S_try stmt_c = { std::move(body_try), std::move(except_name), std::move(body_catch) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_nonblock_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // non-block-statement ::=
      //   null-statement |
      //   variable-definition | immutable-variable-definition | function-definition |
      //   expression-statement |
      //   if-statement | switch-statement |
      //   do-while-statement | while-statement | for-statement |
      //   break-statement | continue-statement | throw-statement | return-statement |
      //   try-statement
      return do_accept_null_statement(stmt_out, toks_io) ||
             do_accept_variable_definition(stmt_out, toks_io) ||
             do_accept_immutable_variable_definition(stmt_out, toks_io) ||
             do_accept_function_definition(stmt_out, toks_io) ||
             do_accept_expression_statement(stmt_out, toks_io) ||
             do_accept_if_statement(stmt_out, toks_io) ||
             do_accept_switch_statement(stmt_out, toks_io) ||
             do_accept_do_while_statement(stmt_out, toks_io) ||
             do_accept_while_statement(stmt_out, toks_io) ||
             do_accept_for_statement(stmt_out, toks_io) ||
             do_accept_break_statement(stmt_out, toks_io) ||
             do_accept_continue_statement(stmt_out, toks_io) ||
             do_accept_throw_statement(stmt_out, toks_io) ||
             do_accept_return_statement(stmt_out, toks_io) ||
             do_accept_try_statement(stmt_out, toks_io);
    }
  bool do_accept_nonblock_statement(Block &block_out, Token_stream &toks_io)
    {
      Statement stmt;
      if(do_accept_nonblock_statement(stmt, toks_io) == false) {
        return false;
      }
      Vector<Statement> stmts;
      stmts.emplace_back(std::move(stmt));
      block_out = std::move(stmts);
      return true;
    }

  template<typename SinkT>
    bool do_accept_statement(SinkT &sink_out, Token_stream &toks_io)
      {
        ASTERIA_DEBUG_LOG("Looking for a statement: ", toks_io.empty() ? String::shallow("<no token>") : ASTERIA_FORMAT_STRING(*(toks_io.peek_opt())));
        // statement ::=
        //   block | non-block-statement
        return do_accept_block(sink_out, toks_io) ||
               do_accept_nonblock_statement(sink_out, toks_io);
      }

  bool do_accept_directive_or_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // directive-or-statement ::=
      //   directive | statement
      // directive ::=
      //   export-directive | import-directive
      return do_accept_export_directive(stmt_out, toks_io) ||
             do_accept_import_directive(stmt_out, toks_io) ||
             do_accept_statement(stmt_out, toks_io);
    }

}

Parser_result Parser::load(Token_stream &toks_io)
  try {
    // This has to be done before anything else because of possibility of exceptions.
    this->m_stor.set(nullptr);
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Parse the document recursively.
    ///////////////////////////////////////////////////////////////////////////
    Vector<Statement> stmts;
    while(toks_io.empty() == false) {
      // document ::=
      //   directive-or-statement-list-opt
      // directive-or-statement-list-opt ::=
      //   directive-or-statement-list | ""
      // directive-or-statement-list ::=
      //   directive-or-statement directive-or-statement-list-opt
      Statement stmt;
      if(do_accept_directive_or_statement(stmt, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      stmts.emplace_back(std::move(stmt));
    }
    // Accept the result.
    this->m_stor.emplace<Block>(std::move(stmts));
    return Parser_result(0, 0, 0, Parser_result::error_success);
  } catch(Parser_result &e) {  // Don't play this at home.
    ASTERIA_DEBUG_LOG("Parser error: ", e.get_error(), " (", Parser_result::describe_error(e.get_error()), ")");
    this->m_stor.set(e);
    return e;
  }
void Parser::clear() noexcept
  {
    this->m_stor.set(nullptr);
  }

Parser_result Parser::get_result() const
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        return this->m_stor.as<Parser_result>();
      }
      case state_success: {
        return Parser_result(0, 0, 0, Parser_result::error_success);
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }
const Block & Parser::get_document() const
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
      case state_success: {
        return this->m_stor.as<Block>();
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
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
        auto block = std::move(this->m_stor.as<Block>());
        this->m_stor.set(nullptr);
        return block;
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }

}
