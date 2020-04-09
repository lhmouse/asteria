// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "loader_lock.hpp"
#include "../utilities.hpp"
#include <sys/stat.h>
#include <unistd.h>  // ::fstat()

namespace Asteria {

Loader_Lock::~Loader_Lock()
  {
  }

Loader_Lock::element_type* Loader_Lock::do_lock_stream(const char* path)
  {
    // Open the file first.
    ::rocket::unique_posix_file file(::fopen(path, "r"), ::fclose);
    if(!file)
      ASTERIA_THROW_SYSTEM_ERROR("fopen");

    // Make the unique identifier of this file from its device ID and inode number.
    struct ::stat info;
    if(::fstat(::fileno(file), &info))
      ASTERIA_THROW_SYSTEM_ERROR("fstat");
    auto skey = format_string("dev:$1/ino:$2", info.st_dev, info.st_ino);

    // Mark the stream locked.
    auto result = this->m_strms.try_emplace(::std::move(skey), ::std::move(file));
    if(!result.second)
      ASTERIA_THROW("recursive import denied (loading `$1`, identified as `$2`)", path, skey);
    return &*(result.first);
  }

void Loader_Lock::do_unlock_stream(element_type* qelem) noexcept
  {
    ROCKET_ASSERT(qelem);
    auto count = this->m_strms.erase(qelem->first);
    ROCKET_ASSERT(count == 1);
  }

}  // namespace Asteria
