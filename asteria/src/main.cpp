// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "runtime/simple_script.hpp"
#include "runtime/global_context.hpp"
#include "runtime/exception.hpp"
#include "library/bindings_chrono.hpp"
#include "../rocket/cow_istringstream.hpp"
#include <iostream>
#include <iomanip>

using namespace Asteria;

int main(int argc, char** argv)
  try {
    cow_vector<Value> vargs;
    for(int i = 0; i < argc; ++i) {
      if(argv[i])
        vargs.emplace_back(G_string(argv[i]));
      else
        vargs.emplace_back(G_null());
    }
    // prepare test code.
#if !defined(__OPTIMIZE__) || (__OPTIMIZE__ == 0)
    std::cerr << "# Input your program:" << std::endl
              << "---" << std::endl;
    Simple_Script code(std::cin, rocket::sref("<stdin>"));
#else
    static constexpr char src[] =
      R"_____(
        func fib(n) {
          var r = n;
          if!(n <= 1) {
            r = fib(n-1) + fib(n-2);
          }
          return& r;
        }
        return fib(30);
      )_____";
    cow_isstream iss(rocket::sref(src));
    std::cerr << "# Source code:" << std::endl
              << "---" << std::endl
              << src << std::endl
              << "---" << std::endl;;
    Simple_Script code(iss, rocket::sref("my_file"));
#endif
    std::cerr << std::endl
              << "---" << std::endl
              << "# Running..." << std::endl;
    // run it and measure the time.
    Global_Context global;
    auto t1 = std_chrono_hires_now();
    auto res = code.execute(global, rocket::move(vargs));
    auto t2 = std_chrono_hires_now();
    // print the time elapsed and the result.
    std::cerr << std::endl
              << "---" << std::endl
              << "# Finished in " << std::fixed << std::setprecision(3) << (t2 - t1) << " ms:" << std::endl
              << "---" << std::endl;
    std::cout << res.read() << std::endl;
    // finish.
    return 0;
  }
  catch(Exception& e) {
    // print the exception.
    std::cerr << std::endl
              << "---" << std::endl
              << "# Caught `Exception`:" << std::endl
              << e << std::endl;
    for(size_t i = e.count_frames() - 1; i != SIZE_MAX; --i) {
      const auto& f = e.get_frame(i);
      std::cerr << "  thrown from \'" << f.sloc() << "\' <" << f.what_type() << ">: " << f.value() << std::endl;
    }
    return 1;
  }
  catch(std::exception& e) {
    // print the exception.
    std::cerr << std::endl
              << "---" << std::endl
              << "# Caught `std::exception`:" << std::endl
              << e.what() << std::endl;
    return 1;
  }
