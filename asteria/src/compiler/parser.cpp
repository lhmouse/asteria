// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "parser.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "../syntax/statement.hpp"
#include "../syntax/xprunit.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    Parser_Error do_make_parser_error(const Token_Stream& tstrm, Parser_Error::Code code)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return Parser_Error(UINT32_MAX, SIZE_MAX, 0, code);
        }
        return Parser_Error(qtok->line(), qtok->offset(), qtok->length(), code);
      }

    Source_Location do_tell_source_location(const Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return Source_Location(rocket::sref("<end of stream>"), UINT32_MAX);
        }
        return Source_Location(qtok->file(), qtok->line());
      }

    bool do_match_keyword(Token_Stream& tstrm, Token::Keyword keyword)
      {
        auto qtok = tstrm.peek_opt();
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
        tstrm.shift();
        return true;
      }

    bool do_match_punctuator(Token_Stream& tstrm, Token::Punctuator punct)
      {
        auto qtok = tstrm.peek_opt();
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
        tstrm.shift();
        return true;
      }

    bool do_accept_identifier(Cow_String& name, Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_identifier>();
        if(!qalt) {
          return false;
        }
        name = qalt->name;
        tstrm.shift();
        return true;
      }

    bool do_accept_string_literal(Cow_String& value, Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_string_literal>();
        if(!qalt) {
          return false;
        }
        value = qalt->value;
        tstrm.shift();
        return true;
      }

    bool do_accept_keyword_as_identifier(Cow_String& name, Token_Stream& tstrm)
      {
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_keyword>();
        if(!qalt) {
          return false;
        }
        name = rocket::sref(Token::get_keyword(qalt->keyword));
        tstrm.shift();
        return true;
      }

    bool do_accept_prefix_operator(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // prefix-operator ::=
        //   "+" | "-" | "~" | "!" | "++" | "--" | "unset" | "lengthof" | "typeof"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        Xprunit::Xop xop;
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword:
          {
            const auto& alt = qtok->check<Token::S_keyword>();
            switch(rocket::weaken_enum(alt.keyword)) {
            case Token::keyword_unset:
              {
                xop = Xprunit::xop_prefix_unset;
                tstrm.shift();
                break;
              }
            case Token::keyword_lengthof:
              {
                xop = Xprunit::xop_prefix_lengthof;
                tstrm.shift();
                break;
              }
            case Token::keyword_typeof:
              {
                xop = Xprunit::xop_prefix_typeof;
                tstrm.shift();
                break;
              }
            case Token::keyword_not:
              {
                xop = Xprunit::xop_prefix_notl;
                tstrm.shift();
                break;
              }
            default:
              return false;
            }
            break;
          }
        case Token::index_punctuator:
          {
            const auto& alt = qtok->check<Token::S_punctuator>();
            switch(rocket::weaken_enum(alt.punct)) {
            case Token::punctuator_add:
              {
                xop = Xprunit::xop_prefix_pos;
                tstrm.shift();
                break;
              }
            case Token::punctuator_sub:
              {
                xop = Xprunit::xop_prefix_neg;
                tstrm.shift();
                break;
              }
            case Token::punctuator_notb:
              {
                xop = Xprunit::xop_prefix_notb;
                tstrm.shift();
                break;
              }
            case Token::punctuator_notl:
              {
                xop = Xprunit::xop_prefix_notl;
                tstrm.shift();
                break;
              }
            case Token::punctuator_inc:
              {
                xop = Xprunit::xop_prefix_inc;
                tstrm.shift();
                break;
              }
            case Token::punctuator_dec:
              {
                xop = Xprunit::xop_prefix_dec;
                tstrm.shift();
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
        Xprunit::S_operator_rpn xpru_c = { xop, false };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_postfix_operator(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // postfix-operator ::=
        //   "++" | "--"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_punctuator>();
        if(!qalt) {
          return false;
        }
        Xprunit::Xop xop;
        switch(rocket::weaken_enum(qalt->punct)) {
        case Token::punctuator_inc:
          {
            xop = Xprunit::xop_postfix_inc;
            tstrm.shift();
            break;
          }
        case Token::punctuator_dec:
          {
            xop = Xprunit::xop_postfix_dec;
            tstrm.shift();
            break;
          }
        default:
          return false;
        }
        Xprunit::S_operator_rpn xpru_c = { xop, false };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_literal(Value& value, Token_Stream& tstrm)
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
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword:
          {
            const auto& alt = qtok->check<Token::S_keyword>();
            switch(rocket::weaken_enum(alt.keyword)) {
            case Token::keyword_null:
              {
                value = D_null();
                tstrm.shift();
                break;
              }
            case Token::keyword_false:
              {
                value = D_boolean(false);
                tstrm.shift();
                break;
              }
            case Token::keyword_true:
              {
                value = D_boolean(true);
                tstrm.shift();
                break;
              }
            case Token::keyword_nan:
              {
                value = D_real(NAN);
                tstrm.shift();
                break;
              }
            case Token::keyword_infinity:
              {
                value = D_real(INFINITY);
                tstrm.shift();
                break;
              }
            default:
              return false;
            }
            break;
          }
        case Token::index_integer_literal:
          {
            const auto& alt = qtok->check<Token::S_integer_literal>();
            value = D_integer(alt.value);
            tstrm.shift();
            break;
          }
        case Token::index_real_literal:
          {
            const auto& alt = qtok->check<Token::S_real_literal>();
            value = D_real(alt.value);
            tstrm.shift();
            break;
          }
        case Token::index_string_literal:
          {
            const auto& alt = qtok->check<Token::S_string_literal>();
            value = D_string(alt.value);
            tstrm.shift();
            break;
          }
        default:
          return false;
        }
        return true;
      }

    bool do_accept_negation(bool& negative, Token_Stream& tstrm)
      {
        // negation ::=
        //   "!" | "not"
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        switch(rocket::weaken_enum(qtok->index())) {
        case Token::index_keyword:
          {
            const auto& alt = qtok->check<Token::S_keyword>();
            switch(rocket::weaken_enum(alt.keyword)) {
            case Token::keyword_not:
              {
                negative = true;
                tstrm.shift();
                break;
              }
            default:
              return false;
            }
            break;
          }
        case Token::index_punctuator:
          {
            const auto& alt = qtok->check<Token::S_punctuator>();
            switch(rocket::weaken_enum(alt.punct)) {
            case Token::punctuator_notl:
              {
                negative = true;
                tstrm.shift();
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

    extern bool do_accept_statement_as_block(Cow_Vector<Statement>& stmts, Token_Stream& tstrm);
    extern bool do_accept_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm);
    extern bool do_accept_expression(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm);

    bool do_accept_block_statement_list(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // block ::=
        //   "{" statement-list-opt "}"
        // statement-list-opt ::=
        //   statement-list | ""
        // statement-list ::=
        //   statement statement-list-opt
        if(!do_match_punctuator(tstrm, Token::punctuator_brace_op)) {
          return false;
        }
        for(;;) {
          bool stmt_got = do_accept_statement(stmts, tstrm);
          if(!stmt_got) {
            break;
          }
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_brace_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_brace_or_statement_expected);
        }
        return true;
      }

    bool do_accept_block_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        Cow_Vector<Statement> body;
        if(!do_accept_block_statement_list(body, tstrm)) {
          return false;
        }
        Statement::S_block stmt_c = { rocket::move(body) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_identifier_list(Cow_Vector<PreHashed_String>& names, Token_Stream& tstrm)
      {
        // identifier-list-opt ::=
        //   identifier-list | ""
        // identifier-list ::=
        //   identifier ( "," identifier-list | "" )
        Cow_String name;
        if(!do_accept_identifier(name, tstrm)) {
          return false;
        }
        for(;;) {
          names.emplace_back(rocket::move(name));
          if(!do_match_punctuator(tstrm, Token::punctuator_comma)) {
            break;
          }
          if(!do_accept_identifier(name, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
          }
        }
        return true;
      }

    bool do_accept_named_reference(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // Get a name first.
        Cow_String name;
        if(!do_accept_identifier(name, tstrm)) {
          return false;
        }
        // Handle special names.
        if(name == "__file") {
          Xprunit::S_literal xpru_c = { D_string(sloc.file()) };
          xprus.emplace_back(rocket::move(xpru_c));
          return true;
        }
        if(name == "__line") {
          Xprunit::S_literal xpru_c = { D_integer(sloc.line()) };
          xprus.emplace_back(rocket::move(xpru_c));
          return true;
        }
        Xprunit::S_named_reference xpru_c = { rocket::move(name) };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_literal(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        Value value;
        if(!do_accept_literal(value, tstrm)) {
          return false;
        }
        Xprunit::S_literal xpru_c = { rocket::move(value) };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_this(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        if(!do_match_keyword(tstrm, Token::keyword_this)) {
          return false;
        }
        Xprunit::S_named_reference xpru_c = { rocket::sref("__this") };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_closure_function(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // closure-function ::=
        //   "func" parameter-list ( block | "return" expression )
        if(!do_match_keyword(tstrm, Token::keyword_func)) {
          return false;
        }
        Cow_Vector<PreHashed_String> params;
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        if(do_accept_identifier_list(params, tstrm)) {
          // This is optional.
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> body;
        if(!do_accept_block_statement_list(body, tstrm)) {
          // An expression is expected, which behaves as if it was the operand of a `return&` ststement.
          Cow_Vector<Xprunit> xprus_ret;
          if(!do_accept_expression(xprus_ret, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_open_brace_or_expression_expected);
          }
          Statement::S_return stmt_c = { true, rocket::move(xprus_ret) };
          body.emplace_back(rocket::move(stmt_c));
        }
        Xprunit::S_closure_function xpru_c = { rocket::move(sloc), rocket::move(params), rocket::move(body) };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_unnamed_array(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // unnamed-array ::=
        //   "[" array-element-list-opt "]"
        // array-element-list-opt ::=
        //   array-element-list | ""
        // array-element-list ::=
        //   expression ( ( "," | ";" ) array-element-list-opt | "" )
        if(!do_match_punctuator(tstrm, Token::punctuator_bracket_op)) {
          return false;
        }
        std::size_t nelems = 0;
        for(;;) {
          if(!do_accept_expression(xprus, tstrm)) {
            break;
          }
          ++nelems;
          bool has_next = do_match_punctuator(tstrm, Token::punctuator_comma) ||
                          do_match_punctuator(tstrm, Token::punctuator_semicol);
          if(!has_next) {
            break;
          }
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_bracket_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_bracket_or_expression_expected);
        }
        Xprunit::S_unnamed_array xpru_c = { nelems };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_unnamed_object(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // unnamed-object ::=
        //   "{" key-mapped-list-opt "}"
        // key-mapped-list-opt ::=
        //   key-mapped-list | ""
        // key-mapped-list ::=
        //   ( string-literal | identifier ) ( "=" | ":" ) expression ( ( "," | ";" ) key-mapped-list-opt | "" )
        if(!do_match_punctuator(tstrm, Token::punctuator_brace_op)) {
          return false;
        }
        Cow_Vector<PreHashed_String> keys;
        for(;;) {
          auto duplicate_key_error = do_make_parser_error(tstrm, Parser_Error::code_duplicate_object_key);
          Cow_String key;
          bool key_got = do_accept_string_literal(key, tstrm) ||
                         do_accept_identifier(key, tstrm) ||
                         do_accept_keyword_as_identifier(key, tstrm);
          if(!key_got) {
            break;
          }
          if(!do_match_punctuator(tstrm, Token::punctuator_assign)) {
            if(!do_match_punctuator(tstrm, Token::punctuator_colon)) {
              throw do_make_parser_error(tstrm, Parser_Error::code_equals_sign_or_colon_expected);
            }
          }
          if(std::find(keys.begin(), keys.end(), key) != keys.end()) {
            throw duplicate_key_error;
          }
          if(!do_accept_expression(xprus, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
          }
          keys.emplace_back(rocket::move(key));
          bool has_next = do_match_punctuator(tstrm, Token::punctuator_comma) ||
                          do_match_punctuator(tstrm, Token::punctuator_semicol);
          if(!has_next) {
            break;
          }
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_brace_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_brace_or_object_key_expected);
        }
        Xprunit::S_unnamed_object xpru_c = { rocket::move(keys) };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_nested_expression(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // nested-expression ::=
        //   "(" expression ")"
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_op)) {
          return false;
        }
        if(!do_accept_expression(xprus, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        return true;
      }

    bool do_accept_primary_expression(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // primary-expression ::=
        //   identifier | literal | "this" | closure-function | unnamed-array | unnamed-object | nested-expression
        return do_accept_named_reference(xprus, tstrm) ||
               do_accept_literal(xprus, tstrm) ||
               do_accept_this(xprus, tstrm) ||
               do_accept_closure_function(xprus, tstrm) ||
               do_accept_unnamed_object(xprus, tstrm) ||
               do_accept_unnamed_array(xprus, tstrm) ||
               do_accept_nested_expression(xprus, tstrm);
      }

    bool do_accept_postfix_function_call(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // postfix-function-call ::=
        //   "(" expression-list-opt ")"
        // expression-list ::=
        //   expression ( "," expression-list | "" )
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_op)) {
          return false;
        }
        std::size_t nargs = 0;
        if(do_accept_expression(xprus, tstrm)) {
          for(;;) {
            ++nargs;
            if(!do_match_punctuator(tstrm, Token::punctuator_comma)) {
              break;
            }
            if(!do_accept_expression(xprus, tstrm)) {
              throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
            }
          }
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_or_argument_expected);
        }
        Xprunit::S_function_call xpru_c = { rocket::move(sloc), nargs };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_postfix_subscript(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // postfix-subscript ::=
        //   "[" expression "]"
        if(!do_match_punctuator(tstrm, Token::punctuator_bracket_op)) {
          return false;
        }
        if(!do_accept_expression(xprus, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_bracket_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_bracket_expected);
        }
        Xprunit::S_operator_rpn xpru_c = { Xprunit::xop_postfix_at, false };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_postfix_member_access(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
      {
        // postfix-member-access ::=
        //   "." ( string-literal | identifier )
        if(!do_match_punctuator(tstrm, Token::punctuator_dot)) {
          return false;
        }
        Cow_String key;
        bool key_got = do_accept_string_literal(key, tstrm) ||
                       do_accept_identifier(key, tstrm) ||
                       do_accept_keyword_as_identifier(key, tstrm);
        if(!key_got) {
          throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
        }
        Xprunit::S_member_access xpru_c = { rocket::move(key) };
        xprus.emplace_back(rocket::move(xpru_c));
        return true;
      }

    bool do_accept_infix_element(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
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
        Cow_Vector<Xprunit> prefixes;
        if(!do_accept_prefix_operator(prefixes, tstrm)) {
          if(!do_accept_primary_expression(xprus, tstrm)) {
            return false;
          }
        } else {
          for(;;) {
            bool prefix_got = do_accept_prefix_operator(prefixes, tstrm);
            if(!prefix_got) {
              break;
            }
          }
          if(!do_accept_primary_expression(xprus, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
          }
        }
        for(;;) {
          bool postfix_got = do_accept_postfix_operator(xprus, tstrm) ||
                             do_accept_postfix_function_call(xprus, tstrm) ||
                             do_accept_postfix_subscript(xprus, tstrm) ||
                             do_accept_postfix_member_access(xprus, tstrm);
          if(!postfix_got) {
            break;
          }
        }
        while(!prefixes.empty()) {
          xprus.emplace_back(rocket::move(prefixes.mut_back()));
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
        virtual void extract(Cow_Vector<Xprunit>& xprus) = 0;
        virtual void append(Infix_Element_Base&& elem) = 0;
      };

    class Infix_Head : public Infix_Element_Base
      {
      private:
        Cow_Vector<Xprunit> m_xprus;

      public:
        explicit Infix_Head(Cow_Vector<Xprunit>&& xprus)
          : m_xprus(rocket::move(xprus))
          {
          }

      public:
        Precedence precedence() const noexcept override
          {
            return precedence_max;
          }
        void extract(Cow_Vector<Xprunit>& xprus) override
          {
            xprus.append(std::make_move_iterator(this->m_xprus.mut_begin()), std::make_move_iterator(this->m_xprus.mut_end()));
          }
        void append(Infix_Element_Base&& elem) override
          {
            elem.extract(this->m_xprus);
          }
      };

    bool do_accept_infix_head(Uptr<Infix_Element_Base>& elem, Token_Stream& tstrm)
      {
        Cow_Vector<Xprunit> xprus;
        if(!do_accept_infix_element(xprus, tstrm)) {
          return false;
        }
        elem = rocket::make_unique<Infix_Head>(rocket::move(xprus));
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
        Cow_Vector<Xprunit> m_branch_true;
        Cow_Vector<Xprunit> m_branch_false;

      public:
        Infix_Selection(Sop sop, bool assign, Cow_Vector<Xprunit>&& branch_true, Cow_Vector<Xprunit>&& branch_false)
          : m_sop(sop), m_assign(assign), m_branch_true(rocket::move(branch_true)), m_branch_false(rocket::move(branch_false))
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
        void extract(Cow_Vector<Xprunit>& xprus) override
          {
            if(this->m_sop == sop_coales) {
              Xprunit::S_coalescence xpru_c = { rocket::move(this->m_branch_false), this->m_assign };
              xprus.emplace_back(rocket::move(xpru_c));
              return;
            }
            Xprunit::S_branch xpru_c = { rocket::move(this->m_branch_true), rocket::move(this->m_branch_false), this->m_assign };
            xprus.emplace_back(rocket::move(xpru_c));
          }
        void append(Infix_Element_Base&& elem) override
          {
            elem.extract(this->m_branch_false);
          }
      };

    bool do_accept_infix_selection_quest(Uptr<Infix_Element_Base>& elem, Token_Stream& tstrm)
      {
        bool assign = false;
        if(!do_match_punctuator(tstrm, Token::punctuator_quest)) {
          if(!do_match_punctuator(tstrm, Token::punctuator_quest_eq)) {
            return false;
          }
          assign = true;
        }
        Cow_Vector<Xprunit> branch_true;
        if(!do_accept_expression(branch_true, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_colon)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_colon_expected);
        }
        Cow_Vector<Xprunit> branch_false;
        if(!do_accept_infix_element(branch_false, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        elem = rocket::make_unique<Infix_Selection>(Infix_Selection::sop_quest, assign, rocket::move(branch_true), rocket::move(branch_false));
        return true;
      }

    bool do_accept_infix_selection_and(Uptr<Infix_Element_Base>& elem, Token_Stream& tstrm)
      {
        bool assign = false;
        if(!do_match_punctuator(tstrm, Token::punctuator_andl) && !do_match_keyword(tstrm, Token::keyword_and)) {
          if(!do_match_punctuator(tstrm, Token::punctuator_andl_eq)) {
            return false;
          }
          assign = true;
        }
        Cow_Vector<Xprunit> branch_true;
        if(!do_accept_infix_element(branch_true, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        elem = rocket::make_unique<Infix_Selection>(Infix_Selection::sop_and, assign, rocket::move(branch_true), Cow_Vector<Xprunit>());
        return true;
      }

    bool do_accept_infix_selection_or(Uptr<Infix_Element_Base>& elem, Token_Stream& tstrm)
      {
        bool assign = false;
        if(!do_match_punctuator(tstrm, Token::punctuator_orl) && !do_match_keyword(tstrm, Token::keyword_or)) {
          if(!do_match_punctuator(tstrm, Token::punctuator_orl_eq)) {
            return false;
          }
          assign = true;
        }
        Cow_Vector<Xprunit> branch_false;
        if(!do_accept_infix_element(branch_false, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        elem = rocket::make_unique<Infix_Selection>(Infix_Selection::sop_or, assign, Cow_Vector<Xprunit>(), rocket::move(branch_false));
        return true;
      }

    bool do_accept_infix_selection_coales(Uptr<Infix_Element_Base>& elem, Token_Stream& tstrm)
      {
        bool assign = false;
        if(!do_match_punctuator(tstrm, Token::punctuator_coales)) {
          if(!do_match_punctuator(tstrm, Token::punctuator_coales_eq)) {
            return false;
          }
          assign = true;
        }
        Cow_Vector<Xprunit> branch_null;
        if(!do_accept_infix_element(branch_null, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        elem = rocket::make_unique<Infix_Selection>(Infix_Selection::sop_coales, assign, Cow_Vector<Xprunit>(), rocket::move(branch_null));
        return true;
      }

    class Infix_Carriage : public Infix_Element_Base
      {
      private:
        Xprunit::Xop m_xop;
        bool m_assign;
        Cow_Vector<Xprunit> m_rhs;

      public:
        Infix_Carriage(Xprunit::Xop xop, bool assign, Cow_Vector<Xprunit>&& rhs)
          : m_xop(xop), m_assign(assign), m_rhs(rocket::move(rhs))
          {
          }

      public:
        Precedence precedence() const noexcept override
          {
            if(this->m_assign) {
              return precedence_assignment;
            }
            switch(rocket::weaken_enum(this->m_xop)) {
            case Xprunit::xop_infix_mul:
            case Xprunit::xop_infix_div:
            case Xprunit::xop_infix_mod:
              {
                return precedence_multiplicative;
              }
            case Xprunit::xop_infix_add:
            case Xprunit::xop_infix_sub:
              {
                return precedence_additive;
              }
            case Xprunit::xop_infix_sla:
            case Xprunit::xop_infix_sra:
            case Xprunit::xop_infix_sll:
            case Xprunit::xop_infix_srl:
              {
                return precedence_shift;
              }
            case Xprunit::xop_infix_cmp_lt:
            case Xprunit::xop_infix_cmp_lte:
            case Xprunit::xop_infix_cmp_gt:
            case Xprunit::xop_infix_cmp_gte:
              {
                return precedence_relational;
              }
            case Xprunit::xop_infix_cmp_eq:
            case Xprunit::xop_infix_cmp_ne:
            case Xprunit::xop_infix_cmp_3way:
              {
                return precedence_equality;
              }
            case Xprunit::xop_infix_andb:
              {
                return precedence_bitwise_and;
              }
            case Xprunit::xop_infix_xorb:
              {
                return precedence_bitwise_xor;
              }
            case Xprunit::xop_infix_orb:
              {
                return precedence_bitwise_or;
              }
            case Xprunit::xop_infix_assign:
              {
                return precedence_assignment;
              }
            default:
              ASTERIA_TERMINATE("Invalid infix operator `", this->m_xop, "` has been encountered.");
            }
          }
        void extract(Cow_Vector<Xprunit>& xprus) override
          {
            xprus.append(std::make_move_iterator(this->m_rhs.mut_begin()), std::make_move_iterator(this->m_rhs.mut_end()));
            // Don't forget the operator!
            Xprunit::S_operator_rpn xpru_c = { this->m_xop, this->m_assign };
            xprus.emplace_back(rocket::move(xpru_c));
          }
        void append(Infix_Element_Base&& elem) override
          {
            elem.extract(this->m_rhs);
          }
      };

    bool do_accept_infix_carriage(Uptr<Infix_Element_Base>& elem, Token_Stream& tstrm)
      {
        // infix-carriage ::=
        //   ( "+"  | "-"  | "*"  | "/"  | "%"  | "<<"  | ">>"  | "<<<"  | ">>>"  | "&"  | "|"  | "^"  |
        //     "+=" | "-=" | "*=" | "/=" | "%=" | "<<=" | ">>=" | "<<<=" | ">>>=" | "&=" | "|=" | "^=" |
        //     "="  | "==" | "!=" | "<"  | ">"  | "<="  | ">="  | "<=>"  ) infix-element
        auto qtok = tstrm.peek_opt();
        if(!qtok) {
          return false;
        }
        auto qalt = qtok->opt<Token::S_punctuator>();
        if(!qalt) {
          return false;
        }
        bool assign = false;
        Xprunit::Xop xop;
        switch(rocket::weaken_enum(qalt->punct)) {
        case Token::punctuator_add_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_add:
            xop = Xprunit::xop_infix_add;
            tstrm.shift();
            break;
          }
        case Token::punctuator_sub_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_sub:
            xop = Xprunit::xop_infix_sub;
            tstrm.shift();
            break;
          }
        case Token::punctuator_mul_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_mul:
            xop = Xprunit::xop_infix_mul;
            tstrm.shift();
            break;
          }
        case Token::punctuator_div_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_div:
            xop = Xprunit::xop_infix_div;
            tstrm.shift();
            break;
          }
        case Token::punctuator_mod_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_mod:
            xop = Xprunit::xop_infix_mod;
            tstrm.shift();
            break;
          }
        case Token::punctuator_sla_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_sla:
            xop = Xprunit::xop_infix_sla;
            tstrm.shift();
            break;
          }
        case Token::punctuator_sra_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_sra:
            xop = Xprunit::xop_infix_sra;
            tstrm.shift();
            break;
          }
        case Token::punctuator_sll_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_sll:
            xop = Xprunit::xop_infix_sll;
            tstrm.shift();
            break;
          }
        case Token::punctuator_srl_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_srl:
            xop = Xprunit::xop_infix_srl;
            tstrm.shift();
            break;
          }
        case Token::punctuator_andb_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_andb:
            xop = Xprunit::xop_infix_andb;
            tstrm.shift();
            break;
          }
        case Token::punctuator_orb_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_orb:
            xop = Xprunit::xop_infix_orb;
            tstrm.shift();
            break;
          }
        case Token::punctuator_xorb_eq:
          {
            assign = true;
            // Fallthrough.
        case Token::punctuator_xorb:
            xop = Xprunit::xop_infix_xorb;
            tstrm.shift();
            break;
          }
        case Token::punctuator_assign:
          {
            xop = Xprunit::xop_infix_assign;
            tstrm.shift();
            break;
          }
        case Token::punctuator_cmp_eq:
          {
            xop = Xprunit::xop_infix_cmp_eq;
            tstrm.shift();
            break;
          }
        case Token::punctuator_cmp_ne:
          {
            xop = Xprunit::xop_infix_cmp_ne;
            tstrm.shift();
            break;
          }
        case Token::punctuator_cmp_lt:
          {
            xop = Xprunit::xop_infix_cmp_lt;
            tstrm.shift();
            break;
          }
        case Token::punctuator_cmp_gt:
          {
            xop = Xprunit::xop_infix_cmp_gt;
            tstrm.shift();
            break;
          }
        case Token::punctuator_cmp_lte:
          {
            xop = Xprunit::xop_infix_cmp_lte;
            tstrm.shift();
            break;
          }
        case Token::punctuator_cmp_gte:
          {
            xop = Xprunit::xop_infix_cmp_gte;
            tstrm.shift();
            break;
          }
        case Token::punctuator_spaceship:
          {
            xop = Xprunit::xop_infix_cmp_3way;
            tstrm.shift();
            break;
          }
        default:
          return false;
        }
        Cow_Vector<Xprunit> rhs;
        if(!do_accept_infix_element(rhs, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        elem = rocket::make_unique<Infix_Carriage>(xop, assign, rocket::move(rhs));
        return true;
      }

    bool do_accept_expression(Cow_Vector<Xprunit>& xprus, Token_Stream& tstrm)
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
        if(!do_accept_infix_head(elem, tstrm)) {
          return false;
        }
        Cow_Vector<Uptr<Infix_Element_Base>> stack;
        stack.emplace_back(rocket::move(elem));
        for(;;) {
          bool elem_got = do_accept_infix_selection_quest(elem, tstrm) ||
                          do_accept_infix_selection_and(elem, tstrm) ||
                          do_accept_infix_selection_or(elem, tstrm) ||
                          do_accept_infix_selection_coales(elem, tstrm) ||
                          do_accept_infix_carriage(elem, tstrm);
          if(!elem_got) {
            break;
          }
          // Assignment operations have the lowest precedence and group from right to left.
          auto prec_top = stack.back()->precedence();
          if(prec_top < Infix_Element_Base::precedence_assignment) {
            while((stack.size() > 1) && (prec_top <= elem->precedence())) {
              stack.rbegin()[1]->append(rocket::move(*(stack.back())));
              stack.pop_back();
            }
          }
          stack.emplace_back(rocket::move(elem));
        }
        while(stack.size() > 1) {
          stack.rbegin()[1]->append(rocket::move(*(stack.back())));
          stack.pop_back();
        }
        stack.front()->extract(xprus);
        return true;
      }

    bool do_accept_variable_definition(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // variable-definition ::=
        //   "var" identifier equal-initailizer-opt ( "," identifier equal-initializer-opt | "" ) ";"
        // equal-initializer-opt ::=
        //   equal-initializer | ""
        // equal-initializer ::=
        //   "=" expression
        if(!do_match_keyword(tstrm, Token::keyword_var)) {
          return false;
        }
        Cow_Vector<Statement::Variable_Declaration> vars;
        for(;;) {
          Cow_String name;
          if(!do_accept_identifier(name, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
          }
          Cow_Vector<Xprunit> init;
          if(do_match_punctuator(tstrm, Token::punctuator_assign)) {
            // The initializer is optional.
            if(!do_accept_expression(init, tstrm)) {
              throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
            }
          }
          Statement::Variable_Declaration var = { rocket::move(name), rocket::move(init) };
          vars.emplace_back(rocket::move(var));
          if(!do_match_punctuator(tstrm, Token::punctuator_comma)) {
            break;
          }
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_variable stmt_c = { rocket::move(sloc), false, rocket::move(vars) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_immutable_variable_definition(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // immutable-variable-definition ::=
        //   "const" identifier equal-initailizer ( "," identifier equal-initializer | "" ) ";"
        // equal-initializer ::=
        //   "=" expression
        if(!do_match_keyword(tstrm, Token::keyword_const)) {
          return false;
        }
        Cow_Vector<Statement::Variable_Declaration> vars;
        for(;;) {
          Cow_String name;
          if(!do_accept_identifier(name, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
          }
          if(!do_match_punctuator(tstrm, Token::punctuator_assign)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_equals_sign_expected);
          }
          Cow_Vector<Xprunit> init;
          if(!do_accept_expression(init, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
          }
          Statement::Variable_Declaration var = { rocket::move(name), rocket::move(init) };
          vars.emplace_back(rocket::move(var));
          if(!do_match_punctuator(tstrm, Token::punctuator_comma)) {
            break;
          }
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_variable stmt_c = { rocket::move(sloc), true, rocket::move(vars) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_function_definition(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // function-definition ::=
        //   "func" identifier parameter-list statement
        // parameter-list ::=
        //   "(" ( identifier-list | "" ) ")"
        if(!do_match_keyword(tstrm, Token::keyword_func)) {
          return false;
        }
        Cow_String name;
        if(!do_accept_identifier(name, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
        }
        Cow_Vector<PreHashed_String> params;
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        if(do_accept_identifier_list(params, tstrm)) {
          // This is optional.
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> body;
        if(!do_accept_statement_as_block(body, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        Statement::S_function stmt_c = { rocket::move(sloc), rocket::move(name), rocket::move(params), rocket::move(body) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_expression_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // expression-statement ::=
        //   expression ";"
        Cow_Vector<Xprunit> expr;
        if(!do_accept_expression(expr, tstrm)) {
          return false;
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_expression stmt_c = { rocket::move(expr) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_if_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // if-statement ::=
        //   "if" negation-opt "(" expression ")" statement ( "else" statement | "" )
        // negation-opt ::=
        //   negation | ""
        if(!do_match_keyword(tstrm, Token::keyword_if)) {
          return false;
        }
        bool negative = false;
        if(do_accept_negation(negative, tstrm)) {
          // This is optional.
        }
        Cow_Vector<Xprunit> cond;
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        if(!do_accept_expression(cond, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> branch_true;
        if(!do_accept_statement_as_block(branch_true, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        Cow_Vector<Statement> branch_false;
        if(do_match_keyword(tstrm, Token::keyword_else)) {
          // The `else` branch is optional.
          if(!do_accept_statement_as_block(branch_false, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
          }
        }
        Statement::S_if stmt_c = { negative, rocket::move(cond), rocket::move(branch_true), rocket::move(branch_false) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_switch_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // switch-statement ::=
        //   "switch" "(" expression ")" switch-block
        // switch-block ::=
        //   "{" swtich-clause-list-opt "}"
        // switch-clause-list-opt ::=
        //   switch-clause-list | ""
        // switch-clause-list ::=
        //   ( "case" expression | "default" ) ":" statement-list-opt switch-clause-list-opt
        if(!do_match_keyword(tstrm, Token::keyword_switch)) {
          return false;
        }
        Cow_Vector<Xprunit> ctrl;
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        if(!do_accept_expression(ctrl, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement::Switch_Clause> clauses;
        if(!do_match_punctuator(tstrm, Token::punctuator_brace_op)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_brace_expected);
        }
        for(;;) {
          Cow_Vector<Xprunit> cond;
          if(!do_match_keyword(tstrm, Token::keyword_default)) {
            if(!do_match_keyword(tstrm, Token::keyword_case)) {
              break;
            }
            if(!do_accept_expression(cond, tstrm)) {
              throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
            }
          }
          if(!do_match_punctuator(tstrm, Token::punctuator_colon)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_colon_expected);
          }
          Cow_Vector<Statement> body;
          for(;;) {
            bool stmt_got = do_accept_statement(body, tstrm);
            if(!stmt_got) {
              break;
            }
          }
          Statement::Switch_Clause clause = { rocket::move(cond), rocket::move(body) };
          clauses.emplace_back(rocket::move(clause));
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_brace_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_brace_or_switch_clause_expected);
        }
        Statement::S_switch stmt_c = { rocket::move(ctrl), rocket::move(clauses) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_do_while_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // do-while-statement ::=
        //   "do" statement "while" negation-opt "(" expression ")" ";"
        if(!do_match_keyword(tstrm, Token::keyword_do)) {
          return false;
        }
        Cow_Vector<Statement> body;
        if(!do_accept_statement_as_block(body, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        if(!do_match_keyword(tstrm, Token::keyword_while)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_keyword_while_expected);
        }
        bool negative = false;
        if(do_accept_negation(negative, tstrm)) {
          // This is optional.
        }
        Cow_Vector<Xprunit> cond;
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        if(!do_accept_expression(cond, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_do_while stmt_c = { rocket::move(body), negative, rocket::move(cond) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_while_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // while-statement ::=
        //   "while" negation-opt "(" expression ")" statement
        if(!do_match_keyword(tstrm, Token::keyword_while)) {
          return false;
        }
        bool negative = false;
        if(do_accept_negation(negative, tstrm)) {
          // This is optional.
        }
        Cow_Vector<Xprunit> cond;
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        if(!do_accept_expression(cond, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> body;
        if(!do_accept_statement_as_block(body, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        Statement::S_while stmt_c = { negative, rocket::move(cond), rocket::move(body) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_for_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // for-statement ::=
        //   "for" "(" ( for-statement-range | for-statement-triplet ) ")" statement
        // for-statement-range ::=
        //   "each" identifier ( "," identifier | "") ":" expression
        // for-statement-triplet ::=
        //   ( null-statement | variable-definition | expression-statement ) expression-opt ";" expression-opt
        if(!do_match_keyword(tstrm, Token::keyword_for)) {
          return false;
        }
        // This for-statement is ranged if and only if `key_name` is non-empty, where `step` is used as the range initializer.
        Cow_String key_name;
        Cow_String mapped_name;
        Cow_Vector<Statement> init;
        Cow_Vector<Xprunit> cond;
        Cow_Vector<Xprunit> step;
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        if(do_match_keyword(tstrm, Token::keyword_each)) {
          if(!do_accept_identifier(key_name, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
          }
          if(do_match_punctuator(tstrm, Token::punctuator_comma)) {
            // The mapped reference is optional.
            if(!do_accept_identifier(mapped_name, tstrm)) {
              throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
            }
          }
          if(!do_match_punctuator(tstrm, Token::punctuator_colon)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_colon_expected);
          }
          if(!do_accept_expression(step, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
          }
        } else {
          bool init_got = do_accept_variable_definition(init, tstrm) ||
                          do_match_punctuator(tstrm, Token::punctuator_semicol) ||
                          do_accept_expression_statement(init, tstrm);
          if(!init_got) {
            throw do_make_parser_error(tstrm, Parser_Error::code_for_statement_initializer_expected);
          }
          if(do_accept_expression(cond, tstrm)) {
            // This is optional.
          }
          if(!do_match_punctuator(tstrm, Token::punctuator_semicol)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
          }
          if(do_accept_expression(step, tstrm)) {
            // This is optional.
          }
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> body;
        if(!do_accept_statement_as_block(body, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        if(key_name.empty()) {
          Statement::S_for stmt_c = { rocket::move(init), rocket::move(cond), rocket::move(step), rocket::move(body) };
          stmts.emplace_back(rocket::move(stmt_c));
          return true;
        }
        Statement::S_for_each stmt_c = { rocket::move(key_name), rocket::move(mapped_name), rocket::move(step), rocket::move(body) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_break_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // break-statement ::=
        //   "break" ( "switch" | "while" | "for" ) ";"
        if(!do_match_keyword(tstrm, Token::keyword_break)) {
          return false;
        }
        Statement::Target target = Statement::target_unspec;
        if(do_match_keyword(tstrm, Token::keyword_switch)) {
          target = Statement::target_switch;
          goto z;
        }
        if(do_match_keyword(tstrm, Token::keyword_while)) {
          target = Statement::target_while;
          goto z;
        }
        if(do_match_keyword(tstrm, Token::keyword_for)) {
          target = Statement::target_for;
          goto z;
        }
      z:
        if(!do_match_punctuator(tstrm, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_break stmt_c = { target };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_continue_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // continue-statement ::=
        //   "continue" ( "while" | "for" ) ";"
        if(!do_match_keyword(tstrm, Token::keyword_continue)) {
          return false;
        }
        Statement::Target target = Statement::target_unspec;
        if(do_match_keyword(tstrm, Token::keyword_while)) {
          target = Statement::target_while;
          goto z;
        }
        if(do_match_keyword(tstrm, Token::keyword_for)) {
          target = Statement::target_for;
          goto z;
        }
      z:
        if(!do_match_punctuator(tstrm, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_continue stmt_c = { target };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_throw_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // throw-statement ::=
        //   "throw" expression ";"
        if(!do_match_keyword(tstrm, Token::keyword_throw)) {
          return false;
        }
        Cow_Vector<Xprunit> expr;
        if(!do_accept_expression(expr, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_throw stmt_c = { rocket::move(sloc), rocket::move(expr) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_return_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // return-statement ::=
        //   "return" ( "&" | "" ) expression-opt ";"
        if(!do_match_keyword(tstrm, Token::keyword_return)) {
          return false;
        }
        bool by_ref = false;
        if(do_match_punctuator(tstrm, Token::punctuator_andb)) {
          // The reference specifier is optional.
          by_ref = true;
        }
        Cow_Vector<Xprunit> expr;
        if(do_accept_expression(expr, tstrm)) {
          // This is optional.
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_or_expression_expected);
        }
        Statement::S_return stmt_c = { by_ref, rocket::move(expr) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_assert_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // assert-statement ::=
        //  "assert" negation-opt expression ( ":" string-literal | "" ) ";"
        if(!do_match_keyword(tstrm, Token::keyword_assert)) {
          return false;
        }
        bool negative = false;
        if(do_accept_negation(negative, tstrm)) {
          // This is optional.
        }
        Cow_Vector<Xprunit> expr;
        if(!do_accept_expression(expr, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_expression_expected);
        }
        Cow_String msg;
        if(do_match_punctuator(tstrm, Token::punctuator_colon)) {
          // The descriptive message is optional.
          if(!do_accept_string_literal(msg, tstrm)) {
            throw do_make_parser_error(tstrm, Parser_Error::code_string_literal_expected);
          }
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_semicol)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_semicolon_expected);
        }
        Statement::S_assert stmt_c = { rocket::move(sloc), negative, rocket::move(expr), rocket::move(msg) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_try_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // Copy these parameters before reading from the stream which is destructive.
        auto sloc = do_tell_source_location(tstrm);
        // try-statement ::=
        //   "try" statement "catch" "(" identifier ")" statement
        if(!do_match_keyword(tstrm, Token::keyword_try)) {
          return false;
        }
        Cow_Vector<Statement> body_try;
        if(!do_accept_statement_as_block(body_try, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        if(!do_match_keyword(tstrm, Token::keyword_catch)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_keyword_catch_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_op)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_open_parenthesis_expected);
        }
        Cow_String except_name;
        if(!do_accept_identifier(except_name, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_identifier_expected);
        }
        if(!do_match_punctuator(tstrm, Token::punctuator_parenth_cl)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_closed_parenthesis_expected);
        }
        Cow_Vector<Statement> body_catch;
        if(!do_accept_statement_as_block(body_catch, tstrm)) {
          throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
        }
        Statement::S_try stmt_c = { rocket::move(body_try), rocket::move(sloc), rocket::move(except_name), rocket::move(body_catch) };
        stmts.emplace_back(rocket::move(stmt_c));
        return true;
      }

    bool do_accept_nonblock_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // nonblock-statement ::=
        //   null-statement |
        //   variable-definition | immutable-variable-definition | function-definition |
        //   expression-statement |
        //   if-statement | switch-statement |
        //   do-while-statement | while-statement | for-statement |
        //   break-statement | continue-statement | throw-statement | return-statement | assert-statement |
        //   try-statement
        return do_match_punctuator(tstrm, Token::punctuator_semicol) ||
               do_accept_variable_definition(stmts, tstrm) ||
               do_accept_immutable_variable_definition(stmts, tstrm) ||
               do_accept_function_definition(stmts, tstrm) ||
               do_accept_expression_statement(stmts, tstrm) ||
               do_accept_if_statement(stmts, tstrm) ||
               do_accept_switch_statement(stmts, tstrm) ||
               do_accept_do_while_statement(stmts, tstrm) ||
               do_accept_while_statement(stmts, tstrm) ||
               do_accept_for_statement(stmts, tstrm) ||
               do_accept_break_statement(stmts, tstrm) ||
               do_accept_continue_statement(stmts, tstrm) ||
               do_accept_throw_statement(stmts, tstrm) ||
               do_accept_return_statement(stmts, tstrm) ||
               do_accept_assert_statement(stmts, tstrm) ||
               do_accept_try_statement(stmts, tstrm);
      }

    bool do_accept_statement_as_block(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // statement ::=
        //   block-statement | nonblock-statement
        return do_accept_block_statement_list(stmts, tstrm) ||
               do_accept_nonblock_statement(stmts, tstrm);
      }

    bool do_accept_statement(Cow_Vector<Statement>& stmts, Token_Stream& tstrm)
      {
        // statement ::=
        //   block-statement | nonblock-statement
        return do_accept_block_statement(stmts, tstrm) ||
               do_accept_nonblock_statement(stmts, tstrm);
      }

    }  // namespace

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

bool Parser::load(Token_Stream& tstrm, const Parser_Options& /*options*/)
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
      bool stmt_got = do_accept_statement(stmts, tstrm);
      if(!stmt_got) {
        break;
      }
    }
    if(!tstrm.empty()) {
      throw do_make_parser_error(tstrm, Parser_Error::code_statement_expected);
    }
    ///////////////////////////////////////////////////////////////////////////
    // Finish
    ///////////////////////////////////////////////////////////////////////////
    this->m_stor = rocket::move(stmts);
    return true;
  } catch(Parser_Error& err) {  // Don't play with this at home.
    ASTERIA_DEBUG_LOG("Caught `Parser_Error`:\n",
                      "line = ", err.line(), ", offset = ", err.offset(), ", length = ", err.length(), "\n",
                      "code = ", err.code(), ": ", Parser_Error::get_code_description(err.code()));
    this->m_stor = rocket::move(err);
    return false;
  }

void Parser::clear() noexcept
  {
    this->m_stor = nullptr;
  }

const Cow_Vector<Statement>& Parser::get_statements() const
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

}  // namespace Asteria
