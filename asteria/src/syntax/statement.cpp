// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "statement.hpp"
#include "xprunit.hpp"
#include "../runtime/analytic_context.hpp"
#include "../runtime/executive_context.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/evaluation_stack.hpp"
#include "../runtime/instantiated_function.hpp"
#include "../runtime/exception.hpp"
#include "../llds/air_queue.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    template<typename XrefT> void do_set_user_declared_reference(Cow_Vector<PreHashed_String>* names_opt, Abstract_Context& ctx,
                                                                 const char* desc, const PreHashed_String& name, XrefT&& xref)
      {
        if(name.empty()) {
          return;
        }
        if(name.rdstr().starts_with("__")) {
          ASTERIA_THROW_RUNTIME_ERROR("The name `", name, "` for this ", desc, " is reserved and cannot be used.");
        }
        if(names_opt) {
          names_opt->emplace_back(name);
        }
        ctx.open_named_reference(name) = rocket::forward<XrefT>(xref);
      }

    Rcptr<Variable> do_safe_create_variable(Cow_Vector<PreHashed_String>* names_opt, Executive_Context& ctx,
                                            const char* desc, const PreHashed_String& name)
      {
        auto var = ctx.global().create_variable();
        Reference_Root::S_variable xref = { var };
        do_set_user_declared_reference(names_opt, ctx, desc, name, rocket::move(xref));
        return var;
      }

    Air_Queue do_generate_code_statement_list(Cow_Vector<PreHashed_String>* names_opt, Analytic_Context& ctx,
                                                         const Compiler_Options& options, const Cow_Vector<Statement>& stmts)
      {
        Air_Queue code;
        rocket::for_each(stmts, [&](const Statement& stmt) { stmt.generate_code(code, names_opt, ctx, options);  });
        return code;
      }

    Air_Queue do_generate_code_block(const Compiler_Options& options, const Analytic_Context& ctx, const Cow_Vector<Statement>& block)
      {
        Analytic_Context ctx_next(1, ctx);
        auto code = do_generate_code_statement_list(nullptr, ctx_next, options, block);
        return code;
      }

    Air_Node::Status do_execute_statement_list(Executive_Context& ctx, const Air_Queue& code)
      {
        return code.execute(ctx);
      }

    Air_Node::Status do_execute_block(const Air_Queue& code, const Executive_Context& ctx)
      {
        Executive_Context ctx_next(1, ctx);
        auto status = do_execute_statement_list(ctx_next, code);
        return status;
      }

    Air_Node::Status do_execute_catch(const Air_Queue& code, const PreHashed_String& except_name, const Exception& except, const Executive_Context& ctx)
      {
        Executive_Context ctx_catch(1, ctx);
        Reference_Root::S_temporary xref = { except.get_value() };
        do_set_user_declared_reference(nullptr, ctx_catch, "exception reference", except_name, rocket::move(xref));
        // Initialize backtrace information.
        G_array backtrace;
        for(std::size_t i = 0; i < except.count_frames(); ++i) {
          const auto& frame = except.get_frame(i);
          G_object elem;
          // Translate the frame.
          elem.try_emplace(rocket::sref("ftype"), G_string(rocket::sref(Backtrace_Frame::stringify_ftype(frame.ftype()))));
          elem.try_emplace(rocket::sref("file"), G_string(frame.file()));
          elem.try_emplace(rocket::sref("line"), G_integer(frame.line()));
          elem.try_emplace(rocket::sref("value"), frame.value());
          // Append the frame. Note that frames are stored from bottom to top, in contrast to what you usually see in a debugger.
          backtrace.emplace_back(rocket::move(elem));
        }
        xref.value = rocket::move(backtrace);
         ASTERIA_DEBUG_LOG("Exception backtrace:\n", xref.value);
        ctx_catch.open_named_reference(rocket::sref("__backtrace")) = rocket::move(xref);
        // Execute the `catch` body.
        return do_execute_statement_list(ctx_catch, code);
      }

    Air_Queue do_generate_code_expression(const Compiler_Options& options, const Analytic_Context& ctx, const Cow_Vector<Xprunit>& expr)
      {
        Air_Queue code;
        rocket::for_each(expr, [&](const Xprunit& unit) { unit.generate_code(code, options, Xprunit::tco_none, ctx);  });
        return code;
      }

    Reference&& do_evaluate_expression_nonempty(const Air_Queue& code, const Executive_Context& ctx)
      {
        ROCKET_ASSERT(!code.empty());
        // Evaluate the expression. The result will be pushed on `stack`.
        ctx.stack().clear_references();
        code.execute(const_cast<Executive_Context&>(ctx));
        return rocket::move(ctx.stack().open_top_reference());
      }

    class Air_execute_clear_stack : public Air_Node
      {
      private:
        //

      public:
        Air_execute_clear_stack()
          // :
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // We push a null reference in case of empty expressions.
            ctx.stack().clear_references();
            ctx.stack().push_reference(Reference_Root::S_null());
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_block : public Air_Node
      {
      private:
        Air_Queue m_code;

      public:
        explicit Air_execute_block(Air_Queue&& code)
          : m_code(rocket::move(code))
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Execute the block without affecting `ctx`.
            return do_execute_block(this->m_code, ctx);
          }
        void enumerate_variables(const Abstract_Variable_Callback& callback) const override
          {
            this->m_code.enumerate_variables(callback);
          }
      };

    class Air_define_uninitialized_variable : public Air_Node
      {
      private:
        Source_Location m_sloc;
        bool m_immutable;
        PreHashed_String m_name;

      public:
        Air_define_uninitialized_variable(const Source_Location& sloc, bool immutable, const PreHashed_String& name)
          : m_sloc(sloc), m_immutable(immutable), m_name(name)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Allocate a variable.
            auto var = do_safe_create_variable(nullptr, ctx, "variable", this->m_name);
            // Initialize the variable.
            var->reset(this->m_sloc, G_null(), this->m_immutable);
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_declare_variable_and_clear_stack : public Air_Node
      {
      private:
        PreHashed_String m_name;

      public:
        explicit Air_declare_variable_and_clear_stack(const PreHashed_String& name)
          : m_name(name)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Allocate a variable.
            auto var = do_safe_create_variable(nullptr, ctx, "variable placeholder", this->m_name);
            ctx.stack().set_last_variable(rocket::move(var));
            // Note that the initializer must not be empty for this code.
            ctx.stack().clear_references();
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_initialize_variable : public Air_Node
      {
      private:
        Source_Location m_sloc;
        bool m_immutable;

      public:
        Air_initialize_variable(const Source_Location& sloc, bool immutable)
          : m_sloc(sloc), m_immutable(immutable)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Read the value of the initializer.
            // Note that the initializer must not have been empty for this code.
            auto value = ctx.stack().get_top_reference().read();
            ctx.stack().pop_reference();
            // Get back the variable that has been allocated in `do_declare_variable_and_clear_stack()`.
            auto var = ctx.stack().release_last_variable_opt();
            ROCKET_ASSERT(var);
            var->reset(this->m_sloc, rocket::move(value), this->m_immutable);
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_define_function : public Air_Node
      {
      private:
        Compiler_Options m_options;
        Source_Location m_sloc;
        PreHashed_String m_name;
        Cow_Vector<PreHashed_String> m_params;
        Cow_Vector<Statement> m_body;

      public:
        Air_define_function(const Compiler_Options& options, const Source_Location& sloc,
                            const PreHashed_String& name, const Cow_Vector<PreHashed_String>& params, const Cow_Vector<Statement>& body)
          : m_options(options), m_sloc(sloc),
            m_name(name), m_params(params), m_body(body)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Create a dummy reference for further name lookups.
            // A function becomes visible before its definition, where it is initialized to `null`.
            auto var = do_safe_create_variable(nullptr, ctx, "function", this->m_name);
            // Generate code for the function body.
            Air_Queue code_func;
            Analytic_Context ctx_func(1, ctx, this->m_params);
            rocket::for_each(this->m_body, [&](const Statement& stmt) { stmt.generate_code(code_func, nullptr, ctx_func, this->m_options);  });
            // Format the prototype string.
            Cow_osstream fmtss;
            fmtss.imbue(std::locale::classic());
            fmtss << this->m_name << "(";
            if(!this->m_params.empty()) {
              std::for_each(this->m_params.begin(), this->m_params.end() - 1, [&](const PreHashed_String& param) { fmtss << param << ", ";  });
              fmtss << this->m_params.back();
            }
            fmtss <<")";
            Rcobj<Instantiated_Function> closure(this->m_sloc, fmtss.extract_string(), this->m_params, rocket::move(code_func));
            // Initialized the function variable.
            var->reset(this->m_sloc, G_function(rocket::move(closure)), true);
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_branch : public Air_Node
      {
      private:
        bool m_negative;
        Air_Queue m_code_true;
        Air_Queue m_code_false;

      public:
        Air_execute_branch(bool negative, Air_Queue&& code_true, Air_Queue&& code_false)
          : m_negative(negative), m_code_true(rocket::move(code_true)), m_code_false(rocket::move(code_false))
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // Pick a branch basing on the condition.
            if(ctx.stack().get_top_reference().read().test() != this->m_negative) {
              // Execute the true branch. Forward any status codes unexpected to the caller.
              return do_execute_block(this->m_code_true, ctx);
            }
            else {
              // Execute the false branch. Forward any status codes unexpected to the caller.
              return do_execute_block(this->m_code_false, ctx);
            }
          }
        void enumerate_variables(const Abstract_Variable_Callback& callback) const override
          {
            this->m_code_true.enumerate_variables(callback);
            this->m_code_false.enumerate_variables(callback);
          }
      };

    struct S_xswitch_clause
      {
        Air_Queue code_cond;
        Air_Queue code_clause;
        Cow_Vector<PreHashed_String> names;
      };

    class Air_execute_switch : public Air_Node
      {
      private:
        Cow_Vector<S_xswitch_clause> m_clauses;

      public:
        explicit Air_execute_switch(Cow_Vector<S_xswitch_clause>&& clauses)
          : m_clauses(rocket::move(clauses))
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This is different from a C `switch` statement where `case` labels must have constant operands.
            // Evaluate the control expression.
            auto ctrl_value = ctx.stack().get_top_reference().read();
            // Set the target clause.
            auto target = this->m_clauses.end();
            // Iterate over all `case` labels and evaluate them. Stop if the result value equals `ctrl_value`.
            // In this loop, `qtarget` points to the `default` clause.
            for(auto it = this->m_clauses.begin(); it != this->m_clauses.end(); ++it) {
              if(it->code_cond.empty()) {
                // This is a `default` clause.
                if(target != this->m_clauses.end()) {
                  ASTERIA_THROW_RUNTIME_ERROR("Multiple `default` clauses have been found in this `switch` statement.");
                }
                target = it;
                continue;
              }
              // This is a `case` clause.
              // Evaluate the operand and check whether it equals `ctrl_value`.
              if(do_evaluate_expression_nonempty(it->code_cond, ctx).read().compare(ctrl_value) == Value::compare_equal) {
                // Found a `case` label. Stop.
                target = it;
                break;
              }
            }
            if(target == this->m_clauses.end()) {
              // No match clause has been found.
              return Air_Node::status_next;
            }
            // Jump to the clause denoted by `*qtarget`.
            // Note that all clauses share the same context.
            Executive_Context ctx_body(1, ctx);
            // Skip clauses that precede `*qtarget`.
            for(auto it = this->m_clauses.begin(); it != target; ++it) {
              // Inject all names into this scope.
              rocket::for_each(it->names, [&](const PreHashed_String& name) { do_set_user_declared_reference(nullptr, ctx_body, "skipped reference",
                                                                                                             name, Reference_Root::S_null());  });
            }
            // Execute all clauses from `*qtarget` to the end of this block.
            for(auto it = target; it != this->m_clauses.end(); ++it) {
              // Execute the clause. Break out of the block if requested. Forward any status codes unexpected to the caller.
              auto status = do_execute_statement_list(ctx_body, it->code_clause);
              if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_switch })) {
                break;
              }
              if(status != Air_Node::status_next) {
                return status;
              }
            }
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& callback) const override
          {
            for(auto it = this->m_clauses.begin(); it != this->m_clauses.end(); ++it) {
              it->code_cond.enumerate_variables(callback);
              it->code_clause.enumerate_variables(callback);
            }
          }
      };

    class Air_execute_do_while : public Air_Node
      {
      private:
        Air_Queue m_code_body;
        bool m_negative;
        Air_Queue m_code_cond;

      public:
        Air_execute_do_while(Air_Queue&& code_body, bool negative, Air_Queue&& code_cond)
          : m_code_body(rocket::move(code_body)), m_negative(negative), m_code_cond(rocket::move(code_cond))
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This is the same as a `do...while` loop in C.
            for(;;) {
              // Execute the body. Break out of the loop if requested. Forward any status codes unexpected to the caller.
              auto status = do_execute_block(this->m_code_body, ctx);
              if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_while })) {
                break;
              }
              if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_while })) {
                return status;
              }
              // Check the condition.
              if(do_evaluate_expression_nonempty(this->m_code_cond, ctx).read().test() == this->m_negative) {
                break;
              }
            }
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& callback) const override
          {
            this->m_code_body.enumerate_variables(callback);
            this->m_code_cond.enumerate_variables(callback);
          }
      };

    class Air_execute_while : public Air_Node
      {
      private:
        bool m_negative;
        Air_Queue m_code_cond;
        Air_Queue m_code_body;

      public:
        Air_execute_while(bool negative, Air_Queue&& code_cond, Air_Queue&& code_body)
          : m_negative(negative), m_code_cond(rocket::move(code_cond)), m_code_body(rocket::move(code_body))
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This is the same as a `while` loop in C.
            for(;;) {
              // Check the condition.
              if(do_evaluate_expression_nonempty(this->m_code_cond, ctx).read().test() == this->m_negative) {
                break;
              }
              // Execute the body. Break out of the loop if requested. Forward any status codes unexpected to the caller.
              auto status = do_execute_block(this->m_code_body, ctx);
              if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_while })) {
                break;
              }
              if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_while })) {
                return status;
              }
            }
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& callback) const override
          {
            this->m_code_body.enumerate_variables(callback);
            this->m_code_cond.enumerate_variables(callback);
          }
      };

    class Air_execute_for_each : public Air_Node
      {
      private:
        PreHashed_String m_key_name;
        PreHashed_String m_mapped_name;
        Air_Queue m_code_init;
        Air_Queue m_code_body;

      public:
        Air_execute_for_each(const PreHashed_String& key_name, const PreHashed_String& mapped_name, Air_Queue&& code_init,
                             Air_Queue&& code_body)
          : m_key_name(key_name), m_mapped_name(mapped_name), m_code_init(rocket::move(code_init)),
            m_code_body(rocket::move(code_body))
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This is the same as a ranged-`for` loop in C++.
            Executive_Context ctx_for(1, ctx);
            auto key_var = do_safe_create_variable(nullptr, ctx_for, "key variable", this->m_key_name);
            do_set_user_declared_reference(nullptr, ctx_for, "mapped reference", this->m_mapped_name, Reference_Root::S_null());
            // Evaluate the range initializer.
            auto range_ref = do_evaluate_expression_nonempty(this->m_code_init, ctx_for);
            auto range_value = range_ref.read();
            // Iterate over the range.
            if(range_value.is_array()) {
              const auto& array = range_value.as_array();
              for(auto it = array.begin(); it != array.end(); ++it) {
                // Create a fresh context for the loop body.
                Executive_Context ctx_body(1, ctx_for);
                // Set up the key variable, which is immutable.
                key_var->reset(Source_Location(rocket::sref("<built-in>"), 0), G_integer(it - array.begin()), true);
                // Set up the mapped reference.
                Reference_Modifier::S_array_index xrefm = { it - array.begin() };
                range_ref.zoom_in(rocket::move(xrefm));
                do_set_user_declared_reference(nullptr, ctx_for, "mapped reference", this->m_mapped_name, range_ref);
                range_ref.zoom_out();
                // Execute the loop body.
                auto status = do_execute_statement_list(ctx_body, this->m_code_body);
                if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_for })) {
                  break;
                }
                if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_for })) {
                  return status;
                }
              }
              return Air_Node::status_next;
            }
            else if(range_value.is_object()) {
              const auto& object = range_value.as_object();
              for(auto it = object.begin(); it != object.end(); ++it) {
                // Create a fresh context for the loop body.
                Executive_Context ctx_body(1, ctx_for);
                // Set up the key variable, which is immutable.
                key_var->reset(Source_Location(rocket::sref("<built-in>"), 0), G_string(it->first), true);
                // Set up the mapped reference.
                Reference_Modifier::S_object_key xrefm = { it->first };
                range_ref.zoom_in(rocket::move(xrefm));
                do_set_user_declared_reference(nullptr, ctx_for, "mapped reference", this->m_mapped_name, range_ref);
                range_ref.zoom_out();
                // Execute the loop body.
                auto status = do_execute_statement_list(ctx_body, this->m_code_body);
                if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_for })) {
                  break;
                }
                if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_for })) {
                  return status;
                }
              }
              return Air_Node::status_next;
            }
            else {
              ASTERIA_THROW_RUNTIME_ERROR("The `for each` statement does not accept a range of type `", range_value.gtype_name(), "`.");
            }
          }
        void enumerate_variables(const Abstract_Variable_Callback& callback) const override
          {
            this->m_code_init.enumerate_variables(callback);
            this->m_code_body.enumerate_variables(callback);
          }
      };

    class Air_execute_for : public Air_Node
      {
      private:
        Air_Queue m_code_init;
        Air_Queue m_code_cond;
        Air_Queue m_code_step;
        Air_Queue m_code_body;

      public:
        Air_execute_for(Air_Queue&& code_init, Air_Queue&& code_cond, Air_Queue&& code_step,
                        Air_Queue&& code_body)
          : m_code_init(rocket::move(code_init)), m_code_cond(rocket::move(code_cond)), m_code_step(rocket::move(code_step)),
            m_code_body(rocket::move(code_body))
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This is the same as a `for` loop in C.
            Executive_Context ctx_for(1, ctx);
            do_execute_statement_list(ctx_for, this->m_code_init);
            for(;;) {
              // Treat an empty condition as being always true.
              if(!this->m_code_cond.empty()) {
                // Check the condition.
                if(!do_evaluate_expression_nonempty(this->m_code_cond, ctx_for).read().test()) {
                  break;
                }
              }
              // Execute the body. Break out of the loop if requested. Forward any status codes unexpected to the caller.
              auto status = do_execute_block(this->m_code_body, ctx_for);
              if(rocket::is_any_of(status, { Air_Node::status_break_unspec, Air_Node::status_break_for })) {
                break;
              }
              if(rocket::is_none_of(status, { Air_Node::status_next, Air_Node::status_continue_unspec, Air_Node::status_continue_for })) {
                return status;
              }
              // Evaluate the step expression and discard its value.
              if(!this->m_code_step.empty()) {
                do_evaluate_expression_nonempty(this->m_code_step, ctx_for);
              }
            }
            return Air_Node::status_next;
          }
        void enumerate_variables(const Abstract_Variable_Callback& callback) const override
          {
            this->m_code_init.enumerate_variables(callback);
            this->m_code_cond.enumerate_variables(callback);
            this->m_code_step.enumerate_variables(callback);
            this->m_code_body.enumerate_variables(callback);
          }
      };

    class Air_execute_try : public Air_Node
      {
      private:
        Air_Queue m_code_try;
        Source_Location m_sloc;
        PreHashed_String m_except_name;
        Air_Queue m_code_catch;

      public:
        Air_execute_try(Air_Queue&& code_try,
                        const Source_Location& sloc, const PreHashed_String& except_name, Air_Queue&& code_catch)
          : m_code_try(rocket::move(code_try)), m_sloc(sloc), m_except_name(except_name),
            m_code_catch(rocket::move(code_catch))
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // This is the same as a `try...catch` block in C++.
            try {
              // Execute the `try` clause. If no exception is thrown, this will have little overhead.
              return do_execute_block(this->m_code_try, ctx);
            }
            catch(Exception& except) {
              // Reuse the exception object. Don't bother allocating a new one.
              except.push_frame_catch(this->m_sloc);
              ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", except);
              return do_execute_catch(this->m_code_catch, this->m_except_name, except, ctx);
            }
            catch(const std::exception& stdex) {
              // Translate the exception.
              Exception except(stdex);
              except.push_frame_catch(this->m_sloc);
              ASTERIA_DEBUG_LOG("Translated `std::exception`: ", except);
              return do_execute_catch(this->m_code_catch, this->m_except_name, except, ctx);
            }
          }
        void enumerate_variables(const Abstract_Variable_Callback& callback) const override
          {
            this->m_code_try.enumerate_variables(callback);
            this->m_code_catch.enumerate_variables(callback);
          }
      };

    class Air_return_status_simple : public Air_Node
      {
      private:
        Air_Node::Status m_status;

      public:
        explicit Air_return_status_simple(Air_Node::Status status)
          : m_status(status)
          {
          }

      public:
        Status execute(Executive_Context& /*ctx*/) const override
          {
            return this->m_status;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_throw : public Air_Node
      {
      private:
        Source_Location m_sloc;

      public:
        explicit Air_execute_throw(const Source_Location& sloc)
          : m_sloc(sloc)
          {
          }

      public:
        [[noreturn]] Status execute(Executive_Context& ctx) const override
          {
            // What to throw?
            const auto& value = ctx.stack().get_top_reference().read();
            // Unpack the nested exception, if any.
            Opt<Exception> qnested;
            try {
              // Rethrow the current exception to get its effective type.
              auto eptr = std::current_exception();
              if(eptr) {
                std::rethrow_exception(eptr);
              }
            }
            catch(Exception& except) {
              // Modify the excpetion in place. Don't bother allocating a new one.
              except.push_frame_throw(this->m_sloc, value);
              throw;
            }
            catch(const std::exception& stdex) {
              // Translate the exception.
              qnested.emplace(stdex);
            }
            if(!qnested) {
              // If no nested exception exists, throw a fresh one.
              qnested.emplace(this->m_sloc, value);
            }
            throw *qnested;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_return_by_value : public Air_Node
      {
      private:
        //

      public:
        Air_execute_return_by_value()
          // :
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            // What to return?
            auto& self = ctx.stack().open_top_reference();
            if(self.is_lvalue()) {
              // If the result is not an rvalue and it is not passed by reference, convert it to an rvalue.
              Reference_Root::S_temporary xroot = { self.read() };
              self = rocket::move(xroot);
            }
            return Air_Node::status_return;
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    class Air_execute_assert : public Air_Node
      {
      private:
        Source_Location m_sloc;
        bool m_negative;
        Cow_String m_msg;

      public:
        Air_execute_assert(const Source_Location& sloc, bool negative, const Cow_String& msg)
          : m_sloc(sloc), m_negative(negative), m_msg(msg)
          {
          }

      public:
        Status execute(Executive_Context& ctx) const override
          {
            if(ROCKET_EXPECT(ctx.stack().get_top_reference().read().test() != this->m_negative)) {
              // If the assertion succeeds, there is no effect.
              return Air_Node::status_next;
            }
            // Throw a `Runtime_Error` if the assertion fails.
            Cow_osstream fmtss;
            fmtss.imbue(std::locale::classic());
            fmtss << "Assertion failed at \'" << this->m_sloc << "\'";
            if(this->m_msg.empty()) {
              fmtss << "!";
            }
            else {
              fmtss << ": " << this->m_msg;
            }
            throw_runtime_error(__func__, fmtss.extract_string());
          }
        void enumerate_variables(const Abstract_Variable_Callback& /*callback*/) const override
          {
          }
      };

    }  // namespace

void Statement::generate_code(Air_Queue& code, Cow_Vector<PreHashed_String>* names_opt, Analytic_Context& ctx, const Compiler_Options& options) const
  {
    switch(this->index()) {
    case index_expression:
      {
        const auto& altr = this->m_stor.as<index_expression>();
        if(altr.expr.empty()) {
          // Generate nothing for empty expressions.
          return;
        }
        // Generate preparation code.
        code.push<Air_execute_clear_stack>();
        // Generate inline code for the expression.
        rocket::for_each(altr.expr, [&](const Xprunit& unit) { unit.generate_code(code, options, Xprunit::tco_none, ctx);  });
        return;
      }
    case index_block:
      {
        const auto& altr = this->m_stor.as<index_block>();
        // Generate code for the body.
        auto code_body = do_generate_code_block(options, ctx, altr.body);
        // Encode arguments.
        code.push<Air_execute_block>(rocket::move(code_body));
        return;
      }
    case index_variable:
      {
        const auto& altr = this->m_stor.as<index_variable>();
        // There may be multiple variables.
        for(const auto& pair : altr.vars) {
          // Create a dummy reference for further name lookups.
          do_set_user_declared_reference(names_opt, ctx, "variable placeholder", pair.first, Reference_Root::S_null());
          // Distinguish uninitialized variables from initialized ones.
          if(pair.second.empty()) {
            code.push<Air_define_uninitialized_variable>(altr.sloc, altr.immutable, pair.first);
            continue;
          }
          // A variable becomes visible before its initializer, where it is initialized to `null`.
          code.push<Air_declare_variable_and_clear_stack>(pair.first);
          // Generate inline code for the initializer.
          rocket::for_each(pair.second, [&](const Xprunit& unit) { unit.generate_code(code, options, Xprunit::tco_none, ctx);  });
          // Generate code to initialize the variable.
          code.push<Air_initialize_variable>(altr.sloc, altr.immutable);
        }
        return;
      }
    case index_function:
      {
        const auto& altr = this->m_stor.as<index_function>();
        // Create a dummy reference for further name lookups.
        do_set_user_declared_reference(names_opt, ctx, "function placeholder", altr.name, Reference_Root::S_null());
        // Encode arguments.
        code.push<Air_define_function>(options, altr.sloc, altr.name, altr.params, altr.body);
        return;
      }
    case index_if:
      {
        const auto& altr = this->m_stor.as<index_if>();
        // Generate preparation code.
        code.push<Air_execute_clear_stack>();
        // Generate inline code for the condition expression.
        rocket::for_each(altr.cond, [&](const Xprunit& unit) { unit.generate_code(code, options, Xprunit::tco_none, ctx);  });
        // Generate code for branches.
        auto code_true = do_generate_code_block(options, ctx, altr.branch_true);
        auto code_false = do_generate_code_block(options, ctx, altr.branch_false);
        // Encode arguments.
        code.push<Air_execute_branch>(altr.negative, rocket::move(code_true), rocket::move(code_false));
        return;
      }
    case index_switch:
      {
        const auto& altr = this->m_stor.as<index_switch>();
        // Generate preparation code.
        code.push<Air_execute_clear_stack>();
        // Generate inline code for the condition expression.
        rocket::for_each(altr.ctrl, [&](const Xprunit& unit) { unit.generate_code(code, options, Xprunit::tco_none, ctx);  });
        // Create a fresh context for the `switch` body.
        // Note that all clauses inside a `switch` statement share the same context.
        Analytic_Context ctx_switch(1, ctx);
        // Generate code for all clauses. Names are accumulated.
        Cow_Vector<S_xswitch_clause> clauses;
        Cow_Vector<PreHashed_String> names;
        for(const auto& pair : altr.clauses) {
          auto& clause = clauses.emplace_back();
          clause.code_cond = do_generate_code_expression(options, ctx_switch, pair.first);
          clause.code_clause = do_generate_code_statement_list(&names, ctx_switch, options, pair.second);
          clause.names = names;
        }
        code.push<Air_execute_switch>(rocket::move(clauses));
        return;
      }
    case index_do_while:
      {
        const auto& altr = this->m_stor.as<index_do_while>();
        // Generate code.
        auto code_body = do_generate_code_block(options, ctx, altr.body);
        auto code_cond = do_generate_code_expression(options, ctx, altr.cond);
        // Encode arguments.
        code.push<Air_execute_do_while>(rocket::move(code_body), altr.negative, rocket::move(code_cond));
        return;
      }
    case index_while:
      {
        const auto& altr = this->m_stor.as<index_while>();
        // Generate code.
        auto code_cond = do_generate_code_expression(options, ctx, altr.cond);
        auto code_body = do_generate_code_block(options, ctx, altr.body);
        // Encode arguments.
        code.push<Air_execute_while>(altr.negative, rocket::move(code_cond), rocket::move(code_body));
        return;
      }
    case index_for_each:
      {
        const auto& altr = this->m_stor.as<index_for_each>();
        // Create a fresh context for the `for` loop.
        Analytic_Context ctx_for(1, ctx);
        do_set_user_declared_reference(nullptr, ctx_for, "key placeholder", altr.key_name, Reference_Root::S_null());
        do_set_user_declared_reference(nullptr, ctx_for, "mapped placeholder", altr.mapped_name, Reference_Root::S_null());
        // Generate code.
        auto code_init = do_generate_code_expression(options, ctx_for, altr.init);
        auto code_body = do_generate_code_block(options, ctx_for, altr.body);
        // Encode arguments.
        code.push<Air_execute_for_each>(altr.key_name, altr.mapped_name, rocket::move(code_init), rocket::move(code_body));
        return;
      }
    case index_for:
      {
        const auto& altr = this->m_stor.as<index_for>();
        // Create a fresh context for the `for` loop.
        Analytic_Context ctx_for(1, ctx);
        // Generate code.
        auto code_init = do_generate_code_statement_list(nullptr, ctx_for, options, altr.init);
        auto code_cond = do_generate_code_expression(options, ctx_for, altr.cond);
        auto code_step = do_generate_code_expression(options, ctx_for, altr.step);
        auto code_body = do_generate_code_block(options, ctx_for, altr.body);
        // Encode arguments.
        code.push<Air_execute_for>(rocket::move(code_init), rocket::move(code_cond), rocket::move(code_step), rocket::move(code_body));
        return;
      }
    case index_try:
      {
        const auto& altr = this->m_stor.as<index_try>();
        // Create a fresh context for the `catch` clause.
        Analytic_Context ctx_catch(1, ctx);
        do_set_user_declared_reference(nullptr, ctx_catch, "exception placeholder", altr.except_name, Reference_Root::S_null());
        ctx_catch.open_named_reference(rocket::sref("__backtrace")) /*= Reference_Root::S_null()*/;
        // Generate code.
        auto code_try = do_generate_code_block(options, ctx, altr.body_try);
        auto code_catch = do_generate_code_statement_list(nullptr, ctx_catch, options, altr.body_catch);
        // Encode arguments.
        code.push<Air_execute_try>(rocket::move(code_try), altr.sloc, altr.except_name, rocket::move(code_catch));
        return;
      }
    case index_break:
      {
        const auto& altr = this->m_stor.as<index_break>();
        switch(altr.target) {
        case Statement::target_unspec:
          {
            code.push<Air_return_status_simple>(Air_Node::status_break_unspec);
            break;
          }
        case Statement::target_switch:
          {
            code.push<Air_return_status_simple>(Air_Node::status_break_switch);
            break;
          }
        case Statement::target_while:
          {
            code.push<Air_return_status_simple>(Air_Node::status_break_while);
            break;
          }
        case Statement::target_for:
          {
            code.push<Air_return_status_simple>(Air_Node::status_break_for);
            break;
          }
        default:
          ASTERIA_TERMINATE("An unknown target scope type `", altr.target, "` has been encountered.");
        }
        return;
      }
    case index_continue:
      {
        const auto& altr = this->m_stor.as<index_continue>();
        switch(altr.target) {
        case Statement::target_unspec:
          {
            code.push<Air_return_status_simple>(Air_Node::status_continue_unspec);
            break;
          }
        case Statement::target_switch:
          {
            ASTERIA_TERMINATE("`target_switch` is not allowed to follow `continue`.");
          }
        case Statement::target_while:
          {
            code.push<Air_return_status_simple>(Air_Node::status_continue_while);
            break;
          }
        case Statement::target_for:
          {
            code.push<Air_return_status_simple>(Air_Node::status_continue_for);
            break;
          }
        default:
          ASTERIA_TERMINATE("An unknown target scope type `", altr.target, "` has been encountered.");
        }
        return;
      }
    case index_throw:
      {
        const auto& altr = this->m_stor.as<index_throw>();
        // Generate preparation code.
        code.push<Air_execute_clear_stack>();
        // Generate inline code for the operand.
        rocket::for_each(altr.expr, [&](const Xprunit& unit) { unit.generate_code(code, options, Xprunit::tco_none, ctx);  });
        // Encode arguments.
        code.push<Air_execute_throw>(altr.sloc);
        return;
      }
    case index_return:
      {
        const auto& altr = this->m_stor.as<index_return>();
        // Generate preparation code.
        code.push<Air_execute_clear_stack>();
        if(altr.by_ref) {
          // Generate inline code for the operand. Only the last operator can be TCO'd.
          rocket::for_each(altr.expr, [&](const Xprunit& unit) { unit.generate_code(code, options,
                                                                                    rocket::same(unit, altr.expr.back()) ? Xprunit::tco_by_ref
                                                                                                                         : Xprunit::tco_none, ctx);  });
          // Return the reference as is.
          code.push<Air_return_status_simple>(Air_Node::status_return);
        }
        else {
          // Generate inline code for the operand. Only the last operator can be TCO'd.
          rocket::for_each(altr.expr, [&](const Xprunit& unit) { unit.generate_code(code, options,
                                                                                    rocket::same(unit, altr.expr.back()) ? Xprunit::tco_by_value
                                                                                                                         : Xprunit::tco_none, ctx);  });
          // Return the reference as is.
          code.push<Air_execute_return_by_value>();
        }
        return;
      }
    case index_assert:
      {
        const auto& altr = this->m_stor.as<index_assert>();
        // Generate preparation code.
        code.push<Air_execute_clear_stack>();
        // Generate inline code for the operand.
        rocket::for_each(altr.expr, [&](const Xprunit& unit) { unit.generate_code(code, options, Xprunit::tco_none, ctx);  });
        // Encode arguments.
        code.push<Air_execute_assert>(altr.sloc, altr.negative, altr.msg);
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown statement type enumeration `", this->index(), "` has been encountered.");
    }
  }

}  // namespace Asteria
