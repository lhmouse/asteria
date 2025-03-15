// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      &__FILE__, __LINE__, &R"__(
///////////////////////////////////////////////////////////////////////////////

      for(each name: obj -> [1,2,3]) {   if(1) continue; }

      var count, sum;

      count = 0;
      sum = 0;
      while(count != 10) {
        count ++;
        if(count % 2 == 0)
          continue;
        else if(count == 7)
          break;
        sum += count;
      }
      assert count == 7;
      assert sum == 1 + 3 + 5;

      count = 0;
      sum = 0;
      while(count != 10) {
        count ++;
        sum += count;
        continue;
      }
      assert count == 10;
      assert sum == 55;

      count = 0;
      sum = 0;
      do {
        count ++;
        if(count % 2 == 0)
          continue;
        else if(count == 7)
          break;
        sum += count;
      }
      while(count != 10);
      assert count == 7;
      assert sum == 1 + 3 + 5;

      count = 0;
      sum = 0;
      do {
        count ++;
        sum += count;
        continue;
      }
      while(count != 10);
      assert count == 10;
      assert sum == 55;

      count = 0;
      sum = 0;
      for(var i = 0;  i != 10;  ++i) {
        count ++;
        if(count % 2 == 0)
          continue;
        else if(count == 7)
          break;
        sum += count;
      }
      assert count == 7;
      assert sum == 1 + 3 + 5;

      count = 0;
      sum = 0;
      for(var i = 0;  i != 10;  ++i) {
        count ++;
        sum += count;
        continue;
      }
      assert count == 10;
      assert sum == 55;

      count = 0;
      sum = 0;
      for(each k: v -> [10,11,12,13,14,15,16,17,18,19]) {
        count ++;
        if(count % 2 == 0)
          continue;
        else if(count == 7)
          break;
        sum += count;
      }
      assert count == 7;
      assert sum == 1 + 3 + 5;

      count = 0;
      sum = 0;
      for(each k: v -> [10,11,12,13,14,15,16,17,18,19]) {
        count ++;
        sum += count;
        continue;
      }
      assert count == 10;
      assert sum == 55;

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
