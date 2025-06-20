// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "rsa.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../utils.hpp"
#define OPENSSL_API_COMPAT  0x10100000L
#include <openssl/pem.h>
#include <openssl/pem.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
namespace asteria {
namespace {

void
do_md5(uint8_t* md, const V_string& data)
  {
    ::MD5_CTX ctx;
    ::MD5_Init(&ctx);
    ::MD5_Update(&ctx, data.data(), data.size());
    ::MD5_Final(md, &ctx);
  }

void
do_sha1(uint8_t* md, const V_string& data)
  {
    ::SHA_CTX ctx;
    ::SHA1_Init(&ctx);
    ::SHA1_Update(&ctx, data.data(), data.size());
    ::SHA1_Final(md, &ctx);
  }

void
do_sha256(uint8_t* md, const V_string& data)
  {
    ::SHA256_CTX ctx;
    ::SHA256_Init(&ctx);
    ::SHA256_Update(&ctx, data.data(), data.size());
    ::SHA256_Final(md, &ctx);
  }

V_string
do_rsa_sign(const V_string& private_key_path, int type, const uint8_t* m, unsigned mlen)
  {
    ::rocket::unique_posix_file key_file(::fopen(private_key_path.safe_c_str(), "rb"));
    if(!key_file)
      ASTERIA_THROW(("Could not open private key file '$1'"), private_key_path);

    unique_ptr<::RSA, void (::RSA*)> rsa(::PEM_read_RSAPrivateKey(key_file, nullptr, nullptr, nullptr), ::RSA_free);
    if(!rsa)
      ASTERIA_THROW(("Could not read private key file '$1'"), private_key_path);

    V_string sig;
    sig.append((uint32_t) ::RSA_size(rsa), '/');
    unsigned siglen;
    if(!::RSA_sign(type, m, mlen, (uint8_t*) sig.mut_data(), &siglen, rsa))
      ASTERIA_THROW(("Could not sign data with private key file '$1'"), private_key_path);

    sig.erase(siglen);
    return sig;
  }

bool
do_rsa_verify(const V_string& public_key_path, int type, const uint8_t* m, unsigned mlen, const V_string& sig)
  {
    ::rocket::unique_posix_file key_file(::fopen(public_key_path.safe_c_str(), "rb"));
    if(!key_file)
      ASTERIA_THROW(("Could not open public key file '$1'"), public_key_path);

    unique_ptr<::RSA, void (::RSA*)> rsa(::PEM_read_RSA_PUBKEY(key_file, nullptr, nullptr, nullptr), ::RSA_free);
    if(!rsa)
      ASTERIA_THROW(("Could not read public key file '$1'"), public_key_path);

    int result = ::RSA_verify(type, m, mlen, (const uint8_t*) sig.data(), (unsigned) sig.size(), rsa);
    return result == 1;
  }

}  // namespace

V_string
std_rsa_sign_md5(V_string private_key_path, V_string data)
  {
    uint8_t m[64];
    do_md5(m, data);
    return do_rsa_sign(private_key_path, NID_md5, m, MD5_DIGEST_LENGTH);
  }

V_boolean
std_rsa_verify_md5(V_string public_key_path, V_string data, V_string sig)
  {
    uint8_t m[64];
    do_md5(m, data);
    return do_rsa_verify(public_key_path, NID_md5, m, MD5_DIGEST_LENGTH, sig);
  }

V_string
std_rsa_sign_sha1(V_string private_key_path, V_string data)
  {
    uint8_t m[64];
    do_sha1(m, data);
    return do_rsa_sign(private_key_path, NID_sha1, m, SHA_DIGEST_LENGTH);
  }

V_boolean
std_rsa_verify_sha1(V_string public_key_path, V_string data, V_string sig)
  {
    uint8_t m[64];
    do_sha1(m, data);
    return do_rsa_verify(public_key_path, NID_sha1, m, SHA_DIGEST_LENGTH, sig);
  }

V_string
std_rsa_sign_sha256(V_string private_key_path, V_string data)
  {
    uint8_t m[64];
    do_sha256(m, data);
    return do_rsa_sign(private_key_path, NID_sha256, m, SHA256_DIGEST_LENGTH);
  }

V_boolean
std_rsa_verify_sha256(V_string public_key_path, V_string data, V_string sig)
  {
    uint8_t m[64];
    do_sha256(m, data);
    return do_rsa_verify(public_key_path, NID_sha256, m, SHA256_DIGEST_LENGTH, sig);
  }

void
create_bindings_rsa(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(&"sign_md5",
      ASTERIA_BINDING(
        "std.rsa.sign_md5", "",
        Argument_Reader&& reader)
      {
        V_string private_key_path, data;

        reader.start_overload();
        reader.required(private_key_path);
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_rsa_sign_md5(private_key_path, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"verify_md5",
      ASTERIA_BINDING(
        "std.rsa.verify_md5", "",
        Argument_Reader&& reader)
      {
        V_string public_key_path, data, sig;

        reader.start_overload();
        reader.required(public_key_path);
        reader.required(data);
        reader.required(sig);
        if(reader.end_overload())
          return (Value) std_rsa_verify_md5(public_key_path, data, sig);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"sign_sha1",
      ASTERIA_BINDING(
        "std.rsa.sign_sha1", "",
        Argument_Reader&& reader)
      {
        V_string private_key_path, data;

        reader.start_overload();
        reader.required(private_key_path);
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_rsa_sign_sha1(private_key_path, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"verify_sha1",
      ASTERIA_BINDING(
        "std.rsa.verify_sha1", "",
        Argument_Reader&& reader)
      {
        V_string public_key_path, data, sig;

        reader.start_overload();
        reader.required(public_key_path);
        reader.required(data);
        reader.required(sig);
        if(reader.end_overload())
          return (Value) std_rsa_verify_sha1(public_key_path, data, sig);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"sign_sha256",
      ASTERIA_BINDING(
        "std.rsa.sign_sha256", "",
        Argument_Reader&& reader)
      {
        V_string private_key_path, data;

        reader.start_overload();
        reader.required(private_key_path);
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_rsa_sign_sha256(private_key_path, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"verify_sha256",
      ASTERIA_BINDING(
        "std.rsa.verify_sha256", "",
        Argument_Reader&& reader)
      {
        V_string public_key_path, data, sig;

        reader.start_overload();
        reader.required(public_key_path);
        reader.required(data);
        reader.required(sig);
        if(reader.end_overload())
          return (Value) std_rsa_verify_sha256(public_key_path, data, sig);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
