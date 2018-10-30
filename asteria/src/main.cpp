// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "simple_source_file.hpp"
#include "global_context.hpp"
#include <iostream>
#include <sstream>
#include <chrono>

using namespace Asteria;

int main()
  {
    std::istringstream iss(R"__(
      func fib(n) {
        return n <= 1 ? 1 : fib(n-1) + fib(n-2);
      }
      return fib(20);
    )__");
    Simple_source_file code(iss, rocket::cow_string::shallow("my_file"));
    Global_context global;
    const auto t1 = std::chrono::high_resolution_clock::now();
    auto res = code.execute(global, { });
    const auto t2 = std::chrono::high_resolution_clock::now();
    std::cout <<"result = " <<res.read() <<std::endl
              <<"time   = " <<std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() <<" ms" <<std::endl;
  }
