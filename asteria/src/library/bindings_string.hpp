// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_STRING_HPP_
#define ASTERIA_LIBRARY_BINDINGS_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_string std_string_slice(G_string text, G_integer from, opt<G_integer> length = ::rocket::clear);
extern G_string std_string_replace_slice(G_string text, G_integer from, G_string replacement);
extern G_string std_string_replace_slice(G_string text, G_integer from, opt<G_integer> length, G_string replacement);

extern G_integer std_string_compare(G_string text1, G_string text2, opt<G_integer> length = ::rocket::clear);
extern G_boolean std_string_starts_with(G_string text, G_string prefix);
extern G_boolean std_string_ends_with(G_string text, G_string suffix);

extern opt<G_integer> std_string_find(G_string text, G_string pattern);
extern opt<G_integer> std_string_find(G_string text, G_integer from, G_string pattern);
extern opt<G_integer> std_string_find(G_string text, G_integer from, opt<G_integer> length, G_string pattern);
extern opt<G_integer> std_string_rfind(G_string text, G_string pattern);
extern opt<G_integer> std_string_rfind(G_string text, G_integer from, G_string pattern);
extern opt<G_integer> std_string_rfind(G_string text, G_integer from, opt<G_integer> length, G_string pattern);

extern G_string std_string_find_and_replace(G_string text, G_string pattern, G_string replacement);
extern G_string std_string_find_and_replace(G_string text, G_integer from, G_string pattern, G_string replacement);
extern G_string std_string_find_and_replace(G_string text, G_integer from, opt<G_integer> length, G_string pattern, G_string replacement);

extern opt<G_integer> std_string_find_any_of(G_string text, G_string accept);
extern opt<G_integer> std_string_find_any_of(G_string text, G_integer from, G_string accept);
extern opt<G_integer> std_string_find_any_of(G_string text, G_integer from, opt<G_integer> length, G_string accept);
extern opt<G_integer> std_string_find_not_of(G_string text, G_string reject);
extern opt<G_integer> std_string_find_not_of(G_string text, G_integer from, G_string reject);
extern opt<G_integer> std_string_find_not_of(G_string text, G_integer from, opt<G_integer> length, G_string reject);
extern opt<G_integer> std_string_rfind_any_of(G_string text, G_string accept);
extern opt<G_integer> std_string_rfind_any_of(G_string text, G_integer from, G_string accept);
extern opt<G_integer> std_string_rfind_any_of(G_string text, G_integer from, opt<G_integer> length, G_string accept);
extern opt<G_integer> std_string_rfind_not_of(G_string text, G_string reject);
extern opt<G_integer> std_string_rfind_not_of(G_string text, G_integer from, G_string reject);
extern opt<G_integer> std_string_rfind_not_of(G_string text, G_integer from, opt<G_integer> length, G_string reject);

extern G_string std_string_reverse(G_string text);
extern G_string std_string_trim(G_string text, opt<G_string> reject = ::rocket::clear);
extern G_string std_string_ltrim(G_string text, opt<G_string> reject = ::rocket::clear);
extern G_string std_string_rtrim(G_string text, opt<G_string> reject = ::rocket::clear);
extern G_string std_string_lpad(G_string text, G_integer length, opt<G_string> padding = ::rocket::clear);
extern G_string std_string_rpad(G_string text, G_integer length, opt<G_string> padding = ::rocket::clear);
extern G_string std_string_to_upper(G_string text);
extern G_string std_string_to_lower(G_string text);
extern G_string std_string_translate(G_string text, G_string inputs, opt<G_string> outputs = ::rocket::clear);

extern G_array std_string_explode(G_string text, opt<G_string> delim, opt<G_integer> limit = ::rocket::clear);
extern G_string std_string_implode(G_array segments, opt<G_string> delim = ::rocket::clear);

extern G_string std_string_hex_encode(G_string data, opt<G_boolean> lowercase = ::rocket::clear, opt<G_string> delim = ::rocket::clear);
extern opt<G_string> std_string_hex_decode(G_string text);

extern G_string std_string_base32_encode(G_string data, opt<G_boolean> lowercase = ::rocket::clear);
extern opt<G_string> std_string_base32_decode(G_string text);

extern G_string std_string_base64_encode(G_string data);
extern opt<G_string> std_string_base64_decode(G_string text);

extern opt<G_string> std_string_utf8_encode(G_integer code_point, opt<G_boolean> permissive = ::rocket::clear);
extern opt<G_string> std_string_utf8_encode(G_array code_points, opt<G_boolean> permissive = ::rocket::clear);
extern opt<G_array> std_string_utf8_decode(G_string text, opt<G_boolean> permissive = ::rocket::clear);

extern G_string std_string_pack_8(G_integer value);
extern G_string std_string_pack_8(G_array values);
extern G_array std_string_unpack_8(G_string text);

extern G_string std_string_pack_16be(G_integer value);
extern G_string std_string_pack_16be(G_array values);
extern G_array std_string_unpack_16be(G_string text);

extern G_string std_string_pack_16le(G_integer value);
extern G_string std_string_pack_16le(G_array values);
extern G_array std_string_unpack_16le(G_string text);

extern G_string std_string_pack_32be(G_integer value);
extern G_string std_string_pack_32be(G_array values);
extern G_array std_string_unpack_32be(G_string text);

extern G_string std_string_pack_32le(G_integer value);
extern G_string std_string_pack_32le(G_array values);
extern G_array std_string_unpack_32le(G_string text);

extern G_string std_string_pack_64be(G_integer value);
extern G_string std_string_pack_64be(G_array values);
extern G_array std_string_unpack_64be(G_string text);

extern G_string std_string_pack_64le(G_integer value);
extern G_string std_string_pack_64le(G_array values);
extern G_array std_string_unpack_64le(G_string text);

extern G_string std_string_format(G_string templ, cow_vector<Value> values);

// Create an object that is to be referenced as `std.string`.
extern void create_bindings_string(G_object& result, API_Version version);

}  // namespace Asteria

#endif
