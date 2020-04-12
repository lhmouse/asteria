// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_STRING_HPP_
#define ASTERIA_LIBRARY_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

// `std.string.slice`
Sval
std_string_slice(Sval text, Ival from, Iopt length);

// `std.string.replace_slice`
Sval
std_string_replace_slice(Sval text, Ival from, Iopt length, Sval replacement);

// `std.string.compare`
Ival
std_string_compare(Sval text1, Sval text2, Iopt length);

// `std.string.starts_with`
Bval
std_string_starts_with(Sval text, Sval prefix);

// `std.string.ends_with`
Bval
std_string_ends_with(Sval text, Sval suffix);

// `std.string.find`
Iopt
std_string_find(Sval text, Ival from, Iopt length, Sval pattern);

// `std.string.rfind`
Iopt
std_string_rfind(Sval text, Ival from, Iopt length, Sval pattern);

// `std.string.find_and_replace`
Sval
std_string_find_and_replace(Sval text, Ival from, Iopt length, Sval pattern, Sval replacement);

// `std.string.find_any_of`
Iopt
std_string_find_any_of(Sval text, Ival from, Iopt length, Sval accept);

// `std.string.find_not_of`
Iopt
std_string_find_not_of(Sval text, Ival from, Iopt length, Sval reject);

// `std.string.rfind_any_of`
Iopt
std_string_rfind_any_of(Sval text, Ival from, Iopt length, Sval accept);

// `std.string.rfind_not_of`
Iopt
std_string_rfind_not_of(Sval text, Ival from, Iopt length, Sval reject);

// `std.string.reverse`
Sval
std_string_reverse(Sval text);

// `std.string.trim`
Sval
std_string_trim(Sval text, Sopt reject);

// `std.string.triml`
Sval
std_string_triml(Sval text, Sopt reject);

// `std.string.trimr`
Sval
std_string_trimr(Sval text, Sopt reject);

// `std.string.padl`
Sval
std_string_padl(Sval text, Ival length, Sopt padding);

// `std.string.padr`
Sval
std_string_padr(Sval text, Ival length, Sopt padding);

// `std.string.to_upper`
Sval
std_string_to_upper(Sval text);

// `std.string.to_lower`
Sval
std_string_to_lower(Sval text);

// `std.string.translate`
Sval
std_string_translate(Sval text, Sval inputs, Sopt outputs);

// `std.string.explode`
Aval
std_string_explode(Sval text, Sopt delim, Iopt limit);

// `std.string.implode`
Sval
std_string_implode(Aval segments, Sopt delim);

// `std.string.hex_encode`
Sval
std_string_hex_encode(Sval data, Bopt lowercase, Sopt delim);

// `std.string.hex_decode`
Sval
std_string_hex_decode(Sval text);

// `std.string.base32_encode`
Sval
std_string_base32_encode(Sval data, Bopt lowercase);

// `std.string.base32_decode`
Sval
std_string_base32_decode(Sval text);

// `std.string.base64_encode`
Sval
std_string_base64_encode(Sval data);

// `std.string.base64_decode`
Sval
std_string_base64_decode(Sval text);

// `std.string.url_encode`
Sval
std_string_url_encode(Sval data, Bopt lowercase);

// `std.string.url_decode`
Sval
std_string_url_decode(Sval text);

// `std.string.url_encode_query`
Sval
std_string_url_encode_query(Sval data, Bopt lowercase);

// `std.string.url_decode_query`
Sval
std_string_url_decode_query(Sval text);

// `std.string.utf8_validate`
Bval
std_string_utf8_validate(Sval text);

// `std.string.utf8_encode`
Sval
std_string_utf8_encode(Ival code_point, Bopt permissive);

// `std.string.utf8_encode`
Sval
std_string_utf8_encode(Aval code_points, Bopt permissive);

// `std.string.utf8_decode`
Aval
std_string_utf8_decode(Sval text, Bopt permissive);

// `std.string.pack_8`
Sval
std_string_pack_8(Ival value);

// `std.string.pack_8`
Sval
std_string_pack_8(Aval values);

// `std.string.unpack_8`
Aval
std_string_unpack_8(Sval text);

// `std.string.pack_16be`
Sval
std_string_pack_16be(Ival value);

// `std.string.pack_16be`
Sval
std_string_pack_16be(Aval values);

// `std.string.unpack_16be`
Aval
std_string_unpack_16be(Sval text);

// `std.string.pack_16le`
Sval
std_string_pack_16le(Ival value);

// `std.string.pack_16le`
Sval
std_string_pack_16le(Aval values);

// `std.string.unpack_16le`
Aval
std_string_unpack_16le(Sval text);

// `std.string.pack_32be`
Sval
std_string_pack_32be(Ival value);

// `std.string.pack_32be`
Sval
std_string_pack_32be(Aval values);

// `std.string.unpack_32be`
Aval
std_string_unpack_32be(Sval text);

// `std.string.pack_32le`
Sval
std_string_pack_32le(Ival value);

// `std.string.pack_32le`
Sval
std_string_pack_32le(Aval values);

// `std.string.unpack_32le`
Aval
std_string_unpack_32le(Sval text);

// `std.string.pack_64be`
Sval
std_string_pack_64be(Ival value);

// `std.string.pack_64be`
Sval
std_string_pack_64be(Aval values);

// `std.string.unpack_64be`
Aval
std_string_unpack_64be(Sval text);

// `std.string.pack_64le`
Sval
std_string_pack_64le(Ival value);

// `std.string.pack_64le`
Sval
std_string_pack_64le(Aval values);

// `std.string.unpack_64le`
Aval
std_string_unpack_64le(Sval text);

// `std.string.format`
Sval
std_string_format(Sval templ, cow_vector<Value> values);

// `std.stringval`.
opt<pair<Ival, Ival>>
std_string_regex_find(Sval text, Ival from, Iopt length, Sval pattern);

// `std.string.regex_match`
Aopt
std_string_regex_match(Sval text, Ival from, Iopt length, Sval pattern);

// `std.string.regex_replace`
Sval
std_string_regex_replace(Sval text, Ival from, Iopt length, Sval pattern, Sval replacement);

// Create an object that is to be referenced as `std.string`.
void
create_bindings_string(V_object& result, API_Version version);

}  // namespace Asteria

#endif
