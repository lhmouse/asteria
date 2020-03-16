// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_STRING_HPP_
#define ASTERIA_LIBRARY_BINDINGS_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

Sval std_string_slice(Sval text, Ival from, Iopt length = { });
Sval std_string_replace_slice(Sval text, Ival from, Sval replacement);
Sval std_string_replace_slice(Sval text, Ival from, Iopt length, Sval replacement);

Ival std_string_compare(Sval text1, Sval text2, Iopt length = { });
Bval std_string_starts_with(Sval text, Sval prefix);
Bval std_string_ends_with(Sval text, Sval suffix);

Iopt std_string_find(Sval text, Sval pattern);
Iopt std_string_find(Sval text, Ival from, Sval pattern);
Iopt std_string_find(Sval text, Ival from, Iopt length, Sval pattern);
Iopt std_string_rfind(Sval text, Sval pattern);
Iopt std_string_rfind(Sval text, Ival from, Sval pattern);
Iopt std_string_rfind(Sval text, Ival from, Iopt length, Sval pattern);
Sval std_string_find_and_replace(Sval text, Sval pattern, Sval replacement);
Sval std_string_find_and_replace(Sval text, Ival from, Sval pattern, Sval replacement);
Sval std_string_find_and_replace(Sval text, Ival from, Iopt length, Sval pattern, Sval replacement);

Iopt std_string_find_any_of(Sval text, Sval accept);
Iopt std_string_find_any_of(Sval text, Ival from, Sval accept);
Iopt std_string_find_any_of(Sval text, Ival from, Iopt length, Sval accept);
Iopt std_string_find_not_of(Sval text, Sval reject);
Iopt std_string_find_not_of(Sval text, Ival from, Sval reject);
Iopt std_string_find_not_of(Sval text, Ival from, Iopt length, Sval reject);
Iopt std_string_rfind_any_of(Sval text, Sval accept);
Iopt std_string_rfind_any_of(Sval text, Ival from, Sval accept);
Iopt std_string_rfind_any_of(Sval text, Ival from, Iopt length, Sval accept);
Iopt std_string_rfind_not_of(Sval text, Sval reject);
Iopt std_string_rfind_not_of(Sval text, Ival from, Sval reject);
Iopt std_string_rfind_not_of(Sval text, Ival from, Iopt length, Sval reject);

Sval std_string_reverse(Sval text);
Sval std_string_trim(Sval text, Sopt reject = { });
Sval std_string_ltrim(Sval text, Sopt reject = { });
Sval std_string_rtrim(Sval text, Sopt reject = { });
Sval std_string_lpad(Sval text, Ival length, Sopt padding = { });
Sval std_string_rpad(Sval text, Ival length, Sopt padding = { });
Sval std_string_to_upper(Sval text);
Sval std_string_to_lower(Sval text);
Sval std_string_translate(Sval text, Sval inputs, Sopt outputs = { });

Aval std_string_explode(Sval text, Sopt delim, Iopt limit = { });
Sval std_string_implode(Aval segments, Sopt delim = { });

Sval std_string_hex_encode(Sval data, Bopt lowercase = { }, Sopt delim = { });
Sval std_string_hex_decode(Sval text);
Sval std_string_base32_encode(Sval data, Bopt lowercase = { });
Sval std_string_base32_decode(Sval text);
Sval std_string_base64_encode(Sval data);
Sval std_string_base64_decode(Sval text);

Sval std_string_url_encode(Sval data, Bopt lowercase = { });
Sval std_string_url_decode(Sval text);
Sval std_string_url_encode_query(Sval data, Bopt lowercase = { });
Sval std_string_url_decode_query(Sval text);

Bval std_string_utf8_validate(Sval text);
Sval std_string_utf8_encode(Ival code_point, Bopt permissive = { });
Sval std_string_utf8_encode(Aval code_points, Bopt permissive = { });
Aval std_string_utf8_decode(Sval text, Bopt permissive = { });

Sval std_string_pack_8(Ival value);
Sval std_string_pack_8(Aval values);
Aval std_string_unpack_8(Sval text);
Sval std_string_pack_16be(Ival value);
Sval std_string_pack_16be(Aval values);
Aval std_string_unpack_16be(Sval text);
Sval std_string_pack_16le(Ival value);
Sval std_string_pack_16le(Aval values);
Aval std_string_unpack_16le(Sval text);
Sval std_string_pack_32be(Ival value);
Sval std_string_pack_32be(Aval values);
Aval std_string_unpack_32be(Sval text);
Sval std_string_pack_32le(Ival value);
Sval std_string_pack_32le(Aval values);
Aval std_string_unpack_32le(Sval text);
Sval std_string_pack_64be(Ival value);
Sval std_string_pack_64be(Aval values);
Aval std_string_unpack_64be(Sval text);
Sval std_string_pack_64le(Ival value);
Sval std_string_pack_64le(Aval values);
Aval std_string_unpack_64le(Sval text);

Sval std_string_format(Sval templ, cow_vector<Value> values);

opt<pair<Ival, Ival>> std_string_regex_find(Sval text, Sval pattern);
opt<pair<Ival, Ival>> std_string_regex_find(Sval text, Ival from, Sval pattern);
opt<pair<Ival, Ival>> std_string_regex_find(Sval text, Ival from, Iopt length, Sval pattern);
Aopt std_string_regex_match(Sval text, Sval pattern);
Aopt std_string_regex_match(Sval text, Ival from, Sval pattern);
Aopt std_string_regex_match(Sval text, Ival from, Iopt length, Sval pattern);
Sval std_string_regex_replace(Sval text, Sval pattern, Sval replacement);
Sval std_string_regex_replace(Sval text, Ival from, Sval pattern, Sval replacement);
Sval std_string_regex_replace(Sval text, Ival from, Iopt length, Sval pattern, Sval replacement);

// Create an object that is to be referenced as `std.string`.
void create_bindings_string(Oval& result, API_Version version);

}  // namespace Asteria

#endif
