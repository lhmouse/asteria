// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "standard_bindings_chrono.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <time.h>
#endif

namespace Asteria {

D_integer std_chrono_utc_now()
  {
    D_integer time_utc;
#ifdef _WIN32
    // Get UTC time from the system.
    ::FILETIME ft_u;
    ::GetSystemTimeAsFileTime(&ft_u);
    // Convert it to the number of milliseconds.
    ::ULARGE_INTEGER ti;
    ti.LowPart = ft_u.dwLowDateTime;
    ti.HighPart = ft_u.dwHighDateTime;
    // `0x019DB1DED53E8000` = duration from `1601-01-01` to `1970-01-01` in 100 nanoseconds.
    time_utc = static_cast<std::int64_t>(ti.QuadPart - 0x019DB1DED53E8000) / 10000;
#else
    // Get UTC time from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    // Convert it to the number of milliseconds.
    time_utc = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
    return time_utc;
  }

D_integer std_chrono_local_now()
  {
    D_integer time_local;
#ifdef _WIN32
    // Get local time from the system.
    ::FILETIME ft_u;
    ::GetSystemTimeAsFileTime(&ft_u);
    ::FILETIME ft_l;
    ::FileTimeToLocalFileTime(&ft_u, &ft_l);
    // Convert it to the number of milliseconds.
    ::ULARGE_INTEGER ti;
    ti.LowPart = ft_l.dwLowDateTime;
    ti.HighPart = ft_l.dwHighDateTime;
    // `0x019DB1DED53E8000` = duration from `1601-01-01` to `1970-01-01` in 100 nanoseconds.
    time_local = static_cast<std::int64_t>(ti.QuadPart - 0x019DB1DED53E8000) / 10000;
#else
    // Get local time and GMT offset from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    ::tm tr;
    ::localtime_r(&(ts.tv_sec), &tr);
    // Convert it to the number of milliseconds.
    time_local = (ts.tv_sec + tr.tm_gmtoff) * 1000 + ts.tv_nsec / 1000000;
#endif
    return time_local;
  }

D_real std_chrono_hires_now()
  {
    D_real time_hires;
#ifdef _WIN32
    // Read the performance counter.
    // The performance counter frequency only has to be obtained once.
    static std::atomic<double> s_freq_recip;
    ::LARGE_INTEGER ti;
    auto freq_recip = s_freq_recip.load(std::memory_order_relaxed);
    if(ROCKET_UNEXPECT(!(freq_recip > 0))) {
      ::QueryPerformanceFrequency(&ti);
      freq_recip = 1000 / static_cast<double>(ti.QuadPart);
      s_freq_recip.store(freq_recip, std::memory_order_relaxed);
    }
    ::QueryPerformanceCounter(&ti);
    // Convert it to the number of milliseconds.
    time_hires = static_cast<double>(ti.QuadPart) * freq_recip;
#else
    // Get the time since the system was started.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    // Convert it to the number of milliseconds.
    time_hires = static_cast<double>(ts.tv_sec) * 1000 + static_cast<double>(ts.tv_nsec) / 1000000;
#endif
    // Add a random offset to the result for discovery of bugs.
    time_hires += 0x987654321;
    return time_hires;
  }

D_integer std_chrono_steady_now()
  {
    D_integer time_steady;
#ifdef _WIN32
    // Get the system tick count.
    time_steady = static_cast<std::int64_t>(::GetTickCount64());
#else
    // Get the time since the system was started.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    // Convert it to the number of milliseconds.
    time_steady = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
    // Add a random offset to the result for discovery of bugs.
    time_steady += 0x123456789;
    return time_steady;
  }

D_integer std_chrono_local_from_utc(D_integer time_utc)
  {
    D_integer time_local;
    // Get the offset of the local time from UTC in milliseconds.
    std::int64_t offset;
#ifdef _WIN32
    ::DYNAMIC_TIME_ZONE_INFORMATION dtz;
    ::GetDynamicTimeZoneInformation(&dtz);
    offset = dtz.bias * -60000;
#else
    ::time_t tp = 0;
    ::tm tr;
    ::localtime_r(&tp, &tr);
    offset = tr.tm_gmtoff * 1000;
#endif
    // Add the time zone offset to `time_local` to get the local time.
    // Watch out for integral overflows.
    if((offset >= 0) && (time_utc > INT64_MAX - offset)) {
      time_local = INT64_MAX;
    } else if((offset < 0) && (time_utc < INT64_MIN - offset)) {
      time_local = INT64_MIN;
    } else {
      time_local = time_utc + offset;
    }
    return time_local;
  }

D_integer std_chrono_utc_from_local(D_integer time_local)
  {
    D_integer time_utc;
    // Get the offset of the local time from UTC in milliseconds.
    std::int64_t offset;
#ifdef _WIN32
    ::DYNAMIC_TIME_ZONE_INFORMATION dtz;
    ::GetDynamicTimeZoneInformation(&dtz);
    offset = dtz.bias * -60000;
#else
    ::time_t tp = 0;
    ::tm tr;
    ::localtime_r(&tp, &tr);
    offset = tr.tm_gmtoff * 1000;
#endif
    // Subtract the time zone offset from `time_local` to get the UTC time.
    // Watch out for integral overflows.
    if((offset >= 0) && (time_local < INT64_MIN + offset)) {
      time_utc = INT64_MIN;
    } else if((offset < 0) && (time_local > INT64_MAX + offset)) {
      time_utc = INT64_MAX;
    } else {
      time_utc = time_local - offset;
    }
    return time_utc;
  }

bool std_chrono_parse_datetime(D_integer &time_point_out, const D_string &time_str);
D_string std_chrono_format_datetime(D_integer time_point);

D_object create_standard_bindings_chrono()
  {
    D_object chrono;
    //===================================================================
    // `std.chrono.utc_now()`
    //===================================================================
    chrono.try_emplace(rocket::sref("utc_now"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.utc_now()`"
                     "\n  * Retrieves the wall clock time in UTC."
                     "\n  * Returns the number of milliseconds since the Unix epoch,"
                     "\n    represented as an `integer`."),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.utc_now"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              auto time_utc = std_chrono_utc_now();
              // Return it.
              Reference_Root::S_temporary ref_c = { time_utc };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.local_now()`
    //===================================================================
    chrono.try_emplace(rocket::sref("local_now"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.local_now()`"
                     "\n  * Retrieves the wall clock time in the local time zone."
                     "\n  * Returns the number of milliseconds since `1970-01-01 00:00:00`"
                     "\n    in the local time zone, represented as an `integer`."),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.local_now"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              auto time_local = std_chrono_local_now();
              // Return it.
              Reference_Root::S_temporary ref_c = { time_local };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.hires_now()`
    //===================================================================
    chrono.try_emplace(rocket::sref("hires_now"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.hires_now()`"
                     "\n  * Retrieves a time point from a high resolution clock. The clock"
                     "\n    goes monotonically and cannot be adjusted, being suitable for"
                     "\n    time measurement. This function provides accuracy and might be"
                     "\n    quite heavyweight."
                     "\n  * Returns the number of milliseconds since an unspecified time"
                     "\n    point, represented as a `real`."),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.hires_now"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              auto time_hires = std_chrono_hires_now();
              // Return it.
              Reference_Root::S_temporary ref_c = { time_hires };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.steady_now()`
    //===================================================================
    chrono.try_emplace(rocket::sref("steady_now"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.steady_now()`"
                     "\n  * Retrieves a time point from a steady clock. The clock goes"
                     "\n    monotonically and cannot be adjusted, being suitable for time"
                     "\n    measurement. This function is supposed to be fast and might"
                     "\n    have poor accuracy."
                     "\n  * Returns the number of milliseconds since an unspecified time"
                     "\n    point, represented as an `integer`."),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.steady_now"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              auto time_steady = std_chrono_steady_now();
              // Return it.
              Reference_Root::S_temporary ref_c = { time_steady };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.local_from_utc(time_utc)`
    //===================================================================
    chrono.try_emplace(rocket::sref("local_from_utc"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.local_from_utc(time_utc)`"
                     "\n  * Converts a UTC time point to a local one. `time_utc` shall be"
                     "\n    the number of milliseconds since the Unix epoch."
                     "\n  * Returns the number of milliseconds since `1970-01-01 00:00:00`"
                     "\n    in the local time zone, represented as an `integer`."),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.local_from_utc"), args);
            // Parse arguments.
            D_integer time_utc;
            if(reader.start().req(time_utc).finish()) {
              // Call the binding function.
              auto time_local = std_chrono_local_from_utc(time_utc);
              // Return it.
              Reference_Root::S_temporary ref_c = { time_local };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.utc_from_local(time_local)`
    //===================================================================
    chrono.try_emplace(rocket::sref("utc_from_local"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.utc_from_local(time_local)`"
                     "\n  * Converts a local time point to a UTC one. `time_local` shall"
                     "\n    be the number of milliseconds since `1970-01-01 00:00:00` in"
                     "\n    the local time zone."
                     "\n  * Returns the number of milliseconds since the Unix epoch,"
                     "\n    represented as an `integer`."),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.utc_from_local"), args);
            // Parse arguments.
            D_integer time_local;
            if(reader.start().req(time_local).finish()) {
              // Call the binding function.
              auto time_utc = std_chrono_utc_from_local(time_local);
              // Return it.
              Reference_Root::S_temporary ref_c = { time_utc };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // End of `std.chrono`
    //===================================================================
    return chrono;
  }

}  // namespace Asteria
