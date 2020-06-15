// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "filesystem.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../utilities.hpp"
#include <sys/stat.h>  // ::stat(), ::fstat(), ::lstat(), ::mkdir(), ::fchmod()
#include <dirent.h>  // ::opendir(), ::closedir()
#include <fcntl.h>  // ::open()
#include <unistd.h>  // ::rmdir(), ::close(), ::read(), ::write(), ::unlink()
#include <stdio.h>  // ::rename()
#include <errno.h>  // errno

namespace asteria {
namespace {

enum RM_Disp
  {
    rm_disp_rmdir,     // a subdirectory which should be empty and can be removed
    rm_disp_unlink,    // a plain file to be unlinked
    rm_disp_expand,    // a subdirectory to be expanded
  };

struct RM_Element
  {
    RM_Disp disp;
    V_string path;
  };

int64_t
do_remove_recursive(const char* path)
  {
    // Push the first element.
    cow_vector<RM_Element> stack;
    stack.push_back({ rm_disp_expand, ::rocket::sref(path) });

    // Expand non-empty directories and remove all contents.
    int64_t nremoved = 0;
    while(stack.size()) {
      auto elem = ::std::move(stack.mut_back());
      stack.pop_back();

      // Process this element.
      switch(elem.disp) {
        case rm_disp_rmdir: {
          // This is an empty directory. Remove it.
          if(::rmdir(elem.path.c_str()) == 0) {
            nremoved++;
            break;
          }

          // Hmm... avoid TOCTTOU errors.
          if(errno == ENOENT)
            break;

          ASTERIA_THROW("Could not remove directory '$2'\n"
                        "[`rmdir()` failed: $1]",
                        noadl::format_errno(errno), elem.path);
        }

        case rm_disp_unlink: {
          // This is a non-directory. Unlink it.
          if(::unlink(elem.path.c_str()) == 0) {
            nremoved++;
            break;
          }

          // Hmm... avoid TOCTTOU errors.
          if(errno == ENOENT)
            break;

          ASTERIA_THROW("Could not remove file '$2'\n"
                        "[`unlink()` failed: $1]",
                        noadl::format_errno(errno), elem.path);
        }

        case rm_disp_expand: {
          // This is a subdirectory that has not been expanded.
          // Push the directory itself.
          // Since elements are maintained in LIFO order, only when this element is encountered
          // for a second time, will all of its children have been removed.
          stack.push_back({ rm_disp_rmdir, elem.path });

          // Open the directory for listing.
          ::rocket::unique_posix_dir dp(::opendir(elem.path.c_str()), ::closedir);
          if(!dp)
            ASTERIA_THROW("Could not open directory '$2'\n"
                          "[`opendir()` failed: $1]",
                          noadl::format_errno(errno), elem.path);

          // Append all entries.
          while(auto next = ::readdir(dp)) {
            // Skip special entries.
            if(::strcmp(next->d_name, ".") == 0)
              continue;

            if(::strcmp(next->d_name, "..") == 0)
              continue;

            // Get the name and type of this entry.
            cow_string child = elem.path + '/' + next->d_name;
            bool is_dir = false;

#ifdef _DIRENT_HAVE_D_TYPE
            if(next->d_type != DT_UNKNOWN) {
              // Get the file type if it is available immediately.
              is_dir = next->d_type == DT_DIR;
            }
            else
#endif
            {
              // If the file type is unknown, ask for it.
              struct ::stat stb;
              if(::lstat(child.c_str(), &stb) != 0)
                ASTERIA_THROW("Could not get information about '$2'\n"
                              "[`lstat()` failed: $1]",
                              noadl::format_errno(errno), child);

              // Check whether the child path denotes a directory.
              is_dir = S_ISDIR(stb.st_mode);
            }

            // Append this entry.
            stack.push_back({ is_dir ? rm_disp_expand : rm_disp_unlink, ::std::move(child) });
          }
          break;
        }

        default:
          ROCKET_ASSERT(false);
      }
    }
    return nremoved;
  }

const void*
do_write_loop(int fd, const void* data, size_t size, const V_string& path)
  {
    auto bp = static_cast<const char*>(data);
    auto ep = bp + size;

    while(bp < ep) {
      ::ssize_t nwrtn = ::write(fd, bp, static_cast<size_t>(ep - bp));
      if(nwrtn < 0)
        ASTERIA_THROW("Error writing file '$2'\n"
                      "[`write()` failed: $1]",
                      noadl::format_errno(errno), path);
      bp += nwrtn;
    }
    return bp;
  }

}  // namespace

V_string
std_filesystem_get_working_directory()
  {
    // Pass a null pointer to request dynamic allocation.
    // Note this behavior is an extension that exists almost everywhere.
    uptr<char, void (&)(void*)> qcwd(::getcwd(nullptr, 0), ::free);
    if(!qcwd)
      ASTERIA_THROW("Could not get current working directory\n"
                    "[`getcwd()` failed: $1]",
                    noadl::format_errno(errno));
    return V_string(qcwd);
  }

V_string
std_filesystem_get_real_path(V_string path)
  {
    // Pass a null pointer to request dynamic allocation.
    uptr<char, void (&)(void*)> abspath(::realpath(path.safe_c_str(), nullptr), ::free);
    if(!abspath)
      ASTERIA_THROW("Could not resolve path '$2'\n"
                    "[`realpath()` failed: $1]",
                    noadl::format_errno(errno), path);
    return V_string(abspath);
  }

optV_object
std_filesystem_get_information(V_string path)
  {
    struct ::stat stb;
    if(::lstat(path.safe_c_str(), &stb) != 0)
      return nullopt;

    // Convert the result to an `object`.
    V_object stat;
    stat.try_emplace(::rocket::sref("i_dev"),
      V_integer(
        stb.st_dev  // unique device id on this machine
      ));
    stat.try_emplace(::rocket::sref("i_file"),
      V_integer(
        stb.st_ino  // unique file id on this device
      ));
    stat.try_emplace(::rocket::sref("n_ref"),
      V_integer(
        stb.st_nlink  // number of hard links to this file
      ));
    stat.try_emplace(::rocket::sref("b_dir"),
      V_boolean(
        S_ISDIR(stb.st_mode)  // whether this is a directory
      ));
    stat.try_emplace(::rocket::sref("b_sym"),
      V_boolean(
        S_ISLNK(stb.st_mode)  // whether this is a symbolic link
      ));
    stat.try_emplace(::rocket::sref("n_size"),
      V_integer(
        stb.st_size  // number of bytes this file contains
      ));
    stat.try_emplace(::rocket::sref("n_ocup"),
      V_integer(
        int64_t(stb.st_blocks) * 512  // number of bytes this file occupies
      ));
    stat.try_emplace(::rocket::sref("t_accs"),
      V_integer(
        int64_t(stb.st_atim.tv_sec) * 1000 + stb.st_atim.tv_nsec / 1000000  // timestamp of last access
      ));
    stat.try_emplace(::rocket::sref("t_mod"),
      V_integer(
        int64_t(stb.st_mtim.tv_sec) * 1000 + stb.st_mtim.tv_nsec / 1000000  // timestamp of last modification
      ));
    return ::std::move(stat);
  }

void
std_filesystem_move_from(V_string path_new, V_string path_old)
  {
    if(::rename(path_old.safe_c_str(), path_new.safe_c_str()) != 0)
      ASTERIA_THROW("Could not move file '$2' to '$3'\n"
                    "[`rename()` failed: $1]",
                    noadl::format_errno(errno), path_old, path_new);
  }

V_integer
std_filesystem_remove_recursive(V_string path)
  {
    // Try removing an empty directory
    if(::rmdir(path.safe_c_str()) == 0)
      return 1;

    // Get some detailed information from `errno`.
    switch(errno) {
      case ENOENT:
        // The path does not exist.
        return 0;

      case ENOTDIR: {
        // This is something not a directory.
        if(::unlink(path.safe_c_str()) == 0)
          return 1;

        // Hmm... avoid TOCTTOU errors.
        if(errno == ENOENT)
          return 0;

        ASTERIA_THROW("Could not remove file '$2'\n"
                      "[`unlink()` failed: $1]",
                      noadl::format_errno(errno), path);
      }

      case EEXIST:
      case ENOTEMPTY:
        // Remove contents first.
        return do_remove_recursive(path.safe_c_str());
    }

    // Throw an exception for general failures.
    ASTERIA_THROW("Could not remove directory '$2'\n"
                  "[`rmdir()` failed: $1]",
                  noadl::format_errno(errno), path);
  }

V_object
std_filesystem_directory_list(V_string path)
  {
    // Try opening t he directory.
    ::rocket::unique_posix_dir dp(::opendir(path.safe_c_str()), ::closedir);
    if(!dp)
      ASTERIA_THROW("Could not open directory '$2'\n"
                    "[`opendir()` failed: $1]",
                    noadl::format_errno(errno), path);

    // Append all entries.
    V_object entries;
    while(auto next = ::readdir(dp)) {
      // Skip special entries.
      if(::strcmp(next->d_name, ".") == 0)
        continue;

      if(::strcmp(next->d_name, "..") == 0)
        continue;

      // Compose the full path of the child.
      cow_string child = path + '/' + next->d_name;
      bool is_dir = false;
      bool is_sym = false;

#ifdef _DIRENT_HAVE_D_TYPE
      if(next->d_type != DT_UNKNOWN) {
        // Get the file type if it is available immediately.
        is_dir = next->d_type == DT_DIR;
        is_dir = next->d_type == DT_LNK;
      }
      else
#endif
      {
        // If the file type is unknown, ask for it.
        struct ::stat stb;
        if(::lstat(child.c_str(), &stb) != 0)
          ASTERIA_THROW("Could not get information about '$2'\n"
                        "[`lstat()` failed: $1]",
                        noadl::format_errno(errno), child);

        // Check whether the child path denotes a directory.
        is_dir = S_ISDIR(stb.st_mode);
        is_sym = S_ISLNK(stb.st_mode);
      }

      // Append this entry, assuming the name is in UTF-8.
      V_object entry;
      entry.try_emplace(::rocket::sref("b_dir"),
        V_boolean(
          is_dir
        ));
      entry.try_emplace(::rocket::sref("b_sym"),
        V_boolean(
          is_sym
        ));
      entries.try_emplace(cow_string(next->d_name), ::std::move(entry));
    }
    return entries;
  }

V_integer
std_filesystem_directory_create(V_string path)
  {
    // Try creating an empty directory.
    if(::mkdir(path.safe_c_str(), 0777) == 0)
      return 1;

    // If the path references a directory or a symlink to a directory, don't fail.
    if(errno == EEXIST) {
      struct ::stat stb;
      if(::stat(path.c_str(), &stb) != 0)
        ASTERIA_THROW("Could not get information about '$2'\n"
                      "[`stat()` failed: $1]",
                      noadl::format_errno(errno), path);

      if(S_ISDIR(stb.st_mode))
        return 0;

      // Throw an exception about the previous error.
      ASTERIA_THROW("Could not create directory '$2'\n"
                    "[`mkdir()` failed: $1]",
                    noadl::format_errno(EEXIST), path);
    }

    // Throw an exception for general failures.
    ASTERIA_THROW("Could not create directory '$2'\n"
                  "[`mkdir()` failed: $1]",
                  noadl::format_errno(errno), path);
  }

V_integer
std_filesystem_directory_remove(V_string path)
  {
    // Try removing an empty directory.
    if(::rmdir(path.safe_c_str()) == 0)
      return 1;

    // If the path does not exist, don't fail.
    if(errno == ENOENT)
      return 0;

    // Throw an exception for general failures.
    ASTERIA_THROW("Could remove directory '$2'\n"
                  "[`rmdir()` failed: $1]",
                  noadl::format_errno(errno), path);
  }

V_string
std_filesystem_file_read(V_string path, optV_integer offset, optV_integer limit)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW("Negative file offset (offset `$1`)", *offset);
    int64_t roffset = offset.value_or(0);
    int64_t rlimit = ::rocket::clamp(limit.value_or(INT32_MAX), 0, 0x10'00000);

    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), O_RDONLY), ::close);
    if(!fd)
      ASTERIA_THROW("Could not open file '$2'\n"
                    "[`open()` failed: $1]",
                    noadl::format_errno(errno), path);

    // We return data that have been read as a byte string.
    V_string data;
    ::ssize_t nread;

    // Don't read too many bytes at a time.
    data.resize(static_cast<size_t>(rlimit));
    if(offset) {
      nread = ::pread(fd, data.mut_data(), data.size(), roffset);
      if(nread < 0)
        ASTERIA_THROW("Error reading file '$2'\n"
                      "[`pread()` failed: $1]",
                      noadl::format_errno(errno), path);
    }
    else {
      nread = ::read(fd, data.mut_data(), data.size());
      if(nread < 0)
        ASTERIA_THROW("Error reading file '$2'\n"
                      "[`read()` failed: $1]",
                      noadl::format_errno(errno), path);
    }
    data.erase(static_cast<size_t>(nread));
    return data;
  }

