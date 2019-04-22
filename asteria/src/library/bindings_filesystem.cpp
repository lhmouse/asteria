// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_filesystem.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"
#ifdef _WIN32
#  include <windows.h>  // ::CreateFile(), ::CloseHandle(), ::GetFileInformationByHandleEx(),
                        // ::FindFirstFile(), ::FindNextFile(), ::CreateDirectory(), ::RemoveDirectory(),
                        // ::ReadFile(), ::WriteFile(), ::DeleteFile()
#else
#  include <sys/stat.h>  // ::lstat(), ::stat(), ::mkdir()
#  include <dirent.h>  // ::opendir(), ::closedir()
#  include <fcntl.h>  // ::open()
#  include <unistd.h>  // ::rmdir(), ::close(), ::pread(), ::pwrite(), ::unlink()
#  include <stdio.h>  // ::rename()
#endif

namespace Asteria {

    namespace {

#ifdef _WIN32
    // UTF-16 is used on Windows.
    rocket::cow_u16string do_translate_winnt_path(const G_string& path)
      {
        rocket::cow_u16string u16str;
        u16str.reserve(path.size() + 8);
        // If `path` is an absolute path, translate it to an NT path for long filename support.
        if((path.size() >= 2) && (path[1] == L':')) {
          // Convert lowercase letters to uppercase ones.
          auto letter = path[0] & ~0x20;
          if((L'A' <= letter) && (letter <= L'Z')) {
            u16str.append(uR"(\\?\)");
          }
        }
        // Convert all characters.
        std::size_t offset = 0;
        while(offset < path.size()) {
          char32_t cp;
          if(!utf8_decode(cp, path, offset)) {
            ASTERIA_THROW_RUNTIME_ERROR("The path `", path, "` is not a valid UTF-8 string.");
          }
          utf16_encode(u16str, cp);
        }
        return u16str;
      }

    // Compose a pair of `DWORD`s to form an `uint64_t`.
    constexpr std::uint64_t do_compose(::DWORD high, ::DWORD low) noexcept
      {
        return (static_cast<std::uint64_t>(high) << 32) + low;
      }
#endif

    class File
      {
      public:
        // `Handle` is the native handle for a file.
#ifdef _WIN32
        using Handle = ::HANDLE;
#else
        using Handle = int;
#endif

        // `Closer` is used to close an opened file.
        struct Closer
          {
            Handle null() const noexcept
              {
#ifdef _WIN32
                return INVALID_HANDLE_VALUE;
#else
                return -1;
#endif
              }
            bool is_null(Handle hf) const noexcept
              {
                return hf == this->null();
              }
            void close(Handle hf) const noexcept
              {
#ifdef _WIN32
                ::CloseHandle(hf);
#else
                ::close(hf);
#endif
              }
          };

      private:
        rocket::unique_handle<Handle, Closer> m_hf;

      public:
        explicit File(Handle hf) noexcept
          : m_hf(hf)
          {
          }

