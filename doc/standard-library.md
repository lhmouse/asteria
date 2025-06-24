# Asteria Standard Library

All standard library components are to be accessed through the global `std`
variable. Individual components are categorized into sub-objects.

## `std.version`

### `std.version.major`

* Denotes the major version number of the standard library that has been
  enabled. This field is always set, even when there is no standard library.

### `std.version.minor`

* Denotes the minor version number of the standard library that has been
  enabled. This field is always set, even when there is no standard library.

## `std.gc`

### `std.gc.count_variables(generation)`

* Gets the number of variables that are being tracked by the collector for
  `generation`. Valid values for `generation` are `0`, `1` and `2`.

* Returns the number of tracked variables as an integer. This value is only
  informative.

* Throws an exception if `generation` is out of range.

### `std.gc.get_threshold(generation)`

* Gets the threshold of the collector for `generation`. Valid values for
  `generation` are `0`, `1` and `2`.

* Returns the threshold as an integer.

* Throws an exception if `generation` is out of range.

### `std.gc.set_threshold(generation, threshold)`

* Sets the threshold of the collector for `generation` to `threshold`. Valid
  values for `generation` are `0`, `1` and `2`. Valid values for `threshold`
  range from `0` to an unspecified positive integer; overlarge values will
  be capped silently without failure. Larger `threshold` values make garbage
  collection run less often, but slower. Setting `threshold` to `0` ensures
  all unreachable variables be collected immediately.

* Returns the threshold before the call, as an integer.

* Throws an exception if `generation` is out of range.

### `std.gc.collect([generation_limit])`

* Performs garbage collection on all generations including and up to
  `generation_limit`. If it is absent, all generations are collected. The
  garbage collector is unable to collect local variables that are still in
  the caller scope.

* Returns the number of variables that have been collected in total.

## `std.system`

### `std.system.get_working_directory()`

* Gets the absolute path of the current working directory.

* Returns the path as a string.

### `std.system.get_environment_variable(name)`

* Retrieves an environment variable with `name`.

* Returns the environment variable as a string if one is found, or `null` if
  no such variable exists.

### `std.system.get_environment_variables()`

* Retrieves all environment variables as an object.

* Returns an object of all environment variables.

### `std.system.get_properties()`

* Retrieves properties about the running operating system.

* Returns an object consisting of the following fields:

  * `os`       string: name of the operating system
  * `kernel`   string: name and release of the kernel
  * `arch`     string: name of the CPU architecture
  * `nprocs`   integer: number of active CPU cores

### `std.system.random_uuid()`

* Generates a random UUID of the form `xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx`,
  where `M` is always `4` (UUID version) and `N` is any of `8`-`f` (UUID
  variant), and the others are random bits. Hexadecimal digits above nine are
  encoded in lowercase.

* Returns a UUID as a string of 36 characters, without braces.

### `std.system.get_pid()`

* Gets the ID of the current process.

* Returns the current process ID as an integer.

### `std.system.get_ppid()`

* Gets the ID of the parent process.

* Returns the parent process ID as an integer.

### `std.system.get_uid()`

* Gets the real user ID of the current process.

* Returns the real user ID as an integer.

### `std.system.get_euid()`

* Gets the effective user ID of the current process.

* Returns the effective user ID as an integer.

### `std.system.call(cmd, [argv, [envp]])`

* Launches the program denoted by `cmd`, awaits its termination, and returns
  its exit status. If `argv` is provided, it shall be an array of strings,
  which specifies additional arguments to pass to the program. If `envp` is
  given, it shall also be an array of strings, which specifies environment
  variables to the program.

* Returns the exit status of the new process as an integer. If the process
  has exited due to a signal, the exit status is `128+N` where `N` is the
  signal number.

* Throws an exception if the program could not be launched or its exit status
  could not be retrieved.

### `std.system.pipe(cmd, [argv, [envp]], [input])`

* Launches the program denoted by `cmd`, sends `input` to its standard input,
  awaits its termination, and returns its standard output. If `argv` is
  provided, it shall be an array of strings, which specifies additional
  arguments to pass to the program. If `envp` is given, it shall also be an
  array of strings, which specifies environment variables to the program. If
  `input` is null, the standard input of the new process will be closed.

* Returns the standard output of the new process as a string. If the process
  does not exit successfully, `null` is returned.

* Throws an exception if the program could not be launched or its exit status
  could not be retrieved.

### `std.system.daemonize()`

* Detaches the current process from its controlling terminal and continues
  execution in the background. The calling process terminates on success so
  this function never returns.

* Throws an exception upon failure.

### `std.system.sleep(duration)`

* Suspends execution for `duration` milliseconds, which may be either an
  integer or real number. If `duration` is not a non-negative number, this
  function returns immediately.

* Returns the remaining time in milliseconds as a real number, which will be
  positive only if the sleep has been interrupted.

### `std.system.load_conf(path)`

* Loads the configuration file denoted by `path`. Its syntax is similar to
  JSON5, except that commas, semicolons and top-level braces are omitted for
  simplicity, and single-quoted strings do not allow escapes. [Here is a
  sample.](sample.conf)

* Returns an object of the file, if it has been parsed successfully.

* Throws an exception if the file cannot be opened or contains an error.

## `std.debug`

### `std.debug.logf(templ, ...)`

* Composee a string in the same way as `std.string.format()`, but instead of
  returning it, writes it to standard error. A line break is appended to
  terminate the line.

* Returns the number of bytes written if the operation succeeds, or `null`
  otherwise.

* Throws an exception if `templ` is not a valid format string.

### `std.debug.dump([value], [indent])`

* Prints the value to standard error with detailed information. `indent`
  specifies the number of spaces, clamped between `0` and `10`, to use for
  each level of indention. If it is set to `0`, no line break is inserted
  and output lines are not indented. Its default value is `2`.

* Returns the number of bytes written if the operation succeeds, or `null`
  otherwise.

## `std.chrono`

### `std.chrono.now()`

* Retrieves the wall clock time.

* Returns the number of milliseconds since `1970-01-01 00:00:00 UTC`,
  represented as an integer.

### `std.chrono.format(time_point, [utc_offset])`

* Converts `time_point`, which shall be an integer representing the number
  of milliseconds since `1970-01-01 00:00:00 UTC`, to a string in the
  modified ISO 8601 format, which uses

  * a space instead of the T date/time delimiter
  * a space between time and time zone
  * no colon between hours and minutes of the timezone

  `utc_offset` specifies the number of minutes of local time ahead of UTC
  time, as an integer. If it is absent, it is obtained from the local system.

* Returns a string representing the time point.

### `std.chrono.format_ms(time_point, [utc_offset])`

* Converts `time_point`, which shall be an integer representing the number
  of milliseconds since `1970-01-01 00:00:00.000 UTC`, to a string in the
  modified ISO 8601 format, which uses

  * a space instead of the T date/time delimiter
  * a space between time and time zone
  * no colon between hours and minutes of the timezone
  * a decimal point and three digits for the millisecond part

  `utc_offset` specifies the number of minutes of local time ahead of UTC
  time, as an integer. If it is absent, it is obtained from the local system.

* Returns a string representing the time point.

### `std.chrono.parse(time_str)`

* Parses `time_str`, which shall be a string representing a time point in
  the modified ISO 8601 format, as the result of `format()`. The decimal
  part is optional and may have fewer or more digits. The timezone part may
  be `"UTC"` or `"GMT"` for UTC, or an explicit value such as `"+0400"`. If
  it is absent, the time is assumed to be in the local time zone. Leading or
  trailing spaces are allowed.

* Returns the number of milliseconds since `1970-01-01 00:00:00 UTC`, as an
  integer.

* Throws an exception if the string is invalid.

### `std.chrono.hires_now()`

* Retrieves a time point from a high resolution clock, which shall go
  monotonically and cannot be adjusted. The result is guaranteed accuracy and
  is suitable for time measurement.

* Returns the number of milliseconds since an unspecified time point, as a
  real number.

### `std.chrono.steady_now()`

* Retrieves a time point from a steady clock, which shall go monotonically
  and cannot be adjusted.

* Returns the number of milliseconds since an unspecified time point, as an
  integer.

## `std.string`

### `std.string.slice(text, from, [length])`

* Copies a subrange of `text` to create a new byte string. Bytes are copied
  from `from` if it is non-negative, or from `countof text + from` otherwise.
  If `length` is set to an integer, no more than this number of bytes will be
  copied. If it is absent, all bytes from `from` to the end of `text` will be
  copied. If `from` is outside `text`, an empty string is returned.

* Returns the specified substring of `text`.

### `std.string.replace_slice(text, from, [length], replacement, [rfrom, [rlength]])`

* Replaces a subrange of `text` with `replacement` to create a new byte
  string. `from` specifies the start of the subrange to replace. If `from` is
  negative, it specifies an offset from the end of `text`. `length` specifies
  the maximum number of bytes to replace. If it is set to `null`, all bytes
  up to the end of `text` will be replaced. If `rfrom` is specified, the
  replacement string is determined as `slice(replacement, rfrom, rlength)`.
  This function returns a new string without modifying `text`.

* Returns a string with the subrange replaced.

### `std.string.compare(text1, text2, [length])`

* Performs lexicographical comparison on two byte strings. If `length` is set
  to an integer, no more than this number of bytes will be compared. This
  function behaves like the `strncmp()` function in C, except that null
  characters are considered parts of strings.

* Returns a positive integer if `text1` compares greater than `text2`, a
  negative integer if `text1` compares less than `text2`, or zero if `text1`
  compares equal to `text2`.

### `std.string.starts_with(text, prefix)`

* Checks whether `prefix` is a prefix of `text`. An empty string is a prefix
  of any string.

* Returns `true` if `prefix` is a prefix of `text`, or `false` otherwise.

### `std.string.ends_with(text, suffix)`

* Checks whether `suffix` is a suffix of `text`. An empty string is a suffix
  of any string.

* Returns `true` if `suffix` is a suffix of `text`, or `false` otherwise.

### `std.string.find(text, [from, [length]], pattern)`

* Searches `text` for the first occurrence of `pattern`. The search operation
  is performed on the same subrange that would have been returned by
  `slice(text, from, length)`.

