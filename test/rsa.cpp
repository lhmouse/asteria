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

    const data_dir = std.string.pcre_replace(__file, '/rsa\.cpp$', '/rsa');

    var data1 = std.filesystem.read(data_dir / 'data1');
    var sig1 = std.filesystem.read(data_dir / 'data1.sig');

    assert std.string.base64_encode(std.rsa.sign_sha256(data_dir / 'private_key.pem', data1)) == sig1;
    assert std.rsa.verify_sha256(data_dir / 'public_key.pem', data1, std.string.base64_decode(sig1)) == true;

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
