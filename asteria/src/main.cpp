// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "compiler/simple_source_file.hpp"
#include "runtime/global_context.hpp"
#include "runtime/traceable_exception.hpp"
#include "rocket/insertable_istream.hpp"
#include <iostream>
#include <chrono>

using namespace Asteria;

int main()
  try {
    // prepare test code.
    static constexpr char src[] = R"_____(
      func fib(n) {
        if(n <= 1) {
          return& n;
        }
        return& fib(n-1) + fib(n-2);
      }
      return fib(30);
    )_____";
    rocket::insertable_istream iss(rocket::sref(src));
    std::cerr << "Source code:\n---\n" << src << "\n---" << std::endl;;
    // parse it.
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    // run it and measure the time.
    const auto t1 = std::chrono::high_resolution_clock::now();
    auto res = code.execute(global, { });
    const auto t2 = std::chrono::high_resolution_clock::now();
    // print the time elasped and the result.
    std::cerr << "Finished in " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms:\n---" << std::endl;
    std::cout << res.read() << std::endl;
    // finish.
    return 0;
  } catch(Traceable_Exception &e) {
    // print the exception.
    std::cerr << "Caught `Asteria::Traceable_Exception`:\n---" << std::endl;
    std::cerr << e.get_value() << std::endl;
    return 1;
  } catch(std::exception &e) {
    // print the exception.
    std::cerr << "Caught `std::exception`:\n---" << std::endl;
    std::cerr << e.what() << std::endl;
    return 1;
  }
