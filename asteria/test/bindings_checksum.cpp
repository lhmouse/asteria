// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/compiler/simple_source_file.hpp"
#include "../src/runtime/global_context.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    static constexpr char s_source[] =
      R"__(
        const s = "abcdefg";
        var h;

        const crc32_results = [
          0x00000000, 0x312A6AA6, 0x7FD808AF, 0xE643CF1E, 0x0A321D29, 0x601B0C2E, 0xF57CB98C, 0xCEC343C6,
          0x2F0CB3EB, 0x9172ABF9, 0x35F9C1EF, 0x9CA74BF6, 0x668A8270, 0x6B210FB2, 0x6710F04C, 0x6102348A,
          0x7BFD59A7, 0x943D6D08, 0xDD3C3281, 0xE6161985, 0xD3698066, 0x673F04EA, 0x88EB3104, 0x7DAAF2A7,
          0x48BEC022, 0x89B79A00, 0xBF617236, 0x5D305635, 0x89E06440, 0x5EC16CFD, 0xB4124764, 0xD4BE25AB,
          0x2526E10C, 0xF2FF4D5D, 0x26E5E2C3, 0xF23EED74, 0xB1A94E09, 0x49199071, 0xEDBF453E, 0x20612956,
          0x55EC1316, 0xA1916629, 0xAC138A6E, 0x7575214F, 0x224A86B4, 0x3581D18D, 0x4AFABFAF, 0x9D789824,
          0x7D657170, 0xD5BBE495, 0x77921A07, 0x0BEE9980, 0xC3B03960, 0xEDD639DA, 0x3F4C73D4, 0xD65B6F20,
          0x4BC9F62A, 0x313CA3E6, 0xE2945088, 0xDEADCEA7, 0xD6F86D0F, 0x0873FB12, 0xC5F8BC6B, 0xEC44A41F,
          0x13EBD570, 0x821939AE, 0x2F6B1FB3, 0xC48476E1, 0xF4CF46DA, 0x85D92126, 0x0FD5EBB2, 0xE3353F11,
          0xA8A49184, 0x9631691D, 0x1CB2E674, 0x99D8D552, 0xBE5E8512, 0xF5982A29, 0x9DF6BAFD, 0x078C9D28,
          0x85417DEA, 0x9E52BA10, 0xF2E9CC47, 0xDF07841C, 0xD862BF0D, 0xBB731F9D, 0x39B49E90, 0x91EA7256,
          0x505D435E, 0xC0828456, 0x4965ED11, 0xED971A0A, 0x702476F0, 0xD75197FB, 0xE9C04525, 0xB158012B,
          0x9A617990, 0x84601FAE, 0x2F3724FA, 0x864993EF, 0xC2E10B6A, 0xBDDB412C, 0xEC0E3888, 0x41630B12,
          0x56962547, 0x57C7371A, 0xE6149DE3, 0xC4EEFC17, 0x9FC8116B, 0x4C57F16F, 0xC9EB6848, 0xF6288BCD,
          0xBA2B5D48, 0x3200FA7A, 0x9719EF96, 0x1747AC19, 0x8AAF8951, 0xFDA493AD, 0x7D0F4BC9, 0x1635A5C1,
          0x0E224171, 0xE4EF63A7, 0xCEDF7CD3, 0x4AD6CF79, 0x5A78619F, 0x8610F444, 0xA7BF1EA6, 0x6EA65160,
        ];
        h = std.checksum.crc32_new();
        for(each k, v : crc32_results) {
          // split
          for(var i = 0; i < k; ++i) {
            h.write(s);
          }
          assert h.finish() == v;
          // simple
          assert std.checksum.crc32(s * k) == v;
        }

        
      )__";

    std::istringstream iss(s_source);
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global, { });
  }
