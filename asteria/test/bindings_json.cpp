// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    tinybuf_str cbuf;
    cbuf.set_string(rocket::sref(
      R"__(
        assert std.json.format(null) == "null";
        assert std.json.format(true) == "true";
        assert std.json.format(false) == "false";
        assert std.json.format(42) == "42";
        assert std.json.format(76.5) == "76.5";
        assert std.json.format("hello") == "\"hello\"";
        assert std.json.format("喵") == "\"\\u55B5\"";
        assert std.json.format("\a\b\v\f\n\r\t") == "\"\\u0007\\b\\u000B\\f\\n\\r\\t\"";

        assert std.json.format([]) == "[]";
        assert std.json.format([0]) == "[0]";
        assert std.json.format([0,1]) == "[0,1]";
        assert std.json.format([0,nan,2]) == "[0,null,2]";

        assert std.json.format({}) == "{}";
        assert std.json.format({a:1}) == "{\"a\":1}";
        assert std.json.format({a:infinity}) == "{\"a\":null}";
        assert std.json.format({a:1,b:std.json.format}) == "{\"a\":1}";

        assert std.json.format([[1,2],[3,4]]) == "[[1,2],[3,4]]";
        assert std.json.format([[1,2],[3,4]], "") == "[[1,2],[3,4]]";
        assert std.json.format([[1,2],[3,4]], "!*") == "[\n!*[\n!*!*1,\n!*!*2\n!*],\n!*[\n!*!*3,\n!*!*4\n!*]\n]";
        assert std.json.format([[1,2],[3,4]], 0) == "[[1,2],[3,4]]";
        assert std.json.format([[1,2],[3,4]], -1) == "[[1,2],[3,4]]";
        assert std.json.format([[1,2],[3,4]], 2) == "[\n  [\n    1,\n    2\n  ],\n  [\n    3,\n    4\n  ]\n]";

        assert std.json.parse("") == null;
        assert std.json.parse("null") == null;
        assert std.json.parse("true  ") == true;
        assert std.json.parse("  false") == false;
        assert std.json.parse("  42  ") == 42;
        assert std.json.parse("  76.5") == 76.5;
        assert std.json.parse("2 1") == null;
        assert std.json.parse("'hello'") == "hello";
        assert std.json.parse("\"\u55B5\"") == "喵";
        assert std.json.parse("\"\u55b5\"") == "喵";
        assert std.json.parse("\"\\u0007\\b\\u000B\\f\\n\\r\\t\"") == "\a\b\v\f\n\r\t";

        assert std.json.parse("[]") == [];
        assert std.json.parse("[0]") == [0];
        assert std.json.parse("[0,1]") == [0,1];
        assert std.json.parse("[0,null,2]") == [0,null,2];

        assert lengthof std.json.parse("{}") == 0;
        assert std.json.parse("{\"a\":1}").a == 1;
        assert std.json.parse("{b:Infinity}").b == infinity;
        assert __isnan std.json.parse("{b:NaN}").b;
        assert std.json.parse("{c:1,d:2,}").c == 1;
        assert std.json.parse("{c:1,d:2,}").d == 2;

        var r = std.json.parse("[{a:1,b:[]},{c:{},d:4}]");
        assert r[0].a == 1;
        assert r[0].b == [];
        assert typeof r[1].c == "object";
        assert lengthof r[1].c == 0;
        assert r[1].d == 4;

        const depth = 1000;
        var r = [];
        for(var i = 1; i < depth; ++i) {
          r = [r];
        }
        assert std.json.format(r) == '[' * depth + ']' * depth;
      )__"), tinybuf::open_read);

    Simple_Script code(cbuf, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global);
  }
