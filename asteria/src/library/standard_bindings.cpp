// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "standard_bindings.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

namespace Asteria {

D_boolean std_debug_print(const Cow_Vector<Value> &values)
  {
    rocket::insertable_ostream mos;
    rocket::for_each(values, [&](const Value &value) { value.print(mos);  });
    return write_log_to_stderr(__FILE__, __LINE__, mos.extract_string());
  }

D_boolean std_debug_var_dump(const Value &value, std::size_t indent_increment)
  {
    rocket::insertable_ostream mos;
    value.var_dump(mos, indent_increment);
    return write_log_to_stderr(__FILE__, __LINE__, mos.extract_string());
  }

    namespace {

    template<typename AccessorT> inline D_object & do_open_object(D_object &parent, const PreHashed_String &name, AccessorT &&accessor)
      {
        auto &child = parent.try_emplace(name, D_object()).first->second.check<D_object>();
        std::forward<AccessorT>(accessor)(child);
        return child;
      }

    }

D_object create_standard_bindings(const Rcptr<Generational_Collector> &coll)
  {
    D_object root;
    // `std.debug`
    do_open_object(root, rocket::sref("debug"),
      [](D_object &debug)
        {
          //===================================================================
          // `std.debug.print()`
          //===================================================================
          debug.try_emplace(rocket::sref("print"),
            D_function(make_simple_binding(
              // Description
              rocket::sref("std.debug.print([args...]):\n"
                           "* Prints all `args` to the standard error stream, followed by a line break.\n"
                           "* Returns `true` if the operation succeeds."),
              // Definition
              [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
                {
                  Argument_Reader reader(rocket::sref("std.debug.print"), args);
                  // Parse variadic arguments.
                  if(!reader.start()) {
                    reader.throw_no_matching_function_call();
                  }
                  Cow_Vector<Value> values;
                  for(Value value; reader.opt(value); ) {
                    values.emplace_back(rocket::move(value));
                  }
                  // Call the binding function.
                  auto succ = std_debug_print(values);
                  // Forward the result.
                  Reference_Root::S_temporary ref_c = { D_boolean(succ) };
                  return ref_c;
                },
              // Opaque parameters
              { }
            )));
          //===================================================================
          // `std.debug.var_dump()`
          //===================================================================
          debug.try_emplace(rocket::sref("var_dump"),
            D_function(make_simple_binding(
              // Description
              rocket::sref("std.debug.var_dump(value[, indent_increment]):\n"
                           "* Prints the value to the standard error stream with detailed information.\n"
                           "  `indent_increment` is the number of spaces used as a single level of indent.\n"
                           "  Its value is clamped within the range [0, 10] inclusive. If it is set to `0`,\n"
                           "  no line break is inserted and the output is not indented. The default value is `2`.\n"
                           "* Returns `true` if the operation succeeds."),
              // Definition
              [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
                {
                  Argument_Reader reader(rocket::sref("std.debug.var_dump"), args);
                  // Parse arguments.
                  Value value;
                  D_integer indent_increment;
                  if(!reader.start().opt(value).opt(indent_increment, 2).finish()) {
                    reader.throw_no_matching_function_call();
                  }
                  indent_increment = rocket::mclamp(0, indent_increment, 2);
                  // Call the binding function.
                  auto succ = std_debug_var_dump(value, static_cast<std::size_t>(indent_increment));
                  // Forward the result.
                  Reference_Root::S_temporary ref_c = { D_boolean(succ) };
                  return ref_c;
                },
              // Opaque parameters
              { }
            )));
        }
      );
    return root;
  }

}  // namespace Asteria
