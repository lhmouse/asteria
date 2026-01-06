// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
#include "../asteria/source_location.hpp"
#include "../asteria/runtime/abstract_hooks.hpp"
#include "../asteria/runtime/runtime_error.hpp"
using namespace ::asteria;

struct Test_Hooks : Abstract_Hooks
  {
    ::rocket::tinyfmt_str fmt;

    void
    on_call(const Source_Location& sloc, const cow_function& /*target*/) override
      {
        this->fmt << "call " << sloc.line() << "; ";
      }

    void
    on_return(const Source_Location& sloc, PTC_Aware /*ptc*/) override
      {
        this->fmt << "return " << sloc.line() << "; ";
      }
  };

int main()
  {
    const auto hooks = ::rocket::make_refcnt<Test_Hooks>();
    Simple_Script code;
    code.mut_global().set_hooks(hooks);

    code.reload_string(
      &__FILE__, __LINE__, &R"__(
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
      )__");
    hooks->fmt.clear_string();
    code.execute();
    ::fprintf(stderr, "no_ptc ===> %s\n", hooks->fmt.c_str());
    ASTERIA_TEST_CHECK(hooks->fmt.get_string() ==
        "call 54; call 51; call 47; call 43; return 39; return 43; return 47; return 51; return 54; ");

    code.reload_string(
      &__FILE__, __LINE__, &R"__(
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
      )__");
    hooks->fmt.clear_string();
    code.execute();
    ::fprintf(stderr, "ptc ===> %s\n", hooks->fmt.c_str());
    ASTERIA_TEST_CHECK(hooks->fmt.get_string() ==
        "call 84; call 81; call 77; call 73; return 69; return 73; return 77; return 81; return 84; ");
  }
