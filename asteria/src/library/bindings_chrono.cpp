// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_chrono.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"
#include <time.h>

namespace Asteria {

G_integer std_chrono_utc_now()
  {
    // Get UTC time from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    // Convert it to the number of milliseconds.
    return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
  }

G_integer std_chrono_local_now()
  {
    // Get local time and GMT offset from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    ::tm tr;
    ::localtime_r(&(ts.tv_sec), &tr);
    // Convert it to the number of milliseconds.
    return (static_cast<int64_t>(ts.tv_sec) + tr.tm_gmtoff) * 1000 + ts.tv_nsec / 1000000;
  }

G_real std_chrono_hires_now()
  {
    // Get the time since the system was started.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    // Convert it to the number of milliseconds.
    // Add a random offset to the result to help debugging.
    return static_cast<double>(ts.tv_sec) * 1000 + static_cast<double>(ts.tv_nsec) / 1000000 + 0x987654321;
  }

G_integer std_chrono_steady_now()
  {
    // Get the time since the system was started.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    // Convert it to the number of milliseconds.
    // Add a random offset to the result to help debugging.
    return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000 + 0x123456789;
  }

G_integer std_chrono_local_from_utc(aref<G_integer> time_utc)
  {
    // Handle special time values.
    if(time_utc <= -11644473600000)
      return INT64_MIN;
    if(time_utc >= 253370764800000)
      return INT64_MAX;
    // Calculate the local time.
    ::time_t tp = 0;
    ::tm tr;
    ::localtime_r(&tp, &tr);
    int64_t time_local = time_utc + tr.tm_gmtoff * 1000;
    // Handle results that are out of the finite range.
    if(time_local <= -11644473600000)
      return INT64_MIN;
    if(time_local >= 253370764800000)
      return INT64_MAX;
    else
      return time_local;
  }

G_integer std_chrono_utc_from_local(aref<G_integer> time_local)
  {
    // Handle special time values.
    if(time_local <= -11644473600000)
      return INT64_MIN;
    if(time_local >= 253370764800000)
      return INT64_MAX;
    // Calculate the UTC time.
    ::time_t tp = 0;
    ::tm tr;
    ::localtime_r(&tp, &tr);
    int64_t time_utc = time_local - tr.tm_gmtoff * 1000;
    // Handle results that are out of the finite range.
    if(time_utc <= -11644473600000)
      return INT64_MIN;
    if(time_utc >= 253370764800000)
      return INT64_MAX;
    else
      return time_utc;
  }

    namespace {

    constexpr char s_min_str[2][32] = { "1601-01-01 00:00:00", "1601-01-01 00:00:00.000" };
    constexpr char s_max_str[2][32] = { "9999-01-01 00:00:00", "9999-01-01 00:00:00.000" };
    constexpr char s_spaces[] = " \f\n\r\t\v";

    template<typename InputT> bool do_rput(char*& p, InputT in, size_t width)
      {
        uint32_t reg = 0;
        // Load the value, ignoring any overflows.
        reg = static_cast<uint32_t>(in);
        // Write digits backwards.
        for(size_t i = 0; i < width; ++i) {
          char c = static_cast<char>('0' + reg % 10);
          reg /= 10;
          p[i] = c;
        }
        // Succeed.
        p += width;
        return true;
      }
    bool do_rput(char*& p, char c)
      {
        // Write a character.
        p[0] = c;
        // Succeed.
        p += 1;
        return true;
      }

