// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/parser.hpp"
#include "../src/token_stream.hpp"
#include "../src/executive_context.hpp"
#include "../src/reference.hpp"
#include "../src/exception.hpp"
#include "../src/garbage_collector.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    std::istringstream iss(R"__(
      var one = 1;
      func fib(n) {
        return n <= one ? one : fib(n-1) + fib(n-2);
      }
      return fib(10) + one;
    )__");
    Token_stream tis;
    ASTERIA_TEST_CHECK(tis.load(iss, String::shallow("my_file")));
    Parser pr;
    ASTERIA_TEST_CHECK(pr.load(tis));
    const auto code = pr.extract_document();

    Garbage_collector gc;
    {
      Reference result;
      Executive_context ctx;
      code.execute_in_place(result, ctx, nullptr);
      auto q = ctx.get_named_reference_opt(String::shallow("fib"));
      ASTERIA_TEST_CHECK(q);
      q->collect_variables(
        [](void *param, const Rcptr<Variable> &var)
          { return static_cast<Garbage_collector *>(param)->track_variable(var), false; },
        &gc);
    }
    gc.collect();
  }
