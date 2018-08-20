// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "context.hpp"
#include "abstract_function.hpp"
#include "utilities.hpp"

namespace Asteria
{

Context::~Context() = default;

namespace
  {
    class Varg_getter
      : public Abstract_function
      {
      private:
        String m_file;
        Unsigned m_line;
        Vector<Reference> m_vargs;

      public:
        Varg_getter(String file, Unsigned line, Vector<Reference> &&vargs)
          : m_file(std::move(file)), m_line(line), m_vargs(std::move(vargs))
          {
          }

      public:
        String describe() const override
          {
            return ASTERIA_FORMAT_STRING("variadic argument getter at \'", m_file, ':', m_line, "\'");
          }
        Reference invoke(Reference /*self*/, Vector<Reference> args) const override
          {
            switch(args.size())
              {
              case 0:
                {
                  // When no argument is given, return the number of variadic arguments.
                  return reference_constant(static_cast<D_integer>(m_vargs.size()));
                }
              case 1:
                {
                  const auto iref = read_reference(args.at(0));
                  if(iref.which() != Value::type_integer) {
                    ASTERIA_THROW_RUNTIME_ERROR("The argument passed to `__varg` must be of type `integer`.");
                  }
                  const auto index = static_cast<Signed>(iref.as<D_integer>());
                  auto rindex = index;
                  if(rindex < 0) {
                    // Wrap negative indices.
                    rindex += static_cast<Signed>(m_vargs.size());
                  }
                  if(rindex < 0) {
                    ASTERIA_DEBUG_LOG("Variadic argument index fell before the front: index = ", index, ", vargs = ", m_vargs.size());
                    return Reference();
                  }
                  if(rindex >= static_cast<Signed>(m_vargs.size())){
                    ASTERIA_DEBUG_LOG("Variadic argument index fell after the back: index = ", index, ", vargs = ", m_vargs.size());
                    return Reference();
                  }
                  return m_vargs.at(static_cast<std::size_t>(rindex));
                }
              default:
                ASTERIA_THROW_RUNTIME_ERROR("`__varg` accepts no more than one argument.");
              }
          }
      };

    template<typename GeneratorT>
      void do_set_reference(Spref<Context> ctx_out, const String &name, GeneratorT &&generator)
        {
          Reference ref;
          if(ctx_out->is_feigned() == false) {
            ref = std::forward<GeneratorT>(generator)();
          }
          ctx_out->set_named_reference(name, std::move(ref));
        }
  }

void initialize_function_context(Spref<Context> ctx_out, const Vector<String> &params, const String &file, Unsigned line, Reference self, Vector<Reference> args)
  {
    // Set up parameters.
    for(const auto &name : params) {
      Reference ref;
      if(args.empty() == false) {
        // Shift an argument.
        ref = std::move(args.mut_front());
        args.erase(args.begin());
      }
      if(name.empty() == false) {
        if((name == "this") || name.starts_with("__")) {
          ASTERIA_THROW_RUNTIME_ERROR("The parameter name `", name, "` is reserved.");
        }
        do_set_reference(ctx_out, name, [&] { return std::move(ref); });
      }
    }
    // Set up system variables.
    do_set_reference(ctx_out, String::shallow("__file"), [&] { return reference_constant(D_string(file)); });
    do_set_reference(ctx_out, String::shallow("__line"), [&] { return reference_constant(D_integer(line)); });
    do_set_reference(ctx_out, String::shallow("this"), [&] { return std::move(self); });
    do_set_reference(ctx_out, String::shallow("__varg"), [&] { return reference_constant(D_function(allocate<Varg_getter>(file, line, std::move(args)))); });
  }

}
