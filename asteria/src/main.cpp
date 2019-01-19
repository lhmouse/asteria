// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "compiler/simple_source_file.hpp"
#include "runtime/global_context.hpp"
#include "runtime/traceable_exception.hpp"
#include <iostream>

using namespace Asteria;

int main(int argc, char **argv)
  {
    Cow_Vector<Reference> args;
    for(int i = 0; i < argc; ++i) {
      D_string arg;
      if(argv[i]) {
        arg += argv[i];
      }
      Reference_Root::S_constant ref_c = { std::move(arg) };
      args.emplace_back(std::move(ref_c));
    }
    std::cerr << "# Input your program:" << std::endl
              << "---" << std::endl;
    try {
      Global_Context global;
      Simple_Source_File code(std::cin, std::ref("<stdin>"));
      auto res = code.execute(global, std::move(args));
      std::cerr << std::endl
                << "---" << std::endl;
      std::cout << res.read() << std::endl;
    } catch(Traceable_Exception &e) {
      std::cerr << std::endl
                << "---" << std::endl
                << "# Caught `Traceable_Exception`:" << std::endl
                << e.get_value() << std::endl;
    } catch(std::exception &e) {
      std::cerr << std::endl
                << "---" << std::endl
                << "# Caught `std::exception`:" << std::endl
                << e.what() << std::endl;
    }
  }
