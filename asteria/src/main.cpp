// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include "value.hpp"
#include "reference.hpp"
#include "exception.hpp"
#include "token_stream.hpp"
#include "parser.hpp"
#include "rocket/insertable_istream.hpp"

using namespace Asteria;

int main()
  {
    D_array arr;
    arr.emplace_back(D_null());
    arr.emplace_back(D_boolean(true));
    Value first(std::move(arr));

    arr.clear();
    arr.emplace_back(D_integer(42));
    arr.emplace_back(D_real(123.456));
    Value second(std::move(arr));

    arr.clear();
    arr.emplace_back(D_string("hello"));
    Value third(std::move(arr));

    D_object obj;
    obj.try_emplace(String::shallow("first"), std::move(first));
    obj.try_emplace(String::shallow("second"), std::move(second));
    Value route(std::move(obj));

    obj.clear();
    obj.try_emplace(String::shallow("third"), std::move(third));
    obj.try_emplace(String::shallow("route"), std::move(route));
    obj.try_emplace(String::shallow("world"), D_string("世界"));
    Value root(std::move(obj));

    Value copy(root);
    copy.check<D_object>().try_emplace(String::shallow("new"), D_string("my string"));
    copy.check<D_object>().try_emplace(String::shallow("empty_array"), D_array());
    copy.check<D_object>().try_emplace(String::shallow("empty_object"), D_object());

    ASTERIA_DEBUG_LOG("---> ", root);
    ASTERIA_DEBUG_LOG("<--- ", copy);

    Token_stream ts;
    rocket::insertable_istream iss(String::shallow(R"___(
      // var stdio = require "stdio";
        { }
        { var i; }
        //  { const i; }
        { const i = 1; }
      func meow(param1, param2)
        {
          var i;
          const j = 1;
        }
      if(1.5e4)
        var k = 1;
      else if(x) {
        const r = 2;
        const z = 2;
      }
      ;
      for(each k, v : f)
        throw 1;
      try var i = 1; catch(e) { throw 2; }
      do
        const i = 4;
      while(0);
      while(0) var k = 2;
        switch (42)  {
          case "meow":
            continue while;
            continue for;
            continue  ;
            throw 42;
            return 42;
            return;
          case "woof":
          default  :
            const meow = 1;
          case 42 :
            break switch;
            break while;
            break for;
            break  ;
          case 2:
        }
      var sum = 0;
      1 + 2 * 3 * (4 + 5) + ++--sum++--("a","b",false)++[1].at;
      for(var i = 0; i < 100; ++i) {
        sum  += i;
      }
      for(;;)
        break;
      54  ;
      ;
      print(sum);
    )___"));
    bool r = ts.load(iss, String::shallow("dummy_file"));
    ROCKET_ASSERT(r);
    Parser p;
    r = p.load(ts);
    ROCKET_ASSERT(r);
  }
