// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "air_node.hpp"
#include "../syntax/statement.hpp"
#include "../utilities.hpp"

namespace Asteria {

Air_Node::Status Air_Node::execute(Executive_Context& ctx) const
  {
    return (*(this->m_fptr))(ctx, this->m_params);
  }

    namespace {

    class Variable_Enumerator
      {
      private:
        std::reference_wrapper<const Abstract_Variable_Callback> m_callback;

      public:
        explicit Variable_Enumerator(const Abstract_Variable_Callback& callback) noexcept
          : m_callback(callback)
          {
          }

      public:
        void operator()(const std::int64_t& /*altr*/) const
          {
            // There is nothing to do.
          }
        void operator()(const PreHashed_String& /*altr*/) const
          {
            // There is nothing to do.
          }
        void operator()(const Cow_Vector<PreHashed_String>& /*altr*/) const
          {
            // There is nothing to do.
          }
        void operator()(const Source_Location& /*altr*/) const
          {
            // There is nothing to do.
          }
        void operator()(const Cow_Vector<Statement>& /*altr*/) const
          {
            // There is nothing to do.
          }
        void operator()(const Value& altr) const
          {
            altr.enumerate_variables(this->m_callback);
          }
        void operator()(const Reference& altr) const
          {
            altr.enumerate_variables(this->m_callback);
          }
        void operator()(const Cow_Vector<Air_Node>& altr) const
          {
            rocket::for_each(altr, [&](const Air_Node& node) { node.enumerate_variables(this->m_callback);  });
          }
        void operator()(const Compiler_Options& /*altr*/) const
          {
            // There is nothing to do.
          }
      };

    }

void Air_Node::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    rocket::for_each(this->m_params, [&](const Parameter& param) { param.visit(Variable_Enumerator(callback));  });
  }

}  // namespace Asteria
