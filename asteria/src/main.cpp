// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "compiler/simple_source_file.hpp"
#include "runtime/global_context.hpp"
#include "runtime/exception.hpp"
#include <iostream>
#include <sstream>
#include <chrono>

using namespace Asteria;

int main()
  {
    std::istringstream iss(R"__(
      func fib(n) {
        var r = n;
        if!(n <= 1) {
          r = fib(n-1) + fib(n-2);
        }
        return& r;
      }
      return fib(30);
    )__");
    Simple_Source_File code(iss, std::ref("my_file"));
    Global_Context global;
    try {
      const auto t1 = std::chrono::high_resolution_clock::now();
      auto res = code.execute(global, { });
      const auto t2 = std::chrono::high_resolution_clock::now();
      std::cout << "result = " << res.read() << std::endl
                << "time   = " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms" << std::endl;
    } catch(Exception &e) {
      std::cout << "caught `Exception`:" << std::endl
                << e.get_value() << std::endl;
    } catch(std::exception &e) {
      std::cout << "caught `std::exception`:" << std::endl
                << e.what() << std::endl;
    }
  }
