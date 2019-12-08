// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_STRING_HPP_
#define ASTERIA_LIBRARY_BINDINGS_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_string std_string_slice(aref<G_string> text, aref<G_integer> from, aopt<G_integer> length = rocket::clear);
extern G_string std_string_replace_slice(aref<G_string> text, aref<G_integer> from, aref<G_string> replacement);
extern G_string std_string_replace_slice(aref<G_string> text, aref<G_integer> from, aopt<G_integer> length, aref<G_string> replacement);

extern G_integer std_string_compare(aref<G_string> text1, aref<G_string> text2, aopt<G_integer> length = rocket::clear);
extern G_boolean std_string_starts_with(aref<G_string> text, aref<G_string> prefix);
extern G_boolean std_string_ends_with(aref<G_string> text, aref<G_string> suffix);

extern opt<G_integer> std_string_find(aref<G_string> text, aref<G_string> pattern);
extern opt<G_integer> std_string_find(aref<G_string> text, aref<G_integer> from, aref<G_string> pattern);
extern opt<G_integer> std_string_find(aref<G_string> text, aref<G_integer> from, aopt<G_integer> length, aref<G_string> pattern);
extern opt<G_integer> std_string_rfind(aref<G_string> text, aref<G_string> pattern);
extern opt<G_integer> std_string_rfind(aref<G_string> text, aref<G_integer> from, aref<G_string> pattern);
extern opt<G_integer> std_string_rfind(aref<G_string> text, aref<G_integer> from, aopt<G_integer> length, aref<G_string> pattern);

extern G_string std_string_find_and_replace(aref<G_string> text, aref<G_string> pattern, aref<G_string> replacement);
extern G_string std_string_find_and_replace(aref<G_string> text, aref<G_integer> from, aref<G_string> pattern, aref<G_string> replacement);
extern G_string std_string_find_and_replace(aref<G_string> text, aref<G_integer> from, aopt<G_integer> length, aref<G_string> pattern, aref<G_string> replacement);

extern opt<G_integer> std_string_find_any_of(aref<G_string> text, aref<G_string> accept);
extern opt<G_integer> std_string_find_any_of(aref<G_string> text, aref<G_integer> from, aref<G_string> accept);
extern opt<G_integer> std_string_find_any_of(aref<G_string> text, aref<G_integer> from, aopt<G_integer> length, aref<G_string> accept);
extern opt<G_integer> std_string_find_not_of(aref<G_string> text, aref<G_string> reject);
extern opt<G_integer> std_string_find_not_of(aref<G_string> text, aref<G_integer> from, aref<G_string> reject);
extern opt<G_integer> std_string_find_not_of(aref<G_string> text, aref<G_integer> from, aopt<G_integer> length, aref<G_string> reject);
extern opt<G_integer> std_string_rfind_any_of(aref<G_string> text, aref<G_string> accept);
extern opt<G_integer> std_string_rfind_any_of(aref<G_string> text, aref<G_integer> from, aref<G_string> accept);
extern opt<G_integer> std_string_rfind_any_of(aref<G_string> text, aref<G_integer> from, aopt<G_integer> length, aref<G_string> accept);
extern opt<G_integer> std_string_rfind_not_of(aref<G_string> text, aref<G_string> reject);
extern opt<G_integer> std_string_rfind_not_of(aref<G_string> text, aref<G_integer> from, aref<G_string> reject);
extern opt<G_integer> std_string_rfind_not_of(aref<G_string> text, aref<G_integer> from, aopt<G_integer> length, aref<G_string> reject);

extern G_string std_string_reverse(aref<G_string> text);
extern G_string std_string_trim(aref<G_string> text, aopt<G_string> reject = rocket::clear);
extern G_string std_string_ltrim(aref<G_string> text, aopt<G_string> reject = rocket::clear);
extern G_string std_string_rtrim(aref<G_string> text, aopt<G_string> reject = rocket::clear);
extern G_string std_string_lpad(aref<G_string> text, aref<G_integer> length, aopt<G_string> padding = rocket::clear);
extern G_string std_string_rpad(aref<G_string> text, aref<G_integer> length, aopt<G_string> padding = rocket::clear);
extern G_string std_string_to_upper(aref<G_string> text);
extern G_string std_string_to_lower(aref<G_string> text);
extern G_string std_string_translate(aref<G_string> text, aref<G_string> inputs, aopt<G_string> outputs = rocket::clear);

extern G_array std_string_explode(aref<G_string> text, aopt<G_string> delim, aopt<G_integer> limit = rocket::clear);
extern G_string std_string_implode(aref<G_array> segments, aopt<G_string> delim = rocket::clear);

extern G_string std_string_hex_encode(aref<G_string> data, aopt<G_boolean> lowercase = rocket::clear, aopt<G_string> delim = rocket::clear);
extern opt<G_string> std_string_hex_decode(aref<G_string> text);

extern G_string std_string_base32_encode(aref<G_string> data, aopt<G_boolean> lowercase = rocket::clear);
extern opt<G_string> std_string_base32_decode(aref<G_string> text);

extern G_string std_string_base64_encode(aref<G_string> data);
extern opt<G_string> std_string_base64_decode(aref<G_string> text);

extern opt<G_string> std_string_utf8_encode(aref<G_integer> code_point, aopt<G_boolean> permissive = rocket::clear);
extern opt<G_string> std_string_utf8_encode(aref<G_array> code_points, aopt<G_boolean> permissive = rocket::clear);
extern opt<G_array> std_string_utf8_decode(aref<G_string> text, aopt<G_boolean> permissive = rocket::clear);

extern G_string std_string_pack8(aref<G_integer> value);
extern G_string std_string_pack8(aref<G_array> values);
extern G_array std_string_unpack8(aref<G_string> text);

extern G_string std_string_pack_16be(aref<G_integer> value);
extern G_string std_string_pack_16be(aref<G_array> values);
extern G_array std_string_unpack_16be(aref<G_string> text);

extern G_string std_string_pack_16le(aref<G_integer> value);
extern G_string std_string_pack_16le(aref<G_array> values);
extern G_array std_string_unpack_16le(aref<G_string> text);

extern G_string std_string_pack_32be(aref<G_integer> value);
extern G_string std_string_pack_32be(aref<G_array> values);
extern G_array std_string_unpack_32be(aref<G_string> text);

extern G_string std_string_pack_32le(aref<G_integer> value);
extern G_string std_string_pack_32le(aref<G_array> values);
extern G_array std_string_unpack_32le(aref<G_string> text);

extern G_string std_string_pack_64be(aref<G_integer> value);
extern G_string std_string_pack_64be(aref<G_array> values);
extern G_array std_string_unpack_64be(aref<G_string> text);

extern G_string std_string_pack_64le(aref<G_integer> value);
extern G_string std_string_pack_64le(aref<G_array> values);
extern G_array std_string_unpack_64le(aref<G_string> text);

// Create an object that is to be referenced as `std.string`.
extern void create_bindings_string(G_object& result, API_Version version);

}  // namespace Asteria

#endif
