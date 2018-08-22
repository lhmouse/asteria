// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include "value.hpp"
#include "reference.hpp"
#include "backtracer.hpp"
#include "exception.hpp"
#include <iostream>

using namespace Asteria;

void test_throw(unsigned i)
  try {
    if(i < 10) {
      test_throw(i + 1);
      throw Exception(reference_constant(D_string("some exception")));
    }
  } catch(...){
    throw Backtracer(String::shallow("test_file"), i);
  }

int main()
  {
    D_array arr;
    arr.emplace_back(D_null());
    arr.emplace_back(D_boolean(true));
    Value first(std::move(arr));

    arr.clear();
    arr.emplace_back(D_integer(42));
    arr.emplace_back(D_double(123.456));
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
    copy.as<D_object>().try_emplace(String::shallow("new"), D_string("my string"));
    copy.as<D_object>().try_emplace(String::shallow("empty_array"), D_array());
    copy.as<D_object>().try_emplace(String::shallow("empty_object"), D_object());

    std::cerr <<root <<std::endl;
    ASTERIA_DEBUG_LOG("---> ", "hello: ", 42);
    std::cerr <<copy <<std::endl;
    ASTERIA_DEBUG_LOG("<--- ", "good bye: ", 43);
    std::cerr <<root <<std::endl;

    try {
      test_throw(0);
    } catch(...) {
      Bivector<String, Unsigned> bt;
      try {
        unpack_backtrace_and_rethrow(bt, std::current_exception());
      } catch(Exception &e) {
        ASTERIA_DEBUG_LOG("Caught: ", read_reference(e.get_reference()));
        for(auto it = bt.rbegin(); it != bt.rend(); ++it) {
          ASTERIA_DEBUG_LOG("  inside ", it->first, ':', it->second);
        }
      }
    }
  }
