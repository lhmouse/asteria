// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_STRING_HPP_
#define ASTERIA_LIBRARY_BINDINGS_STRING_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern D_integer std_string_compare(const D_string& text1, const D_string& text2, const Opt<D_integer>& length = rocket::nullopt);
extern D_boolean std_string_starts_with(const D_string& text, const D_string& prefix);
extern D_boolean std_string_ends_with(const D_string& text, const D_string& suffix);

extern D_string std_string_substr(const D_string& text, const D_integer& from, const Opt<D_integer>& length = rocket::nullopt);
extern D_string std_string_replace_substr(const D_string& text, const D_integer& from, const D_string& replacement);
extern D_string std_string_replace_substr(const D_string& text, const D_integer& from, const Opt<D_integer>& length, const D_string& replacement);

extern D_string std_string_reverse(const D_string& text);
extern D_string std_string_trim(const D_string& text, const Opt<D_string>& reject = rocket::nullopt);
extern D_string std_string_trim_left(const D_string& text, const Opt<D_string>& reject = rocket::nullopt);
extern D_string std_string_trim_right(const D_string& text, const Opt<D_string>& reject = rocket::nullopt);
extern D_string std_string_to_upper(const D_string& text);
extern D_string std_string_to_lower(const D_string& text);
extern D_string std_string_translate(const D_string& text, const D_string& inputs, const Opt<D_string>& outputs = rocket::nullopt);

extern D_array std_string_explode(const D_string& text, const Opt<D_string>& delim, const Opt<D_integer>& limit = rocket::nullopt);
extern D_string std_string_implode(const D_array& segments, const Opt<D_string>& delim = rocket::nullopt);

extern D_string std_string_hex_encode(const D_string& text, const Opt<D_string>& delim = rocket::nullopt, const Opt<D_boolean>& uppercase = rocket::nullopt);
extern Opt<D_string> std_string_hex_decode(const D_string& hstr);

extern Opt<D_string> std_string_utf8_encode(const D_array& code_points, const Opt<D_boolean>& permissive = rocket::nullopt);
extern Opt<D_array> std_string_utf8_decode(const D_string& text, const Opt<D_boolean>& permissive = rocket::nullopt);

extern D_string std_string_pack_8(const D_array& ints);
extern D_array std_string_unpack_8(const D_string& text);
extern D_string std_string_pack_16be(const D_array& ints);
extern D_array std_string_unpack_16be(const D_string& text);
extern D_string std_string_pack_16le(const D_array& ints);
extern D_array std_string_unpack_16le(const D_string& text);
extern D_string std_string_pack_32be(const D_array& ints);
extern D_array std_string_unpack_32be(const D_string& text);
extern D_string std_string_pack_32le(const D_array& ints);
extern D_array std_string_unpack_32le(const D_string& text);
extern D_string std_string_pack_64be(const D_array& ints);
extern D_array std_string_unpack_64be(const D_string& text);
extern D_string std_string_pack_64le(const D_array& ints);
extern D_array std_string_unpack_64le(const D_string& text);

// Create an object that is to be referenced as `std.string`.
extern D_object create_bindings_string();

}  // namespace Asteria

#endif
