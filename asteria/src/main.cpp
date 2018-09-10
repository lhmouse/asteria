// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include "value.hpp"
#include "reference.hpp"
#include "backtracer.hpp"
#include "exception.hpp"
#include "token_stream.hpp"
#include "parser.hpp"
#include "rocket/insertable_istream.hpp"

using namespace Asteria;

void test_throw(unsigned i)
  try {
    if(i < 10) {
      test_throw(i + 1);
      Reference_root::S_constant ref_c = { D_string("some exception") };
      throw Exception(std::move(ref_c));
    }
  } catch(...) {
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

    try {
      test_throw(0);
    } catch(...) {
      Bivector<String, Uint64> bt;
      try {
        Backtracer::unpack_and_rethrow(bt, std::current_exception());
      } catch(Exception &e) {
        ASTERIA_DEBUG_LOG("Caught: ", e.get_reference().read());
        for(auto it = bt.rbegin(); it != bt.rend(); ++it) {
          ASTERIA_DEBUG_LOG("  inside ", it->first, ':', it->second);
        }
      }
    }

    Token_stream ts;
#define NL "\n"
    rocket::insertable_istream iss(String::shallow("import \"stdio\";  " NL
                                                   "  { }  " NL
                                                   "  { var i; }  " NL
                                                   "//  { const i; }  " NL
                                                   "  { const i = 1; }  " NL
                                                   "func meow(param1, param2)  " NL
                                                   "  {  " NL
                                                   "    var i;  " NL
                                                   "    const j = 1;  " NL
                                                   "  }  " NL
                                                   "if(true) " NL
                                                   "  var k = 1; " NL
                                                   "else if(true) {" NL
                                                   "  const r = 2; " NL
                                                   "  const z = 2; " NL
                                                   "} " NL
                                                   "  switch (42)  {   " NL
                                                   "    case \"meow\": " NL
                                                   "      continue while;  " NL
                                                   "      continue for;  " NL
                                                   "      continue  ;  " NL
                                                   "      break switch;  " NL
                                                   "      break while;  " NL
                                                   "      break for;  " NL
                                                   "      break  ;  " NL
                                                   "    case \"woof\": " NL
                                                   "    default  : " NL
                                                   "      const meow = 1; " NL
                                                   "    case 42 : " NL
                                                   "  } " NL
                                                   "var sum = 0;  " NL
                                                   "for(var i = 0; i < 100; ++i) {  " NL
                                                   "  sum  += i;  " NL
                                                   "}  " NL
                                                   "null  ;" NL
                                                   ";" NL
                                                   "print(sum);  " NL
                                                   "export sum;  " NL));
    auto r = ts.load(iss, String::shallow("dummy_file"));
    ASTERIA_DEBUG_LOG("tokenizer error = ", r.get_error());
    Parser p;
    r = p.load(ts);
    ASTERIA_DEBUG_LOG("parser error = ", r.get_error());
  }
