// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
#include "../asteria/source_location.hpp"
#include "../asteria/runtime/abstract_hooks.hpp"
#include "../asteria/runtime/runtime_error.hpp"
using namespace ::asteria;

int main()
  {
    struct Test_Hooks : Abstract_Hooks
      {
        ::rocket::tinyfmt_str fmt;

        virtual
        void
        on_function_call(const Source_Location& sloc, const cow_function&) final
          {
            this->fmt << "call " << sloc.line() << "; ";
          }

        virtual
        void
        on_function_return(const Source_Location& sloc, const cow_function&, const Reference&) final
          {
            this->fmt << "return " << sloc.line() << "; ";
          }
      };

    const auto hooks = ::rocket::make_refcnt<Test_Hooks>();
    Simple_Script code;
    code.global().set_hooks(hooks);

    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        func no_ptc_throw() {
          return 1;
        }

        func no_ptc_three() {
          return no_ptc_throw() + 1;
        }

        func no_ptc_two() {
          return ref no_ptc_three() + 1;
        }

        func no_ptc_one() {
          return no_ptc_two() + 1;
        }

        no_ptc_one();

///////////////////////////////////////////////////////////////////////////////
      )__"));
    hooks->fmt.clear_string();
    code.execute();
    ::fprintf(stderr, "no_ptc ===> %s\n", hooks->fmt.c_str());
    ASTERIA_TEST_CHECK(hooks->fmt.get_string() ==
        "call 56; call 53; call 49; call 45; return 45; return 49; return 53; return 56; ");

    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        func ptc_throw() {
          return 1;
        }

        func ptc_three() {
          return ptc_throw();
        }

        func ptc_two() {
          return ref ptc_three();
        }

        func ptc_one() {
          return ptc_two();
        }

        ptc_one();

///////////////////////////////////////////////////////////////////////////////
      )__"));
    hooks->fmt.clear_string();
    code.execute();
    ::fprintf(stderr, "ptc ===> %s\n", hooks->fmt.c_str());
    ASTERIA_TEST_CHECK(hooks->fmt.get_string() ==
        "call 86; call 83; call 79; call 75; return 75; return 79; return 83; return 86; ");
  }