* Returns the subscript of the first byte of the first match of `pattern` in
  `text` if one is found, which is always non-negative; or `null` otherwise.

### `std.string.rfind(text, [from, [length]], pattern)`

* Searches `text` for the last occurrence of `pattern`. The search operation
  is performed on the same subrange that would have been returned by
  `slice(text, from, length)`.

* Returns the subscript of the first byte of the last match of `pattern` in
  `text` if one is found, which is always non-negative; or `null` otherwise.

### `std.string.replace(text, [from, [length]], pattern, replacement)`

* Searches `text` for `pattern`, and replaces all its occurrences with
  `replacement`. The search operation is performed on the same subrange that
  would have been returned by `slice(text, from, length)`. This function
  returns a new string without modifying `text`.

* Returns the string with `pattern` replaced by `replacement`. If `text` does
  not contain `pattern`, it is returned intact.

### `std.string.find_any_of(text, [from, [length]], accept)`

* Searches `text` for the first byte that exists in `accept`. The search
  operation is performed on the same subrange that would have been returned
  by `slice(text, from, length)`.

* Returns the subscript of the first match, which is always non-negative; or
  `null` if no such byte exists.

### `std.string.rfind_any_of(text, [from, [length]], accept)`

* Searches `text` for the last byte that exists in `accept`. The search
  operation is performed on the same subrange that would have been returned
  by `slice(text, from, length)`.

* Returns the subscript of the last match, which is always non-negative; or
  `null` if no such byte exists.

### `std.string.find_not_of(text, [from, [length]], reject)`

* Searches `text` for the first byte that does not exist in `reject`. The
  search operation is performed on the same subrange that would have been
  returned by `slice(text, from, length)`.

* Returns the subscript of the first non-match, which is always non-negative;
  or `null` if no such byte exists.

### `std.string.rfind_not_of(text, [from, [length]], reject)`

* Searches `text` for the last byte that does not exist in `reject`. The
  search operation is performed on the same subrange that would have been
  returned by `slice(text, from, length)`.

* Returns the subscript of the last non-match, which is always non-negative;
  or `null` if no such byte exists.

### `std.string.reverse(text)`

* Reverses a byte string. This function returns a new string without
  modifying `text`.

* Returns the reversed string.

### `std.string.trim(text, [reject])`

* Removes the longest prefix and suffix consisting of bytes from `reject`. If
  `reject` is empty, no byte will be removed. The default value of `reject`
  comprises a space and a tab. This function returns a new string without
  modifying `text`.

* Returns the trimmed string.

### `std.string.triml(text, [reject])`

* Removes the longest prefix consisting of bytes from `reject`. If `reject`
  is empty, no byte will be removed. The default value of `reject` comprises
  a space and a tab. This function returns a new string without modifying
  `text`.

* Returns the trimmed string.

### `std.string.trimr(text, [reject])`

* Removes the longest suffix consisting of bytes from `reject`. If `reject`
  is empty, no byte will be removed. The default value of `reject` comprises
  a space and a tab. This function returns a new string without modifying
  `text`.

* Returns the trimmed string.

### `std.string.padl(text, length, [padding])`

* Prepends `text` with `padding` repeatedly, until its length would exceed
  `length`. The default value of `padding` is a single space. This function
  returns a new string without modifying `text`.

* Returns the padded string.

* Throws an exception if `padding` is empty.

### `std.string.padr(text, length, [padding])`

* Appends `text` with `padding` repeatedly, until its length would exceed
  `length`. The default value of `padding` is a string consisting of a space.
  This function returns a new string without modifying `text`.

* Returns the padded string.

* Throws an exception if `padding` is empty.

### `std.string.to_upper(text)`

* Converts all lowercase ASCII (English) letters in `text` to their uppercase
  counterparts. This function returns a new string without modifying `text`.

* Returns a new string after the conversion.

### `std.string.to_lower(text)`

* Converts all uppercase ASCII (English) letters in `text` to their lowercase
  counterparts. This function returns a new string without modifying `text`.

* Returns a new string after the conversion.

### `std.string.translate(text, inputs, [outputs])`

* Performs bytewise translation on the given string. For every byte in `text`
  that is found in `inputs`, if there is a corresponding replacement byte in
  `outputs` at the same subscript, it is replaced with the latter; if no such
  replacement exists, because `outputs` either is shorter than `inputs` or is
  null, it is deleted. If `outputs` is longer than `inputs`, excess bytes are
  ignored. This function returns a new string without modifying `text`.

* Returns the translated string.

### `std.string.explode(text, [delim, [limit]])`

* Breaks `text` down into segments, separated by `delim`. If `delim` is an
  empty string or absent, every byte will become a segment. If `limit` is set
  to a positive integer, there will be no more segments than this number; the
  last segment will contain all remaining bytes from `text`.

* Returns broken-down segments as an array. If `text` is empty, an empty
  array is returned.

* Throws an exception if `limit` is negative or zero.

### `std.string.implode(segments, [delim])`

* Concatenates `segments`, which shall be an array of strings, to create a
  long string. If `delim` is specified, it is inserted between adjacent
  segments.

* Returns the concatenated string. If `segments` is empty, an empty string is
  returned.

### `std.string.hex_encode(data, [delim])`

* Encodes all bytes in `data` as 2-digit hexadecimal numbers and concatenates
  them. If `delim` is set to a string, it is inserted between adjacent bytes.
  Hexadecimal digits above nine are encoded as capital letters.

* Returns the encoded string.

### `std.string.hex_decode(text)`

* Decodes all hexadecimal digits from `text` and converts them to bytes.
  Whitespace can be used to delimit bytes, but it shall not occur between
  digits for the same byte. The total number of non-whitespace characters
  must be a multiple of two.

* Returns decoded bytes as a string. If `text` is empty or comprises only
  whitespaces, an empty string is returned.

* Throws an exception if `text` is invalid.

### `std.string.base32_encode(data)`

