// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_STRING_HPP_
#define ASTERIA_LIBRARY_BINDINGS_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern Sval std_string_slice(Sval text, Ival from, Iopt length = clear);
extern Sval std_string_replace_slice(Sval text, Ival from, Sval replacement);
extern Sval std_string_replace_slice(Sval text, Ival from, Iopt length, Sval replacement);

extern Ival std_string_compare(Sval text1, Sval text2, Iopt length = clear);
extern Bval std_string_starts_with(Sval text, Sval prefix);
extern Bval std_string_ends_with(Sval text, Sval suffix);

extern Iopt std_string_find(Sval text, Sval pattern);
extern Iopt std_string_find(Sval text, Ival from, Sval pattern);
extern Iopt std_string_find(Sval text, Ival from, Iopt length, Sval pattern);
extern Iopt std_string_rfind(Sval text, Sval pattern);
extern Iopt std_string_rfind(Sval text, Ival from, Sval pattern);
extern Iopt std_string_rfind(Sval text, Ival from, Iopt length, Sval pattern);

extern Sval std_string_find_and_replace(Sval text, Sval pattern, Sval replacement);
extern Sval std_string_find_and_replace(Sval text, Ival from, Sval pattern, Sval replacement);
extern Sval std_string_find_and_replace(Sval text, Ival from, Iopt length, Sval pattern, Sval replacement);

extern Iopt std_string_find_any_of(Sval text, Sval accept);
extern Iopt std_string_find_any_of(Sval text, Ival from, Sval accept);
extern Iopt std_string_find_any_of(Sval text, Ival from, Iopt length, Sval accept);
extern Iopt std_string_find_not_of(Sval text, Sval reject);
extern Iopt std_string_find_not_of(Sval text, Ival from, Sval reject);
extern Iopt std_string_find_not_of(Sval text, Ival from, Iopt length, Sval reject);
extern Iopt std_string_rfind_any_of(Sval text, Sval accept);
extern Iopt std_string_rfind_any_of(Sval text, Ival from, Sval accept);
extern Iopt std_string_rfind_any_of(Sval text, Ival from, Iopt length, Sval accept);
extern Iopt std_string_rfind_not_of(Sval text, Sval reject);
extern Iopt std_string_rfind_not_of(Sval text, Ival from, Sval reject);
extern Iopt std_string_rfind_not_of(Sval text, Ival from, Iopt length, Sval reject);

extern Sval std_string_reverse(Sval text);
extern Sval std_string_trim(Sval text, Sopt reject = clear);
extern Sval std_string_ltrim(Sval text, Sopt reject = clear);
extern Sval std_string_rtrim(Sval text, Sopt reject = clear);
extern Sval std_string_lpad(Sval text, Ival length, Sopt padding = clear);
extern Sval std_string_rpad(Sval text, Ival length, Sopt padding = clear);
extern Sval std_string_to_upper(Sval text);
extern Sval std_string_to_lower(Sval text);
extern Sval std_string_translate(Sval text, Sval inputs, Sopt outputs = clear);

extern Aval std_string_explode(Sval text, Sopt delim, Iopt limit = clear);
extern Sval std_string_implode(Aval segments, Sopt delim = clear);

extern Sval std_string_hex_encode(Sval data, Bopt lowercase = clear, Sopt delim = clear);
extern Sopt std_string_hex_decode(Sval text);

extern Sval std_string_base32_encode(Sval data, Bopt lowercase = clear);
extern Sopt std_string_base32_decode(Sval text);

extern Sval std_string_base64_encode(Sval data);
extern Sopt std_string_base64_decode(Sval text);

extern Sopt std_string_utf8_encode(Ival code_point, Bopt permissive = clear);
extern Sopt std_string_utf8_encode(Aval code_points, Bopt permissive = clear);
extern Aopt std_string_utf8_decode(Sval text, Bopt permissive = clear);

extern Sval std_string_pack_8(Ival value);
extern Sval std_string_pack_8(Aval values);
extern Aval std_string_unpack_8(Sval text);

extern Sval std_string_pack_16be(Ival value);
extern Sval std_string_pack_16be(Aval values);
extern Aval std_string_unpack_16be(Sval text);

extern Sval std_string_pack_16le(Ival value);
extern Sval std_string_pack_16le(Aval values);
extern Aval std_string_unpack_16le(Sval text);

extern Sval std_string_pack_32be(Ival value);
extern Sval std_string_pack_32be(Aval values);
extern Aval std_string_unpack_32be(Sval text);

extern Sval std_string_pack_32le(Ival value);
extern Sval std_string_pack_32le(Aval values);
extern Aval std_string_unpack_32le(Sval text);

extern Sval std_string_pack_64be(Ival value);
extern Sval std_string_pack_64be(Aval values);
extern Aval std_string_unpack_64be(Sval text);

extern Sval std_string_pack_64le(Ival value);
extern Sval std_string_pack_64le(Aval values);
extern Aval std_string_unpack_64le(Sval text);

extern Sval std_string_format(Sval templ, cow_vector<Value> values);

// Create an object that is to be referenced as `std.string`.
extern void create_bindings_string(Oval& result, API_Version version);

}  // namespace Asteria

#endif
