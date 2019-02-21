// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "compiler/simple_source_file.hpp"
#include "runtime/global_context.hpp"
#include "runtime/traceable_exception.hpp"
#include "rocket/insertable_istream.hpp"
#include <iostream>
#include <chrono>

using namespace Asteria;

int main(int argc, char **argv)
  try {
    CoW_Vector<Reference> args;
    for(int i = 0; i < argc; ++i) {
      D_string arg;
      if(argv[i]) {
        arg += argv[i];
      }
      Reference_Root::S_constant ref_c = { std::move(arg) };
      args.emplace_back(std::move(ref_c));
    }
    // prepare test code.
#if 0
    std::cerr << "# Input your program:" << std::endl
              << "---" << std::endl;
    Simple_Source_File code(std::cin, rocket::sref("<stdin>"));
#else
    static constexpr char src[] = R"_____(
      func fib(n) {
        var r = n;
        if!(n <= 1) {
          r = fib(n-1) + fib(n-2);
        }
        return& r;
      }
      return fib(30);
    )_____";
    rocket::insertable_istream iss(rocket::sref(src));
    std::cerr << "Source code:" << std::endl
              << "---" << std::endl
              << src << std::endl
              << "---" << std::endl;;
    Simple_Source_File code(iss, rocket::sref("my_file"));
#endif
    Global_Context global;
    // run it and measure the time.
    auto t1 = std::chrono::high_resolution_clock::now();
    auto res = code.execute(global, std::move(args));
    auto t2 = std::chrono::high_resolution_clock::now();
    // print the time elasped and the result.
    std::cerr << std::endl
              << "---" << std::endl
              << "Finished in " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms:" << std::endl
              << "---" << std::endl;
    std::cout << res.read() << std::endl;
    // finish.
    return 0;
  } catch(Traceable_Exception &e) {
    // print the exception.
    std::cerr << std::endl
              << "---" << std::endl
              << "# Caught `Traceable_Exception`:" << std::endl
              << e.get_value() << std::endl;
    for(std::size_t i = 0; i < e.get_frame_count(); ++i) {
      std::cerr << "[thrown from `" << e.get_frame(i).function_signature() << "` at '" << e.get_frame(i).source_location() << "']" << std::endl;
    }
    return 1;
  } catch(std::exception &e) {
    // print the exception.
    std::cerr << std::endl
              << "---" << std::endl
              << "# Caught `std::exception`:" << std::endl
              << e.what() << std::endl;
    return 1;
  }
