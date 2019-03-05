// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "parser.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "../syntax/statement.hpp"
#include "../syntax/block.hpp"
#include "../syntax/xpnode.hpp"
#include "../syntax/expression.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    Parser_Error do_make_parser_error(const Token_Stream &tstrm_io, Parser_Error::Code code)
      {
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return { 0, SIZE_MAX, 0, code };
        }
        return { qtok->line(), qtok->offset(), qtok->length(), code };
      }

    Source_Location do_tell_source_location(const Token_Stream &tstrm_io)
      {
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return { rocket::sref("<end of stream>"), 0 };
        }
        return { qtok->file(), qtok->line() };
      }

    bool do_match_keyword(Token_Stream &tstrm_io, Token::Keyword keyword)
      {
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_keyword>();
        if(!qalt) {
          return false;
        }
        if(qalt->keyword != keyword) {
          return false;
        }
        tstrm_io.shift();
        return true;
      }

    bool do_match_punctuator(Token_Stream &tstrm_io, Token::Punctuator punct)
      {
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_punctuator>();
        if(!qalt) {
          return false;
        }
        if(qalt->punct != punct) {
          return false;
        }
        tstrm_io.shift();
        return true;
      }

    bool do_accept_identifier(Cow_String &name_out, Token_Stream &tstrm_io)
      {
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_identifier>();
        if(!qalt) {
          return false;
        }
        name_out = qalt->name;
        tstrm_io.shift();
        return true;
      }

    bool do_accept_string_literal(Cow_String &value_out, Token_Stream &tstrm_io)
      {
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_string_literal>();
        if(!qalt) {
          return false;
        }
        value_out = qalt->value;
        tstrm_io.shift();
        return true;
      }

    bool do_accept_keyword_as_identifier(Cow_String &name_out, Token_Stream &tstrm_io)
      {
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_keyword>();
        if(!qalt) {
          return false;
        }
        name_out = rocket::sref(Token::get_keyword(qalt->keyword));
        tstrm_io.shift();
        return true;
      }

    bool do_accept_prefix_operator(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        // prefix-operator ::=
        //   "+" | "-" | "~" | "!" | "++" | "--" | "unset" | "lengthof" | "typeof"
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return false;
        }
        Xpnode::Xop xop;
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword:
          {
            const auto &alt = qtok->check<Token::S_keyword>();
            switch(rocket::weaken_enum(alt.keyword)) {
            case Token::keyword_unset:
              {
                xop = Xpnode::xop_prefix_unset;
                tstrm_io.shift();
                break;
              }
            case Token::keyword_lengthof:
              {
                xop = Xpnode::xop_prefix_lengthof;
                tstrm_io.shift();
                break;
              }
            case Token::keyword_typeof:
              {
                xop = Xpnode::xop_prefix_typeof;
                tstrm_io.shift();
                break;
              }
            case Token::keyword_not:
              {
                xop = Xpnode::xop_prefix_notl;
                tstrm_io.shift();
                break;
              }
            default:
              return false;
            }
            break;
          }
        case Token::index_punctuator:
          {
            const auto &alt = qtok->check<Token::S_punctuator>();
            switch(rocket::weaken_enum(alt.punct)) {
            case Token::punctuator_add:
              {
                xop = Xpnode::xop_prefix_pos;
                tstrm_io.shift();
                break;
              }
            case Token::punctuator_sub:
              {
                xop = Xpnode::xop_prefix_neg;
                tstrm_io.shift();
                break;
              }
            case Token::punctuator_notb:
              {
                xop = Xpnode::xop_prefix_notb;
                tstrm_io.shift();
                break;
              }
            case Token::punctuator_notl:
              {
                xop = Xpnode::xop_prefix_notl;
                tstrm_io.shift();
                break;
              }
            case Token::punctuator_inc:
              {
                xop = Xpnode::xop_prefix_inc;
                tstrm_io.shift();
                break;
              }
            case Token::punctuator_dec:
              {
                xop = Xpnode::xop_prefix_dec;
                tstrm_io.shift();
                break;
              }
            default:
              return false;
            }
            break;
          }
        default:
          return false;
        }
        Xpnode::S_operator_rpn node_c = { xop, false };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_postfix_operator(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        // postfix-operator ::=
        //   "++" | "--"
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_punctuator>();
        if(!qalt) {
          return false;
        }
        Xpnode::Xop xop;
        switch(rocket::weaken_enum(qalt->punct)) {
        case Token::punctuator_inc:
          {
            xop = Xpnode::xop_postfix_inc;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_dec:
          {
            xop = Xpnode::xop_postfix_dec;
            tstrm_io.shift();
            break;
          }
        default:
          return false;
        }
        Xpnode::S_operator_rpn node_c = { xop, false };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_literal(Value &value_out, Token_Stream &tstrm_io)
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
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return false;
        }
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword:
          {
            const auto &alt = qtok->check<Token::S_keyword>();
            switch(rocket::weaken_enum(alt.keyword)) {
            case Token::keyword_null:
              {
                value_out = D_null();
                tstrm_io.shift();
                break;
              }
            case Token::keyword_false:
              {
                value_out = D_boolean(false);
                tstrm_io.shift();
                break;
              }
            case Token::keyword_true:
              {
                value_out = D_boolean(true);
                tstrm_io.shift();
                break;
              }
            case Token::keyword_nan:
              {
                value_out = D_real(NAN);
                tstrm_io.shift();
                break;
              }
            case Token::keyword_infinity:
              {
                value_out = D_real(INFINITY);
                tstrm_io.shift();
                break;
              }
            default:
              return false;
            }
            break;
          }
        case Token::index_integer_literal:
          {
            const auto &alt = qtok->check<Token::S_integer_literal>();
            value_out = D_integer(alt.value);
            tstrm_io.shift();
            break;
          }
        case Token::index_real_literal:
          {
            const auto &alt = qtok->check<Token::S_real_literal>();
            value_out = D_real(alt.value);
            tstrm_io.shift();
            break;
          }
        case Token::index_string_literal:
          {
            const auto &alt = qtok->check<Token::S_string_literal>();
            value_out = D_string(alt.value);
            tstrm_io.shift();
            break;
          }
        default:
          return false;
        }
        return true;
      }

    bool do_accept_negation(bool &negative_out, Token_Stream &tstrm_io)
      {
        // negation ::=
        //   "!" | "not"
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return false;
        }
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword:
          {
            const auto &alt = qtok->check<Token::S_keyword>();
            switch(rocket::weaken_enum(alt.keyword)) {
            case Token::keyword_not:
              {
                negative_out = true;
                tstrm_io.shift();
                break;
              }
            default:
              return false;
            }
            break;
          }
        case Token::index_punctuator:
          {
            const auto &alt = qtok->check<Token::S_punctuator>();
            switch(rocket::weaken_enum(alt.punct)) {
            case Token::punctuator_notl:
              {
                negative_out = true;
                tstrm_io.shift();
                break;
              }
            default:
              return false;
            }
            break;
          }
        default:
          return false;
        }
        return true;
      }

    extern bool do_accept_statement_as_block(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io);
    extern bool do_accept_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io);
    extern bool do_accept_expression(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io);

    bool do_accept_block_statement_list(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // block ::=
        //   "{" statement-list-opt "}"
        // statement-list-opt ::=
        //   statement-list | ""
        // statement-list ::=
        //   statement statement-list-opt
        if(!do_match_punctuator(tstrm_io, Token::punctuator_brace_op)) {
          return false;
        }
        for(;;) {
          bool stmt_got = do_accept_statement(stmts_out, tstrm_io);
          if(!stmt_got) {
            break;
          }
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_brace_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_brace_or_statement_expected);
        }
        return true;
      }

    bool do_accept_block_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        Cow_Vector<Statement> stmts;
        if(!do_accept_block_statement_list(stmts, tstrm_io)) {
          return false;
        }
        Statement::S_block stmt_c = { std::move(stmts) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_identifier_list(Cow_Vector<PreHashed_String> &names_out, Token_Stream &tstrm_io)
      {
        // identifier-list-opt ::=
        //   identifier-list | ""
        // identifier-list ::=
        //   identifier ( "," identifier-list | "" )
        Cow_String name;
        if(!do_accept_identifier(name, tstrm_io)) {
          return false;
        }
        for(;;) {
          names_out.emplace_back(std::move(name));
          if(!do_match_punctuator(tstrm_io, Token::punctuator_comma)) {
            break;
          }
          if(!do_accept_identifier(name, tstrm_io)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_identifier_expected);
          }
        }
        return true;
      }

    bool do_accept_named_reference(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        Cow_String name;
        if(!do_accept_identifier(name, tstrm_io)) {
          return false;
        }
        Xpnode::S_named_reference node_c = { std::move(name) };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_literal(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        Value value;
        if(!do_accept_literal(value, tstrm_io)) {
          return false;
        }
        Xpnode::S_literal node_c = { std::move(value) };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_this(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        if(!do_match_keyword(tstrm_io, Token::keyword_this)) {
          return false;
        }
        Xpnode::S_named_reference node_c = { rocket::sref("__this") };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_closure_function(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm_io);
        // closure-function ::=
        //   "func" "(" identifier-list-opt ")" ( block | expression )
        if(!do_match_keyword(tstrm_io, Token::keyword_func)) {
          return false;
        }
        Cow_Vector<PreHashed_String> params;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_open_parenthesis_expected);
        }
        if(do_accept_identifier_list(params, tstrm_io)) {
          // This is optional.
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> body;
        if(!do_accept_block_statement_list(body, tstrm_io)) {
          // An expression is expected, which behaves as if it was the operand of a `return&` ststement.
          Cow_Vector<Xpnode> nodes_ret;
          if(!do_accept_expression(nodes_ret, tstrm_io)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_open_brace_or_expression_expected);
          }
          Statement::S_return stmt_c = { true, std::move(nodes_ret) };
          body.emplace_back(std::move(stmt_c));
        }
        Xpnode::S_closure_function node_c = { std::move(sloc), std::move(params), std::move(body) };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_unnamed_array(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        // unnamed-array ::=
        //   "[" array-element-list-opt "]"
        // array-element-list-opt ::=
        //   array-element-list | ""
        // array-element-list ::=
        //   expression ( ( "," | ";" ) array-element-list-opt | "" )
        if(!do_match_punctuator(tstrm_io, Token::punctuator_bracket_op)) {
          return false;
        }
        std::size_t nelems = 0;
        for(;;) {
          if(!do_accept_expression(nodes_out, tstrm_io)) {
            break;
          }
          ++nelems;
          bool has_next = do_match_punctuator(tstrm_io, Token::punctuator_comma) ||
                          do_match_punctuator(tstrm_io, Token::punctuator_semicol);
          if(!has_next) {
            break;
          }
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_bracket_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_bracket_or_expression_expected);
        }
        Xpnode::S_unnamed_array node_c = { nelems };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_unnamed_object(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        // unnamed-object ::=
        //   "{" key-mapped-list-opt "}"
        // key-mapped-list-opt ::=
        //   key-mapped-list | ""
        // key-mapped-list ::=
        //   ( string-literal | identifier ) ( "=" | ":" ) expression ( ( "," | ";" ) key-mapped-list-opt | "" )
        if(!do_match_punctuator(tstrm_io, Token::punctuator_brace_op)) {
          return false;
        }
        Cow_Vector<PreHashed_String> keys;
        for(;;) {
          auto duplicate_key_error = do_make_parser_error(tstrm_io, Parser_Error::code_duplicate_object_key);
          Cow_String key;
          bool key_got = do_accept_string_literal(key, tstrm_io) ||
                         do_accept_identifier(key, tstrm_io) ||
                         do_accept_keyword_as_identifier(key, tstrm_io);
          if(!key_got) {
            break;
          }
          if(!do_match_punctuator(tstrm_io, Token::punctuator_assign)) {
            if(!do_match_punctuator(tstrm_io, Token::punctuator_colon)) {
              throw do_make_parser_error(tstrm_io, Parser_Error::code_equals_sign_or_colon_expected);
            }
          }
          if(std::find(keys.begin(), keys.end(), key) != keys.end()) {
            throw duplicate_key_error;
          }
          if(!do_accept_expression(nodes_out, tstrm_io)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
          }
          keys.emplace_back(std::move(key));
          bool has_next = do_match_punctuator(tstrm_io, Token::punctuator_comma) ||
                          do_match_punctuator(tstrm_io, Token::punctuator_semicol);
          if(!has_next) {
            break;
          }
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_brace_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_brace_or_object_key_expected);
        }
        Xpnode::S_unnamed_object node_c = { std::move(keys) };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_nested_expression(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        // nested-expression ::=
        //   "(" expression ")"
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_op)) {
          return false;
        }
        if(!do_accept_expression(nodes_out, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_parenthesis_expected);
        }
        return true;
      }

    bool do_accept_primary_expression(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
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

    bool do_accept_postfix_function_call(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm_io);
        // postfix-function-call ::=
        //   "(" argument-list-opt ")"
        // argument-list-opt ::=
        //   argument-list | ""
        // argument-list ::=
        //   expression ( "," argument-list | "" )
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_op)) {
          return false;
        }
        std::size_t nargs = 0;
        if(do_accept_expression(nodes_out, tstrm_io)) {
          for(;;) {
            ++nargs;
            if(!do_match_punctuator(tstrm_io, Token::punctuator_comma)) {
              break;
            }
            if(!do_accept_expression(nodes_out, tstrm_io)) {
              throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
            }
          }
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_parenthesis_or_argument_expected);
        }
        Xpnode::S_function_call node_c = { std::move(sloc), nargs };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_postfix_subscript(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        // postfix-subscript ::=
        //   "[" expression "]"
        if(!do_match_punctuator(tstrm_io, Token::punctuator_bracket_op)) {
          return false;
        }
        if(!do_accept_expression(nodes_out, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_bracket_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_bracket_expected);
        }
        Xpnode::S_operator_rpn node_c = { Xpnode::xop_postfix_at, false };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_postfix_member_access(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        // postfix-member-access ::=
        //   "." ( string-literal | identifier )
        if(!do_match_punctuator(tstrm_io, Token::punctuator_dot)) {
          return false;
        }
        Cow_String key;
        bool key_got = do_accept_string_literal(key, tstrm_io) ||
                       do_accept_identifier(key, tstrm_io) ||
                       do_accept_keyword_as_identifier(key, tstrm_io);
        if(!key_got) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_identifier_expected);
        }
        Xpnode::S_member_access node_c = { std::move(key) };
        nodes_out.emplace_back(std::move(node_c));
        return true;
      }

    bool do_accept_infix_element(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
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
        Cow_Vector<Xpnode> prefixes;
        if(!do_accept_prefix_operator(prefixes, tstrm_io)) {
          if(!do_accept_primary_expression(nodes_out, tstrm_io)) {
            return false;
          }
        } else {
          for(;;) {
            bool prefix_got = do_accept_prefix_operator(prefixes, tstrm_io);
            if(!prefix_got) {
              break;
            }
          }
          if(!do_accept_primary_expression(nodes_out, tstrm_io)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
          }
        }
        for(;;) {
          bool postfix_got = do_accept_postfix_operator(nodes_out, tstrm_io) ||
                             do_accept_postfix_function_call(nodes_out, tstrm_io) ||
                             do_accept_postfix_subscript(nodes_out, tstrm_io) ||
                             do_accept_postfix_member_access(nodes_out, tstrm_io);
          if(!postfix_got) {
            break;
          }
        }
        while(!prefixes.empty()) {
          nodes_out.emplace_back(std::move(prefixes.mut_back()));
          prefixes.pop_back();
        }
        return true;
      }

    class Infix_Element_Base
      {
      public:
        enum Precedence : unsigned
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
            precedence_max             = 99,
          };

      public:
        Infix_Element_Base() noexcept
          {
          }
        virtual ~Infix_Element_Base()
          {
          }

      public:
        virtual Precedence precedence() const noexcept = 0;
        virtual void extract(Cow_Vector<Xpnode> &nodes_out) = 0;
        virtual void append(Infix_Element_Base &&elem) = 0;
      };

    class Infix_Head : public Infix_Element_Base
      {
      private:
        Cow_Vector<Xpnode> m_nodes;

      public:
        explicit Infix_Head(Cow_Vector<Xpnode> &&nodes)
          : m_nodes(std::move(nodes))
          {
          }

      public:
        Precedence precedence() const noexcept override
          {
            return precedence_max;
          }
        void extract(Cow_Vector<Xpnode> &nodes_out) override
          {
            nodes_out.append(std::make_move_iterator(this->m_nodes.mut_begin()), std::make_move_iterator(this->m_nodes.mut_end()));
          }
        void append(Infix_Element_Base &&elem) override
          {
            elem.extract(this->m_nodes);
          }
      };

    bool do_accept_infix_head(Uptr<Infix_Element_Base> &elem_out, Token_Stream &tstrm_io)
      {
        Cow_Vector<Xpnode> nodes;
        if(!do_accept_infix_element(nodes, tstrm_io)) {
          return false;
        }
        elem_out = rocket::make_unique<Infix_Head>(std::move(nodes));
        return true;
      }

    class Infix_Selection : public Infix_Element_Base
      {
      public:
        enum Sop : std::uint8_t
          {
            sop_quest   = 0,  // ? :
            sop_and     = 1,  // &&
            sop_or      = 2,  // ||
            sop_coales  = 3,  // ??
          };

      private:
        Sop m_sop;
        bool m_assign;
        Cow_Vector<Xpnode> m_branch_true;
        Cow_Vector<Xpnode> m_branch_false;

      public:
        Infix_Selection(Sop sop, bool assign, Cow_Vector<Xpnode> &&branch_true, Cow_Vector<Xpnode> &&branch_false)
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
            case sop_quest:
              {
                return precedence_assignment;
              }
            case sop_and:
              {
                return precedence_logical_and;
              }
            case sop_or:
              {
                return precedence_logical_or;
              }
            case sop_coales:
              {
                return precedence_coalescence;
              }
            default:
              ASTERIA_TERMINATE("Invalid infix selection `", this->m_sop, "` has been encountered.");
            }
          }
        void extract(Cow_Vector<Xpnode> &nodes_out) override
          {
            if(this->m_sop == sop_coales) {
              Xpnode::S_coalescence node_c = { std::move(this->m_branch_false), this->m_assign };
              nodes_out.emplace_back(std::move(node_c));
              return;
            }
            Xpnode::S_branch node_c = { std::move(this->m_branch_true), std::move(this->m_branch_false), this->m_assign };
            nodes_out.emplace_back(std::move(node_c));
          }
        void append(Infix_Element_Base &&elem) override
          {
            elem.extract(this->m_branch_false);
          }
      };

    bool do_accept_infix_selection_quest(Uptr<Infix_Element_Base> &elem_out, Token_Stream &tstrm_io)
      {
        bool assign = false;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_quest)) {
          if(!do_match_punctuator(tstrm_io, Token::punctuator_quest_eq)) {
            return false;
          }
          assign = true;
        }
        Cow_Vector<Xpnode> branch_true;
        if(!do_accept_expression(branch_true, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_colon)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_colon_expected);
        }
        Cow_Vector<Xpnode> branch_false;
        if(!do_accept_infix_element(branch_false, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        elem_out = rocket::make_unique<Infix_Selection>(Infix_Selection::sop_quest, assign, std::move(branch_true), std::move(branch_false));
        return true;
      }

    bool do_accept_infix_selection_and(Uptr<Infix_Element_Base> &elem_out, Token_Stream &tstrm_io)
      {
        bool assign = false;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_andl) && !do_match_keyword(tstrm_io, Token::keyword_and)) {
          if(!do_match_punctuator(tstrm_io, Token::punctuator_andl_eq)) {
            return false;
          }
          assign = true;
        }
        Cow_Vector<Xpnode> branch_true;
        if(!do_accept_infix_element(branch_true, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        elem_out = rocket::make_unique<Infix_Selection>(Infix_Selection::sop_and, assign, std::move(branch_true), Cow_Vector<Xpnode>());
        return true;
      }

    bool do_accept_infix_selection_or(Uptr<Infix_Element_Base> &elem_out, Token_Stream &tstrm_io)
      {
        bool assign = false;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_orl) && !do_match_keyword(tstrm_io, Token::keyword_or)) {
          if(!do_match_punctuator(tstrm_io, Token::punctuator_orl_eq)) {
            return false;
          }
          assign = true;
        }
        Cow_Vector<Xpnode> branch_false;
        if(!do_accept_infix_element(branch_false, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        elem_out = rocket::make_unique<Infix_Selection>(Infix_Selection::sop_or, assign, Cow_Vector<Xpnode>(), std::move(branch_false));
        return true;
      }

    bool do_accept_infix_selection_coales(Uptr<Infix_Element_Base> &elem_out, Token_Stream &tstrm_io)
      {
        bool assign = false;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_coales)) {
          if(!do_match_punctuator(tstrm_io, Token::punctuator_coales_eq)) {
            return false;
          }
          assign = true;
        }
        Cow_Vector<Xpnode> branch_null;
        if(!do_accept_infix_element(branch_null, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        elem_out = rocket::make_unique<Infix_Selection>(Infix_Selection::sop_coales, assign, Cow_Vector<Xpnode>(), std::move(branch_null));
        return true;
      }

    class Infix_Carriage : public Infix_Element_Base
      {
      private:
        Xpnode::Xop m_xop;
        bool m_assign;
        Cow_Vector<Xpnode> m_rhs;

      public:
        Infix_Carriage(Xpnode::Xop xop, bool assign, Cow_Vector<Xpnode> &&rhs)
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
            case Xpnode::xop_infix_mod:
              {
                return precedence_multiplicative;
              }
            case Xpnode::xop_infix_add:
            case Xpnode::xop_infix_sub:
              {
                return precedence_additive;
              }
            case Xpnode::xop_infix_sla:
            case Xpnode::xop_infix_sra:
            case Xpnode::xop_infix_sll:
            case Xpnode::xop_infix_srl:
              {
                return precedence_shift;
              }
            case Xpnode::xop_infix_cmp_lt:
            case Xpnode::xop_infix_cmp_lte:
            case Xpnode::xop_infix_cmp_gt:
            case Xpnode::xop_infix_cmp_gte:
              {
                return precedence_relational;
              }
            case Xpnode::xop_infix_cmp_eq:
            case Xpnode::xop_infix_cmp_ne:
            case Xpnode::xop_infix_cmp_3way:
              {
                return precedence_equality;
              }
            case Xpnode::xop_infix_andb:
              {
                return precedence_bitwise_and;
              }
            case Xpnode::xop_infix_xorb:
              {
                return precedence_bitwise_xor;
              }
            case Xpnode::xop_infix_orb:
              {
                return precedence_bitwise_or;
              }
            case Xpnode::xop_infix_assign:
              {
                return precedence_assignment;
              }
            default:
              ASTERIA_TERMINATE("Invalid infix operator `", this->m_xop, "` has been encountered.");
            }
          }
        void extract(Cow_Vector<Xpnode> &nodes_out) override
          {
            nodes_out.append(std::make_move_iterator(this->m_rhs.mut_begin()), std::make_move_iterator(this->m_rhs.mut_end()));
            // Don't forget the operator!
            Xpnode::S_operator_rpn node_c = { this->m_xop, this->m_assign };
            nodes_out.emplace_back(std::move(node_c));
          }
        void append(Infix_Element_Base &&elem) override
          {
            elem.extract(this->m_rhs);
          }
      };

    bool do_accept_infix_carriage(Uptr<Infix_Element_Base> &elem_out, Token_Stream &tstrm_io)
      {
        // infix-carriage ::=
        //   ( "+"  | "-"  | "*"  | "/"  | "%"  | "<<"  | ">>"  | "<<<"  | ">>>"  | "&"  | "|"  | "^"  |
        //     "+=" | "-=" | "*=" | "/=" | "%=" | "<<=" | ">>=" | "<<<=" | ">>>=" | "&=" | "|=" | "^=" |
        //     "="  | "==" | "!=" | "<"  | ">"  | "<="  | ">="  | "<=>"  ) infix-element
        auto qtok = tstrm_io.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_punctuator>();
        if(!qalt) {
          return false;
        }
        bool assign = false;
        Xpnode::Xop xop;
        switch(rocket::weaken_enum(qalt->punct)) {
        case Token::punctuator_add_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_add:
            xop = Xpnode::xop_infix_add;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_sub_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_sub:
            xop = Xpnode::xop_infix_sub;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_mul_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_mul:
            xop = Xpnode::xop_infix_mul;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_div_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_div:
            xop = Xpnode::xop_infix_div;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_mod_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_mod:
            xop = Xpnode::xop_infix_mod;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_sla_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_sla:
            xop = Xpnode::xop_infix_sla;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_sra_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_sra:
            xop = Xpnode::xop_infix_sra;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_sll_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_sll:
            xop = Xpnode::xop_infix_sll;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_srl_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_srl:
            xop = Xpnode::xop_infix_srl;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_andb_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_andb:
            xop = Xpnode::xop_infix_andb;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_orb_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_orb:
            xop = Xpnode::xop_infix_orb;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_xorb_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_xorb:
            xop = Xpnode::xop_infix_xorb;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_assign:
          {
            xop = Xpnode::xop_infix_assign;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_cmp_eq:
          {
            xop = Xpnode::xop_infix_cmp_eq;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_cmp_ne:
          {
            xop = Xpnode::xop_infix_cmp_ne;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_cmp_lt:
          {
            xop = Xpnode::xop_infix_cmp_lt;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_cmp_gt:
          {
            xop = Xpnode::xop_infix_cmp_gt;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_cmp_lte:
          {
            xop = Xpnode::xop_infix_cmp_lte;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_cmp_gte:
          {
            xop = Xpnode::xop_infix_cmp_gte;
            tstrm_io.shift();
            break;
          }
        case Token::punctuator_spaceship:
          {
            xop = Xpnode::xop_infix_cmp_3way;
            tstrm_io.shift();
            break;
          }
        default:
          return false;
        }
        Cow_Vector<Xpnode> rhs;
        if(!do_accept_infix_element(rhs, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        elem_out = rocket::make_unique<Infix_Carriage>(xop, assign, std::move(rhs));
        return true;
      }

    bool do_accept_expression(Cow_Vector<Xpnode> &nodes_out, Token_Stream &tstrm_io)
      {
        // expression ::=
        //   infix-element infix-carriage-list-opt
        // infix-carriage-list-opt ::=
        //   infix-carriage-list | ""
        // infix-carriage-list ::=
        //   ( infix-selection | infix-carriage ) infix-carriage-list-opt
        // infix-selection ::=
        //   ( "?"  expression ":" | "&&"  | "||"  | "??"  |
        //     "?=" expression ":" | "&&=" | "||=" | "??=" |
        //     "and" | "or" ) infix-element
        Uptr<Infix_Element_Base> elem;
        if(!do_accept_infix_head(elem, tstrm_io)) {
          return false;
        }
        Cow_Vector<Uptr<Infix_Element_Base>> stack;
        stack.emplace_back(std::move(elem));
        for(;;) {
          bool elem_got = do_accept_infix_selection_quest(elem, tstrm_io) ||
                          do_accept_infix_selection_and(elem, tstrm_io) ||
                          do_accept_infix_selection_or(elem, tstrm_io) ||
                          do_accept_infix_selection_coales(elem, tstrm_io) ||
                          do_accept_infix_carriage(elem, tstrm_io);
          if(!elem_got) {
            break;
          }
          // Assignment operations have the lowest precedence and group from right to left.
          auto prec_top = stack.back()->precedence();
          if(prec_top < Infix_Element_Base::precedence_assignment) {
            while((stack.size() > 1) && (prec_top <= elem->precedence())) {
              stack.rbegin()[1]->append(std::move(*(stack.back())));
              stack.pop_back();
            }
          }
          stack.emplace_back(std::move(elem));
        }
        while(stack.size() > 1) {
          stack.rbegin()[1]->append(std::move(*(stack.back())));
          stack.pop_back();
        }
        stack.front()->extract(nodes_out);
        return true;
      }

    bool do_accept_variable_definition(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm_io);
        // variable-definition ::=
        //   "var" identifier equal-initailizer-opt ";"
        // equal-initializer-opt ::=
        //   equal-initializer | ""
        // equal-initializer ::=
        //   "=" expression
        if(!do_match_keyword(tstrm_io, Token::keyword_var)) {
          return false;
        }
        Cow_String name;
        if(!do_accept_identifier(name, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_identifier_expected);
        }
        Cow_Vector<Xpnode> init;
        if(do_match_punctuator(tstrm_io, Token::punctuator_assign)) {
          // The initializer is optional.
          if(!do_accept_expression(init, tstrm_io)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
          }
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_semicolon_expected);
        }
        Statement::S_variable stmt_c = { std::move(sloc), std::move(name), false, std::move(init) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_immutable_variable_definition(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm_io);
        // immutable-variable-definition ::=
        //   "const" identifier equal-initailizer ";"
        // equal-initializer ::=
        //   "=" expression
        if(!do_match_keyword(tstrm_io, Token::keyword_const)) {
          return false;
        }
        Cow_String name;
        if(!do_accept_identifier(name, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_identifier_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_assign)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_equals_sign_expected);
        }
        Cow_Vector<Xpnode> init;
        if(!do_accept_expression(init, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_semicolon_expected);
        }
        Statement::S_variable stmt_c = { std::move(sloc), std::move(name), true, std::move(init) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_function_definition(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm_io);
        // function-definition ::=
        //   "func" identifier "(" identifier-list-opt ")" statement
        if(!do_match_keyword(tstrm_io, Token::keyword_func)) {
          return false;
        }
        Cow_String name;
        if(!do_accept_identifier(name, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_identifier_expected);
        }
        Cow_Vector<PreHashed_String> params;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_open_parenthesis_expected);
        }
        if(do_accept_identifier_list(params, tstrm_io)) {
          // This is optional.
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> body;
        if(!do_accept_statement_as_block(body, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_statement_expected);
        }
        Statement::S_function stmt_c = { std::move(sloc), std::move(name), std::move(params), std::move(body) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_expression_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // expression-statement ::=
        //   expression ";"
        Cow_Vector<Xpnode> expr;
        if(!do_accept_expression(expr, tstrm_io)) {
          return false;
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_semicolon_expected);
        }
        Statement::S_expression stmt_c = { std::move(expr) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_if_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // if-statement ::=
        //   "if" negation-opt "(" expression ")" statement ( "else" statement | "" )
        // negation-opt ::=
        //   negation | ""
        if(!do_match_keyword(tstrm_io, Token::keyword_if)) {
          return false;
        }
        bool neg = false;
        if(do_accept_negation(neg, tstrm_io)) {
          // This is optional.
        }
        Cow_Vector<Xpnode> cond;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_open_parenthesis_expected);
        }
        if(!do_accept_expression(cond, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> branch_true;
        if(!do_accept_statement_as_block(branch_true, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_statement_expected);
        }
        Cow_Vector<Statement> branch_false;
        if(do_match_keyword(tstrm_io, Token::keyword_else)) {
          // The `else` branch is optional.
          if(!do_accept_statement_as_block(branch_false, tstrm_io)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_statement_expected);
          }
        }
        Statement::S_if stmt_c = { neg, std::move(cond), std::move(branch_true), std::move(branch_false) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_switch_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // switch-statement ::=
        //   "switch" "(" expression ")" switch-block
        // switch-block ::=
        //   "{" swtich-clause-list-opt "}"
        // switch-clause-list-opt ::=
        //   switch-clause-list | ""
        // switch-clause-list ::=
        //   ( "case" expression | "default" ) ":" statement-list-opt switch-clause-list-opt
        if(!do_match_keyword(tstrm_io, Token::keyword_switch)) {
          return false;
        }
        Cow_Vector<Xpnode> ctrl;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_open_parenthesis_expected);
        }
        if(!do_accept_expression(ctrl, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<std::pair<Expression, Block>> clauses;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_brace_op)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_open_brace_expected);
        }
        for(;;) {
          Cow_Vector<Xpnode> cond;
          if(!do_match_keyword(tstrm_io, Token::keyword_default)) {
            if(!do_match_keyword(tstrm_io, Token::keyword_case)) {
              break;
            }
            if(!do_accept_expression(cond, tstrm_io)) {
              throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
            }
          }
          if(!do_match_punctuator(tstrm_io, Token::punctuator_colon)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_colon_expected);
          }
          Cow_Vector<Statement> stmts;
          for(;;) {
            bool stmt_got = do_accept_statement(stmts, tstrm_io);
            if(!stmt_got) {
              break;
            }
          }
          clauses.emplace_back(std::move(cond), std::move(stmts));
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_brace_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_brace_or_switch_clause_expected);
        }
        Statement::S_switch stmt_c = { std::move(ctrl), std::move(clauses) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_do_while_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // do-while-statement ::=
        //   "do" statement "while" negation-opt "(" expression ")" ";"
        if(!do_match_keyword(tstrm_io, Token::keyword_do)) {
          return false;
        }
        Cow_Vector<Statement> body;
        if(!do_accept_statement_as_block(body, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_statement_expected);
        }
        if(!do_match_keyword(tstrm_io, Token::keyword_while)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_keyword_while_expected);
        }
        bool neg = false;
        if(do_accept_negation(neg, tstrm_io)) {
          // This is optional.
        }
        Cow_Vector<Xpnode> cond;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_open_parenthesis_expected);
        }
        if(!do_accept_expression(cond, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_parenthesis_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_semicolon_expected);
        }
        Statement::S_do_while stmt_c = { std::move(body), neg, std::move(cond) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_while_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // while-statement ::=
        //   "while" negation-opt "(" expression ")" statement
        if(!do_match_keyword(tstrm_io, Token::keyword_while)) {
          return false;
        }
        bool neg = false;
        if(do_accept_negation(neg, tstrm_io)) {
          // This is optional.
        }
        Cow_Vector<Xpnode> cond;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_open_parenthesis_expected);
        }
        if(!do_accept_expression(cond, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> body;
        if(!do_accept_statement_as_block(body, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_statement_expected);
        }
        Statement::S_while stmt_c = { neg, std::move(cond), std::move(body) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_for_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // for-statement ::=
        //   "for" "(" ( for-statement-range | for-statement-triplet ) ")" statement
        // for-statement-range ::=
        //   "each" identifier ( "," identifier | "") ":" expression
        // for-statement-triplet ::=
        //   ( null-statement | variable-definition | expression-statement ) expression-opt ";" expression-opt
        if(!do_match_keyword(tstrm_io, Token::keyword_for)) {
          return false;
        }
        // This for-statement is ranged if and only if `key_name` is non-empty, where `step` is used as the range initializer.
        Cow_String key_name;
        Cow_String mapped_name;
        Cow_Vector<Statement> init;
        Cow_Vector<Xpnode> cond;
        Cow_Vector<Xpnode> step;
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_open_parenthesis_expected);
        }
        if(do_match_keyword(tstrm_io, Token::keyword_each)) {
          if(!do_accept_identifier(key_name, tstrm_io)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_identifier_expected);
          }
          if(do_match_punctuator(tstrm_io, Token::punctuator_comma)) {
            // The mapped reference is optional.
            if(!do_accept_identifier(mapped_name, tstrm_io)) {
              throw do_make_parser_error(tstrm_io, Parser_Error::code_identifier_expected);
            }
          }
          if(!do_match_punctuator(tstrm_io, Token::punctuator_colon)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_colon_expected);
          }
          if(!do_accept_expression(step, tstrm_io)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
          }
        } else {
          bool init_got = do_accept_variable_definition(init, tstrm_io) ||
                          do_match_punctuator(tstrm_io, Token::punctuator_semicol) ||
                          do_accept_expression_statement(init, tstrm_io);
          if(!init_got) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_for_statement_initializer_expected);
          }
          if(do_accept_expression(cond, tstrm_io)) {
            // This is optional.
          }
          if(!do_match_punctuator(tstrm_io, Token::punctuator_semicol)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_semicolon_expected);
          }
          if(do_accept_expression(step, tstrm_io)) {
            // This is optional.
          }
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> body;
        if(!do_accept_statement_as_block(body, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_statement_expected);
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

    bool do_accept_break_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // break-statement ::=
        //   "break" ( "switch" | "while" | "for" ) ";"
        if(!do_match_keyword(tstrm_io, Token::keyword_break)) {
          return false;
        }
        Statement::Target target = Statement::target_unspec;
        if(do_match_keyword(tstrm_io, Token::keyword_switch)) {
          target = Statement::target_switch;
          goto z;
        }
        if(do_match_keyword(tstrm_io, Token::keyword_while)) {
          target = Statement::target_while;
          goto z;
        }
        if(do_match_keyword(tstrm_io, Token::keyword_for)) {
          target = Statement::target_for;
          goto z;
        }
      z:
        if(!do_match_punctuator(tstrm_io, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_semicolon_expected);
        }
        Statement::S_break stmt_c = { target };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_continue_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // continue-statement ::=
        //   "continue" ( "while" | "for" ) ";"
        if(!do_match_keyword(tstrm_io, Token::keyword_continue)) {
          return false;
        }
        Statement::Target target = Statement::target_unspec;
        if(do_match_keyword(tstrm_io, Token::keyword_while)) {
          target = Statement::target_while;
          goto z;
        }
        if(do_match_keyword(tstrm_io, Token::keyword_for)) {
          target = Statement::target_for;
          goto z;
        }
      z:
        if(!do_match_punctuator(tstrm_io, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_semicolon_expected);
        }
        Statement::S_continue stmt_c = { target };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_throw_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm_io);
        // throw-statement ::=
        //   "throw" expression ";"
        if(!do_match_keyword(tstrm_io, Token::keyword_throw)) {
          return false;
        }
        Cow_Vector<Xpnode> expr;
        if(!do_accept_expression(expr, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_semicolon_expected);
        }
        Statement::S_throw stmt_c = { std::move(sloc), std::move(expr) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_return_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // return-statement ::=
        //   "return" ( "&" | "" ) expression-opt ";"
        if(!do_match_keyword(tstrm_io, Token::keyword_return)) {
          return false;
        }
        bool by_ref = false;
        if(do_match_punctuator(tstrm_io, Token::punctuator_andb)) {
          // The reference specifier is optional.
          by_ref = true;
        }
        Cow_Vector<Xpnode> expr;
        if(do_accept_expression(expr, tstrm_io)) {
          // This is optional.
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_semicolon_or_expression_expected);
        }
        Statement::S_return stmt_c = { by_ref, std::move(expr) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_assert_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // assert-statement ::=
        //  "assert" expression ( ":" string-literal | "" ) ";"
        if(!do_match_keyword(tstrm_io, Token::keyword_assert)) {
          return false;
        }
        Cow_Vector<Xpnode> expr;
        if(!do_accept_expression(expr, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_expression_expected);
        }
        Cow_String msg;
        if(do_match_punctuator(tstrm_io, Token::punctuator_colon)) {
          // The descriptive message is optional.
          if(!do_accept_string_literal(msg, tstrm_io)) {
            throw do_make_parser_error(tstrm_io, Parser_Error::code_string_literal_expected);
          }
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_semicolon_expected);
        }
        Statement::S_assert stmt_c = { std::move(expr), std::move(msg) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_try_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm_io);
        // try-statement ::=
        //   "try" statement "catch" "(" identifier ")" statement
        if(!do_match_keyword(tstrm_io, Token::keyword_try)) {
          return false;
        }
        Cow_Vector<Statement> body_try;
        if(!do_accept_statement_as_block(body_try, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_statement_expected);
        }
        if(!do_match_keyword(tstrm_io, Token::keyword_catch)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_keyword_catch_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_open_parenthesis_expected);
        }
        Cow_String except_name;
        if(!do_accept_identifier(except_name, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_identifier_expected);
        }
        if(!do_match_punctuator(tstrm_io, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> body_catch;
        if(!do_accept_statement_as_block(body_catch, tstrm_io)) {
          throw do_make_parser_error(tstrm_io, Parser_Error::code_statement_expected);
        }
        Statement::S_try stmt_c = { std::move(body_try), std::move(sloc), std::move(except_name), std::move(body_catch) };
        stmts_out.emplace_back(std::move(stmt_c));
        return true;
      }

    bool do_accept_nonblock_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // nonblock-statement ::=
        //   null-statement |
        //   variable-definition | immutable-variable-definition | function-definition |
        //   expression-statement |
        //   if-statement | switch-statement |
        //   do-while-statement | while-statement | for-statement |
        //   break-statement | continue-statement | throw-statement | return-statement | assert-statement |
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
               do_accept_assert_statement(stmts_out, tstrm_io) ||
               do_accept_try_statement(stmts_out, tstrm_io);
      }

    bool do_accept_statement_as_block(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // statement ::=
        //   block-statement | nonblock-statement
        return do_accept_block_statement_list(stmts_out, tstrm_io) ||
               do_accept_nonblock_statement(stmts_out, tstrm_io);
      }

    bool do_accept_statement(Cow_Vector<Statement> &stmts_out, Token_Stream &tstrm_io)
      {
        // statement ::=
        //   block-statement | nonblock-statement
        return do_accept_block_statement(stmts_out, tstrm_io) ||
               do_accept_nonblock_statement(stmts_out, tstrm_io);
      }

    }

