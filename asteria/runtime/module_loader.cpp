// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "module_loader.hpp"
#include "runtime_error.hpp"
#include "../utils.hpp"
#include <sys/stat.h>
#include <sys/file.h>  // ::flock()
#include <unistd.h>  // ::fstat()
namespace asteria {

Module_Loader::
~Module_Loader()
  {
  }

Module_Loader::locked_stream_pair*
Module_Loader::
do_lock_stream(const char* path)
  {
    // Open the file first.
    ::rocket::unique_posix_file file(::fopen(path, "rb"));
    if(!file)
      throw Runtime_Error(Runtime_Error::M_format(),
               "Could not open script file '$1': ${errno:full}", path);

    // Make the unique identifier of this file from its device ID and inode number.
    int fd = ::fileno(file);
    struct ::stat info;
    if(::fstat(fd, &info))
      throw Runtime_Error(Runtime_Error::M_format(),
               "Could not get properties of script file '$1': ${errno:full}", path);

    // Mark the stream locked.
    auto skey = format_string("dev:$1/ino:$2", info.st_dev, info.st_ino);
    auto result = this->m_strms.try_emplace(::std::move(skey), ::std::move(file));
    if(!result.second)
      throw Runtime_Error(Runtime_Error::M_format(),
               "Recursive import denied (loading '$1', file ID `$2`)", path, skey);

    // Lock the file. It will be automatically unlocked when it is closed later.
    // This has to come last because we want user-friendly error messages.
    // Keep in mind that `file` is now null.
    if(::flock(fd, LOCK_EX) != 0)
      throw Runtime_Error(Runtime_Error::M_format(),
               "Could not lock script file '$1': ${errno:full}", path);

    return &*(result.first);
  }

void
Module_Loader::
do_unlock_stream(locked_stream_pair* qstrm) noexcept
  {
    // Erase the strment denoted by `qstrm`.
    ROCKET_ASSERT(qstrm);
    auto count = this->m_strms.erase(qstrm->first);
    ROCKET_ASSERT(count == 1);
  }

}  // namespace asteria
