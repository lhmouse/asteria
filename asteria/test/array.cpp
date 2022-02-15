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

        assert std.array.slice([0,1,2,3,4], 0) == [0,1,2,3,4];
        assert std.array.slice([0,1,2,3,4], 1) == [1,2,3,4];
        assert std.array.slice([0,1,2,3,4], 2) == [2,3,4];
        assert std.array.slice([0,1,2,3,4], 3) == [3,4];
        assert std.array.slice([0,1,2,3,4], 4) == [4];
        assert std.array.slice([0,1,2,3,4], 5) == [];
        assert std.array.slice([0,1,2,3,4], 6) == [];
        assert std.array.slice([0,1,2,3,4], 0, 3) == [0,1,2];
        assert std.array.slice([0,1,2,3,4], 1, 3) == [1,2,3];
        assert std.array.slice([0,1,2,3,4], 2, 3) == [2,3,4];
        assert std.array.slice([0,1,2,3,4], 3, 3) == [3,4];
        assert std.array.slice([0,1,2,3,4], 4, 3) == [4];
        assert std.array.slice([0,1,2,3,4], 5, 3) == [];
        assert std.array.slice([0,1,2,3,4], 6, 3) == [];
        assert std.array.slice([0,1,2,3,4], std.numeric.integer_max) == [];
        assert std.array.slice([0,1,2,3,4], std.numeric.integer_max, std.numeric.integer_max) == [];

        assert std.array.slice([0,1,2,3,4], -1) == [4];
        assert std.array.slice([0,1,2,3,4], -2) == [3,4];
        assert std.array.slice([0,1,2,3,4], -3) == [2,3,4];
        assert std.array.slice([0,1,2,3,4], -4) == [1,2,3,4];
        assert std.array.slice([0,1,2,3,4], -5) == [0,1,2,3,4];
        assert std.array.slice([0,1,2,3,4], -6) == [0,1,2,3,4];
        assert std.array.slice([0,1,2,3,4], -1, 3) == [4];
        assert std.array.slice([0,1,2,3,4], -2, 3) == [3,4];
        assert std.array.slice([0,1,2,3,4], -3, 3) == [2,3,4];
        assert std.array.slice([0,1,2,3,4], -4, 3) == [1,2,3];
        assert std.array.slice([0,1,2,3,4], -5, 3) == [0,1,2];
        assert std.array.slice([0,1,2,3,4], -6, 3) == [0,1];
        assert std.array.slice([0,1,2,3,4], -7, 3) == [0];
        assert std.array.slice([0,1,2,3,4], -8, 3) == [];
        assert std.array.slice([0,1,2,3,4], -9, 3) == [];
        assert std.array.slice([0,1,2,3,4], std.numeric.integer_min) == [0,1,2,3,4];
        assert std.array.slice([0,1,2,3,4], std.numeric.integer_min, std.numeric.integer_max) == [0,1,2,3];

        assert std.array.replace_slice([0,1,2,3,4], 1, ["a","b","c"]) == [0,"a","b","c"];
        assert std.array.replace_slice([0,1,2,3,4], 1, 2, ["a","b","c"]) == [0,"a","b","c",3,4];
        assert std.array.replace_slice([0,1,2,3,4], 9, ["a","b","c"]) == [0,1,2,3,4,"a","b","c"];

        assert std.array.replace_slice([0,1,2,3,4], -2, ["a","b","c"]) == [0,1,2,"a","b","c"];
        assert std.array.replace_slice([0,1,2,3,4], -2, 1, ["a","b","c"]) == [0,1,2,"a","b","c",4];
        assert std.array.replace_slice([0,1,2,3,4], -9, ["a","b","c"]) == ["a","b","c"];
        assert std.array.replace_slice([0,1,2,3,4], -9, 1, ["a","b","c"]) == ["a","b","c",0,1,2,3,4];

        assert std.array.replace_slice([0,1,2,3,4], -2, 1, ["a","b","c","d"], 1) == [0,1,2,"b","c","d",4];
        assert std.array.replace_slice([0,1,2,3,4], -2, 1, ["a","b","c","d"], 1, 2) == [0,1,2,"b","c",4];
        assert std.array.replace_slice([0,1,2,3,4], -2, 1, ["a","b","c","d"], -2) == [0,1,2,"c","d",4];
        assert std.array.replace_slice([0,1,2,3,4], -2, 1, ["a","b","c","d"], -2, 1) == [0,1,2,"c",4];

        assert std.array.find([0,1,2,3,4,3,2,1,0], 2) == 2;
        assert std.array.find([0,1,2,3,4,3,2,1,0], 2, 2) == 2;
        assert std.array.find([0,1,2,3,4,3,2,1,0], 3, 2) == 6;
        assert std.array.find([0,1,2,3,4,3,2,1,0], 7, 2) == null;
        assert std.array.find([0,1,2,3,4,3,2,1,0], 2, 3, 2) == 2;
        assert std.array.find([0,1,2,3,4,3,2,1,0], 3, 3, 2) == null;
        assert std.array.find([0,1,2,3,4,3,2,1,0], 4, 3, 2) == 6;
        assert std.array.find([0,1,2,3,4,3,2,1,0], 2, 5, 2) == 2;

        assert std.array.find_if([0,1,2,3,4,5,6,7,8,9], func(x) = (x % 5 == 3)) == 3;
        assert std.array.find_if([0,1,2,3,4,5,6,7,8,9], 3, func(x) = (x % 5 == 3)) == 3;
        assert std.array.find_if([0,1,2,3,4,5,6,7,8,9], 4, func(x) = (x % 5 == 3)) == 8;
        assert std.array.find_if([0,1,2,3,4,5,6,7,8,9], 9, func(x) = (x % 5 == 3)) == null;
        assert std.array.find_if([0,1,2,3,4,5,6,7,8,9], 3, 4, func(x) = (x % 5 == 3)) == 3;
        assert std.array.find_if([0,1,2,3,4,5,6,7,8,9], 4, 4, func(x) = (x % 5 == 3)) == null;
        assert std.array.find_if([0,1,2,3,4,5,6,7,8,9], 5, 4, func(x) = (x % 5 == 3)) == 8;
        assert std.array.find_if([0,1,2,3,4,5,6,7,8,9], 3, 6, func(x) = (x % 5 == 3)) == 3;

        assert std.array.find_if_not([0,1,2,3,4,5,6,7,8,9], func(x) = (x % 5 != 3)) == 3;
        assert std.array.find_if_not([0,1,2,3,4,5,6,7,8,9], 3, func(x) = (x % 5 != 3)) == 3;
        assert std.array.find_if_not([0,1,2,3,4,5,6,7,8,9], 4, func(x) = (x % 5 != 3)) == 8;
        assert std.array.find_if_not([0,1,2,3,4,5,6,7,8,9], 9, func(x) = (x % 5 != 3)) == null;
        assert std.array.find_if_not([0,1,2,3,4,5,6,7,8,9], 3, 4, func(x) = (x % 5 != 3)) == 3;
        assert std.array.find_if_not([0,1,2,3,4,5,6,7,8,9], 4, 4, func(x) = (x % 5 != 3)) == null;
        assert std.array.find_if_not([0,1,2,3,4,5,6,7,8,9], 5, 4, func(x) = (x % 5 != 3)) == 8;
        assert std.array.find_if_not([0,1,2,3,4,5,6,7,8,9], 3, 6, func(x) = (x % 5 != 3)) == 3;

        assert std.array.rfind([0,1,2,3,4,3,2,1,0], 2) == 6;
        assert std.array.rfind([0,1,2,3,4,3,2,1,0], 2, 2) == 6;
        assert std.array.rfind([0,1,2,3,4,3,2,1,0], 3, 2) == 6;
        assert std.array.rfind([0,1,2,3,4,3,2,1,0], 7, 2) == null;
        assert std.array.rfind([0,1,2,3,4,3,2,1,0], 2, 3, 2) == 2;
        assert std.array.rfind([0,1,2,3,4,3,2,1,0], 3, 3, 2) == null;
        assert std.array.rfind([0,1,2,3,4,3,2,1,0], 4, 3, 2) == 6;
        assert std.array.rfind([0,1,2,3,4,3,2,1,0], 2, 5, 2) == 6;

        assert std.array.rfind_if([0,1,2,3,4,5,6,7,8,9], func(x) = (x % 5 == 3)) == 8;
        assert std.array.rfind_if([0,1,2,3,4,5,6,7,8,9], 3, func(x) = (x % 5 == 3)) == 8;
        assert std.array.rfind_if([0,1,2,3,4,5,6,7,8,9], 4, func(x) = (x % 5 == 3)) == 8;
        assert std.array.rfind_if([0,1,2,3,4,5,6,7,8,9], 9, func(x) = (x % 5 == 3)) == null;
        assert std.array.rfind_if([0,1,2,3,4,5,6,7,8,9], 3, 4, func(x) = (x % 5 == 3)) == 3;
        assert std.array.rfind_if([0,1,2,3,4,5,6,7,8,9], 4, 4, func(x) = (x % 5 == 3)) == null;
        assert std.array.rfind_if([0,1,2,3,4,5,6,7,8,9], 5, 4, func(x) = (x % 5 == 3)) == 8;
        assert std.array.rfind_if([0,1,2,3,4,5,6,7,8,9], 3, 6, func(x) = (x % 5 == 3)) == 8;

        assert std.array.rfind_if_not([0,1,2,3,4,5,6,7,8,9], func(x) = (x % 5 != 3)) == 8;
        assert std.array.rfind_if_not([0,1,2,3,4,5,6,7,8,9], 3, func(x) = (x % 5 != 3)) == 8;
        assert std.array.rfind_if_not([0,1,2,3,4,5,6,7,8,9], 4, func(x) = (x % 5 != 3)) == 8;
        assert std.array.rfind_if_not([0,1,2,3,4,5,6,7,8,9], 9, func(x) = (x % 5 != 3)) == null;
        assert std.array.rfind_if_not([0,1,2,3,4,5,6,7,8,9], 3, 4, func(x) = (x % 5 != 3)) == 3;
        assert std.array.rfind_if_not([0,1,2,3,4,5,6,7,8,9], 4, 4, func(x) = (x % 5 != 3)) == null;
        assert std.array.rfind_if_not([0,1,2,3,4,5,6,7,8,9], 5, 4, func(x) = (x % 5 != 3)) == 8;
        assert std.array.rfind_if_not([0,1,2,3,4,5,6,7,8,9], 3, 6, func(x) = (x % 5 != 3)) == 8;

        assert std.array.count([0,1,2,3,4,3,2,1,0], 2) == 2;
        assert std.array.count([0,1,2,3,4,3,2,1,0], 2, 2) == 2;
        assert std.array.count([0,1,2,3,4,3,2,1,0], 3, 2) == 1;
        assert std.array.count([0,1,2,3,4,3,2,1,0], 7, 2) == 0;
        assert std.array.count([0,1,2,3,4,3,2,1,0], 2, 3, 2) == 1;
        assert std.array.count([0,1,2,3,4,3,2,1,0], 3, 3, 2) == 0;
        assert std.array.count([0,1,2,3,4,3,2,1,0], 4, 3, 2) == 1;
        assert std.array.count([0,1,2,3,4,3,2,1,0], 2, 5, 2) == 2;

        assert std.array.count_if([0,1,2,3,4,5,6,7,8,9], func(x) = (x % 5 == 3)) == 2;
        assert std.array.count_if([0,1,2,3,4,5,6,7,8,9], 3, func(x) = (x % 5 == 3)) == 2;
        assert std.array.count_if([0,1,2,3,4,5,6,7,8,9], 4, func(x) = (x % 5 == 3)) == 1;
        assert std.array.count_if([0,1,2,3,4,5,6,7,8,9], 9, func(x) = (x % 5 == 3)) == 0;
        assert std.array.count_if([0,1,2,3,4,5,6,7,8,9], 3, 4, func(x) = (x % 5 == 3)) == 1;
        assert std.array.count_if([0,1,2,3,4,5,6,7,8,9], 4, 4, func(x) = (x % 5 == 3)) == 0;
        assert std.array.count_if([0,1,2,3,4,5,6,7,8,9], 5, 4, func(x) = (x % 5 == 3)) == 1;
        assert std.array.count_if([0,1,2,3,4,5,6,7,8,9], 3, 6, func(x) = (x % 5 == 3)) == 2;

        assert std.array.count_if_not([0,1,2,3,4,5,6,7,8,9], func(x) = (x % 5 != 3)) == 2;
        assert std.array.count_if_not([0,1,2,3,4,5,6,7,8,9], 3, func(x) = (x % 5 != 3)) == 2;
        assert std.array.count_if_not([0,1,2,3,4,5,6,7,8,9], 4, func(x) = (x % 5 != 3)) == 1;
        assert std.array.count_if_not([0,1,2,3,4,5,6,7,8,9], 9, func(x) = (x % 5 != 3)) == 0;
        assert std.array.count_if_not([0,1,2,3,4,5,6,7,8,9], 3, 4, func(x) = (x % 5 != 3)) == 1;
        assert std.array.count_if_not([0,1,2,3,4,5,6,7,8,9], 4, 4, func(x) = (x % 5 != 3)) == 0;
        assert std.array.count_if_not([0,1,2,3,4,5,6,7,8,9], 5, 4, func(x) = (x % 5 != 3)) == 1;
        assert std.array.count_if_not([0,1,2,3,4,5,6,7,8,9], 3, 6, func(x) = (x % 5 != 3)) == 2;

        assert std.array.exclude([0,1,2,3,4,3,2,1,0], 2) == [0,1,3,4,3,1,0];
        assert std.array.exclude([0,1,2,3,4,3,2,1,0], 2, 2) == [0,1,3,4,3,1,0];
        assert std.array.exclude([0,1,2,3,4,3,2,1,0], 3, 2) == [0,1,2,3,4,3,1,0];
        assert std.array.exclude([0,1,2,3,4,3,2,1,0], 7, 2) == [0,1,2,3,4,3,2,1,0];
        assert std.array.exclude([0,1,2,3,4,3,2,1,0], 2, 3, 2) == [0,1,3,4,3,2,1,0];
        assert std.array.exclude([0,1,2,3,4,3,2,1,0], 3, 3, 2) == [0,1,2,3,4,3,2,1,0];
        assert std.array.exclude([0,1,2,3,4,3,2,1,0], 4, 3, 2) == [0,1,2,3,4,3,1,0];
        assert std.array.exclude([0,1,2,3,4,3,2,1,0], 2, 5, 2) == [0,1,3,4,3,1,0];

        assert std.array.exclude_if([0,1,2,3,4,5,6,7,8,9], func(x) = (x % 5 == 3)) == [0,1,2,4,5,6,7,9];
        assert std.array.exclude_if([0,1,2,3,4,5,6,7,8,9], 3, func(x) = (x % 5 == 3)) == [0,1,2,4,5,6,7,9];
        assert std.array.exclude_if([0,1,2,3,4,5,6,7,8,9], 4, func(x) = (x % 5 == 3)) == [0,1,2,3,4,5,6,7,9];
        assert std.array.exclude_if([0,1,2,3,4,5,6,7,8,9], 9, func(x) = (x % 5 == 3)) == [0,1,2,3,4,5,6,7,8,9];
        assert std.array.exclude_if([0,1,2,3,4,5,6,7,8,9], 3, 4, func(x) = (x % 5 == 3)) == [0,1,2,4,5,6,7,8,9];
        assert std.array.exclude_if([0,1,2,3,4,5,6,7,8,9], 4, 4, func(x) = (x % 5 == 3)) == [0,1,2,3,4,5,6,7,8,9];
        assert std.array.exclude_if([0,1,2,3,4,5,6,7,8,9], 5, 4, func(x) = (x % 5 == 3)) == [0,1,2,3,4,5,6,7,9];
        assert std.array.exclude_if([0,1,2,3,4,5,6,7,8,9], 3, 6, func(x) = (x % 5 == 3)) == [0,1,2,4,5,6,7,9];

        assert std.array.exclude_if_not([0,1,2,3,4,5,6,7,8,9], func(x) = (x % 5 != 3)) == [0,1,2,4,5,6,7,9];
        assert std.array.exclude_if_not([0,1,2,3,4,5,6,7,8,9], 3, func(x) = (x % 5 != 3)) == [0,1,2,4,5,6,7,9];
        assert std.array.exclude_if_not([0,1,2,3,4,5,6,7,8,9], 4, func(x) = (x % 5 != 3)) == [0,1,2,3,4,5,6,7,9];
        assert std.array.exclude_if_not([0,1,2,3,4,5,6,7,8,9], 9, func(x) = (x % 5 != 3)) == [0,1,2,3,4,5,6,7,8,9];
        assert std.array.exclude_if_not([0,1,2,3,4,5,6,7,8,9], 3, 4, func(x) = (x % 5 != 3)) == [0,1,2,4,5,6,7,8,9];
        assert std.array.exclude_if_not([0,1,2,3,4,5,6,7,8,9], 4, 4, func(x) = (x % 5 != 3)) == [0,1,2,3,4,5,6,7,8,9];
        assert std.array.exclude_if_not([0,1,2,3,4,5,6,7,8,9], 5, 4, func(x) = (x % 5 != 3)) == [0,1,2,3,4,5,6,7,9];
        assert std.array.exclude_if_not([0,1,2,3,4,5,6,7,8,9], 3, 6, func(x) = (x % 5 != 3)) == [0,1,2,4,5,6,7,9];

        assert std.array.is_sorted([]) == true;
        assert std.array.is_sorted([10]) == true;
        assert std.array.is_sorted([10,11,12]) == true;
        assert std.array.is_sorted([20,11,32]) == false;
        assert std.array.is_sorted([20,11,32], func(x, y) = (x % 10 <=> y % 10)) == true;

        assert std.array.binary_search([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 14) == 8;
        assert std.array.binary_search([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 24) == null;

        assert std.array.lower_bound([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 12) ==  2;
        assert std.array.lower_bound([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 15) ==  9;
        assert std.array.lower_bound([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16],  0) ==  0;
        assert std.array.lower_bound([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 24) == 15;

        assert std.array.upper_bound([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 12) ==  6;
        assert std.array.upper_bound([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 15) == 14;
        assert std.array.upper_bound([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16],  0) ==  0;
        assert std.array.upper_bound([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 24) == 15;

        assert std.array.equal_range([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 12) == [ 2, 4];
        assert std.array.equal_range([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 15) == [ 9, 5];
        assert std.array.equal_range([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16],  0) == [ 0, 0];
        assert std.array.equal_range([10,11,12,12,12,12,13,13,14,15,15,15,15,15,16], 24) == [15, 0];

        assert std.array.sort([17,13,14,11,16,18,10,15,12,19])
                           == [10,11,12,13,14,15,16,17,18,19];
        assert std.array.sort([32,14,11,22,21,34,31,13,23,24,12,33], func(x, y) = (x % 10 <=> y % 10))
                           == [11,21,31,32,22,12,13,23,33,14,34,24];
        assert std.array.sort(["abb","baa","aaa","bbb","aba","bab","aab","bba"])
                           == ["aaa","aab","aba","abb","baa","bab","bba","bbb"];
        assert std.array.sort(["abb","baa","aaa","bbb","aba","bab","aab","bba"], func(x, y) = std.string.compare(x, y, 2))
                           == ["aaa","aab","abb","aba","baa","bab","bbb","bba"];

        assert std.array.sortu([17,13,14,11,16,18,10,15,12,19])
                            == [10,11,12,13,14,15,16,17,18,19];
        assert std.array.sortu([32,14,11,22,21,34,31,13,23,24,12,33], func(x, y) = (x % 10 <=> y % 10))
                            == [11,32,13,14];
        assert std.array.sortu(["abb","baa","aaa","bbb","aba","bab","aab","bba"])
                            == ["aaa","aab","aba","abb","baa","bab","bba","bbb"];
        assert std.array.sortu(["abb","baa","aaa","bbb","aba","bab","aab","bba"], func(x, y) = std.string.compare(x, y, 2))
                            == ["aaa","abb","baa","bbb"];

        assert std.array.max_of([ ]) == null;
        assert std.array.max_of([5,null,3,"meow",7,4]) == 7;
        assert std.array.max_of([ ], func(x,y) = y<=> x) == null;
        assert std.array.max_of([5,null,3,"meow",7,4], func(x,y) = y<=> x) == 3;

        assert std.array.min_of([ ]) == null;
        assert std.array.min_of([5,null,3,"meow",7,4]) == 3;
        assert std.array.min_of([ ], func(x,y) = y<=> x) == null;
        assert std.array.min_of([5,null,3,"meow",7,4], func(x,y) = y<=> x) == 7;

        assert std.array.reverse([]) == [];
        assert std.array.reverse([0]) == [0];
        assert std.array.reverse([0,1]) == [1,0];
        assert std.array.reverse([0,1,2]) == [2,1,0];
        assert std.array.reverse([0,1,2,3]) == [3,2,1,0];
        assert std.array.reverse([0,1,2,3,4]) == [4,3,2,1,0];

        assert std.array.generate(func(x,v) = x + (v ?? 1), 10) == [1,2,4,7,11,16,22,29,37,46];

        assert std.array.shuffle([1,2,3,4,5], 42) == std.array.shuffle([1,2,3,4,5], 42);
        assert std.array.sort(std.array.shuffle([1,2,3,4,5])) == [1,2,3,4,5];

        assert std.array.rotate([], 0) == [];
        assert std.array.rotate([1,2,3,4], 0) == [1,2,3,4];
        assert std.array.rotate([], 9) == [];
        assert std.array.rotate([1], 9) == [1];
        assert std.array.rotate([1,2,3,4], 9) == [4,1,2,3];
        assert std.array.rotate([1,2,3,4,5], 9) == [2,3,4,5,1];
        assert std.array.rotate([], -9) == [];
        assert std.array.rotate([1], -9) == [1];
        assert std.array.rotate([1,2,3,4], -9) == [2,3,4,1];
        assert std.array.rotate([1,2,3,4,5], -9) == [5,1,2,3,4];

        assert std.array.sort(std.array.copy_keys({a:1,b:2,c:3,d:4})) == ['a','b','c','d'];
        assert std.array.sort(std.array.copy_values({a:1,b:2,c:3,d:4})) == [1,2,3,4];

        assert std.array.ksort({}) == [];
        assert std.array.ksort({foo:1}) == [['foo',1]];
        assert std.array.ksort({foo:1,bar:2}) == [['bar',2],['foo',1]];
        assert std.array.ksort({foo:1,bar:2,def:false}) == [['bar',2],['def',false],['foo',1]];

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