V_integer
std_filesystem_file_stream(Global_Context& global, V_string path, V_function callback,
                           optV_integer offset, optV_integer limit)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW("Negative file offset (offset `$1`)", *offset);
    int64_t roffset = offset.value_or(0);
    int64_t rlimit = ::rocket::clamp(limit.value_or(INT32_MAX), 0, 0x10'00000);

    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), O_RDONLY), ::close);
    if(!fd)
      ASTERIA_THROW("Could not open file '$2'\n"
                    "[`open()` failed: $1]",
                    noadl::format_errno(errno), path);

    // These are reused for each iteration.
    V_string data;
    ::ssize_t nread;

    // Read and process all data in blocks.
    cow_vector<Reference> args;
    int64_t ntlimit = ::rocket::max(limit.value_or(INT64_MAX), 0);
    int64_t ntotal = 0;
    for(;;) {
      // Check for user-provided limit.
      if(ntotal >= ntlimit)
        break;

      // Don't read too many bytes at a time.
      data.resize(static_cast<size_t>(rlimit));
      if(offset) {
        nread = ::pread(fd, data.mut_data(), data.size(), roffset);
        if(nread < 0)
          ASTERIA_THROW("Error reading file '$2'\n"
                        "[`pread()` failed: $1]",
                        noadl::format_errno(errno), path);
      }
      else {
        nread = ::read(fd, data.mut_data(), data.size());
        if(nread < 0)
          ASTERIA_THROW("Error reading file '$2'\n"
                        "[`read()` failed: $1]",
                        noadl::format_errno(errno), path);
      }
      if(nread == 0)
        break;

      data.erase(static_cast<size_t>(nread));

      // Prepare arguments for the user-defined function.
      args.reserve(2);
      Reference_root::S_temporary xref = { roffset };
      args.emplace_back(::std::move(xref));
      xref.val = ::std::move(data);
      args.emplace_back(::std::move(xref));
      // Call the function but discard its return value.
      callback.invoke(global, ::std::move(args));

      // Read the next block.
      roffset += nread;
      ntotal += nread;
    }
    return ntotal;
  }

