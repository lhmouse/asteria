// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_STRING_HPP_
#define ASTERIA_LIBRARY_BINDINGS_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern G_string std_string_slice(const G_string& text, const G_integer& from, const Opt<G_integer>& length = rocket::nullopt);
extern G_string std_string_replace_slice(const G_string& text, const G_integer& from, const G_string& replacement);
extern G_string std_string_replace_slice(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& replacement);

extern G_integer std_string_compare(const G_string& text1, const G_string& text2, const Opt<G_integer>& length = rocket::nullopt);
extern G_boolean std_string_starts_with(const G_string& text, const G_string& prefix);
extern G_boolean std_string_ends_with(const G_string& text, const G_string& suffix);

extern Opt<G_integer> std_string_find(const G_string& text, const G_string& pattern);
extern Opt<G_integer> std_string_find(const G_string& text, const G_integer& from, const G_string& pattern);
extern Opt<G_integer> std_string_find(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& pattern);
extern Opt<G_integer> std_string_rfind(const G_string& text, const G_string& pattern);
extern Opt<G_integer> std_string_rfind(const G_string& text, const G_integer& from, const G_string& pattern);
extern Opt<G_integer> std_string_rfind(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& pattern);

extern G_string std_string_find_and_replace(const G_string& text, const G_string& pattern, const G_string& replacement);
extern G_string std_string_find_and_replace(const G_string& text, const G_integer& from, const G_string& pattern, const G_string& replacement);
extern G_string std_string_find_and_replace(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& pattern, const G_string& replacement);

extern Opt<G_integer> std_string_find_any_of(const G_string& text, const G_string& accept);
extern Opt<G_integer> std_string_find_any_of(const G_string& text, const G_integer& from, const G_string& accept);
extern Opt<G_integer> std_string_find_any_of(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& accept);
extern Opt<G_integer> std_string_find_not_of(const G_string& text, const G_string& reject);
extern Opt<G_integer> std_string_find_not_of(const G_string& text, const G_integer& from, const G_string& reject);
extern Opt<G_integer> std_string_find_not_of(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& reject);
extern Opt<G_integer> std_string_rfind_any_of(const G_string& text, const G_string& accept);
extern Opt<G_integer> std_string_rfind_any_of(const G_string& text, const G_integer& from, const G_string& accept);
extern Opt<G_integer> std_string_rfind_any_of(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& accept);
extern Opt<G_integer> std_string_rfind_not_of(const G_string& text, const G_string& reject);
extern Opt<G_integer> std_string_rfind_not_of(const G_string& text, const G_integer& from, const G_string& reject);
extern Opt<G_integer> std_string_rfind_not_of(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& reject);

extern G_string std_string_reverse(const G_string& text);
extern G_string std_string_trim(const G_string& text, const Opt<G_string>& reject = rocket::nullopt);
extern G_string std_string_ltrim(const G_string& text, const Opt<G_string>& reject = rocket::nullopt);
extern G_string std_string_rtrim(const G_string& text, const Opt<G_string>& reject = rocket::nullopt);
extern G_string std_string_lpad(const G_string& text, const G_integer& length, const Opt<G_string>& padding = rocket::nullopt);
extern G_string std_string_rpad(const G_string& text, const G_integer& length, const Opt<G_string>& padding = rocket::nullopt);
extern G_string std_string_to_upper(const G_string& text);
extern G_string std_string_to_lower(const G_string& text);
extern G_string std_string_translate(const G_string& text, const G_string& inputs, const Opt<G_string>& outputs = rocket::nullopt);

extern G_array std_string_explode(const G_string& text, const Opt<G_string>& delim, const Opt<G_integer>& limit = rocket::nullopt);
extern G_string std_string_implode(const G_array& segments, const Opt<G_string>& delim = rocket::nullopt);

extern G_string std_string_hex_encode(const G_string& text, const Opt<G_string>& delim = rocket::nullopt, const Opt<G_boolean>& uppercase = rocket::nullopt);
extern Opt<G_string> std_string_hex_decode(const G_string& hstr);

extern Opt<G_string> std_string_utf8_encode(const G_integer& code_point, const Opt<G_boolean>& permissive = rocket::nullopt);
extern Opt<G_string> std_string_utf8_encode(const G_array& code_points, const Opt<G_boolean>& permissive = rocket::nullopt);
extern Opt<G_array> std_string_utf8_decode(const G_string& text, const Opt<G_boolean>& permissive = rocket::nullopt);

extern G_string std_string_pack8(const G_integer& value);
extern G_string std_string_pack8(const G_array& values);
extern G_array std_string_unpack8(const G_string& text);

extern G_string std_string_pack16be(const G_integer& value);
extern G_string std_string_pack16be(const G_array& values);
extern G_array std_string_unpack16be(const G_string& text);

extern G_string std_string_pack16le(const G_integer& value);
extern G_string std_string_pack16le(const G_array& values);
extern G_array std_string_unpack16le(const G_string& text);

extern G_string std_string_pack32be(const G_integer& value);
extern G_string std_string_pack32be(const G_array& values);
extern G_array std_string_unpack32be(const G_string& text);

extern G_string std_string_pack32le(const G_integer& value);
extern G_string std_string_pack32le(const G_array& values);
extern G_array std_string_unpack32le(const G_string& text);

extern G_string std_string_pack64be(const G_integer& value);
extern G_string std_string_pack64be(const G_array& values);
extern G_array std_string_unpack64be(const G_string& text);

extern G_string std_string_pack64le(const G_integer& value);
extern G_string std_string_pack64le(const G_array& values);
extern G_array std_string_unpack64le(const G_string& text);

// Create an object that is to be referenced as `std.string`.
extern void create_bindings_string(G_object& result, API_Version version);

}  // namespace Asteria

#endif
