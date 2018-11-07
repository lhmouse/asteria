// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "simple_source_file.hpp"
#include "global_context.hpp"
#include <iostream>
#include <sstream>
#include <chrono>

using namespace Asteria;

int main()
  {
    std::istringstream iss(R"__(
      return "hello" >> 4;
    )__");
    Simple_source_file code(iss, rocket::cow_string::shallow("my_file"));
    Global_context global;
    auto res = code.execute(global, { });
    std::cout <<"result = " <<res.read() <<std::endl;
  }