void
std_filesystem_file_write(V_string path, V_string data, optV_integer offset)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW("Negative file offset (offset `$1`)", *offset);
    int64_t roffset = offset.value_or(0);

    // Calculate the `flags` argument.
    // If we are to write from the beginning, truncate the file at creation.
    int flags = O_WRONLY | O_CREAT | O_APPEND;
    if(roffset == 0)
      flags |= O_TRUNC;

    // Open the file for writing.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), flags, 0666), ::close);
    if(!fd)
      ASTERIA_THROW("Could not open file '$2'\n"
                    "[`open()` failed: $1]",
                    noadl::format_errno(errno), path);

    // Set the file pointer when an offset is specified, even when it is an explicit zero.
    if(offset) {
      // This also ensures it is a normal file (not a pipe or socket whatsoever).
      if(::ftruncate(fd, roffset) != 0)
        ASTERIA_THROW("Could not truncate file '$2'\n"
                      "[`ftruncate()` failed: $1]",
                      noadl::format_errno(errno), path);
    }

    // Write all data.
    do_write_loop(fd, data.data(), data.size(), path);
  }

void
std_filesystem_file_append(V_string path, V_string data, optV_boolean exclusive)
  {
    // Calculate the `flags` argument.
    int flags = O_WRONLY | O_CREAT | O_APPEND;
    if(exclusive == true)
      flags |= O_EXCL;

    // Open the file for appending.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), flags, 0666), ::close);
    if(!fd)
      ASTERIA_THROW("Could not open file '$2'\n"
                    "[`open()` failed: $1]",
                    noadl::format_errno(errno), path);

    // Append all data to the end.
    do_write_loop(fd, data.data(), data.size(), path);
  }

