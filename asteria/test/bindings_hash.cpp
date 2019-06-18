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
        // These are CRC-32 checksums of "", "a", "aa", "aaa", "aaaa", and so on.
        const crc32_results = [
          0x00000000, 0xE8B7BE43, 0x078A19D7, 0xF007732D, 0xAD98E545, 0xEEAC93B9, 0x5AE419F8, 0x5B8B2074,
          0xBF848046, 0x77B7DE66, 0x4C11CDF0, 0x55465D92, 0xF6E30A76, 0x51278940, 0x9E3AD85A, 0x63973C71,
          0xCFD668D5, 0x1EC14E70, 0xB8AC0E31, 0xB9D11277, 0x266F8BCE, 0x944D3E7F, 0x28999FD0, 0x6E4CF508,
          0xE6027A84, 0x0284FB00, 0xE8B53AB8, 0x2DE530C7, 0xED9A0C60, 0xA5E84517, 0x6BC1D3C1, 0x04BF8DB6,
          0xCAB11777, 0x261CEBCB, 0xE427B990, 0x185C0ABE, 0xC4767CC2, 0x9D196BA3, 0xA7F5557A, 0x58C0D334,
          0xC95B8A25, 0xA37A318E, 0xE2146A55, 0xF3540F52, 0x6D21DA94, 0x1FB8C8C4, 0x74A10022, 0x3DA35EA7,
          0xA0382B56, 0x6A1F72A9, 0x47D7BA7D, 0xC6446478, 0xB6AF0329, 0xAAB3892C, 0xDAC56129, 0xAADFE34E,
          0x79790D37, 0x50736241, 0xE93CBC27, 0x4D543794, 0x1F98BD29, 0xAA1ABE92, 0xF61C5695, 0x6824C5DE,
          0x89B46555, 0xF33FAF5D, 0xFD9EACA5, 0x4EF67788, 0x0B9A4326, 0x3AB1A1FD, 0x2B818143, 0x07492FE8,
          0x46619D26, 0x3AFC5A23, 0x4AEA336B, 0x329DECA0, 0x3E538047, 0x003139F0, 0x550A7D66, 0x4C337053,
          0x1A998D7D, 0xC6192A4F, 0x0E12FB68, 0xABD045D2, 0x80C1DDFE, 0xB232A085, 0x75D7FB4C, 0x97A86403,
          0x7129479D, 0x667878FD, 0x2BDD489A, 0xF8461951, 0xF4239938, 0xC0412544, 0x99C67AEF, 0xD89B87D0,
          0x6EBCF710, 0xF56E12D0, 0x6E910285, 0x750B58EE, 0xAF707A64, 0xA2C76B78, 0xB6CB8026, 0x3A0CF03E,
          0x29ECAF18, 0xFBF2CABA, 0xC3F8161B, 0x62118FB9, 0x5A68A4E4, 0x4F8AF086, 0xEC2312A6, 0xD7EE9B8C,
          0x0C6E9FD3, 0xF76153B2, 0xCD2F0DB0, 0x231B22C2, 0x9DFE06FD, 0x2B26CEE4, 0x4FFBBEEC, 0x4144EBAE,
          0xD9987447, 0x00D6F204, 0xEFDAACA8, 0x30554F35, 0xBE342F2F, 0x43D8B735, 0xBE47A2D7, 0xF0BEBE96,
        ];
        var h = std.hash.crc32_new();
        for(each k, v : crc32_results) {
          // split
          for(var i = 0; i < k; ++i) {
            h.write("a");
          }
          assert h.finish() == v;
          // simple
          assert std.hash.crc32("a" * k) == v;
        }

        
      )__";

    std::istringstream iss(s_source);
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global, { });
  }
