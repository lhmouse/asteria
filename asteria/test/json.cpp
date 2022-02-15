// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"

using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

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
        assert std.json.format({'$42':1}) == "{\"$42\":1}";
        assert std.json.format({ab:infinity}) == "{\"ab\":null}";
        assert std.json.format({ab:1,cde:std.json.format}) == "{\"ab\":1}";

        assert std.json.format([[1,2],[3,4]]) == "[[1,2],[3,4]]";
        assert std.json.format([[1,2],[3,4]], "") == "[[1,2],[3,4]]";
        assert std.json.format([[1,2],[3,4]], "!*") == "[\n!*[\n!*!*1,\n!*!*2\n!*],\n!*[\n!*!*3,\n!*!*4\n!*]\n]";
        assert std.json.format([[1,2],[3,4]], 0) == "[[1,2],[3,4]]";
        assert std.json.format([[1,2],[3,4]], -1) == "[[1,2],[3,4]]";
        assert std.json.format([[1,2],[3,4]], 2) == "[\n  [\n    1,\n    2\n  ],\n  [\n    3,\n    4\n  ]\n]";

        assert std.json.format({a:1,bc:2}, 2) == "{\n  \"a\": 1,\n  \"bc\": 2\n}" ||
               std.json.format({a:1,bc:2}, 2) == "{\n  \"bc\": 2,\n  \"a\": 1\n}";

        assert std.json.format5(null) == "null";
        assert std.json.format5(true) == "true";
        assert std.json.format5(false) == "false";
        assert std.json.format5(42) == "42";
        assert std.json.format5(76.5) == "76.5";
        assert std.json.format5("hello") == "\"hello\"";
        assert std.json.format5("喵") == "\"\\u55B5\"";
        assert std.json.format5("\a\b\v\f\n\r\t") == "\"\\u0007\\b\\u000B\\f\\n\\r\\t\"";

        assert std.json.format5([]) == "[]";
        assert std.json.format5([0]) == "[0]";
        assert std.json.format5([0,1]) == "[0,1]";
        assert std.json.format5([0,nan,2]) == "[0,NaN,2]";

        assert std.json.format5({}) == "{}";
        assert std.json.format5({a:1}) == "{a:1}";
        assert std.json.format5({'$42':1}) == "{\"$42\":1}";
        assert std.json.format5({ab:infinity}) == "{ab:Infinity}";
        assert std.json.format5({ab:1,cde:std.json.format5}) == "{ab:1}";

        assert std.json.format5([[1,2],[3,4]]) == "[[1,2],[3,4]]";
        assert std.json.format5([[1,2],[3,4]], "") == "[[1,2],[3,4]]";
        assert std.json.format5([[1,2],[3,4]], "!*") == "[\n!*[\n!*!*1,\n!*!*2,\n!*],\n!*[\n!*!*3,\n!*!*4,\n!*],\n]";
        assert std.json.format5([[1,2],[3,4]], 0) == "[[1,2],[3,4]]";
        assert std.json.format5([[1,2],[3,4]], -1) == "[[1,2],[3,4]]";
        assert std.json.format5([[1,2],[3,4]], 2) == "[\n  [\n    1,\n    2,\n  ],\n  [\n    3,\n    4,\n  ],\n]";

        assert std.json.format5({a:1,bcd:2}, 2) == "{\n  a: 1,\n  bcd: 2,\n}" ||
               std.json.format5({a:1,bcd:2}, 2) == "{\n  bcd: 2,\n  a: 1,\n}";

        assert catch( std.json.parse("") ) != null;
        assert std.json.parse("null") == null;
        assert std.json.parse("true  ") == true;
        assert std.json.parse("  false") == false;
        assert std.json.parse("  42  ") == 42;
        assert std.json.parse("  76.5") == 76.5;
        assert catch( std.json.parse("2 1") ) != null;
        assert std.json.parse("'hello'") == "hello";
        assert std.json.parse("\"\u55B5\"") == "喵";
        assert std.json.parse("\"\u55b5\"") == "喵";
        assert std.json.parse("\"\\u0007\\b\\u000B\\f\\n\\r\\t\"") == "\a\b\v\f\n\r\t";

        assert std.json.parse("[]") == [];
        assert std.json.parse("[0]") == [0];
        assert std.json.parse("[0,1]") == [0,1];
        assert std.json.parse("[0,null,2]") == [0,null,2];

        assert countof std.json.parse("{}") == 0;
        assert std.json.parse("{\"a\":1}").a == 1;
        assert std.json.parse("{b:Infinity}").b == infinity;
        assert __isnan std.json.parse("{b:NaN}").b;
        assert std.json.parse("{c:1,d:2,}").c == 1;
        assert std.json.parse("{c:1,d:2,}").d == 2;

        var r = std.json.parse("[{a:1,b:[]},{c:{},d:4}]");
        assert r[0].a == 1;
        assert r[0].b == [];
        assert typeof r[1].c == "object";
        assert countof r[1].c == 0;
        assert r[1].d == 4;

        const depth = 1000;
        var r = [];
        for(var i = 1; i < depth; ++i) {
          r = [r];
        }
        assert std.json.format(r) == '[' * depth + ']' * depth;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