    template<typename OutputT> bool do_getx(const char*& p, OutputT& out, size_t width)
      {
        uint32_t reg = 0;
        // Read digits forwards.
        for(size_t i = 0; i < width; ++i) {
          char c = p[i];
          if((c < '0') || ('9' < c))
            return false;
          reg *= 10;
          reg += static_cast<uint32_t>(c - '0');
        }
        // Save the value, ignoring any overflows.
        out = static_cast<OutputT>(reg);
        // Succeed.
        p += width;
        return true;
      }
    bool do_getx(const char*& p, std::initializer_list<char> accept)
      {
        // Read a character.
        char c = p[0];
        if(rocket::is_none_of(c, accept))
          return false;
        // Succeed.
        p += 1;
        return true;
      }

    }  // namespace

G_string std_chrono_utc_format(aref<G_integer> time_point, aopt<G_boolean> with_ms)
  {
    // No millisecond part is added by default.
    bool pms = with_ms.value_or(false);
    // Deal with special time points.
    if(time_point <= -11644473600000)
      return rocket::sref(s_min_str[pms]);
    if(time_point >= 253370764800000)
      return rocket::sref(s_max_str[pms]);
    // Write the string backwards.
    char wbuf[32];
    char* qwt = wbuf;
    // Write fields backwards.
    // Get the second and millisecond parts.
    // Note that the number of seconds shall be rounded towards negative infinity.
    int ms = static_cast<int>((static_cast<uint64_t>(time_point) + 9223372036854775000) % 1000);
    ::time_t tp = static_cast<::time_t>((time_point - ms) / 1000);
    if(pms) {
      do_rput(qwt, ms, 3);
      do_rput(qwt, '.');
    }
    ::tm tr;
    ::gmtime_r(&tp, &tr);
    do_rput(qwt, tr.tm_sec, 2);
    do_rput(qwt, ':');
    do_rput(qwt, tr.tm_min, 2);
    do_rput(qwt, ':');
    do_rput(qwt, tr.tm_hour, 2);
    do_rput(qwt, ' ');
    do_rput(qwt, tr.tm_mday, 2);
    do_rput(qwt, '-');
    do_rput(qwt, tr.tm_mon + 1, 2);
    do_rput(qwt, '-');
    do_rput(qwt, tr.tm_year + 1900, 4);
    return G_string(std::make_reverse_iterator(qwt), std::make_reverse_iterator(wbuf));
  }

opt<G_integer> std_chrono_utc_parse(aref<G_string> time_str)
  {
    auto n = time_str.find_first_not_of(s_spaces);
    if(n == G_string::npos) {
      // `time_str` consists of only spaces. Fail.
      return rocket::clear;
    }
    const char* qrd = time_str.data() + n;
    n = time_str.find_last_not_of(s_spaces) + 1 - n;
    if(n < std::strlen(s_min_str[0])) {
      // `time_str` contains too few characters. Fail.
      return rocket::clear;
    }
    const char* qend = qrd + n;
    // Parse the shortest acceptable substring, i.e. the substring without milliseconds.
    ::tm tr;
    bool succ = do_getx(qrd, tr.tm_year, 4) &&
                do_getx(qrd, { '-', '/' }) &&
                do_getx(qrd, tr.tm_mon, 2) &&
                do_getx(qrd, { '-', '/' }) &&
                do_getx(qrd, tr.tm_mday, 2) &&
                do_getx(qrd, { ' ', 'T' }) &&
                do_getx(qrd, tr.tm_hour, 2) &&
                do_getx(qrd, { ':' }) &&
                do_getx(qrd, tr.tm_min, 2) &&
                do_getx(qrd, { ':' }) &&
                do_getx(qrd, tr.tm_sec, 2);
    if(!succ)
      return rocket::clear;
    if(tr.tm_year < 1600)
      return INT64_MIN;
    if(tr.tm_year > 9998)
      return INT64_MAX;
    // Assemble the parts without milliseconds.
    tr.tm_year -= 1900;
    tr.tm_mon -= 1;
    tr.tm_isdst = 0;
    ::time_t tp = ::timegm(&tr);
    if(tp == static_cast<::time_t>(-1))
      return rocket::clear;
    // Convert it to the number of milliseconds.
    auto time_point = static_cast<int64_t>(tp) * 1000;
    // Parse the subsecond part if any.
    if(do_getx(qrd, { '.', ',' })) {
      // Parse milliseconds backwards.
      double r = 0;
      while(qend > qrd) {
        char c = *--qend;
        if((c < '0') || ('9' < c))
          return rocket::clear;
        r += c - '0';
        r /= 10;
      }
      time_point += std::lround(r * 1000);
    }
    if(qrd != qend) {
      // Reject invalid characters at the end of `time_str`.
      return rocket::clear;
    }
    return time_point;
  }

void create_bindings_chrono(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.chrono.utc_now()`
    //===================================================================
    result.insert_or_assign(rocket::sref("utc_now"),
      G_function(rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        rocket::sref(
          "\n"
          "`std.chrono.utc_now()`\n"
          "\n"
          "  * Retrieves the wall clock time in UTC.\n"
          "\n"
          "  * Returns the number of milliseconds since the Unix epoch,\n"
          "    represented as an `integer`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/,
                   Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.chrono.utc_now"), rocket::ref(args));
          // Parse arguments.
          if(reader.start().finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_utc_now() };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.local_now()`
    //===================================================================
    result.insert_or_assign(rocket::sref("local_now"),
      G_function(rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        rocket::sref(
          "\n"
          "`std.chrono.local_now()`\n"
          "\n"
          "  * Retrieves the wall clock time in the local time zone.\n"
          "\n"
          "  * Returns the number of milliseconds since `1970-01-01 00:00:00`\n"
          "    in the local time zone, represented as an `integer`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/,
                   Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.chrono.local_now"), rocket::ref(args));
          // Parse arguments.
          if(reader.start().finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_local_now() };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.hires_now()`
    //===================================================================
    result.insert_or_assign(rocket::sref("hires_now"),
      G_function(rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        rocket::sref(
          "\n"
          "`std.chrono.hires_now()`\n"
          "\n"
          "  * Retrieves a time point from a high resolution clock. The clock\n"
          "    goes monotonically and cannot be adjusted, being suitable for\n"
          "    time measurement. This function provides accuracy and might be\n"
          "    quite heavyweight.\n"
          "\n"
          "  * Returns the number of milliseconds since an unspecified time\n"
          "    point, represented as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/,
                   Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.chrono.hires_now"), rocket::ref(args));
          // Parse arguments.
          if(reader.start().finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_hires_now() };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.steady_now()`
    //===================================================================
    result.insert_or_assign(rocket::sref("steady_now"),
      G_function(rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        rocket::sref(
          "\n"
          "`std.chrono.steady_now()`\n"
          "\n"
          "  * Retrieves a time point from a steady clock. The clock goes\n"
          "    monotonically and cannot be adjusted, being suitable for time\n"
          "    measurement. This function is supposed to be fast and might\n"
          "    have poor accuracy.\n"
          "\n"
          "  * Returns the number of milliseconds since an unspecified time\n"
          "    point, represented as an `integer`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/,
                   Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.chrono.steady_now"), rocket::ref(args));
          // Parse arguments.
          if(reader.start().finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_steady_now() };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.local_from_utc()`
    //===================================================================
    result.insert_or_assign(rocket::sref("local_from_utc"),
      G_function(rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        rocket::sref(
          "\n"
          "`std.chrono.local_from_utc(time_utc)`\n"
          "\n"
          "  * Converts a UTC time point to a local one. `time_utc` shall be\n"
          "    the number of milliseconds since the Unix epoch.\n"
          "\n"
          "  * Returns the number of milliseconds since `1970-01-01 00:00:00`\n"
          "    in the local time zone, represented as an `integer`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/,
                   Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.chrono.local_from_utc"), rocket::ref(args));
          // Parse arguments.
          G_integer time_utc;
          if(reader.start().g(time_utc).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_local_from_utc(time_utc) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.utc_from_local()`
    //===================================================================
    result.insert_or_assign(rocket::sref("utc_from_local"),
      G_function(rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        rocket::sref(
          "\n"
          "`std.chrono.utc_from_local(time_local)`\n"
          "\n"
          "  * Converts a local time point to a UTC one. `time_local` shall\n"
          "    be the number of milliseconds since `1970-01-01 00:00:00` in\n"
          "    the local time zone.\n"
          "\n"
          "  * Returns the number of milliseconds since the Unix epoch,\n"
          "    represented as an `integer`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/,
                   Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.chrono.utc_from_local"), rocket::ref(args));
          // Parse arguments.
          G_integer time_local;
          if(reader.start().g(time_local).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_utc_from_local(time_local) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.utc_format()`
    //===================================================================
    result.insert_or_assign(rocket::sref("utc_format"),
      G_function(rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        rocket::sref(
          "\n"
          "`std.chrono.utc_format(time_point, [with_ms])`\n"
          "\n"
          "  * Converts `time_point`, which represents the number of\n"
          "    milliseconds since `1970-01-01 00:00:00`, to an ASCII string in\n"
          "    the aforementioned format, according to the ISO 8601 standard.\n"
          "    If `with_ms` is set to `true`, the string will have a 3-digit\n"
          "    fractional part. By default, no fractional part is added.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/,
                   Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.chrono.utc_format"), rocket::ref(args));
          // Parse arguments.
          G_integer time_point;
          opt<G_boolean> with_ms;
          if(reader.start().g(time_point).g(with_ms).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_utc_format(time_point, with_ms) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.utc_parse()`
    //===================================================================
    result.insert_or_assign(rocket::sref("utc_parse"),
      G_function(rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        rocket::sref(
          "\n"
          "`std.chrono.utc_parse(time_str)`\n"
          "\n"
          "  * Parses `time_str`, which is an ASCII string representing a time\n"
          "    point in the format `1970-01-01 00:00:00.000`, according to the\n"
          "    ISO 8601 standard. The subsecond part is optional and may have\n"
          "    fewer or more digits. There may be leading or trailing spaces.\n"
          "\n"
          "  * Returns the number of milliseconds since `1970-01-01 00:00:00`\n"
          "    if the time string has been parsed successfully, or `null`\n"
          "    otherwise.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/,
                   Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.chrono.utc_parse"), rocket::ref(args));
          // Parse arguments.
          G_string time_str;
          if(reader.start().g(time_str).finish()) {
            // Call the binding function.
            auto qres = std_chrono_utc_parse(time_str);
            if(!qres) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { rocket::move(*qres) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // End of `std.chrono`
    //===================================================================
  }

}  // namespace Asteria
