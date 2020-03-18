// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_STRING_HPP_
#define ASTERIA_LIBRARY_BINDINGS_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

Sval string_slice(Sval text, Ival from, Iopt length = { });
Sval string_replace_slice(Sval text, Ival from, Sval replacement);
Sval string_replace_slice(Sval text, Ival from, Iopt length, Sval replacement);

Ival string_compare(Sval text1, Sval text2, Iopt length = { });
Bval string_starts_with(Sval text, Sval prefix);
Bval string_ends_with(Sval text, Sval suffix);

Iopt string_find(Sval text, Sval pattern);
Iopt string_find(Sval text, Ival from, Sval pattern);
Iopt string_find(Sval text, Ival from, Iopt length, Sval pattern);
Iopt string_rfind(Sval text, Sval pattern);
Iopt string_rfind(Sval text, Ival from, Sval pattern);
Iopt string_rfind(Sval text, Ival from, Iopt length, Sval pattern);
Sval string_find_and_replace(Sval text, Sval pattern, Sval replacement);
Sval string_find_and_replace(Sval text, Ival from, Sval pattern, Sval replacement);
Sval string_find_and_replace(Sval text, Ival from, Iopt length, Sval pattern, Sval replacement);

Iopt string_find_any_of(Sval text, Sval accept);
Iopt string_find_any_of(Sval text, Ival from, Sval accept);
Iopt string_find_any_of(Sval text, Ival from, Iopt length, Sval accept);
Iopt string_find_not_of(Sval text, Sval reject);
Iopt string_find_not_of(Sval text, Ival from, Sval reject);
Iopt string_find_not_of(Sval text, Ival from, Iopt length, Sval reject);
Iopt string_rfind_any_of(Sval text, Sval accept);
Iopt string_rfind_any_of(Sval text, Ival from, Sval accept);
Iopt string_rfind_any_of(Sval text, Ival from, Iopt length, Sval accept);
Iopt string_rfind_not_of(Sval text, Sval reject);
Iopt string_rfind_not_of(Sval text, Ival from, Sval reject);
Iopt string_rfind_not_of(Sval text, Ival from, Iopt length, Sval reject);

Sval string_reverse(Sval text);
Sval string_trim(Sval text, Sopt reject = { });
Sval string_ltrim(Sval text, Sopt reject = { });
Sval string_rtrim(Sval text, Sopt reject = { });
Sval string_lpad(Sval text, Ival length, Sopt padding = { });
Sval string_rpad(Sval text, Ival length, Sopt padding = { });
Sval string_to_upper(Sval text);
Sval string_to_lower(Sval text);
Sval string_translate(Sval text, Sval inputs, Sopt outputs = { });

Aval string_explode(Sval text, Sopt delim, Iopt limit = { });
Sval string_implode(Aval segments, Sopt delim = { });

Sval string_hex_encode(Sval data, Bopt lowercase = { }, Sopt delim = { });
Sval string_hex_decode(Sval text);
Sval string_base32_encode(Sval data, Bopt lowercase = { });
Sval string_base32_decode(Sval text);
Sval string_base64_encode(Sval data);
Sval string_base64_decode(Sval text);

Sval string_url_encode(Sval data, Bopt lowercase = { });
Sval string_url_decode(Sval text);
Sval string_url_encode_query(Sval data, Bopt lowercase = { });
Sval string_url_decode_query(Sval text);

Bval string_utf8_validate(Sval text);
Sval string_utf8_encode(Ival code_point, Bopt permissive = { });
Sval string_utf8_encode(Aval code_points, Bopt permissive = { });
Aval string_utf8_decode(Sval text, Bopt permissive = { });

Sval string_pack_8(Ival value);
Sval string_pack_8(Aval values);
Aval string_unpack_8(Sval text);
Sval string_pack_16be(Ival value);
Sval string_pack_16be(Aval values);
Aval string_unpack_16be(Sval text);
Sval string_pack_16le(Ival value);
Sval string_pack_16le(Aval values);
Aval string_unpack_16le(Sval text);
Sval string_pack_32be(Ival value);
Sval string_pack_32be(Aval values);
Aval string_unpack_32be(Sval text);
Sval string_pack_32le(Ival value);
Sval string_pack_32le(Aval values);
Aval string_unpack_32le(Sval text);
Sval string_pack_64be(Ival value);
Sval string_pack_64be(Aval values);
Aval string_unpack_64be(Sval text);
Sval string_pack_64le(Ival value);
Sval string_pack_64le(Aval values);
Aval string_unpack_64le(Sval text);

Sval string_format(Sval templ, cow_vector<Value> values);

opt<pair<Ival, Ival>> string_regex_find(Sval text, Sval pattern);
opt<pair<Ival, Ival>> string_regex_find(Sval text, Ival from, Sval pattern);
opt<pair<Ival, Ival>> string_regex_find(Sval text, Ival from, Iopt length, Sval pattern);
Aopt string_regex_match(Sval text, Sval pattern);
Aopt string_regex_match(Sval text, Ival from, Sval pattern);
Aopt string_regex_match(Sval text, Ival from, Iopt length, Sval pattern);
Sval string_regex_replace(Sval text, Sval pattern, Sval replacement);
Sval string_regex_replace(Sval text, Ival from, Sval pattern, Sval replacement);
Sval string_regex_replace(Sval text, Ival from, Iopt length, Sval pattern, Sval replacement);

// Create an object that is to be referenced as `std.string`.
void create_bindings_string(Oval& result, API_Version version);

}  // namespace Asteria

#endif