Parser_Error Parser::get_parser_error() const noexcept
  {
    switch(this->state()) {
    case state_empty:
      {
        return Parser_Error(0, 0, 0, Parser_Error::code_no_data_loaded);
      }
    case state_error:
      {
        return this->m_stor.as<Parser_Error>();
      }
    case state_success:
      {
        return Parser_Error(0, 0, 0, Parser_Error::code_success);
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
    }
  }

bool Parser::empty() const noexcept
  {
    switch(this->state()) {
    case state_empty:
      {
        return true;
      }
    case state_error:
      {
        return true;
      }
    case state_success:
      {
        return this->m_stor.as<Cow_Vector<Statement>>().empty();
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
    }
  }

bool Parser::load(Token_Stream &tstrm_io, const Parser_Options & /*options*/)
  try {
    // This has to be done before anything else because of possibility of exceptions.
    this->m_stor = nullptr;
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Parse the document recursively.
    ///////////////////////////////////////////////////////////////////////////
    Cow_Vector<Statement> stmts;
    // document ::=
    //   statement-list-opt
    for(;;) {
      bool stmt_got = do_accept_statement(stmts, tstrm_io);
      if(!stmt_got) {
        break;
      }
    }
    if(!tstrm_io.empty()) {
      throw do_make_parser_error(tstrm_io, Parser_Error::code_statement_expected);
    }
    ///////////////////////////////////////////////////////////////////////////
    // Finish
    ///////////////////////////////////////////////////////////////////////////
    this->m_stor = std::move(stmts);
    return true;
  } catch(Parser_Error &err) {  // Don't play with this at home.
    ASTERIA_DEBUG_LOG("Caught `Parser_Error`:\n",
                      "line = ", err.line(), ", offset = ", err.offset(), ", length = ", err.length(), "\n",
                      "code = ", err.code(), ": ", Parser_Error::get_code_description(err.code()));
    this->m_stor = std::move(err);
    return false;
  }

void Parser::clear() noexcept
  {
    this->m_stor = nullptr;
  }

Cow_Vector<Statement> Parser::get_statements() const noexcept
  {
    switch(this->state()) {
    case state_empty:
      {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
    case state_error:
      {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
    case state_success:
      {
        return this->m_stor.as<Cow_Vector<Statement>>();
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
    }
  }

}
