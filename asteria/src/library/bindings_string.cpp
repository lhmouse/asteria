// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_string.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

namespace Asteria {

D_object create_bindings_string()
  {
    D_object root;
    //===================================================================
    // `std.root.print()`
    //===================================================================
//    root.try_emplace(rocket::sref("print"),
//      D_function(make_simple_binding(
//        // Description
//        rocket::sref("`std.root.print([args...])`"
//                     "\n  * Prints all `args` to the standard error stream, separated by   "
//                     "\n    spaces. A line break is appended to terminate the line.        "
//                     "\n  * Returns `true` if the operation succeeds.                      "),
//        // Definition
//        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
//          {
//            Argument_Reader reader(rocket::sref("std.root.print"), args);
//            // Parse variadic arguments.
//            Cow_Vector<Value> values(args.size());
//            reader.start();
//            std::for_each(values.mut_begin(), values.mut_end(), [&](Value &value) { reader.opt(value);  });
//            if(reader.finish()) {
//              // Call the binding function.
//              if(!std_root_print(values)) {
//                // Fail.
//                return Reference_Root::S_null();
//              }
//              // Return `true`.
//              Reference_Root::S_temporary ref_c = { D_boolean(true) };
//              return rocket::move(ref_c);
//            }
//            // Fail.
//            reader.throw_no_matching_function_call();
//          },
//        // Opaque parameters
//        { }
//      )));
    //===================================================================
    // End of `std.root`
    //===================================================================
    return root;
  }

}  // namespace Asteria
