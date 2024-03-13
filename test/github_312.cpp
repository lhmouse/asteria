// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      &__FILE__, __LINE__, &R"__(
///////////////////////////////////////////////////////////////////////////////

      func test_switch_1(x) {
        switch(x) {
          default:       return 'def';
          each [0,5):    return '[0,5)';
          case 4:        return '4';
          case 8:        return '8';
          each [5,9):    return '[5,9)';
         }
      }

      assert test_switch_1(0) == '[0,5)';
      assert test_switch_1(4) == '[0,5)';
      assert test_switch_1(5) == '[5,9)';
      assert test_switch_1(8) == '8';
      assert test_switch_1(9) == 'def';

      func test_switch_2(x) {
        switch(x) {
          each [0,1):    return '[0,1)';
          each (2,3):    return '(2,3)';
          each (4,5]:    return '(4,5]';
          each [6,7]:    return '[6,7]';
         }
      }

      assert test_switch_2(0) == '[0,1)';
      assert __isvoid test_switch_2(1);
      assert __isvoid test_switch_2(2);
      assert __isvoid test_switch_2(3);
      assert __isvoid test_switch_2(4);
      assert test_switch_2(5) == '(4,5]';
      assert test_switch_2(6) == '[6,7]';
      assert test_switch_2(7) == '[6,7]';

      func test_switch_str(x) {
        switch(x) {
          each ['abc','abx'):   return 1;
          each ('ccc','ccx']:   return 2;
        }
      }

      assert test_switch_str('abc') == 1;
      assert test_switch_str('abe') == 1;
      assert __isvoid test_switch_str('abx');
      assert __isvoid test_switch_str('aby');
      assert __isvoid test_switch_str('ccc');
      assert test_switch_str('ccd') == 2;
      assert test_switch_str('ccx') == 2;
      assert __isvoid test_switch_str('ccy');

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