      public:
        explicit operator bool () const noexcept
          {
            return !!this->m_hf;
          }
        operator Handle () const noexcept
          {
            return this->m_hf.get();
          }
      };

#ifdef _WIN32
    struct Directory_Closer
      {
        ::HANDLE null() const noexcept
          {
            return INVALID_HANDLE_VALUE;
          }
        bool is_null(::HANDLE h) const noexcept
          {
            return h == INVALID_HANDLE_VALUE;
          }
        void close(::HANDLE h) const noexcept
          {
            ::FindClose(h);
          }
      };
    using Directory_Handle = rocket::unique_handle<::HANDLE, Directory_Closer>;
#else
    struct Directory_Closer
      {
        ::DIR* null() const noexcept
          {
            return nullptr;
          }
        bool is_null(::DIR* dp) const noexcept
          {
            return dp == nullptr;
          }
        void close(::DIR* dp) const noexcept
          {
            ::closedir(dp);
          }
      };
    using Directory_Handle = rocket::unique_handle<::DIR*, Directory_Closer>;
#endif
    }

G_string std_filesystem_get_working_directory()
  {
    G_string cwd;
#ifdef _WIN32
    // Get the current directory as UTF-16.
    rocket::cow_u16string ucwd(MAX_PATH, L'*');
    auto nreq = ::GetCurrentDirectoryW(static_cast<::DWORD>(ucwd.size()), reinterpret_cast<wchar_t*>(ucwd.mut_data()));
    if(nreq > ucwd.size()) {
      // The buffer was too small.
      ucwd.append(nreq - ucwd.size(), L'*');
      nreq = ::GetCurrentDirectoryW(nreq, reinterpret_cast<wchar_t*>(ucwd.mut_data()));
    }
    // Convert UTF-16 to UTF-8.
    // We only want to stop when a NUL character is encountered.
    cwd.reserve(ucwd.size() + 20);
    auto pos = ucwd.c_str();
    for(;;) {
      char32_t cp;
      if(!utf16_decode(cp, pos, SIZE_MAX)) {
        ASTERIA_THROW_RUNTIME_ERROR("The path of the current working directory is not valid UTF-16.");
      }
      if(cp == 0) {
        break;
      }
      utf8_encode(cwd, cp);
    }
#else
    // Get the current directory, resizing the buffer as needed.
    cwd.resize(1);
    while(::getcwd(cwd.mut_data(), cwd.size()) == nullptr) {
      auto err = errno;
      if(err != ERANGE) {
        ASTERIA_THROW_RUNTIME_ERROR("`getcwd()` failed.");
      }
      cwd.append(cwd.size() / 2, '*');
    }
    cwd.erase(cwd.find('\0'));
#endif
    return cwd;
  }

Opt<G_object> std_filesystem_get_information(const G_string& path)
  {
    G_object stat;
#ifdef _WIN32
    auto wpath = do_translate_winnt_path(path);
    // Open the file or directory.
    File hf(::CreateFileW(reinterpret_cast<const wchar_t*>(wpath.c_str()),
                          FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL));
    if(!hf) {
      auto err = ::GetLastError();
      ASTERIA_DEBUG_LOG("`CreateFileW()` failed on \'", path, "\' (last error was `", err, "`).");
      return rocket::nullopt;
    }
    ::BY_HANDLE_FILE_INFORMATION fbi;
    if(::GetFileInformationByHandle(hf, &fbi) == FALSE) {
      auto err = ::GetLastError();
      ASTERIA_DEBUG_LOG("`GetFileInformationByHandle()` failed on \'", path, "\' (last error was `", err, "`).");
      return rocket::nullopt;
    }
    ::FILE_STANDARD_INFO fsi;
    if(::GetFileInformationByHandleEx(hf, FileStandardInfo, &fsi, sizeof(fsi)) == FALSE) {
      auto err = ::GetLastError();
      ASTERIA_DEBUG_LOG("`GetFileInformationByHandleEx()` failed on \'", path, "\' (last error was `", err, "`).");
      return rocket::nullopt;
    }
    // Fill `stat`.
    stat.insert_or_assign(rocket::sref("i_dev"),
      G_integer(
        fbi.dwVolumeSerialNumber  // unique device id on this machine.
      ));
    stat.insert_or_assign(rocket::sref("i_file"),
      G_integer(
        do_compose(fbi.nFileIndexHigh, fbi.nFileIndexLow)  // unique file id on this device.
      ));
    stat.insert_or_assign(rocket::sref("n_ref"),
      G_integer(
        fbi.nNumberOfLinks  // number of hard links to this file.
      ));
    stat.insert_or_assign(rocket::sref("b_dir"),
      G_boolean(
        fbi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY  // whether this is a directory.
      ));
    stat.insert_or_assign(rocket::sref("b_sym"),
      G_boolean(
        fbi.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT  // whether this is a symbol link.
      ));
    stat.insert_or_assign(rocket::sref("n_size"),
      G_integer(
        fsi.EndOfFile.QuadPart  // number of bytes this file contains.
      ));
    stat.insert_or_assign(rocket::sref("n_ocup"),
      G_integer(
        fsi.AllocationSize.QuadPart  // number of bytes this file occupies.
      ));
    stat.insert_or_assign(rocket::sref("t_accs"),
      G_integer(
        (do_compose(fbi.ftLastAccessTime.dwHighDateTime, fbi.ftLastAccessTime.dwLowDateTime) - 116444736000000000) / 10000  // timestamp of last access.
      ));
    stat.insert_or_assign(rocket::sref("t_mod"),
      G_integer(
        (do_compose(fbi.ftLastWriteTime.dwHighDateTime, fbi.ftLastWriteTime.dwLowDateTime) - 116444736000000000) / 10000  // timestamp of last modification.
      ));
#else
    struct ::stat stb;
    if(::lstat(path.c_str(), &stb) != 0) {
      auto err = errno;
      ASTERIA_DEBUG_LOG("`lstat()` failed on \'", path, "\' (errno was `", err, "`).");
      return rocket::nullopt;
    }
    // Fill `stat`.
    stat.insert_or_assign(rocket::sref("i_dev"),
      G_integer(
        stb.st_dev  // unique device id on this machine.
      ));
    stat.insert_or_assign(rocket::sref("i_file"),
      G_integer(
        stb.st_ino  // unique file id on this device.
      ));
    stat.insert_or_assign(rocket::sref("n_ref"),
      G_integer(
        stb.st_nlink  // number of hard links to this file.
      ));
    stat.insert_or_assign(rocket::sref("b_dir"),
      G_boolean(
        S_ISDIR(stb.st_mode)  // whether this is a directory.
      ));
    stat.insert_or_assign(rocket::sref("b_sym"),
      G_boolean(
        S_ISLNK(stb.st_mode)  // whether this is a symbol link.
      ));
    stat.insert_or_assign(rocket::sref("n_size"),
      G_integer(
        stb.st_size  // number of bytes this file contains.
      ));
    stat.insert_or_assign(rocket::sref("n_ocup"),
      G_integer(
        static_cast<std::uint64_t>(stb.st_blocks) * 512  // number of bytes this file occupies.
      ));
    stat.insert_or_assign(rocket::sref("t_accs"),
      G_integer(
        static_cast<std::int64_t>(stb.st_atim.tv_sec) * 1000 + stb.st_atim.tv_nsec / 1000000  // timestamp of last access.
      ));
    stat.insert_or_assign(rocket::sref("t_mod"),
      G_integer(
        static_cast<std::int64_t>(stb.st_mtim.tv_sec) * 1000 + stb.st_mtim.tv_nsec / 1000000  // timestamp of last modification.
      ));
#endif
    return rocket::move(stat);
  }

bool std_filesystem_move_from(const G_string& path_new, const G_string& path_old)
  {
#ifdef _WIN32
    auto wpath_new = do_translate_winnt_path(path_new);
    auto wpath_old = do_translate_winnt_path(path_old);
    if(::MoveFileExW(reinterpret_cast<const wchar_t*>(wpath_old.c_str()), reinterpret_cast<const wchar_t*>(wpath_new.c_str()), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) == FALSE) {
      auto err = ::GetLastError();
      ASTERIA_DEBUG_LOG("`MoveFileExW()` failed on \'", path_old, "\' and \'", path_new, "\' (last error was `", err, "`).");
#else
    if(::rename(path_old.c_str(), path_new.c_str()) != 0) {
      auto err = errno;
      ASTERIA_DEBUG_LOG("`rename()` failed on \'", path_old, "\' and \'", path_new, "\' (errno was `", err, "`).");
#endif
      return false;
    }
    return true;
  }

Opt<G_integer> std_filesystem_remove_recursive(const G_string& path)
  {
    G_integer count = 0;
    return 8;
  }

Opt<G_integer> std_filesystem_directory_create(const G_string& path)
  {
    G_integer count = 1;
#ifdef _WIN32
    auto wpath = do_translate_winnt_path(path);
    if(::CreateDirectoryW(reinterpret_cast<const wchar_t*>(wpath.c_str()), nullptr) == FALSE) {
      auto err = ::GetLastError();
      if(err != ERROR_ALREADY_EXISTS) {
        ASTERIA_DEBUG_LOG("`CreateDirectoryW()` failed on \'", path, "\' (last error was `", err, "`).");
        return rocket::nullopt;
      }
      // Fail only if it is not a directory that exists.
      auto attr = ::GetFileAttributesW(reinterpret_cast<const wchar_t*>(wpath.c_str()));
      if(attr == INVALID_FILE_ATTRIBUTES) {
        err = ::GetLastError();
        ASTERIA_DEBUG_LOG("`GetFileAttributesW()` failed on \'", path, "\' (last error was `", err, "`).");
        return rocket::nullopt;
      }
      if(!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
#else
    if(::mkdir(path.c_str(), 0777) != 0) {
      auto err = errno;
      if(err != EEXIST) {
        ASTERIA_DEBUG_LOG("`mkdir()` failed on \'", path, "\' (errno was `", err, "`).");
        return rocket::nullopt;
      }
      // Fail only if it is not a directory that exists.
      struct ::stat stb;
      if(::stat(path.c_str(), &stb) != 0) {
        err = errno;
        ASTERIA_DEBUG_LOG("`stat()` failed on \'", path, "\' (errno was `", err, "`).");
        return rocket::nullopt;
      }
      if(!S_ISDIR(stb.st_mode)) {
#endif
        ASTERIA_DEBUG_LOG("A file that is not a directory exists on \'", path, "\'.");
        return rocket::nullopt;
      }
      count = 0;
    }
    return rocket::move(count);
  }

Opt<G_array> std_filesystem_directory_list(const G_string& path)
  {
    G_array children;
#ifdef _WIN32
    auto wpath = do_translate_winnt_path(path);
    // Remove trailing slashes if any.
    wpath.erase(wpath.find_last_not_of(u"\\/") + 1);
    // Append a wild card which will match all files and subdirectories.
    wpath.append(u"\\*");
    // Open it later...
    Directory_Handle hd;
    for(;;) {
      ::WIN32_FIND_DATAW entry;
      if(!hd) {
        // Open a find handle and read the first file.
        if(!hd.reset(::FindFirstFileW(reinterpret_cast<const wchar_t*>(wpath.c_str()), &entry))) {
          auto err = ::GetLastError();
          if(err != ERROR_FILE_NOT_FOUND) {
            ASTERIA_DEBUG_LOG("`FindFirstFileW()` failed on \'", path, "\' (last error was `", err, "`).");
            return rocket::nullopt;
          }
          // The directory is empty.
          break;
        }
      } else {
        // Read the next file.
        if(!::FindNextFileW(hd, &entry)) {
          break;
        }
      }
      if((std::wcscmp(entry.cFileName, L".") == 0) || (std::wcscmp(entry.cFileName, L"..") == 0)) {
        continue;
      }
      // Convert the file name back into UTF-8.
      // We only want to stop when a NUL character is encountered.
      G_string child;
      auto pos = reinterpret_cast<const char16_t*>(entry.cFileName);
      for(;;) {
        char32_t cp;
        if(!utf16_decode(cp, pos, SIZE_MAX)) {
          ASTERIA_THROW_RUNTIME_ERROR("The directory \'", path, "\' contains a file whose name is not valid UTF-16.");
        }
        if(cp == 0) {
          break;
        }
        utf8_encode(child, cp);
      }
      children.emplace_back(rocket::move(child));
    }
#else
    Directory_Handle hd(::opendir(path.c_str()));
    for(;;) {
      // Get an entry.
      auto entry = ::readdir(hd);
      if(!entry) {
        break;
      }
      if((std::strcmp(entry->d_name, ".") == 0) || (std::strcmp(entry->d_name, "..") == 0)) {
        continue;
      }
      children.emplace_back(G_string(entry->d_name));
    }
#endif
    return rocket::move(children);
  }

Opt<G_integer> std_filesystem_directory_remove(const G_string& path)
  {
    G_integer count = 1;
#ifdef _WIN32
    auto wpath = do_translate_winnt_path(path);
    if(::RemoveDirectoryW(reinterpret_cast<const wchar_t*>(wpath.c_str())) == FALSE) {
      auto err = ::GetLastError();
      if(err != ERROR_DIR_NOT_EMPTY) {
        ASTERIA_DEBUG_LOG("`RemoveDirectoryW()` failed on \'", path, "\' (last error was `", err, "`).");
#else
    if(::rmdir(path.c_str()) != 0) {
      auto err = errno;
      if(err != ENOTEMPTY) {
        ASTERIA_DEBUG_LOG("`rmdir()` failed on \'", path, "\' (errno was `", err, "`).");
#endif
        return rocket::nullopt;
      }
      count = 0;
    }
    return rocket::move(count);
  }

Opt<G_string> std_filesystem_file_read(const G_string& path, const Opt<G_integer>& offset, const Opt<G_integer>& limit)
  {
    if(offset && (*offset < 0)) {
      ASTERIA_THROW_RUNTIME_ERROR("The file offset shall not be negative (got `", *offset, "`).");
    }
    std::int64_t roffset = offset.value_or(0);
    // Don't read too many bytes at a time.
    G_string data(static_cast<std::size_t>(rocket::clamp(limit.value_or(65536), 0, 1048576)), '*');
#ifdef _WIN32
    auto wpath = do_translate_winnt_path(path);
    // Open the file for reading.
    File hf(::CreateFileW(reinterpret_cast<const wchar_t*>(wpath.c_str()),
                          FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    if(!hf) {
      auto err = ::GetLastError();
      ASTERIA_DEBUG_LOG("`CreateFileW()` failed on \'", path, "\' (last error was `", err, "`).");
      return rocket::nullopt;
    }
    // Set the read position.
    ::OVERLAPPED ctx = { };
    ctx.OffsetHigh = static_cast<::DWORD>(roffset >> 32);
    ctx.Offset = static_cast<::DWORD>(roffset);
    // Read data from the offset specified.
    ::DWORD nread;
    if(::ReadFile(hf, data.mut_data(), static_cast<::DWORD>(data.size()), &nread, &ctx) == FALSE) {
      auto err = ::GetLastError();
      // This error can happen if `roffset` is past the end of the file.
      if(err != ERROR_HANDLE_EOF) {
        ASTERIA_DEBUG_LOG("`ReadFile()` failed on \'", path, "\' (last error was `", err, "`).");
        return rocket::nullopt;
      }
      nread = 0;
    }
    data.erase(nread);
#else
    // Open the file for reading.
    File hf(::open(path.c_str(), O_RDONLY));
    if(!hf) {
      auto err = errno;
      ASTERIA_DEBUG_LOG("`open()` failed on \'", path, "\' (errno was `", err, "`).");
      return rocket::nullopt;
    }
    // Read data from the offset specified.
    ::ssize_t nread = ::pread(hf, data.mut_data(), data.size(), roffset);
    if(nread < 0) {
      auto err = errno;
      ASTERIA_DEBUG_LOG("`pread()` failed on \'", path, "\' (errno was `", err, "`).");
      return rocket::nullopt;
    }
    data.erase(static_cast<std::size_t>(nread));
#endif
    return rocket::move(data);
  }

bool std_filesystem_file_write(const G_string& path, const G_string& data, const Opt<G_integer>& offset)
  {
    if(offset && (*offset < 0)) {
      ASTERIA_THROW_RUNTIME_ERROR("The file offset shall not be negative (got `", *offset, "`).");
    }
    std::int64_t roffset = offset.value_or(0);
    // This is a signed integer.
    auto nremaining = data.ssize();
#ifdef _WIN32
    auto wpath = do_translate_winnt_path(path);
    // Calculate the `dwCreationDisposition` argument.
    // If we are to write from the beginning, truncate the file at creation.
    // This saves us two syscalls to truncate the file below.
    ::DWORD create_disposition = OPEN_ALWAYS;
    if(roffset == 0) {
      create_disposition = CREATE_ALWAYS;
    }
    // Open the file for writing.
    File hf(::CreateFileW(reinterpret_cast<const wchar_t*>(wpath.c_str()),
                          FILE_WRITE_DATA, 0, nullptr, create_disposition, FILE_ATTRIBUTE_NORMAL, NULL));
    if(!hf) {
      auto err = ::GetLastError();
      ASTERIA_DEBUG_LOG("`CreateFileW()` failed on \'", path, "\' (last error was `", err, "`).");
      return false;
    }
    if(roffset != 0) {
      // If `roffset` is not zero, truncate the file there.
      // Otherwise, the file will have been truncate at creation.
      ::LARGE_INTEGER fpos;
      fpos.QuadPart = roffset;
      if(::SetFilePointerEx(hf, fpos, nullptr, FILE_BEGIN) == FALSE) {
        auto err = ::GetLastError();
        ASTERIA_DEBUG_LOG("`SetFilePointerEx()` failed on \'", path, "\' (last error was `", err, "`).");
        return false;
      }
      if(::SetEndOfFile(hf) == FALSE) {
        auto err = ::GetLastError();
        ASTERIA_DEBUG_LOG("`SetEndOfFile()` failed on \'", path, "\' (last error was `", err, "`).");
        return false;
      }
    }
    // Write data in a loop.
    while(nremaining > 0) {
      // Set the write position.
      ::OVERLAPPED ctx = { };
      ctx.OffsetHigh = static_cast<::DWORD>(roffset >> 32);
      ctx.Offset = static_cast<::DWORD>(roffset);
      // Write data to the offset specified.
      ::DWORD nwritten;
      if(::WriteFile(hf, data.data() + data.size() - nremaining, static_cast<::DWORD>(rocket::min(nremaining, INT_MAX)), &nwritten, &ctx) == FALSE) {
        auto err = ::GetLastError();
        ASTERIA_DEBUG_LOG("`WriteFile()` failed on \'", path, "\' (last error was `", err, "`).");
        return false;
      }
      nremaining -= static_cast<int>(nwritten);
      roffset += nwritten;
    }
#else
    // Calculate the `flags` argument.
    // If we are to write from the beginning, truncate the file at creation.
    // This saves us two syscalls to truncate the file below.
    int flags = O_WRONLY | O_CREAT;
    if(roffset == 0) {
      flags |= O_TRUNC;
    }
    // Open the file for writing.
    File hf(::open(path.c_str(), flags, 0666));
    if(!hf) {
      auto err = errno;
      ASTERIA_DEBUG_LOG("`open()` failed on \'", path, "\' (errno was `", err, "`).");
      return false;
    }
    if(roffset != 0) {
      // If `roffset` is not zero, truncate the file there.
      // Otherwise, the file will have been truncate at creation.
      if(::ftruncate(hf, roffset) != 0) {
        auto err = errno;
        ASTERIA_DEBUG_LOG("`ftruncate()` failed on \'", path, "\' (errno was `", err, "`).");
        return false;
      }
    }
    // Write data in a loop.
    while(nremaining > 0) {
      // Write data to the offset specified.
      ::ssize_t nwritten = ::pwrite(hf, data.data() + data.size() - nremaining, static_cast<std::size_t>(nremaining), roffset);
      if(nwritten < 0) {
        auto err = errno;
        ASTERIA_DEBUG_LOG("`pwrite()` failed on \'", path, "\' (errno was `", err, "`).");
        return false;
      }
      nremaining -= nwritten;
      roffset += nwritten;
    }
#endif
    return true;
  }

bool std_filesystem_file_append(const G_string& path, const G_string& data, const Opt<G_boolean>& exclusive)
  {
    // This is a signed integer.
    auto nremaining = data.ssize();
#ifdef _WIN32
    auto wpath = do_translate_winnt_path(path);
    // Calculate the `dwCreationDisposition` argument.
    // If `exclusive` is set to `true`, fail if the file exists.
    ::DWORD create_disposition = OPEN_ALWAYS;
    if(exclusive == true) {
      create_disposition = CREATE_NEW;
    }
    // Open the file for writing.
    File hf(::CreateFileW(reinterpret_cast<const wchar_t*>(wpath.c_str()),
                          FILE_APPEND_DATA, 0, nullptr, create_disposition, FILE_ATTRIBUTE_NORMAL, NULL));
    if(!hf) {
      auto err = ::GetLastError();
      ASTERIA_DEBUG_LOG("`CreateFileW()` failed on \'", path, "\' (last error was `", err, "`).");
      return false;
    }
    // Write data in a loop.
    while(nremaining > 0) {
      // Set the write position.
      ::OVERLAPPED ctx = { };
      ctx.OffsetHigh = UINT32_MAX;
      ctx.Offset = UINT32_MAX;
      // Write data to the offset specified.
      ::DWORD nwritten;
      if(::WriteFile(hf, data.data() + data.size() - nremaining, static_cast<::DWORD>(rocket::min(nremaining, INT_MAX)), &nwritten, &ctx) == FALSE) {
        auto err = ::GetLastError();
        ASTERIA_DEBUG_LOG("`WriteFile()` failed on \'", path, "\' (last error was `", err, "`).");
        return false;
      }
      nremaining -= static_cast<int>(nwritten);
    }
#else
    // Calculate the `flags` argument.
    // If we are to write from the beginning, truncate the file at creation.
    // This saves us two syscalls to truncate the file below.
    int flags = O_WRONLY | O_CREAT | O_APPEND;
    if(exclusive == true) {
      flags |= O_EXCL;
    }
    // Open the file for writing.
    File hf(::open(path.c_str(), flags, 0666));
    if(!hf) {
      auto err = errno;
      ASTERIA_DEBUG_LOG("`open()` failed on \'", path, "\' (errno was `", err, "`).");
      return false;
    }
    // Write data in a loop.
    while(nremaining > 0) {
      // Write data to the offset specified.
      ::ssize_t nwritten = ::write(hf, data.data() + data.size() - nremaining, static_cast<std::size_t>(nremaining));
      if(nwritten < 0) {
        auto err = errno;
        ASTERIA_DEBUG_LOG("`pwrite()` failed on \'", path, "\' (errno was `", err, "`).");
        return false;
      }
      nremaining -= nwritten;
    }
#endif
    return true;
  }

bool std_filesystem_file_remove(const G_string& path)
  {
#ifdef _WIN32
    auto wpath = do_translate_winnt_path(path);
    if(::DeleteFileW(reinterpret_cast<const wchar_t*>(wpath.c_str())) == FALSE) {
      auto err = ::GetLastError();
      ASTERIA_DEBUG_LOG("`DeleteFileW()` failed on \'", path, "\' (last error was `", err, "`).");
#else
    if(::unlink(path.c_str()) != 0) {
      auto err = errno;
      ASTERIA_DEBUG_LOG("`unlink()` failed on \'", path, "\' (errno was `", err, "`).");
#endif
      return false;
    }
    return true;
  }

void create_bindings_filesystem(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.filesystem.get_working_directory()`
    //===================================================================
    result.insert_or_assign(rocket::sref("get_working_directory"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.get_working_directory()`\n"
            "  * Gets the absolute path of the current working directory.\n"
            "  * Returns a `string` containing the path to the current working\n"
            "    directory.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.get_working_directory"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_filesystem_get_working_directory() };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.filesystem.get_information()`
    //===================================================================
    result.insert_or_assign(rocket::sref("get_information"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.get_information(path)`\n"
            "  * Retrieves information of the file or directory designated by\n"
            "    `path`.\n"
            "  * Returns an `object` consisting of the following members (names\n"
            "    that start with `b_` are `boolean` flags; names that start with\n"
            "    `i_` are IDs as `integer`s; names that start with `n_` are\n"
            "    plain `integer`s; names that start with `t_` are timestamps in\n"
            "    UTC as `integer`s):\n"
            "    * `i_dev`   unique device id on this machine.\n"
            "    * `i_file`  unique file id on this device.\n"
            "    * `n_ref`   number of hard links to this file.\n"
            "    * `b_dir`   whether this is a directory.\n"
            "    * `b_sym`   whether this is a symbol link.\n"
            "    * `n_size`  number of bytes this file contains.\n"
            "    * `n_ocup`  number of bytes this file occupies.\n"
            "    * `t_accs`  timestamp of last access.\n"
            "    * `t_mod`   timestamp of last modification.\n"
            "    On failure, `null` is returned.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.get_information"), args);
            // Parse arguments.
            G_string path;
            if(reader.start().g(path).finish()) {
              // Call the binding function.
              auto qres = std_filesystem_get_information(path);
              if(!qres) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qres) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.filesystem.remove_recursive()`
    //===================================================================
    result.insert_or_assign(rocket::sref("remove_recursive"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.remove_recursive(path)`\n"
            "  * Removes the file or directory at `path`. If `path` designates\n"
            "    a directory, all of its contents are removed recursively.\n"
            "  * Returns the number of files and directories that have been\n"
            "    successfully removed in total, or `null` on failure.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.remove_recursive"), args);
            // Parse arguments.
            G_string path;
            if(reader.start().g(path).finish()) {
              // Call the binding function.
              auto qres = std_filesystem_remove_recursive(path);
              if(!qres) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qres) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.filesystem.move_from(path_new, path_old)`
    //===================================================================
    result.insert_or_assign(rocket::sref("move_from"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.move_from(path_new, path_old)`\n"
            "  * Moves (renames) the file or directory at `path_old` to\n"
            "    `path_new`.\n"
            "  * Returns `true` on success, or `null` on failure.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.move_from"), args);
            // Parse arguments.
            G_string path_new;
            G_string path_old;
            if(reader.start().g(path_new).g(path_old).finish()) {
              // Call the binding function.
              if(!std_filesystem_move_from(path_new, path_old)) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { true };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.filesystem.directory_create()`
    //===================================================================
    result.insert_or_assign(rocket::sref("directory_create"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.directory_create(path)`\n"
            "  * Creates a directory at `path`. Its parent directory must exist\n"
            "    and must be accessible. This function does not fail if either\n"
            "    a directory or a symbol link to a directory already exists on\n"
            "    `path`.\n"
            "  * Returns `1` if a new directory has been created successfully,\n"
            "    `0` if the directory already exists, or `null` on failure.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.directory_create"), args);
            // Parse arguments.
            G_string path;
            if(reader.start().g(path).finish()) {
              // Call the binding function.
              auto qres = std_filesystem_directory_create(path);
              if(!qres) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qres) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.filesystem.directory_list()`
    //===================================================================
    result.insert_or_assign(rocket::sref("directory_list"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.directory_list(path)`\n"
            "  * Lists the contents of the directory at `path`.\n"
            "  * Returns names of all files and subdirectories as an `array` of\n"
            "    `string`s, or `null` on failure.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.directory_list"), args);
            // Parse arguments.
            G_string path;
            if(reader.start().g(path).finish()) {
              // Call the binding function.
              auto qres = std_filesystem_directory_list(path);
              if(!qres) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qres) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.filesystem.directory_remove()`
    //===================================================================
    result.insert_or_assign(rocket::sref("directory_remove"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.directory_remove(path)`\n"
            "  * Removes the directory at `path`. The directory must be empty.\n"
            "    This function fails if `path` does not designate a directory.\n"
            "  * Returns `1` if the directory has been removed successfully, `0`\n"
            "    if it is not empty, or `null` on failure.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.directory_remove"), args);
            // Parse arguments.
            G_string path;
            if(reader.start().g(path).finish()) {
              // Call the binding function.
              auto qres = std_filesystem_directory_remove(path);
              if(!qres) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qres) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.filesystem.file_read()`
    //===================================================================
    result.insert_or_assign(rocket::sref("file_read"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.file_read(path, [offset], [limit])`\n"
            "  * Reads the file at `path` in binary mode. The read operation\n"
            "    starts from the byte offset that is denoted by `offset` if it\n"
            "    is specified, or from the beginning of the file otherwise. If\n"
            "    `limit` is specified, no more than this number of bytes will be\n"
            "    read.\n"
            "  * Returns the bytes that have been read as a `string`, or `null`\n"
            "    on failure.\n"
            "  * Throws an exception of `offset` is negative.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.file_read"), args);
            // Parse arguments.
            G_string path;
            Opt<G_integer> offset;
            Opt<G_integer> limit;
            if(reader.start().g(path).g(offset).g(limit).finish()) {
              // Call the binding function.
              auto qres = std_filesystem_file_read(path, offset, limit);
              if(!qres) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qres) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.filesystem.file_write()`
    //===================================================================
    result.insert_or_assign(rocket::sref("file_write"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.file_write(path, data, [offset])`\n"
            "  * Writes the file at `path` in binary mode. The write operation\n"
            "    starts from the byte offset that is denoted by `offset` if it\n"
            "    is specified, or from the beginning of the file otherwise. The\n"
            "    file is truncated to this length before the write operation;\n"
            "    any existent contents after the write point are discarded. This\n"
            "    function fails if the data can only be written partially.\n"
            "  * Returns `true` if all data have been written successfully, or\n"
            "    `null` on failure.\n"
            "  * Throws an exception of `offset` is negative.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.file_write"), args);
            // Parse arguments.
            G_string path;
            G_string data;
            Opt<G_integer> offset;
            if(reader.start().g(path).g(data).g(offset).finish()) {
              // Call the binding function.
              if(!std_filesystem_file_write(path, data, offset)) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { true };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.filesystem.file_append()`
    //===================================================================
    result.insert_or_assign(rocket::sref("file_append"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.file_append(path, data)`\n"
            "  * Writes the file at `path` in binary mode. The write operation\n"
            "    starts from the end of the file; existent contents of the file\n"
            "    are left intact. If `exclusive` is `true` and a file exists on\n"
            "    `path`, this function fails. This function also fails if the\n"
            "    data can only be written partially.\n"
            "  * Returns `true` if all data have been written successfully, or\n"
            "    `null` on failure.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.file_append"), args);
            // Parse arguments.
            G_string path;
            G_string data;
            Opt<G_boolean> exclusive;
            if(reader.start().g(path).g(data).g(exclusive).finish()) {
              // Call the binding function.
              if(!std_filesystem_file_append(path, data, exclusive)) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { true };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.filesystem.file_remove()`
    //===================================================================
    result.insert_or_assign(rocket::sref("file_remove"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "`std.filesystem.file_remove(path)`\n"
            "  * Removes the file at `path`. This function fails if `path`\n"
            "    designates a directory.\n"
            "  * Returns `true` if the file has been removed successfully, or\n"
            "    `null` on failure.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.filesystem.file_remove"), args);
            // Parse arguments.
            G_string path;
            if(reader.start().g(path).finish()) {
              // Call the binding function.
              if(!std_filesystem_file_remove(path)) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { true };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // End of `std.filesystem`
    //===================================================================
  }

}  // namespace Asteria