* Encodes all bytes in `data` according to the base32 encoding, specified by
  [RFC 4648](https://www.rfc-editor.org/rfc/rfc4648.html). Hexadecimal digits
  above nine are encoded as capital letters. The length of encoded data is
  always a multiple of eight. Padding characters are mandatory.

* Returns the encoded string.

### `std.string.base32_decode(text)`

* Decodes data encoded in the base32 encoding, specified by [RFC 4648](
  https://www.rfc-editor.org/rfc/rfc4648.html). Whitespace can be used to
  delimit encoding units, but it shall not occur between characters in the
  same encoding unit. The total number of non-whitespace characters must be a
  multiple of eight.

* Returns decoded bytes as a string. If `text` is empty or comprises only
  whitespaces, an empty string is returned.

* Throws an exception if `text` is invalid.

### `std.string.base64_encode(data)`

* Encodes all bytes in `data` according to the base64 encoding, specified by
  [RFC 4648](https://www.rfc-editor.org/rfc/rfc4648.html). Hexadecimal digits
  above nine are encoded as capital letters. The length of encoded data is
  always a multiple of four. Padding characters are mandatory.

* Returns the encoded string.

### `std.string.base64_decode(text)`

* Decodes data encoded in the base64 encoding, specified by [RFC 4648](
  https://www.rfc-editor.org/rfc/rfc4648.html). Whitespace can be used to
  delimit encoding units, but it shall not occur between characters in the
  same encoding unit. The total number of non-whitespace characters must be a
  multiple of four.

* Returns decoded bytes as a string. If `text` is empty or comprises only
  whitespaces, an empty string is returned.

* Throws an exception if `text` is invalid.

### `std.string.url_encode(data)`

* Encodes bytes in `data` according to the URI syntax, specified by [RFC
  3986](https://www.rfc-editor.org/rfc/rfc3986.html). Every byte that is not
  a letter, digit, `-`, `.`, `_` or `~` is encoded as a `%` followed by two
  hexadecimal digits. Hexadecimal digits above nine are encoded as capital
  letters.

* Returns the encoded string. If `data` is empty, an empty string is
  returned.

### `std.string.url_decode(text)`

* Decodes percent-encode sequences from `text` and converts them to bytes,
  according to the URI syntax, specified by [RFC 3986](
  https://www.rfc-editor.org/rfc/rfc3986.html). For convenience reasons, both
  reserved and unreserved characters are copied verbatim. Characters that are
  neither reserved nor unreserved (such as control characters and non-ASCII
  characters) cause decode errors.

* Returns decoded bytes as a string.

* Throws an exception if the string contains invalid characters.

### `std.string.url_query_encode(data)`

* Encodes bytes in `data` according to the URI syntax, specified by [RFC
  3986](https://www.rfc-editor.org/rfc/rfc3986.html). This function encodes
  fewer characters than `url_encode()`. A space is encoded as `+` instead of
  the long form `%20`.

* Returns the encoded string. If `data` is empty, an empty string is
  returned.

### `std.string.url_query_decode(text)`

* Decodes percent-encode sequences from `text` and converts them to bytes,
  according to the URI syntax, specified by [RFC 3986](
  https://www.rfc-editor.org/rfc/rfc3986.html). For convenience reasons, both
  reserved and unreserved characters are copied verbatim. `+` is decoded as
  a space. Characters that are neither reserved nor unreserved (such as
  control characters and non-ASCII characters) cause decode errors.

* Returns a string containing decoded bytes.

* Throws an exception if the string contains invalid characters.

### `std.string.utf8_validate(text)`

* Checks whether `text` is a valid UTF-8 string.

* Returns `true` if `text` is valid, or `false` otherwise.

### `std.string.utf8_encode(code_points, [permissive])`

* Encodes code points from `code_points` into a UTF-8 string. `code_points`
  can be either an integer or an array of integers. When an invalid code
  point is encountered, if `permissive` is set to `true`, it is replaced with
  the replacement character `"\uFFFD"` and encoded as `"\xEF\xBF\xBD"`;
  otherwise this function fails.

* Returns the encoded string.

* Throws an exception on failure.

### `std.string.utf8_decode(text, [permissive])`

* Decodes the UTF-8 string, `text`, into UTF code points, represented as an
  array of integers. When an invalid code sequence is encountered, if
  `permissive` is set to `true`, all its code units are re-interpreted as
  isolated bytes according to ISO/IEC 8859-1; otherwise this function fails.

* Returns an array containing decoded code points.

* Throws an exception on failure.

### `std.string.format(templ, ...)`

* Composes a string according to the template string `templ`, where

  * `$$` is replaced with a literal `$`.
  * `${NNN}`, where `NNN` is at most three decimal numerals, is replaced
       with the NNN-th argument. If `NNN` is zero, it is replaced with
       `templ` itself.
  * `$N`, where `N` is a single decimal numeral, is the same as `${N}`.
  * All the other characters are copied verbatim.

* Returns the composed string.

* Throws an exception if `templ` contains invalid placeholder sequences.

### `std.string.PCRE(pattern, [options])`

* Constructs a string matcher object from the Perl-compatible regular
  expression (PCRE) `pattern`. If `options` is set, it shall be an array of
  strings, whose elements specify options for the regular expression (this
  list is partly copied from the PCRE2 manual):

  * `"caseless"`    Do caseless matching
  * `"dotall"`      `.` matches anything including new lines
  * `"extended"`    Ignore whitespace and `#` comments
  * `"multiline"`   `^` and `$` match newlines within data

* Returns a matcher as an object consisting of the following fields:

  * `find(text, [from, [length]])`
  * `match(text, [from, [length]])`
  * `named_match(text, [from, [length]])`
  * `replace(text, [from, [length]], replacement)`

  These functions behave the same way as their counterparts with the `pcre_`
  prefix, as described in this document.

* Throws an exception if `pattern` is not a valid PCRE.

### `std.string.pcre_find(text, [from, [length]], pattern, [options])`

* Searches `text` for the first match of `pattern`. The search operation is
  performed on the same subrange that would have been returned by
  `slice(text, from, length)`. `options` specifies the options to pass to
  `PCRE(pattern, options)`.

* Returns an array of two integers. The first integer specifies the subscript
  of the matching sequence, and the second integer specifies its length. If
  `pattern` is not found, this function returns `null`.

* Throws an exception if `pattern` is not a valid PCRE.

### `std.string.pcre_match(text, [from, [length]], pattern, [options])`

* Searches `text` for the first match of `pattern`. The search operation is
  performed on the same subrange that would have been returned by
  `slice(text, from, length)`. `options` specifies options to pass to
  `PCRE(pattern, options)`.

* Returns an array of strings. The first element is a copy of the entire
  substring that matches `pattern`, and the remaining elements are substrings
  that match positional capturing groups. If a group fails to match, its
  corresponding element is `null`. If `text` does not match `pattern`, `null`
  is returned.

* Throws an exception if `pattern` is not a valid PCRE.

### `std.string.pcre_named_match(text, [from, [length]], pattern, [options])`

* Searches `text` for the first match of `pattern`. The search operation is
  performed on the same subrange that would have been returned by
  `slice(text, from, length)`. `options` specifies options to pass to
  `PCRE(pattern, options)`.

* Returns an object of all named groups. Each key is the name of a group and
  its value is the matched substring, if any. The special key `&` denotes the
  the entire substring that matches `pattern`. If there is no named group in
  `pattern`, an empty object is returned. If a group fails to match, its
  corresponding value is `null`. If `text` does not match `pattern`, `null`
  is returned.

* Throws an exception if `pattern` is not a valid PCRE.

### `std.string.pcre_replace(text, [from, [length]], pattern, replacement, [options])`

* Searches `text` for `pattern` and replaces all matches with `replacement`.
  The search operation is performed on the same subrange that would have been
  returned by `slice(text, from, length)`. `options` specifies options to
  pass to `PCRE(pattern, options)`. This function returns a new string
  without modifying `text`.

* Returns the string with `pattern` replaced. If `text` does not contain
  `pattern`, it is returned intact.

* Throws an exception if `pattern` is not a valid PCRE.

### `std.string.iconv(to_encoding, text, [from_encoding])`

* Converts `text` from `from_encoding` to `to_encoding`. This function is a
  wrapper for the iconv library, and accepts the same set of encoding names.
  The default value of `from_encoding` is `"UTF-8"`.

* Returns the converted byte string.

* Throws an exception in case of an invalid encoding or an invalid input
  string.

### `std.string.visual_length(text)`

* Calculates the visual length of `text` which shall be a UTF-8 string.

* Returns the visual length in columns.

* Throws an exception if `text` is not a valid UTF-8 string, or contains
  non-printable characters.

## `std.array`

### `std.array.slice(data, from, [length])`

* Copies a subrange of `data` to create a new array. Elements are copied from
  `from` if it is non-negative, or from `countof data + from` otherwise. If
  `length` is set to an integer, no more than this number of elements will be
  copied. If it is absent, all elements from `from` to the end of `data` will
  be copied. If `from` is outside `data`, an empty array is returned.

* Returns the specified subarray of `data`.

### `std.array.replace_slice(data, from, [length], replacement, [rfrom, [rlength]])`

* Replaces a subrange of `data` with `replacement` to create a new array.
  `from` specifies the start of the subrange to replace. If `from` is
  negative, it specifies an offset from the end of `data`. `length` denotes
  the maximum number of elements to replace. If it is set to `null`, all
  elements up to the end will be replaced. If `rfrom` is specified, the
  replacement array is determined as `slice(replacement, rfrom, rlength)`.
  This function returns a new array without modifying `data`.

* Returns a new array with the subrange replaced.

### `std.array.find(data, [from, [length]], [target])`

* Searches `data` for the first element `x`, for which either `target` is a
  function and `target(x)` is true, or `target` is not a function and
  `x == target` is true. The search operation is performed on the same
  subrange that would have been returned by `slice(data, from, length)`.

* Returns the subscript as an integer if a match is found, or `null`
  otherwise.

### `std.array.find_not(data, [from, [length]], [target])`

* Searches `data` for the first element `x`, for which either`target` is a
  function and `target(x)` is false, or `target` is not a function and
  `x == target` is false. The search operation is performed on the same
  subrange that would have been returned by `slice(data, from, length)`.

* Returns the subscript as an integer if a non-match is found, or `null`
  otherwise.

### `std.array.rfind(data, [from, [length]], [target])`

* Searches `data` for the last element `x`, for which either `target` is a
  function and `target(x)` is true, or `target` is not a function and
  `x == target` is true. The search operation is performed on the same
  subrange that would have been returned by `slice(data, from, length)`.

* Returns the subscript as an integer if a match is found, or `null`
  otherwise.

### `std.array.rfind_not(data, [from, [length]], [target])`

* Searches `data` for the last element `x`, for which either`target` is a
  function and `target(x)` is false, or `target` is not a function and
  `x == target` is false. The search operation is performed on the same
  subrange that would have been returned by `slice(data, from, length)`.

* Returns the subscript as an integer if a non-match is found, or `null`
  otherwise.

### `std.array.count(data, [from, [length]], [target])`

* Searches `data` for the last element `x`, for which either`target` is a
  function and `target(x)` is true, or `target` is not a function and
  `x == target` is true, then returns the number of all such elements. The
  search operation is performed on the same subrange that would have been
  returned by `slice(data, from, length)`.

* Returns the number of elements as an integer, which is always non-negative.

### `std.array.count_not(data, [from, [length]], predictor)`

* Searches `data` for the last element `x`, for which either`target` is a
  function and `target(x)` is false, or `target` is not a function and
  `x == target` is false, then returns the number of all such elements. The
  search operation is performed on the same subrange that would have been
  returned by `slice(data, from, length)`.

* Returns the number of elements as an integer, which is always non-negative.

### `std.array.exclude(data, [from, [length]], [target])`

* Removes every element `x`, for which either `target` is a function and
  `target(x)` is true, or `target` is not a function and `x == target` is
  true. The search operation is performed on the same subrange that would
  have been returned by `slice(data, from, length)`. This function returns a
  new array without modifying `data`.

* Returns a new array with all matches removed.

### `std.array.is_sorted(data, [comparator])`

* Checks whether there is no pair of adjacent elements in `data` such that
  the first one is greater than or unordered with the second one. Elements
  are compared using `comparator`, which shall be a binary function that
  returns a negative integer if the first argument is less than the second
  one, a positive integer if the first argument is greater than the second
  one, or zero if the arguments are equal; other values indicate that the
  arguments are unordered. If no `comparator` is provided, the built-in
  3-way comparison operator is assumed. An array that contains no elements is
  considered sorted.

* Returns `true` if `data` is sorted or empty, or `false` otherwise.

### `std.array.binary_search(data, [target], [comparator])`

* Finds the first element in `data` that equals `target`. The requirement of
  a user-defined `comparator` is the same as the `is_sorted()` function. It
  is required that `is_sorted(data, comparator)` shall yield `true` prior to
  this call; otherwise the result is unspecified.

* Returns the subscript as an integer if a match has been found, or `null`
  otherwise.

* Throws an exception if `data` has not been sorted properly. Be advised that
  there is no guarantee whether an exception will be thrown or not in this
  case.

### `std.array.lower_bound(data, [target], [comparator])`

* Finds the first element in `data` that is greater than or equal to `target`
  and precedes all elements that are less than `target`. The requirement of a
  user-defined `comparator` is the same as the `is_sorted()` function. It is
  required that `is_sorted(data, comparator)` shall yield `true` before this
  call; otherwise the result is unspecified.

* Returns the subscript of the lower bound as an integer.

* Throws an exception if `data` has not been sorted properly. Be advised that
  there is no guarantee whether an exception will be thrown or not in this
  case.

### `std.array.upper_bound(data, [target], [comparator])`

* Finds the first element in `data` that is greater than `target` and also
  precedes all elements that are less than or equal to `target`. The
  requirement of a user-defined `comparator` is the same as the `is_sorted()`
  function. It is required that `is_sorted(data, comparator)` shall yield
  `true` before this call; otherwise the result is unspecified.

* Returns the subscript of the upper bound as an integer.

* Throws an exception if `data` has not been sorted properly. Be advised that
  there is no guarantee whether an exception will be thrown or not in this
  case.

### `std.array.equal_range(data, [target], [comparator])`

* Gets the range of elements equivalent to `target` in `data` as a single
  function call. This function is a more efficient implementation of `[
  lower_bound(data, target, comparator), upper_bound(data, target,
  comparator) - lower_bound(data, target, comparator) ]`.

* Returns an array of two integers, the first of which specifies the lower
  bound and the second of which specifies the difference from the lower bound
  to the upper bound.

* Throws an exception if `data` has not been sorted properly. Be advised that
  there is no guarantee whether an exception will be thrown or not in this
  case.

### `std.array.sort(data, [comparator])`

* Merge-sorts elements in `data` in ascending order. The requirement of a
  user-defined `comparator` is the same as the `is_sorted()` function. This
  function returns a new array without modifying `data`.

* Returns the sorted array.

* Throws an exception if `data` has not been sorted properly.

### `std.array.usort(data, [comparator])`

* Merge-sorts elements in `data` in ascending order and removes duplicate
  elements. The requirement of a user-defined `comparator` is the same as the
  `is_sorted()` function. This function returns a new array without modifying
  `data`.

* Returns the sorted array, with subsequent duplicate elements removed.

* Throws an exception if `data` has not been sorted properly.

### `std.array.ksort(object, [comparator])`

* Creates an array by sorting elements in `object` by key, as defined as

  ```
  std.array.ksort = func(object, comparator) {
    var pairs = [];

    for(each k: v -> object)
      pairs[$] = [k, v];

    return this.sort(pairs,
      func(x, y) {
        return (comparator == null)
               ? x[0] <=> y[0]
               : comparator(x[0], y[0]);
      });
  };
  ```

* Returns sorted key-value pairs, as an array of two-element arrays.

### `std.array.max_of(data, [comparator])`

* Finds the maximum element in `data`. The requirement of a user-defined
  `comparator` is the same as the `is_sorted()` function. Elements that are
  unordered with the first element are ignored silently.

* Returns a copy of the maximum element, or `null` if `data` is empty.

### `std.array.min_of(data, [comparator])`

* Finds the minimum element in `data`. The requirement of a user-defined
  `comparator` is the same as the `is_sorted()` function. Elements that are
  unordered with the first element are ignored silently.

* Returns a copy of the minimum element, or `null` if `data` is empty.

### `std.array.reverse(data)`

* Reverses an array. This function returns a new array without modifying
  `data`.

* Returns an array with all elements in reverse order.

### `std.array.generate(generator, length)`

* Calls `generator` repeatedly up to `length` times, and returns an array
  consisting of all values that have been returned. `generator` shall be a
  binary function, where the first argument is the number of elements that
  have been generated, and the second argument is a copy of the previous
  element if one has been generated (or `null` for the first one).

* Returns an array of all new values.

### `std.array.shuffle(data, [seed])`

* Shuffles elements in `data` randomly. If `seed` is set to an integer, it
  will be used to initialize the internal pseudo random number generator, so
  the effect will be reproducible. If it is absent, a secure seed is
  generated. This function returns a new array without modifying `data`.

* Returns the shuffled array.

### `std.array.rotate(data, shift)`

* Rotates elements in `data`. The element at subscript `x` is moved to
  subscript `(x + shift) % countof data`. No element is added or removed.

* Returns the rotated array.

### `std.array.copy_keys(source)`

* Copies all keys from `source` object to create an array.

* Returns an array of all keys in `source`.

### `std.array.copy_values(source)`

* Copies all values from `source` object to create an array.

* Returns an array of all values in `source`.

## `std.numeric`

### `std.numeric.integer_max`

* Denotes the maximum value of an integer; `+0x7FFFFFFFFFFFFFFF`.

### `std.numeric.integer_min`

* Denotes the minimum value of an integer; `-0x8000000000000000`.

### `std.numeric.real_max`

* Denotes the maximum finite value of a real (floating-point) number,
  `+0x1FFFFFFFFFFFFFp+1023`.

### `std.numeric.real_min`

* Denotes the minimum finite value of a real (floating-point) number,
  `-0x1FFFFFFFFFFFFFp+1023`.

### `std.numeric.real_epsilon`

* Denotes the minimum finite value of a real (floating-point) number such
  that `1 + real_epsilon > 1`; `+0x1p-52`.

### `std.numeric.size_max`

* Denotes the maximum number of bytes in a string, or elements in an array.
  This value is implementation-dependent. Please be advised that there is no
  guarantee that allocation of a string of this length will actually succeed
  or not.

### `std.numeric.abs(value)`

* Gets the absolute value of `value`, which may be an integer or real number.
  Negating `integer_min` will cause an overflow. Sign bits of real numbers
  are removed which will never cause an overflow.

* Return the absolute value.

* Throws an exception if an overflow happens.

### `std.numeric.sign(value)`

* Gets the sign bit of `value`, which may be an integer or real number.

* Returns `true` if `value` is negative, `-0.0`, `-infinity` or `-nan`; or
  `false` otherwise.

### `std.numeric.is_finite(value)`

* Checks whether `value` is a finite number, which may be an integer or real
  number.

* Returns `true` if `value` is an integer, or is a finite real number; or
  `false` otherwise.

### `std.numeric.is_infinity(value)`

* Checks whether `value` is an infinity, which may be an integer or real
  number.

* Returns `true` if `value` is `+infinity` or `-infinity`, or `false`
  otherwise.

### `std.numeric.is_nan(value)`

* Checks whether `value` is a NaN, which may be an integer or real number.
  A NaN is unordered with everything, even with itself.

* Returns `true` if `value` is `+nan` or `-nan`, or `false` otherwise.

### `std.numeric.max(...)`

* Gets the maximum value of all arguments. `null` arguments are ignored.

* Returns a copy of the first maximum argument. If no argument is given,
  `null` is returned.

* Throws an exception if some arguments are unordered.

### `std.numeric.min(...)`

* Gets the minimum value of all arguments. `null` arguments are ignored.

* Returns a copy of the first minimum argument. If no argument is given,
  `null` is returned.

* Throws an exception if some arguments are unordered.

### `std.numeric.clamp(value, lower, upper)`

* Clamps `value` between `lower` and `upper`.

* Returns `lower` if `value < lower`, `upper` if `value > upper`, or `value`
  otherwise.

* Throws an exception if some arguments are unordered.

### `std.numeric.round(value)`

* Rounds `value`, which may be an integer or real number, to the nearest
  integral value; halfway values are rounded away from zero. If `value` is
  an integer, it is returned intact.

* Returns the rounded value.

### `std.numeric.iround(value)`

* Rounds `value`, which may be an integer or real number, to the nearest
  integer; halfway values are rounded away from zero. If `value` is already
  an integer, it is returned intact.

* Returns the rounded value as an integer.

* Throws an exception if the result cannot be represented as an
  integer.

### `std.numeric.floor(value)`

* Rounds `value`, which may be an integer or real number, to the nearest
  integral value towards negative infinity. If `value` is an integer, it is
  returned intact.

* Returns the rounded value.

### `std.numeric.ifloor(value)`

* Rounds `value`, which may be an integer or real number, to the nearest
  integer towards negative infinity. If `value` is already an integer, it is
  returned intact.

* Returns the rounded value as an integer.

* Throws an exception if the result cannot be represented as an
  integer.

### `std.numeric.ceil(value)`

* Rounds `value`, which may be an integer or real number, to the nearest
  integral value towards positive infinity. If `value` is an integer, it is
  returned intact.

* Returns the rounded value.

### `std.numeric.iceil(value)`

* Rounds `value`, which may be an integer or real number, to the nearest
  integer towards positive infinity. If `value` is already an integer, it is
  returned intact.

* Returns the rounded value as an integer.

* Throws an exception if the result cannot be represented as an
  integer.

### `std.numeric.trunc(value)`

* Rounds `value`, which may be an integer or real number, to the nearest
  integral value towards zero. If `value` is an integer, it is returned
  intact.

* Returns the rounded value.

### `std.numeric.itrunc(value)`

* Rounds `value`, which may be an integer or real number, to the nearest
  integer towards zero. If `value` is already an integer, it is returned
  intact.

* Returns the rounded value as an integer.

* Throws an exception if the result cannot be represented as an
  integer.

### `std.numeric.random([limit])`

* Generates a secure random real number, whose sign is the same with `limit`
  and whose absolute value is less than `abs(limit)`. The default value of
  `limit` is `1.0`.

* Returns a random real number.

* Throws an exception if `limit` is zero or non-finite.

### `std.numeric.remainder(x, y)`

* Calculates the IEEE floating-point remainder of division of `x` by `y`. The
  remainder is defined to be `x - q * y` where `q` is the quotient of `x` by
  `y` rounding to nearest.

* Returns the remainder as a real.

### `std.numeric.frexp(x)`

* Decomposes `x` into normalized fractional and exponential parts such that
  `x = frac * exp2(exp)`, where `frac` and `exp` denote the fraction and
  exponent respectively, and `frac` is always within the range `[0.5,1.0)`.
  If `x` is non-finite, `exp` is unspecified.

* Returns an array of two elements, whose first element is `frac` as a real
  number, and whose second element is `exp` as an integer.

### `std.numeric.ldexp(frac, exp)`

* Composes `frac` and `exp` to make a real number `x`, such that
  `x = frac * exp2(exp)`. `exp` must be an integer. This function is the
  inverse of `frexp()`.

* Returns the product as a real number.

### `std.numeric.rotl(m, x, n)`

* Rotates the rightmost `m` bits of `x` to the left by `n`. All arguments
  must be integers. This has the effect of shifting `x` to the left by `n`
  bits, then filling the vacuum in the right with the last `n` bits that have
  just been shifted out. `n` is modulo `m` so rotating by a negative count to
  the left is equivalent to rotating by its absolute value to the right. All
  the other bits are cleared.

* Returns the rotated value as an integer. If `m` is zero, zero is returned.

* Throws an exception if `m` is negative or greater than `64`.

### `std.numeric.rotr(m, x, n)`

* Rotates the rightmost `m` bits of `x` to the right by `n`. All arguments
  must be integers. This has the effect of shifting `x` to the right by `n`
  bits, then filling the vacuum in the left with the last `n` bits that have
  just been shifted out. `n` is modulo `m` so rotating by a negative count to
  the right is equivalent to rotating by its absolute value to the left. All
  the other bits are zeroed.

* Returns the rotated value as an integer. If `m` is zero, zero is returned.

* Throws an exception if `m` is negative or greater than `64`.

### `std.numeric.format(value, [base, [ebase]])`

* Converts an integer or real number to a string, where the significand is in
  `base` and the exponent is in `ebase`. This function writes as many digits
  as necessary to avoid a precision loss. The default value of `base` is ten.
  If `ebase` is absent, no exponent will be emitted. If `ebase` is specified,
  an exponent is appended to the significand as follows: If `value` is an
  integer, the significand is kept as short as possible; otherwise (when it
  is a real number), it is written in scientific notation; in both cases, the
  exponent comprises at least two digits with an explicit sign. The result is
  exact as long as `base` is a power of two.

* Returns a numeric string converted from `value`.

* Throws an exception if `base` is neither `2` nor `10` nor `16`, or if
  `ebase` is not `10` for decimal, or if `ebase` is not `2` for binary or
  hexadecimal.

### `std.numeric.parse(text)`

* Parses `text` for a number. `text` shall be a string. Leading and trailing
  blank characters are ignored. If the string is empty, this function fails;
  otherwise, it shall match one of the following regular expressions:

  * infinity:     `[+-]?infinity`
  * NaN:          `[+-]?nan`
  * decimal:      `[+-]?[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)?`
  * binary:       `[+-]?0[bB][01]+(\.[01]+)?([eE][-+]?[0-9]+)?`
  * hexadecimal:  `[+-]?0x[0-9a-fA-F]+(\.[0-9a-fA-F]+)?([pP][-+]?[0-9]+)?`

  If the string does not match any of the above, this function fails. The
  result is an integer if `text` contains no decimal point and the value is
  representable as an integer, and is a real number otherwise.

* Returns the numeric value converted from `text`.

* Throws an exception on failure.

### `std.numeric.pack_i8(values)`

* Packs a series of 8-bit integers into a string. `values` can be either an
  integer or an array of integers, all of which are truncated to 8 bits then
  copied into a string.

* Returns the packed string.

### `std.numeric.unpack_i8(text)`

* Unpacks 8-bit integers from a string. `text` is re-interpreted as a series
  of contiguous signed 8-bit integers, all of which are sign-extended to 64
  bits then copied into an array.

* Returns an array of unpacked integers.

### `std.numeric.pack_i16be(values)`

* Packs a series of 16-bit integers into a string, in big-endian byte order.
  `values` can be either an integer or an array of integers, all of which are
  truncated to 16 bits then copied into a string.

* Returns the packed string.

### `std.numeric.unpack_i16be(text)`

* Unpacks 16-bit integers from a string, in big-endian byte order. `text` is
  re-interpreted as a series of contiguous signed 16-bit integers, all of
  which are sign-extended to 64 bits then copied into an array.

* Returns an array of unpacked integers.

* Throws an exception if the length of `text` is not a multiple of 2.

### `std.numeric.pack_i16le(values)`

* Packs a series of 16-bit integers into a string, in little-endian byte
  order. `values` can be either an integer or an array of integers, all of
  which are truncated to 16 bits then copied into a string.

* Returns the packed string.

### `std.numeric.unpack_i16le(text)`

* Unpacks 16-bit integers from a string, in little-endian byte order. `text`
  is re-interpreted as a series of contiguous signed 16-bit integers, all of
  which are sign-extended to 64 bits then copied into an array.

* Returns an array of unpacked integers.

* Throws an exception if the length of `text` is not a multiple of 2.

### `std.numeric.pack_i32be(values)`

* Packs a series of 32-bit integers into a string, in big-endian byte order.
  `values` can be either an integer or an array of integers, all of which are
  truncated to 32 bits then copied into a string.

* Returns the packed string.

### `std.numeric.unpack_i32be(text)`

* Unpacks 32-bit integers from a string, in big-endian byte order. `text` is
  re-interpreted as a series of contiguous signed 32-bit integers, all of
  which are sign-extended to 64 bits then copied into an array.

* Returns an array of unpacked integers.

* Throws an exception if the length of `text` is not a multiple of 4.

### `std.numeric.pack_i32le(values)`

* Packs a series of 32-bit integers into a string, in little-endian byte
  order. `values` can be either an integer or an array of integers, all of
  which are truncated to 32 bits then copied into a string.

* Returns the packed string.

### `std.numeric.unpack_i32le(text)`

* Unpacks 32-bit integers from a string, in little-endian byte order. `text`
  is re-interpreted as a series of contiguous signed 32-bit integers, all of
  which are sign-extended to 64 bits then copied into an array.

* Returns an array of unpacked integers.

* Throws an exception if the length of `text` is not a multiple of 4.

### `std.numeric.pack_i64be(values)`

* Packs a series of 64-bit integers into a string, in big-endian byte order.
  `values` can be either an integer or an array of integers, all of which are
  copied into a string.

* Returns the packed string.

### `std.numeric.unpack_i64be(text)`

* Unpacks 64-bit integers from a string, in big-endian byte order. `text` is
  re-interpreted as a series of contiguous signed 64-bit integers, all of
  which are copied into an array.

* Returns an array of unpacked integers.

* Throws an exception if the length of `text` is not a multiple of 8.

### `std.numeric.pack_i64le(values)`

* Packs a series of 64-bit integers into a string, in little-endian byte
  order. `values` can be either an integer or an array of integers, all of
  which are copied into a string.

* Returns the packed string.

### `std.numeric.unpack_i64le(text)`

* Unpacks 64-bit integers from a string, in little-endian byte order. `text`
  is re-interpreted as a series of contiguous signed 64-bit integers, all of
  which are copied into an array.

* Returns an array of unpacked integers.

* Throws an exception if the length of `text` is not a multiple of 8.

### `std.numeric.pack_f32be(values)`

* Packs a series of 32-bit real numbers into a string, in big-endian byte
  order. `values` can be either a real number or an array of real numbers,
  all of which are converted to single precision then copied into a string.

* Returns the packed string.

### `std.numeric.unpack_f32be(text)`

* Unpacks 32-bit real numbers from a string, in big-endian byte order. `text`
  is re-interpreted as a series of contiguous single-precision floating-point
  numbers, all of which are converted to double precision then copied into an
  array.

* Returns an array of unpacked real numbers.

* Throws an exception if the length of `text` is not a multiple of 4.

### `std.numeric.pack_f32le(values)`

* Packs a series of 32-bit real numbers into a string, in little-endian byte
  order. `values` can be either a real number or an array of real numbers,
  which are converted to single precision then copied into a string.

* Returns the packed string.

### `std.numeric.unpack_f32le(text)`

* Unpacks 32-bit real numbers from a string, in little-endian byte order.
  `text` is re-interpreted as a series of contiguous single-precision
  floating-point numbers, all of which are converted to double precision then
  copied into an array.

* Returns an array of unpacked real numbers.

* Throws an exception if the length of `text` is not a multiple of 4.

### `std.numeric.pack_f64be(values)`

* Packs a series of 64-bit real numbers into a string, in big-endian byte
  order. `values` can be either a real number or an array of real numbers,
  all of which are copied into a string.

* Returns the packed string.

### `std.numeric.unpack_f64be(text)`

* Unpacks 64-bit real numbers from a string, in big-endian byte order. `text`
  is re-interpreted as a series of contiguous single-precision floating-point
  numbers, all of which are copied into an array.

* Returns an array of unpacked real numbers.

* Throws an exception if the length of `text` is not a multiple of 8.

### `std.numeric.pack_f64le(values)`

* Packs a series of 64-bit real numbers into a string, in little-endian byte
  order. `values` can be either a real number or an array of real numbers,
  all of which are copied into a string.

* Returns the packed string.

### `std.numeric.unpack_f64le(text)`

* Unpacks 64-bit real numbers from a string, in little-endian byte order.
  `text` is re-interpreted as a series of contiguous single-precision
  floating-point numbers, all of which are copied into an array.

* Returns an array of unpacked real numbers.

* Throws an exception if the length of `text` is not a multiple of 8.

## `std.math`

### `std.math.pi`

* Denotes the ratio of a circle's circumference to its diameter as a real
  number. The value is `3.1415926535897932384626433832795`.

### `std.math.rad`

* Denotes the number of degrees in a radian as a real number. The value is
  `57.295779513082320876798154814105`.

### `std.math.deg`

* Denotes the number of radians in a degree as a real number. The value is
  `0.01745329251994329576923690768489`.

### `std.math.e`

* Denotes the base of the natural logarithm as a real number. The value is
  `2.7182818284590452353602874713527`.

### `std.math.sqrt2`

* Denotes the square root of two as a real number. The value is
  `1.4142135623730950488016887242097`.

### `std.math.sqrt3`

* Denotes the square root of three as a real number. The value is
  `1.7320508075688772935274463415059`.

### `std.math.cbrt2`

* Denotes the cube root of two as a real number. The value is
  `1.2599210498948731647672106072782`.

### `std.math.lg2`

* Denotes the common logarithm of two as a real number. The value is
  `0.30102999566398119521373889472449`.

### `std.math.lb10`

* Denotes the binary logarithm of ten as a real number. The value is
   `3.3219280948873623478703194294894`.

### `std.math.exp([base], y)`

* Calculates `base` raised to the `y`-th power. The default value of `base`
  is the constant `e`.

* Returns the power as a real number.

### `std.math.log([base], x)`

* Calculates the logarithm base-`base` of `x`. The default value of `base` is
  the constant `e`.

* Returns the logarithm as a real number.

### `std.math.expm1(y)`

* Calculates `exp(y) - 1` without losing precision when `y` is close to zero.

* Returns the result as a real number.

### `std.math.log1p(x)`

* Calculates `log(1 + x)` without losing precision when `x` is close to zero.

* Returns the result as a real number.

### `std.math.sin(x)`

* Calculates the sine of `x` in radians.

* Returns the result as a real number.

### `std.math.cos(x)`

* Calculates the cosine of `x` in radians.

* Returns the result as a real number.

### `std.math.tan(x)`

* Calculates the tangent of `x` in radians.

* Returns the result as a real number.

### `std.math.asin(x)`

* Calculates the arc sine of `x` in radians.

* Returns the result as a real number.

### `std.math.acos(x)`

* Calculates the arc cosine of `x` in radians.

* Returns the result as a real number.

### `std.math.atan(x)`

* Calculates the arc tangent of `x` in radians.

* Returns the result as a real number.

### `std.math.atan2(y, x)`

* Calculates the angle of the vector `(x,y)` in radians.

* Returns the result as a real number.

### `std.math.hypot(...)`

* Calculates the length of the n-dimension vector defined by all arguments.
  `null` is ignored.

* Returns the length as a real number. If no argument is given, zero is
  returned; if any argument is an infinity, a positive infinity is returned;
  if any argument is a NaN, a NaN is returned.

### `std.math.sinh(x)`

* Calculates the hyperbolic sine of `x`.

* Returns the result as a real number.

### `std.math.cosh(x)`

* Calculates the hyperbolic cosine of `x`.

* Returns the result as a real number.

### `std.math.tanh(x)`

* Calculates the hyperbolic tangent of `x`.

* Returns the result as a real number.

### `std.math.asinh(x)`

* Calculates the inverse hyperbolic sine of `x`.

* Returns the result as a real number.

### `std.math.acosh(x)`

* Calculates the inverse hyperbolic cosine of `x`.

* Returns the result as a real number.

### `std.math.atanh(x)`

* Calculates the inverse hyperbolic tangent of `x`.

* Returns the result as a real number.

### `std.math.erf(x)`

* Calculates the error function of `x`.

* Returns the result as a real number.

### `std.math.cerf(x)`

* Calculates the complementary error function of `x`.

* Returns the result as a real number.

### `std.math.gamma(x)`

* Calculates the Gamma function of `x`.

* Returns the result as a real number.

### `std.math.lgamma(x)`

* Calculates the natural logarithm of the absolute value of the Gamma
  function of `x`.

* Returns the result as a real number.

## `std.filesystem`

### `std.filesystem.get_real_path(path)`

* Resolves `path` to an absolute one. The result is a canonical path that
  contains no symbolic links. The path must be accessible.

* Returns the absolute path as a string.

* Throws an exception if `path` is invalid or inaccessible.

### `std.filesystem.get_properties(path)`

* Retrieves properties of the file or directory that is denoted by `path`.

* Returns an object of the following fields:

  * `device`         integer: unique device ID on this machine
  * `inode`          integer: unique file ID on this device
  * `link_count`     integer: number of hard links
  * `is_directory`   boolean: whether this is a directory
  * `is_symbolic`    boolean: whether this is a symbolic link
  * `size`           integer: size of contents in bytes
  * `size_on_disk`   integer: size of storage on disk in bytes
  * `time_accessed`  integer: timestamp of last access
  * `time_modified`  integer: timestamp of last modification

* Throws an exception in case of an error.

### `std.filesystem.move(path_new, path_old)`

* Moves (renames) the file or directory at `path_old` to `path_new`.

* Throws an exception on failure.

### `std.filesystem.remove_recursive(path)`

* Removes the file, or directory with all its contents, at `path`.

* Returns the number of files and directories that have been successfully
  removed in total. If `path` does not reference an existent file or
  directory, `0` is returned.

* Throws an exception if the file or directory at `path` cannot be removed.

### `std.filesystem.glob(pattern)`

* Finds all paths that match `pattern`. If a string ends with '/', it should
  designate a directory; otherwise, it should designate a file.

* Returns matches as an array of strings. If no such path exists, an empty
  array is returned.

* Throws an exception in case of a system error.

### `std.filesystem.list(path)`

* Lists the contents of the directory designated by `path`.

* Returns an object containing all entries of the directory at `path`,
  excluding the special subdirectories `"."` and `".."`. For each field, its
  key is the filename and the value is an object of the following fields:

  * `inode`          integer: unique file ID on this device
  * `is_directory`   boolean: whether this is a directory
  * `is_symbolic`    boolean: whether this is a symbolic link

* Throws an exception if `path` does not designate a directory, or some other
  errors occur.

### `std.filesystem.create_directory(path)`

* Creates a directory at `path`. Its parent directory must exist and must be
  accessible. This function does not fail if either a directory or a symbolic
  link to a directory already exists on `path`.

* Returns `1` if a new directory has been created, or `0` if a directory
  already exists.

* Throws an exception if `path` designates a non-directory, or some other
  errors occur.

### `std.filesystem.remove_directory(path)`

* Removes the directory at `path`. The directory must be empty. This function
  fails if `path` does not designate a directory.

* Returns `1` if a directory has been removed successfully, or `0` if no
  such directory exists.

* Throws an exception if `path` designates a non-directory, or some other
  errors occur.

### `std.filesystem.read(path, [offset, [limit]])`

* Reads the file at `path` in binary mode. The read operation starts from the
  byte offset that is denoted by `offset` if it is specified, or from the
  beginning of the file otherwise. If `limit` is specified, no more than this
  number of bytes will be read.

* Returns the bytes that have been read as a string.

* Throws an exception if `offset` is negative, or a read error occurs.

### `std.filesystem.stream(path, callback, [offset, [limit]])`

* Reads the file at `path` in binary mode and invokes `callback` with the
  data that have been read repeatedly. `callback` shall be a binary function,
  whose first argument is the absolute offset of the data block that has been
  read, and whose second argument is the block as a string. Data may be
  transferred in variable blocks; the caller shall make no assumption about
  the number of times that `callback` will be called, or the size of each
  individual block. The read operation starts from the byte offset that is
  denoted by `offset` if it is specified, or from the beginning of the file
  otherwise. If `limit` is specified, no more than this number of bytes will
  be read.

* Returns the number of bytes that have been read as an integer.

* Throws an exception if `offset` is negative, or a read error occurs.

### `std.filesystem.write(path, [offset], data)`

* Writes the file at `path` in binary mode. The write operation starts from
  the byte offset that is denoted by `offset` if it is specified, or from the
  beginning of the file otherwise. The file is truncated before the write
  operation; any existent contents after the write point are discarded. If
  `data` is only written partially, this function fails.

* Throws an exception if `offset` is negative, or a write error occurs.

### `std.filesystem.append(path, data, [exclusive])`

* Writes the file at `path` in binary mode. The write operation starts from
  the end of the file; existent contents of the file are left intact. If
  `exclusive` is `true` and a file exists on `path`, this function fails. If
  `data` is only written partially, this function fails.

* Throws an exception if a write error occurs.

### `std.filesystem.copy_file(path_new, path_old)`

* Copies the file at `path_old` to `path_new`. If `path_old` is a symbolic
  link, its target file is copied, instead of the symbolic link itself. This
  function fails if `path_old` designates a directory.

* Throws an exception on failure.

### `std.filesystem.remove_file(path)`

* Removes the file at `path`. This function fails if `path` designates a
  directory.

* Returns `1` if a file has been removed successfully, or `0` if no such file
  exists.

* Throws an exception if `path` designates a directory, or some other errors
  occur.

## `std.checksum`

### `std.checksum.CRC32()`

* Creates a CRC-32 hasher according to ISO/IEC 3309. The CRC divisor is
  `0x04C11DB7` (or `0xEDB88320` in reverse form).

* Returns the hasher as an object consisting of the following members:

  * `update(data)`
  * `finish()`
  * `clear()`

  The function `update()` is used to put a byte string into the hasher. After
  all data have been put, the function `finish()` extracts the checksum as an
  integer (whose high-order 32 bits are always zeroes), and then resets the
  hasher, making it suitable for further data as if it had just been created.
  The function `clear()` discards input data and resets the hasher to its
  initial state.

### `std.checksum.crc32(data)`

* Calculates the CRC-32 checksum of `data` which must be a string, as if by

  ```
  std.checksum.crc32 = func(data) {
    var h = this.CRC32();
    h.update(data);
    return h.finish();
  };
  ```

* Returns the CRC-32 checksum as an integer. The high-order 32 bits are
  always zeroes.

### `std.checksum.crc32_file(path)`

* Calculates the CRC-32 checksum of the file denoted by `path`, as if by

  ```
  std.checksum.crc32_file = func(path) {
    var h = this.CRC32();
    this.stream(path, func(off, data) { h.update(data);  });
    return h.finish();
  };
  ```

* Returns the CRC-32 checksum as an integer. The high-order 32 bits are
  always zeroes.

* Throws an exception if a read error occurs.

### `std.checksum.Adler32()`

* Creates a Adler-32 hasher according to ISO/IEC 3309. This algorithm is used
  by zlib.

* Returns the hasher as an object consisting of the following members:

  * `update(data)`
  * `finish()`
  * `clear()`

  The function `update()` is used to put a byte string into the hasher. After
  all data have been put, the function `finish()` extracts the checksum as an
  integer (whose high-order 32 bits are always zeroes), and then resets the
  hasher, making it suitable for further data as if it had just been created.
  The function `clear()` discards input data and resets the hasher to its
  initial state.

### `std.checksum.adler32(data)`

* Calculates the Adler-32 checksum of `data` which must be a string, as if by

  ```
  std.checksum.adler32 = func(data) {
    var h = this.Adler32();
    h.update(data);
    return h.finish();
  };
  ```

* Returns the Adler-32 checksum as an integer. The high-order 32 bits are
  always zeroes.

### `std.checksum.adler32_file(path)`

* Calculates the Adler-32 checksum of the file denoted by `path`, as if by

  ```
  std.checksum.adler32_file = func(path) {
    var h = this.Adler32();
    this.stream(path, func(off, data) { h.update(data);  });
    return h.finish();
  };
  ```

* Returns the Adler-32 checksum as an integer. The high-order 32 bits are
  always zeroes.

* Throws an exception if a read error occurs.

### `std.checksum.FNV1a32()`

* Creates a 32-bit Fowler-Noll-Vo (a.k.a. FNV) hasher of the 32-bit FNV-1a
  variant. The FNV prime is `16777619` and the offset basis is `2166136261`.

* Returns the hasher as an object consisting of the following members:

  * `update(data)`
  * `finish()`
  * `clear()`

  The function `update()` is used to put a byte string into the hasher. After
  all data have been put, the function `finish()` extracts the checksum as an
  integer (whose high-order 32 bits are always zeroes), and then resets the
  hasher, making it suitable for further data as if it had just been created.
  The function `clear()` discards input data and resets the hasher to its
  initial state.

### `std.checksum.fnv1a32(data)`

* Calculates the 32-bit FNV-1a checksum of `data` which must be a string, as
  if by

  ```
  std.checksum.fnv1a32 = func(data) {
    var h = this.FNV1a32();
    h.update(data);
    return h.finish();
  };
  ```

* Returns the 32-bit FNV-1a checksum as an integer. The high-order 32 bits
  are always zeroes.

### `std.checksum.fnv1a32_file(path)`

* Calculates the 32-bit FNV-1a checksum of the file denoted by `path`, as if
  by

  ```
  std.checksum.fnv1a32_file = func(path) {
    var h = this.FNV1a32();
    this.stream(path, func(off, data) { h.update(data);  });
    return h.finish();
  };
  ```

* Returns the 32-bit FNV-1a checksum as an integer. The
  high-order 32 bits are always zeroes.

* Throws an exception if a read error occurs.

### `std.checksum.MD5()`

* Creates an MD5 hasher.

* Returns the hasher as an object consisting of the following members:

  * `update(data)`
  * `finish()`
  * `clear()`

  The function `update()` is used to put a byte string into the hasher. After
  all data have been put, the function `finish()` extracts the checksum as a
  string of 32 hexadecimal digits, then resets the hasher, making it suitable
  for further data as if it had just been created. The function `clear()`
  discards all input data and resets the hasher to its initial state.

### `std.checksum.md5(data)`

* Calculates the MD5 checksum of `data` which must be a string, as if by

  ```
  std.checksum.md5 = func(data) {
    var h = this.MD5();
    h.update(data);
    return h.finish();
  };
  ```

* Returns the MD5 checksum as a string of 32 hexadecimal digits.

### `std.checksum.md5_file(path)`

* Calculates the MD5 checksum of the file denoted by `path`, as if by

  ```
  std.checksum.md5_file = func(path) {
    var h = this.MD5();
    this.stream(path, func(off, data) { h.update(data);  });
    return h.finish();
  };
  ```

* Returns the MD5 checksum as a string of 32 hexadecimal digits.

* Throws an exception if a read error occurs.

### `std.checksum.SHA1()`

* Creates an SHA-1 hasher.

* Returns the hasher as an object consisting of the following members:

  * `update(data)`
  * `finish()`
  * `clear()`

  The function `update()` is used to put a byte string into the hasher. After
  all data have been put, the function `finish()` extracts the checksum as a
  string of 40 hexadecimal digits, then resets the hasher, making it suitable
  for further data as if it had just been created. The function `clear()`
  discards all input data and resets the hasher to its initial state.

### `std.checksum.sha1(data)`

* Calculates the SHA-1 checksum of `data` which must be a string, as if by

  ```
  std.checksum.sha1 = func(data) {
    var h = this.SHA1();
    h.update(data);
    return h.finish();
  };
  ```

* Returns the SHA-1 checksum as a string of 40 hexadecimal digits in
  uppercase.

### `std.checksum.sha1_file(path)`

* Calculates the SHA-1 checksum of the file denoted by `path`, as if by

  ```
  std.checksum.sha1_file = func(path) {
    var h = this.SHA1();
    this.stream(path, func(off, data) { h.update(data);  });
    return h.finish();
  };
  ```

* Returns the SHA-1 checksum as a string of 40 hexadecimal digits in
  uppercase.

* Throws an exception if a read error occurs.

### `std.checksum.SHA224()`

* Creates an SHA-224 hasher.

* Returns the hasher as an object consisting of the following members:

  * `update(data)`
  * `finish()`
  * `clear()`

  The function `update()` is used to put a byte string into the hasher. After
  all data have been put, the function `finish()` extracts the checksum as a
  string of 56 hexadecimal digits, then resets the hasher, making it suitable
  for further data as if it had just been created. The function `clear()`
  discards all input data and resets the hasher to its initial state.

### `std.checksum.sha224(data)`

* Calculates the SHA-224 checksum of `data` which must be a string, as if by

  ```
  std.checksum.sha224 = func(data) {
    var h = this.SHA224();
    h.update(data);
    return h.finish();
  };
  ```

* Returns the SHA-224 checksum as a string of 56 hexadecimal digits in
  uppercase.

### `std.checksum.sha224_file(path)`

* Calculates the SHA-224 checksum of the file denoted by `path`, as if by

  ```
  std.checksum.sha224_file = func(path) {
    var h = this.SHA224();
    this.stream(path, func(off, data) { h.update(data);  });
    return h.finish();
  };
  ```

* Returns the SHA-224 checksum as a string of 56 hexadecimal digits in
  uppercase.

* Throws an exception if a read error occurs.

### `std.checksum.SHA256()`

* Creates an SHA-256 hasher.

* Returns the hasher as an object consisting of the following members:

  * `update(data)`
  * `finish()`
  * `clear()`

  The function `update()` is used to put a byte string into the hasher. After
  all data have been put, the function `finish()` extracts the checksum as a
  string of 64 hexadecimal digits, then resets the hasher, making it suitable
  for further data as if it had just been created. The function `clear()`
  discards all input data and resets the hasher to its initial state.

### `std.checksum.sha256(data)`

* Calculates the SHA-256 checksum of `data` which must be a string, as if by

  ```
  std.checksum.sha256 = func(data) {
    var h = this.SHA256();
    h.update(data);
    return h.finish();
  };
  ```

* Returns the SHA-256 checksum as a string of 64 hexadecimal digits in
  uppercase.

### `std.checksum.sha256_file(path)`

* Calculates the SHA-256 checksum of the file denoted by `path`, as if by

  ```
  std.checksum.sha256_file = func(path) {
    var h = this.SHA256();
    this.stream(path, func(off, data) { h.update(data);  });
    return h.finish();
  };
  ```

* Returns the SHA-256 checksum as a string of 64 hexadecimal digits in
  uppercase.

* Throws an exception if a read error occurs.

### `std.checksum.SHA384()`

* Creates an SHA-384 hasher.

* Returns the hasher as an object consisting of the following members:

  * `update(data)`
  * `finish()`
  * `clear()`

  The function `update()` is used to put a byte string into the hasher. After
  all data have been put, the function `finish()` extracts the checksum as a
  string of 96 hexadecimal digits, then resets the hasher, making it suitable
  for further data as if it had just been created. The function `clear()`
  discards all input data and resets the hasher to its initial state.

### `std.checksum.sha384(data)`

* Calculates the SHA-384 checksum of `data` which must be a string, as if by

  ```
  std.checksum.sha384 = func(data) {
    var h = this.SHA384();
    h.update(data);
    return h.finish();
  };
  ```

* Returns the SHA-384 checksum as a string of 96 hexadecimal digits in
  uppercase.

### `std.checksum.sha384_file(path)`

* Calculates the SHA-384 checksum of the file denoted by `path`, as if by

  ```
  std.checksum.sha384_file = func(path) {
    var h = this.SHA384();
    this.stream(path, func(off, data) { h.update(data);  });
    return h.finish();
  };
  ```

* Returns the SHA-384 checksum as a string of 96 hexadecimal digits in
  uppercase.

* Throws an exception if a read error occurs.

### `std.checksum.SHA512()`

* Creates an SHA-512 hasher.

* Returns the hasher as an object consisting of the following members:

  * `update(data)`
  * `finish()`
  * `clear()`

  The function `update()` is used to put a byte string into the hasher. After
  all data have been put, the function `finish()` extracts the checksum as a
  string of 128 hexadecimal digits, then resets the hasher, making it suitable
  for further data as if it had just been created. The function `clear()`
  discards all input data and resets the hasher to its initial state.

### `std.checksum.sha512(data)`

* Calculates the SHA-512 checksum of `data` which must be a string, as if by

  ```
  std.checksum.sha512 = func(data) {
    var h = this.SHA512();
    h.update(data);
    return h.finish();
  };
  ```

* Returns the SHA-512 checksum as a string of 128 hexadecimal digits in
  uppercase.

### `std.checksum.sha512_file(path)`

* Calculates the SHA-512 checksum of the file denoted by `path`, as if by

  ```
  std.checksum.sha512_file = func(path) {
    var h = this.SHA512();
    this.stream(path, func(off, data) { h.update(data);  });
    return h.finish();
  };
  ```

* Returns the SHA-512 checksum as a string of 128 hexadecimal digits in
  uppercase.

* Throws an exception if a read error occurs.

## `std.json`

### `std.json.format([value], [indent], [json5])`

* Converts a value to a string in the JSON format, according to [RFC 7159](
  https://www.rfc-editor.org/rfc/rfc7159.html). This function generates text
  that conforms to JSON strictly; values whose types cannot be represented in
  JSON are replaced with `null` in arrays, and are discarded elsewhere. If
  `indent` is specified as a non-empty string, it is used for each level of
  indention following a line break; if `indent` is specified as a positive
  integer, it is clamped between `1` and `10` inclusively and this function
  behaves as if a string of this number of spaces was set. If `indent` is an
  empty string or zero, or is absent, no indention or line break will be
  inserted. If `json5` is set to `true`, the [JSON5](https://json5.org/)
  alternative format is used.

* Returns the formatted text as a string.

### `std.json.parse(text)`

* Parses a string containing data encoded in the JSON format. This function
  reuses the tokenizer of Asteria and allows quite a few extensions, many of
  which are also supported by JSON5:

  * Single-line and multiple-line comments are allowed.
  * Binary and hexadecimal numbers are allowed.
  * Numbers can have binary exponents.
  * Infinities and NaNs are allowed.
  * Numbers can start with plus signs.
  * Strings and object keys may be single-quoted.
  * Escape sequences (including UTF-32) are allowed in strings.
  * Element lists of arrays and objects may end in commas.
  * Object keys may be unquoted if they are valid identifiers.

* Returns the parsed value.

* Throws an exception if the string is invalid.

### `std.json.parse_file(path)`

* Parses the contents of the file denoted by `path` as a JSON string for a
  value. This function behaves identically to `parse()` otherwise.

* Returns the parsed value.

* Throws an exception if a read error occurs, or if the string is invalid.

## `std.ini`

### `std.ini.format(object)`

* Converts `object` to a string in the INI format. The INI format is too
  primitive to support complex structures, which limits the domain of
  representable values. `object` shall be an object of objects, where up to
  two nesting levels are allowed. Boolean, integer, real and string values
  are written in their string forms; the others are ignored silently.

* Returns the formatted text as a string.

### `std.ini.parse(text)`

* Parses a string containing data encoded in the INI format and converts them
  to an object.

* Returns the parsed value as an object. Each section in the INI string
  corresponds to a subobject, and each key in this section corresponds to a
  string in this subobject. No type conversion is performed. Comments may
  start with either `;` or `#`.

* Throws an exception if the string is invalid.

### `std.ini.parse_file(path)`

* Parses the contents of the file denoted by `path` as an INI string for an
  object. This function behaves identically to `parse()` otherwise.

* Returns the parsed value as an object.

* Throws an exception if a read error occurs, or if the string is invalid.

## `std.csv`

### `std.csv.format(array)`

* Converts `array` to a string in the CSV format. The CSV format is too
  primitive to support complex structures, which limits the domain of
  representable values. `array` shall be an array of arrays, where up to two
  nesting levels are allowed. Boolean, integer, real and string values are
  written in their string forms; the others are ignored silently.

* Returns the formatted text as a string.

### `std.csv.parse(text)`

* Parses a string containing data encoded in the CSV format and converts it
  to an array.

* Returns the parsed value as an array. Each section in the CSV string
  corresponds to a subarray, and each key in this section corresponds to a
  string in this subarray. No type conversion is performed.

* Throws an exception if the string is invalid.

### `std.csv.parse_file(path)`

* Parses the contents of the file denoted by `path` as an CSV string for an
  array. This function behaves identically to `parse()` otherwise.

* Returns the parsed value as an array.

* Throws an exception if a read error occurs, or if the string is invalid.

## `std.io`

### `std.io.getc()`

* Reads a UTF code point from standard input.

* Returns the code point that has been read as an integer. If the end of
  input is encountered, `null` is returned.

* Throws an exception if standard input is binary-oriented, or if a read
  error occurs.

### `std.io.getln()`

* Reads a UTF-8 string from standard input, which is terminated by either an
  LF character or the end of input. The terminating LF, if any, is not
  included in the returned string.

* Returns the line that has been read as a string. If the end of input is
  encountered, `null` is returned.

* Throws an exception if standard input is binary-oriented, or if a read
  error occurs, or if source data cannot be converted to a valid UTF code
  point sequence.

### `std.io.putc(value)`

* Writes a UTF-8 string to standard output. `value` may be either an integer
  representing a UTF code point or a UTF-8 string.

* Returns the number of UTF code points that have been written.

* Throws an exception if standard output is binary-oriented, or if source
  data cannot be converted to a valid UTF code point sequence, or if a write
  error occurs.

### `std.io.putln(text)`

* Writes a UTF-8 string to standard output, followed by an LF, which may
  flush the stream automatically. `text` shall be a UTF-8 string.

* Returns the number of UTF code points that have been written, including the
  terminating LF.

* Throws an exception if standard output is binary-oriented, or if source
  data cannot be converted to a valid UTF code point sequence, or if a write
  error occurs.

### `std.io.putf(templ, ...)`

* Compose a string in the same way as `std.string.format()`, but instead of
  returning it, write it to standard output.

* Returns the number of UTF code points that have been written.

* Throws an exception if standard output is binary-oriented, or if source
  data cannot be converted to a valid UTF code point sequence, or if a write
  error occurs.

### `std.io.putfln(templ, ...)`

* Compose a string in the same way as `std.string.format()`, but instead of
  returning it, write it to standard output, followed by an LF.

* Returns the number of UTF code points that have been written, including the
  terminating LF.

* Throws an exception if standard output is binary-oriented, or if source
  data cannot be converted to a valid UTF code point sequence, or if a write
  error occurs.

### `std.io.read([limit])`

* Reads a series of bytes from standard input. If `limit` is set, no more
  than this number of bytes will be read.

* Returns the bytes that have been read as a string. If the end of input is
  encountered, `null` is returned.

* Throws an exception if standard input is text-oriented, or if a read error
  occurs, or if source data cannot be converted to a valid UTF code point
  sequence.

### `std.io.write(data)`

* Writes a series of bytes to standard output. `data` shall be a byte string.

* Returns the number of bytes that have been written.

* Throws an exception if standard output is text-oriented, or if a write
  error occurs.

### `std.io.flush()`

* Forces buffered data on standard output to be delivered to its underlying
  device. This function may be called regardless of the orientation of
  standard output.

* Throws an exception if a write error occurs.

## `std.zlib`

### `std.zlib.Deflator(format, [level])`

* Creates a data compressor using the deflate algorithm according to [RFC
  1951](https://www.rfc-editor.org/rfc/rfc1951.html). `format` must be one of
  `"deflate"`, `"gzip"`, or `"raw"`. `level` specifies the compression level,
  which must be an integer between 0 and 9. If it is absent, the library
  default value is used.

* Returns a compressor as an object consisting of the following members:

  * `output`
  * `update(data)`
  * `flush()`
  * `finish()`
  * `clear()`

  The string `output` is where compressed data will be appended. The function
  `update()` puts data into the compressor, which shall be a byte string. The
  function `flush()` causes pending bits to be pushed into the output string
  and aligned on a byte boundary. The `finish()` function marks the end of
  input data and returns a copy of the output string. The function `clear()`
  discards both input and output data and resets the compressor to its
  initial state.

* Throws an exception if `format` is invalid or `level` is out of range.

### `std.zlib.deflate(data, [level])`

* Compresses `data` which must be a byte string, as if by

  ```
  std.zlib.deflate = func(data, level) {
    var r = this.Deflator("deflate", level);
    r.update(data);
    return r.finish();
  };
  ```

* Returns the compressed string.

* Throws an exception if `level` is out of range.

### `std.zlib.gzip(data, [level])`

* Compresses `data` which must be a byte string, as if by

  ```
  std.zlib.deflate = func(data, level) {
    var r = this.Deflator("gzip", level);
    r.update(data);
    return r.finish();
  };
  ```

* Returns the compressed string.

* Throws an exception if `level` is out of range.

### `std.zlib.Inflator(format)`

* Creates a data decompressor which uses the deflate algorithm according to
  [RFC 1951](https://www.rfc-editor.org/rfc/rfc1951.html). `format` must be
  one of `"deflate"`, `"gzip"`, or `"raw"`.

* Returns a decompressor as an object consisting of the following members:

  * `output`
  * `update(data)`
  * `flush()`
  * `finish()`
  * `clear()`

  The string `output` is where decompressed data will be pushed. The function
  `update()` puts data into the decompressor, which shall be a byte string.
  The function `flush()` causes pending decompressed bytes to be appended to
  the output string. The `finish()` function marks the end of input data and
  returns a copy of the output string. The function `clear()` discards both
  input and output data and then resets the decompressor to its initial
  state.

* Throws an exception if `format` is invalid.

### `std.zlib.inflate(data)`

* Decompresses `data` which must be a byte string, as if by

  ```
  std.zlib.inflate = func(data) {
    var r = this.Inflator("deflate");
    r.update(data);
    return r.finish();
  };
  ```

* Returns the decompressed string.

* Throws an exception in case of corrupt input data.

### `std.zlib.gunzip(data)`

* Decompresses `data` which must be a byte string, as if by

  ```
  std.zlib.gunzip = func(data) {
    var r = this.Inflator("gzip");
    r.update(data);
    return r.finish();
  };
  ```

* Returns the decompressed string.

* Throws an exception in case of corrupt input data.

## `std.rsa`

### `std.rsa.sign_md5(private_key_path, data)`

* Signs a string with _md5WithRSAEncryption_. `private_key_path` shall
  denote a PEM file, which contains the RSA private key to use, enclosed by
  `-----BEGIN PRIVATE KEY-----` and `-----END PRIVATE KEY-----`. `data` is a
  byte string to sign.

* Returns the signature as a byte string.

* Throws an exception if the private key is invalid.

### `std.rsa.verify_md5(public_key_path, data, sig)`

* Verifies the signature of a string with _md5WithRSAEncryption_.
  `public_key_path` shall denote a PEM file, which contains the RSA public
  key to use, enclosed by `-----BEGIN PUBLIC KEY-----` and
  `-----END PUBLIC KEY-----`. `data` is the byte string that has been signed.
  `sig` is the signature to verify.

* Returns `true` if the signature is valid, and `false` otherwise.

* Throws an exception if the public key is invalid.

### `std.rsa.sign_sha1(private_key_path, data)`

* Signs a string with _sha1WithRSAEncryption_. `private_key_path` shall
  denote a PEM file, which contains the RSA private key to use, enclosed by
  `-----BEGIN PRIVATE KEY-----` and `-----END PRIVATE KEY-----`. `data` is a
  byte string to sign.

* Returns the signature as a byte string.

* Throws an exception if the private key is invalid.

### `std.rsa.verify_sha1(public_key_path, data, sig)`

* Verifies the signature of a string with _sha1WithRSAEncryption_.
  `public_key_path` shall denote a PEM file, which contains the RSA public
  key to use, enclosed by `-----BEGIN PUBLIC KEY-----` and
  `-----END PUBLIC KEY-----`. `data` is the byte string that has been signed.
  `sig` is the signature to verify.

* Returns `true` if the signature is valid, and `false` otherwise.

* Throws an exception if the public key is invalid.

### `std.rsa.sign_sha256(private_key_path, data)`

* Signs a string with _sha256WithRSAEncryption_. `private_key_path` shall
  denote a PEM file, which contains the RSA private key to use, enclosed by
  `-----BEGIN PRIVATE KEY-----` and `-----END PRIVATE KEY-----`. `data` is a
  byte string to sign.

* Returns the signature as a byte string.

* Throws an exception if the private key is invalid.

### `std.rsa.verify_sha256(public_key_path, data, sig)`

* Verifies the signature of a string with _sha256WithRSAEncryption_.
  `public_key_path` shall denote a PEM file, which contains the RSA public
  key to use, enclosed by `-----BEGIN PUBLIC KEY-----` and
  `-----END PUBLIC KEY-----`. `data` is the byte string that has been signed.
  `sig` is the signature to verify.

* Returns `true` if the signature is valid, and `false` otherwise.

* Throws an exception if the public key is invalid.

