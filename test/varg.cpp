// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/compiler/simple_source_file.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    std::istringstream iss(R"__(
      func binary(a, b) {
        var narg = __varg();
        var arg = __varg(a);
        return [ narg, arg ];
      }
      return [
        binary(1),           // [ 0, null ]
        binary(1,2),         // [ 0, null ]
        binary(1,2,3),       // [ 1, null ]
        binary(1,2,3,4),     // [ 2, 4 ]
        binary(1,2,3,4,5),   // [ 3, 4 ]
      ];
    )__");
    Simple_source_file code(iss, std::ref("my_file"));
    Global_context global;
    auto res = code.execute(global, { });
    const auto &array = res.read().check<D_array>();
    ASTERIA_TEST_CHECK(array.size() == 5);
    ASTERIA_TEST_CHECK(array.at(0).check<D_array>().at(0).check<D_integer>() == 0);
    ASTERIA_TEST_CHECK(array.at(0).check<D_array>().at(1).check<D_null>() == nullptr);
    ASTERIA_TEST_CHECK(array.at(1).check<D_array>().at(0).check<D_integer>() == 0);
    ASTERIA_TEST_CHECK(array.at(1).check<D_array>().at(1).check<D_null>() == nullptr);
    ASTERIA_TEST_CHECK(array.at(2).check<D_array>().at(0).check<D_integer>() == 1);
    ASTERIA_TEST_CHECK(array.at(2).check<D_array>().at(1).check<D_null>() == nullptr);
    ASTERIA_TEST_CHECK(array.at(3).check<D_array>().at(0).check<D_integer>() == 2);
    ASTERIA_TEST_CHECK(array.at(3).check<D_array>().at(1).check<D_integer>() == 4);
    ASTERIA_TEST_CHECK(array.at(4).check<D_array>().at(0).check<D_integer>() == 3);
    ASTERIA_TEST_CHECK(array.at(4).check<D_array>().at(1).check<D_integer>() == 4);
  }
