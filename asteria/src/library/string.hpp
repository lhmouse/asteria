// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_STRING_HPP_
#define ASTERIA_LIBRARY_STRING_HPP_

#include "../fwd.hpp"

namespace asteria {

// `std.string.slice`
V_string
std_string_slice(V_string text, V_integer from, optV_integer length);

// `std.string.replace_slice`
V_string
std_string_replace_slice(V_string text, V_integer from, optV_integer length, V_string replacement);

// `std.string.compare`
V_integer
std_string_compare(V_string text1, V_string text2, optV_integer length);

// `std.string.starts_with`
V_boolean
std_string_starts_with(V_string text, V_string prefix);

// `std.string.ends_with`
V_boolean
std_string_ends_with(V_string text, V_string suffix);

// `std.string.find`
optV_integer
std_string_find(V_string text, V_integer from, optV_integer length, V_string pattern);

// `std.string.rfind`
optV_integer
std_string_rfind(V_string text, V_integer from, optV_integer length, V_string pattern);

// `std.string.find_and_replace`
V_string
std_string_find_and_replace(V_string text, V_integer from, optV_integer length, V_string pattern,
                            V_string replacement);

// `std.string.find_any_of`
optV_integer
std_string_find_any_of(V_string text, V_integer from, optV_integer length, V_string accept);

// `std.string.find_not_of`
optV_integer
std_string_find_not_of(V_string text, V_integer from, optV_integer length, V_string reject);

// `std.string.rfind_any_of`
optV_integer
std_string_rfind_any_of(V_string text, V_integer from, optV_integer length, V_string accept);

// `std.string.rfind_not_of`
optV_integer
std_string_rfind_not_of(V_string text, V_integer from, optV_integer length, V_string reject);

// `std.string.reverse`
V_string
std_string_reverse(V_string text);

// `std.string.trim`
V_string
std_string_trim(V_string text, optV_string reject);

// `std.string.triml`
V_string
std_string_triml(V_string text, optV_string reject);

// `std.string.trimr`
V_string
std_string_trimr(V_string text, optV_string reject);

// `std.string.padl`
V_string
std_string_padl(V_string text, V_integer length, optV_string padding);

// `std.string.padr`
V_string
std_string_padr(V_string text, V_integer length, optV_string padding);

// `std.string.to_upper`
V_string
std_string_to_upper(V_string text);

// `std.string.to_lower`
V_string
std_string_to_lower(V_string text);

// `std.string.translate`
V_string
std_string_translate(V_string text, V_string inputs, optV_string outputs);

// `std.string.explode`
V_array
std_string_explode(V_string text, optV_string delim, optV_integer limit);

// `std.string.implode`
V_string
std_string_implode(V_array segments, optV_string delim);

// `std.string.hex_encode`
V_string
std_string_hex_encode(V_string data, optV_boolean lowercase, optV_string delim);

// `std.string.hex_decode`
V_string
std_string_hex_decode(V_string text);

// `std.string.base32_encode`
V_string
std_string_base32_encode(V_string data, optV_boolean lowercase);

// `std.string.base32_decode`
V_string
std_string_base32_decode(V_string text);

// `std.string.base64_encode`
V_string
std_string_base64_encode(V_string data);

// `std.string.base64_decode`
V_string
std_string_base64_decode(V_string text);

// `std.string.url_encode`
V_string
std_string_url_encode(V_string data, optV_boolean lowercase);

// `std.string.url_decode`
V_string
std_string_url_decode(V_string text);

// `std.string.url_encode_query`
V_string
std_string_url_encode_query(V_string data, optV_boolean lowercase);

// `std.string.url_decode_query`
V_string
std_string_url_decode_query(V_string text);

// `std.string.utf8_validate`
V_boolean
std_string_utf8_validate(V_string text);

// `std.string.utf8_encode`
V_string
std_string_utf8_encode(V_integer code_point, optV_boolean permissive);

// `std.string.utf8_encode`
V_string
std_string_utf8_encode(V_array code_points, optV_boolean permissive);

// `std.string.utf8_decode`
V_array
std_string_utf8_decode(V_string text, optV_boolean permissive);

// `std.string.pack_8`
V_string
std_string_pack_8(V_integer value);

// `std.string.pack_8`
V_string
std_string_pack_8(V_array values);

// `std.string.unpack_8`
V_array
std_string_unpack_8(V_string text);

// `std.string.pack_16be`
V_string
std_string_pack_16be(V_integer value);

// `std.string.pack_16be`
V_string
std_string_pack_16be(V_array values);

// `std.string.unpack_16be`
V_array
std_string_unpack_16be(V_string text);

// `std.string.pack_16le`
V_string
std_string_pack_16le(V_integer value);

// `std.string.pack_16le`
V_string
std_string_pack_16le(V_array values);

// `std.string.unpack_16le`
V_array
std_string_unpack_16le(V_string text);

// `std.string.pack_32be`
V_string
std_string_pack_32be(V_integer value);

// `std.string.pack_32be`
V_string
std_string_pack_32be(V_array values);

// `std.string.unpack_32be`
V_array
std_string_unpack_32be(V_string text);

// `std.string.pack_32le`
V_string
std_string_pack_32le(V_integer value);

// `std.string.pack_32le`
V_string
std_string_pack_32le(V_array values);

// `std.string.unpack_32le`
V_array
std_string_unpack_32le(V_string text);

// `std.string.pack_64be`
V_string
std_string_pack_64be(V_integer value);

// `std.string.pack_64be`
V_string
std_string_pack_64be(V_array values);

// `std.string.unpack_64be`
V_array
std_string_unpack_64be(V_string text);

// `std.string.pack_64le`
V_string
std_string_pack_64le(V_integer value);

// `std.string.pack_64le`
V_string
std_string_pack_64le(V_array values);

// `std.string.unpack_64le`
V_array
std_string_unpack_64le(V_string text);

// `std.string.format`
V_string
std_string_format(V_string templ, cow_vector<Value> values);

// `std.stringval`.
opt<pair<V_integer, V_integer>>
std_string_pcre_find(V_string text, V_integer from, optV_integer length, V_string pattern);

// `std.string.pcre_match`
optV_array
std_string_pcre_match(V_string text, V_integer from, optV_integer length, V_string pattern);

// `std.string.pcre_replace`
V_string
std_string_pcre_replace(V_string text, V_integer from, optV_integer length, V_string pattern,
                        V_string replacement);

// Create an object that is to be referenced as `std.string`.
void
create_bindings_string(V_object& result, API_Version version);

}  // namespace asteria

#endif
