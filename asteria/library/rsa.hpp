// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_RSA_
#define ASTERIA_LIBRARY_RSA_

#include "../fwd.hpp"
namespace asteria {

// `std.rsa.md5WithRSAEncryption_sign`
V_string
std_rsa_md5WithRSAEncryption_sign(V_string private_key_path, V_string data);

// `std.rsa.md5WithRSAEncryption_verify`
V_boolean
std_rsa_md5WithRSAEncryption_verify(V_string public_key_path, V_string data, V_string sig);

// `std.rsa.sha1WithRSAEncryption_sign`
V_string
std_rsa_sha1WithRSAEncryption_sign(V_string private_key_path, V_string data);

// `std.rsa.sha1WithRSAEncryption_verify`
V_boolean
std_rsa_sha1WithRSAEncryption_verify(V_string public_key_path, V_string data, V_string sig);

// `std.rsa.sha256WithRSAEncryption_sign`
V_string
std_rsa_sha256WithRSAEncryption_sign(V_string private_key_path, V_string data);

// `std.rsa.sha256WithRSAEncryption_verify`
V_boolean
std_rsa_sha256WithRSAEncryption_verify(V_string public_key_path, V_string data, V_string sig);

// Initialize an object that is to be referenced as `std.rsa`.
void
create_bindings_rsa(V_object& result, API_Version version);

}  // namespace asteria
#endif