void
std_filesystem_file_copy_from(V_string path_new, V_string path_old)
  {
    // Open the old file.
    ::rocket::unique_posix_fd fd_old(::open(path_old.safe_c_str(), O_RDONLY), ::close);
    if(!fd_old)
      ASTERIA_THROW("Could not open source file '$2'\n"
                    "[`open()` failed: $1]",
                    noadl::format_errno(errno), path_old);

    // Create the new file, discarding its contents.
    // The file is initially write-only.
    ::rocket::unique_posix_fd fd_new(::open(path_new.safe_c_str(),
                                            O_WRONLY | O_CREAT | O_TRUNC | O_APPEND,
                                            0200), ::close);
    if(!fd_new)
      ASTERIA_THROW("Could not create destination file '$2'\n"
                    "[`open()` failed: $1]",
                    noadl::format_errno(errno), path_new);

    // Get the file mode and preferred I/O block size.
    struct ::stat stb_old;
    if(::fstat(fd_old, &stb_old) != 0)
      ASTERIA_THROW("Could not get information about source file '$2'\n"
                    "[`fstat()` failed: $1]",
                    noadl::format_errno(errno), path_old);

    // Allocate the I/O buffer.
    size_t nbuf = static_cast<size_t>(stb_old.st_blksize | 0x10'00000);
    uptr<uint8_t, void (&)(void*)> pbuf(static_cast<uint8_t*>(::operator new(nbuf)),
                                        ::operator delete);

    // Copy all contents.
    for(;;) {
      ::ssize_t nread = ::read(fd_old, pbuf, nbuf);
      if(nread < 0)
        ASTERIA_THROW("Error reading file '$2'\n"
                      "[`read()` failed: $1]",
                      noadl::format_errno(errno), path_old);

      if(nread == 0)
        break;

      // Append all data to the end.
      do_write_loop(fd_new, pbuf, static_cast<size_t>(nread), path_new);
    }

    // Set the file mode. This must be the last operation.
    if(::fchmod(fd_new, stb_old.st_mode) != 0)
      ASTERIA_THROW("Could not set permission of '$2'\n"
                    "[`fchmod()` failed: $1]",
                    noadl::format_errno(errno), path_new);
  }

V_integer
std_filesystem_file_remove(V_string path)
  {
    // Try removing a non-directory.
    if(::unlink(path.safe_c_str()) == 0)
      return 1;

    // If the path does not exist, don't fail.
    if(errno == ENOENT)
      return 0;

    // Throw an exception for general failures.
    ASTERIA_THROW("Could not remove file '$2'\n"
                  "[`unlink()` failed: $1]",
                  noadl::format_errno(errno), path);
  }

void
create_bindings_filesystem(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.filesystem.get_working_directory()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("get_working_directory"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.get_working_directory()`

  * Gets the absolute path of the current working directory.

  * Returns a string containing the path to the current working
    directory.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.get_working_directory"));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_filesystem_get_working_directory() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.get_real_path()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("get_real_path"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.get_real_path(path)`

  * Converts `path` to an absolute one. The result is a canonical
    path that contains no symbolic links. The path must be valid
    and accessible.

  * Returns a string denoting the absolute path.

  * Throws an exception if `path` is invalid or inaccessible.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.get_real_path"));
    // Parse arguments.
    V_string path;
    if(reader.I().v(path).F()) {
      Reference_root::S_temporary xref = { std_filesystem_get_real_path(::std::move(path)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.get_information()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("get_information"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.get_information(path)`

  * Retrieves information of the file or directory designated by
    `path`.

  * Returns an object consisting of the following members (names
    that start with `b_` are boolean values; names that start with
    `i_` are IDs as integers; names that start with `n_` are
    plain integers; names that start with `t_` are timestamps in
    UTC as integers):

    * `i_dev`   unique device id on this machine.
    * `i_file`  unique file id on this device.
    * `n_ref`   number of hard links to this file.
    * `b_dir`   whether this is a directory.
    * `b_sym`   whether this is a symbolic link.
    * `n_size`  number of bytes this file contains.
    * `n_ocup`  number of bytes this file occupies.
    * `t_accs`  timestamp of last access.
    * `t_mod`   timestamp of last modification.

    On failure, `null` is returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.get_information"));
    // Parse arguments.
    V_string path;
    if(reader.I().v(path).F()) {
      Reference_root::S_temporary xref = { std_filesystem_get_information(::std::move(path)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.remove_recursive()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("remove_recursive"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.move_from(path_new, path_old)`

  * Moves (renames) the file or directory at `path_old` to
    `path_new`.

  * Throws an exception on failure.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.remove_recursive"));
    // Parse arguments.
    V_string path;
    if(reader.I().v(path).F()) {
      Reference_root::S_temporary xref = { std_filesystem_remove_recursive(::std::move(path)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.move_from(path_new, path_old)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("move_from"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.remove_recursive(path)`

  * Removes the file or directory at `path`. If `path` designates a
    directory, all of its contents are removed recursively.

  * Returns the number of files and directories that have been
    successfully removed in total. If `path` does not reference an
    existent file or directory, `0` is returned.

  * Throws an exception if the file or directory at `path` cannot
    be removed.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.move_from"));
    // Parse arguments.
    V_string path_new;
    V_string path_old;
    if(reader.I().v(path_new).v(path_old).F()) {
      std_filesystem_move_from(path_new, path_old);
      return self = Reference_root::S_void();
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.directory_list()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("directory_list"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.directory_list(path)`

  * Lists the contents of the directory at `path`.

  * Returns an object containing all entries of the directory at
    `path`, excluding the special subdirectories '.' and '..'. For
    each element, its key specifies the filename and the value is
    an object consisting of the following members (names that
    start with `b_` are boolean values; names that start with `i_`
    are IDs as integers):

    * `b_dir`   whether this is a directory.
    * `b_sym`   whether this is a symbolic link.

    If `path` references a non-existent directory, `null` is
    returned.

  * Throws an exception if `path` does not designate a directory,
    or some other errors occur.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.directory_list"));
    // Parse arguments.
    V_string path;
    if(reader.I().v(path).F()) {
      Reference_root::S_temporary xref = { std_filesystem_directory_list(::std::move(path)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.directory_create()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("directory_create"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.directory_create(path)`

  * Creates a directory at `path`. Its parent directory must exist
    and must be accessible. This function does not fail if either a
    directory or a symbolic link to a directory already exists on
    `path`.

  * Returns `1` if a new directory has been created, or `0` if a
    directory already exists.

  * Throws an exception if `path` designates a non-directory, or
    some other errors occur.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.directory_create"));
    // Parse arguments.
    V_string path;
    if(reader.I().v(path).F()) {
      Reference_root::S_temporary xref = { std_filesystem_directory_create(::std::move(path)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.directory_remove()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("directory_remove"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.directory_remove(path)`

  * Removes the directory at `path`. The directory must be empty.
    This function fails if `path` does not designate a directory.

  * Returns `1` if a directory has been removed successfully, or
    `0` if no such directory exists.

  * Throws an exception if `path` designates a non-directory, or
    some other errors occur.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.directory_remove"));
    // Parse arguments.
    V_string path;
    if(reader.I().v(path).F()) {
      Reference_root::S_temporary xref = { std_filesystem_directory_remove(::std::move(path)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.file_read()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_read"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_read(path, [offset], [limit])`

  * Reads the file at `path` in binary mode. The read operation
    starts from the byte offset that is denoted by `offset` if it
    is specified, or from the beginning of the file otherwise. If
    `limit` is specified, no more than this number of bytes will be
    read.

  * Returns the bytes that have been read as a string.

  * Throws an exception if `offset` is negative, or a read error
    occurs.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.file_read"));
    // Parse arguments.
    V_string path;
    optV_integer offset;
    optV_integer limit;
    if(reader.I().v(path).o(offset).o(limit).F()) {
      Reference_root::S_temporary xref = { std_filesystem_file_read(::std::move(path), ::std::move(offset),
                                                                    ::std::move(limit)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.file_stream()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_stream"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_stream(path, callback, [offset], [limit])`

  * Reads the file at `path` in binary mode and invokes `callback`
    with the data read repeatedly. `callback` shall be a binary
    function whose first argument is the absolute offset of the
    data block that has been read, and whose second argument is the
    bytes read and stored in a string. Data may be transferred in
    multiple blocks of variable sizes; the caller shall make no
    assumption about the number of times that `callback` will be
    called or the size of each individual block. The read operation
    starts from the byte offset that is denoted by `offset` if it
    is specified, or from the beginning of the file otherwise. If
    `limit` is specified, no more than this number of bytes will be
    read.

  * Returns the number of bytes that have been read and processed
    as an integer.

  * Throws an exception if `offset` is negative, or a read error
    occurs.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.file_stream"));
    // Parse arguments.
    V_string path;
    V_function callback;
    optV_integer offset;
    optV_integer limit;
    if(reader.I().v(path).v(callback).o(offset).o(limit).F()) {
      Reference_root::S_temporary xref = { std_filesystem_file_stream(global, path, callback, offset, limit) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.file_write()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_write"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_write(path, data, [offset])`

  * Writes the file at `path` in binary mode. The write operation
    starts from the byte offset that is denoted by `offset` if it
    is specified, or from the beginning of the file otherwise. The
    file is truncated to this length before the write operation;
    any existent contents after the write point are discarded. This
    function fails if the data can only be written partially.

  * Throws an exception if `offset` is negative, or a write error
    occurs.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.file_write"));
    // Parse arguments.
    V_string path;
    V_string data;
    optV_integer offset;
    if(reader.I().v(path).v(data).o(offset).F()) {
      std_filesystem_file_write(path, data, offset);
      return self = Reference_root::S_void();
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.file_append()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_append"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_append(path, data, [exclusive])`

  * Writes the file at `path` in binary mode. The write operation
    starts from the end of the file; existent contents of the file
    are left intact. If `exclusive` is `true` and a file exists on
    `path`, this function fails. This function also fails if the
    data can only be written partially.

  * Throws an exception if `offset` is negative, or a write error
    occurs.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.file_append"));
    // Parse arguments.
    V_string path;
    V_string data;
    optV_boolean exclusive;
    if(reader.I().v(path).v(data).o(exclusive).F()) {
      std_filesystem_file_append(path, data, exclusive);
      return self = Reference_root::S_void();
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.file_copy_from(path_new, path_old)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_copy_from"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_copy_from(path_new, path_old)`

  * Copies the file `path_old` to `path_new`. If `path_old` is a
    symbolic link, it is the target that will be copied, rather
    than the symbolic link itself. This function fails if
    `path_old` designates a directory.

  * Throws an exception on failure.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.file_copy_from"));
    // Parse arguments.
    V_string path_new;
    V_string path_old;
    if(reader.I().v(path_new).v(path_old).F()) {
      std_filesystem_file_copy_from(path_new, path_old);
      return self = Reference_root::S_void();
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.filesystem.file_remove()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_remove"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_remove(path)`

  * Removes the file at `path`. This function fails if `path`
    designates a directory.

  * Returns `1` if a file has been removed successfully, or `0` if
    no such file exists.

  * Throws an exception if `path` designates a directory, or some
    other errors occur.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.filesystem.file_remove"));
    // Parse arguments.
    V_string path;
    if(reader.I().v(path).F()) {
      Reference_root::S_temporary xref = { std_filesystem_file_remove(path) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));
  }

}  // namespace asteria
