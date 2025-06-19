// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_RSA_
#define ASTERIA_LIBRARY_RSA_

#include "../fwd.hpp"
namespace asteria {

// `std.rsa.sign_md5`
V_string
std_rsa_sign_md5(V_string private_key_path, V_string data);

// `std.rsa.verify_md5`
V_boolean
std_rsa_verify_md5(V_string public_key_path, V_string data, V_string sig);

// `std.rsa.sign_sha1`
V_string
std_rsa_sign_sha1(V_string private_key_path, V_string data);

// `std.rsa.verify_sha1`
V_boolean
std_rsa_verify_sha1(V_string public_key_path, V_string data, V_string sig);

// `std.rsa.sign_sha256`
V_string
std_rsa_sign_sha256(V_string private_key_path, V_string data);

// `std.rsa.verify_sha256`
V_boolean
std_rsa_verify_sha256(V_string public_key_path, V_string data, V_string sig);

// Initialize an object that is to be referenced as `std.rsa`.
void
create_bindings_rsa(V_object& result, API_Version version);

}  // namespace asteria
#endif
