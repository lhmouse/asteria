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

        var str = '+';
        switch(1) {
          case 2:
            defer str += '2a';
            defer str += '2b';
          case 1:
            defer str += '1a';
            defer str += '1b';
          case 0:
            defer str += '0a';
            defer str += '0b';
        }
        assert str == '+0b0a1b1a';

        str = '+';
        switch(1) {
          case 2:
            defer str += '2a';
            defer str += '2b';
          case 1:
            defer str += '1a';
            break;
            defer str += '1b';
          case 0:
            defer str += '0a';
            defer str += '0b';
        }
        assert str =='+1a';

        str = '+';
        switch(1) {
          case 2:
            defer str += '2a';
            defer str += '2b';
          case 1:
            defer str += '1a';
            defer str += '1b';
          case 0:
            defer str += '0a';
            break;
            defer str += '0b';
        }
        assert str == '+0a1b1a';

        str = '+';
        try
          switch(1) {
            case 2:
              defer str += '2a';
              defer str += '2b';
            case 1:
              defer str += '1a';
              throw 42;
              defer str += '1b';
            case 0:
              defer str += '0a';
              defer str += '0b';
          }
        catch(e) { }
        assert str =='+1a';

        str = '+';
        try
          switch(1) {
            case 2:
              defer str += '2a';
              defer str += '2b';
            case 1:
              defer str += '1a';
              defer str += '1b';
            case 0:
              defer str += '0a';
              throw 42;
              defer str += '0b';
          }
        catch(e) { }
        assert str == '+0a1b1a';

        str = '+';
        switch(9999) {
          case 2:
            defer str += '2a';
            defer str += '2b';
          case 1:
            defer str += '1a';
            defer str += '1b';
          case 0:
            defer str += '0a';
            defer str += '0b';
        }
        assert str == '+';

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
