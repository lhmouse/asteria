// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      &__FILE__, __LINE__, &R"__(
///////////////////////////////////////////////////////////////////////////////

      ////////////////////////////////////////////////////
      // for
      ////////////////////////////////////////////////////

      var test = 1;
      for(var i = 0; i != 10; ++i)
        continue;
      __ifcomplete
        test = 2;
      assert test == 2;

      var test = 1;
      for(var i = 0; i != 10; ++i)
        continue for;
      __ifcomplete
        test = 2;
      assert test == 2;

      var test = 1;
      for(var i = 0; i != 10; ++i)
        break;
      __ifcomplete
        test = 2;
      assert test == 1;

      var test = 1;
      for(var i = 0; i != 10; ++i)
        break for;
      __ifcomplete
        test = 2;
      assert test == 1;

      var test = 100;
      for( ; false; )
        assert false;
      __ifcomplete
        test = 300;
      assert test == 300;

      // scope
      var sum = 0;
      var t = 100;
      for(var t = 1; t <= 10; ++t)
        sum += t;  // sum = 55
      __ifcomplete
        sum += t;   // sum += 100
      assert sum == 155;

      // scope
      var sum = 0;
      var t = 100;
      for(var t = 1; t <= 10; ++t) {
        if(t > 5) break;
        sum += t;  // sum = 15
      }
      __ifcomplete
        sum += t;   // skipped
      assert sum == 15;

      ////////////////////////////////////////////////////
      // for each
      ////////////////////////////////////////////////////

      for(each root -> [
                         [1,2,3,4,5,6,7,8,9,10];
                         {a:1,b:2,c:3,d:4,e:5,f:6,g:7,h:8,i:9,j:10};
                       ])
      {
        var test = 1;
        for(each v -> root)
          continue;
        __ifcomplete
          test = 2;
        assert test == 2;

        var test = 1;
        for(each v -> root)
          continue for;
        __ifcomplete
          test = 2;
        assert test == 2;

        var test = 1;
        for(each v -> root)
          break;
        __ifcomplete
          test = 2;
        assert test == 1;

        var test = 1;
        for(each v -> root)
          break for;
        __ifcomplete
          test = 2;
        assert test == 1;

        var test = 100;
        for(each v -> [])
          assert false;
        __ifcomplete
          test = 300;
        assert test == 300;

        if(typeof root == "array") {
          // scope
          var sum = 0;
          var t = 100;
          for(each t -> root)
            sum += t;  // sum = 55
          __ifcomplete
            sum += t;   // sum += 100
          assert sum == 155;

          // scope
          var sum = 0;
          var t = 100;
          for(each t -> root) {
            if(t > 5) break;
            sum += t;  // sum = 15
          }
          __ifcomplete
            sum += t;   // skipped
          assert sum == 15;
        }
      }

      ////////////////////////////////////////////////////
      // while
      ////////////////////////////////////////////////////

      var sum = 0;
      var t = 1;
      while(++t <= 10)
        sum += t;  // sum = 54
      __ifcomplete {
        sum *= 2;
      }
      assert sum == 108;

      var sum = 0;
      var t = 1;
      while(++t <= 10) {
        if(t > 5) break;
        sum += t;   // sum = 14
      }
      __ifcomplete {
        sum *= 2;  // skipped
      }
      assert sum == 14;

      var t = 100;
      while(false)
        assert false;
      __ifcomplete
        t = 200;
      assert t == 200;

      ////////////////////////////////////////////////////
      // do...while
      ////////////////////////////////////////////////////

      var sum = 0;
      var t = 1;
      do
        sum += t;  // sum = 55
      while(++t <= 10) __ifcomplete {
        sum *= 2;
      }
      assert sum == 110;

      var sum = 0;
      var t = 1;
      do {
        if(t > 5) break;
        sum += t;   // sum = 15
      }
      while(++t <= 10) __ifcomplete {
        sum *= 2;  // skipped
      }
      assert sum == 15;

      var t = 100;
      do
        t = 300;
      while(false) __ifcomplete {
        assert t == 300;
        t = 200;
      }
      assert t == 200;

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
