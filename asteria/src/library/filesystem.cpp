// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "filesystem.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../llds/reference_stack.hpp"
#include "../utils.hpp"
#include <sys/stat.h>  // ::stat(), ::fstat(), ::lstat(), ::mkdir(), ::fchmod()
#include <dirent.h>  // ::opendir(), ::closedir()
#include <fcntl.h>  // ::open()
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
    stack.push_back({ rm_disp_expand, sref(path) });

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

          ASTERIA_THROW("could not remove directory '$2'\n"
                        "[`rmdir()` failed: $1]",
                        format_errno(errno), elem.path);
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

          ASTERIA_THROW("could not remove file '$2'\n"
                        "[`unlink()` failed: $1]",
                        format_errno(errno), elem.path);
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
            ASTERIA_THROW("could not open directory '$2'\n"
                          "[`opendir()` failed: $1]",
                          format_errno(errno), elem.path);

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
                ASTERIA_THROW("could not get information about '$2'\n"
                              "[`lstat()` failed: $1]",
                              format_errno(errno), child);

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
      if(nwrtn < 0) {
        ASTERIA_THROW("error writing file '$2'\n"
                      "[`write()` failed: $1]",
                      format_errno(errno), path);
      }
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
    auto qcwd = ::rocket::make_unique_handle(::getcwd(nullptr, 0), ::free);
    if(!qcwd) {
      ASTERIA_THROW("could not get current working directory\n"
                    "[`getcwd()` failed: $1]",
                    format_errno(errno));
    }
    return V_string(qcwd);
  }

V_string
std_filesystem_get_real_path(V_string path)
  {
    // Pass a null pointer to request dynamic allocation.
    auto abspath = ::rocket::make_unique_handle(::realpath(path.safe_c_str(), nullptr), ::free);
    if(!abspath) {
      ASTERIA_THROW("could not resolve path '$2'\n"
                    "[`realpath()` failed: $1]",
                    format_errno(errno), path);
    }
    return V_string(abspath);
  }

Opt_object
std_filesystem_get_information(V_string path)
  {
    struct ::stat stb;
    if(::lstat(path.safe_c_str(), &stb) != 0)
      return nullopt;

    // Convert the result to an `object`.
    V_object stat;
    stat.try_emplace(sref("i_dev"),
      V_integer(
        stb.st_dev  // unique device id on this machine
      ));
    stat.try_emplace(sref("i_file"),
      V_integer(
        stb.st_ino  // unique file id on this device
      ));
    stat.try_emplace(sref("n_ref"),
      V_integer(
        stb.st_nlink  // number of hard links to this file
      ));
    stat.try_emplace(sref("b_dir"),
      V_boolean(
        S_ISDIR(stb.st_mode)  // whether this is a directory
      ));
    stat.try_emplace(sref("b_sym"),
      V_boolean(
        S_ISLNK(stb.st_mode)  // whether this is a symbolic link
      ));
    stat.try_emplace(sref("n_size"),
      V_integer(
        stb.st_size  // number of bytes this file contains
      ));
    stat.try_emplace(sref("n_ocup"),
      V_integer(
        int64_t(stb.st_blocks) * 512  // number of bytes this file occupies
      ));
    stat.try_emplace(sref("t_accs"),
      V_integer(
        int64_t(stb.st_atim.tv_sec) * 1000
        + stb.st_atim.tv_nsec / 1000000  // timestamp of last access
      ));
    stat.try_emplace(sref("t_mod"),
      V_integer(
        int64_t(stb.st_mtim.tv_sec) * 1000
        + stb.st_mtim.tv_nsec / 1000000  // timestamp of last modification
      ));
    return ::std::move(stat);
  }

void
std_filesystem_move_from(V_string path_new, V_string path_old)
  {
    if(::rename(path_old.safe_c_str(), path_new.safe_c_str()) != 0)
      ASTERIA_THROW("could not move file '$2' to '$3'\n"
                    "[`rename()` failed: $1]",
                    format_errno(errno), path_old, path_new);
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

        ASTERIA_THROW("could not remove file '$2'\n"
                      "[`unlink()` failed: $1]",
                      format_errno(errno), path);
      }

      case EEXIST:
      case ENOTEMPTY:
        // Remove contents first.
        return do_remove_recursive(path.safe_c_str());
    }

    // Throw an exception for general failures.
    ASTERIA_THROW("could not remove directory '$2'\n"
                  "[`rmdir()` failed: $1]",
                  format_errno(errno), path);
  }

V_object
std_filesystem_dir_list(V_string path)
  {
    // Try opening t he directory.
    ::rocket::unique_posix_dir dp(::opendir(path.safe_c_str()), ::closedir);
    if(!dp)
      ASTERIA_THROW("could not open directory '$2'\n"
                    "[`opendir()` failed: $1]",
                    format_errno(errno), path);

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
        is_sym = next->d_type == DT_LNK;
      }
      else
#endif
      {
        // If the file type is unknown, ask for it.
        struct ::stat stb;
        if(::lstat(child.c_str(), &stb) != 0)
          ASTERIA_THROW("could not get information about '$2'\n"
                        "[`lstat()` failed: $1]",
                        format_errno(errno), child);

        // Check whether the child path denotes a directory.
        is_dir = S_ISDIR(stb.st_mode);
        is_sym = S_ISLNK(stb.st_mode);
      }

      // Append this entry, assuming the name is in UTF-8.
      V_object entry;
      entry.try_emplace(sref("b_dir"),
        V_boolean(
          is_dir
        ));
      entry.try_emplace(sref("b_sym"),
        V_boolean(
          is_sym
        ));
      entries.try_emplace(cow_string(next->d_name), ::std::move(entry));
    }
    return entries;
  }

V_integer
std_filesystem_dir_create(V_string path)
  {
    // Try creating an empty directory.
    if(::mkdir(path.safe_c_str(), 0777) == 0)
      return 1;

    // If the path references a directory or a symlink to a directory, don't fail.
    if(errno == EEXIST) {
      struct ::stat stb;
      if(::stat(path.c_str(), &stb) != 0)
        ASTERIA_THROW("could not get information about '$2'\n"
                      "[`stat()` failed: $1]",
                      format_errno(errno), path);

      if(S_ISDIR(stb.st_mode))
        return 0;

      // Throw an exception about the previous error.
      ASTERIA_THROW("could not create directory '$2'\n"
                    "[`mkdir()` failed: $1]",
                    format_errno(EEXIST), path);
    }

    // Throw an exception for general failures.
    ASTERIA_THROW("could not create directory '$2'\n"
                  "[`mkdir()` failed: $1]",
                  format_errno(errno), path);
  }

V_integer
std_filesystem_dir_remove(V_string path)
  {
    // Try removing an empty directory.
    if(::rmdir(path.safe_c_str()) == 0)
      return 1;

    // If the path does not exist, don't fail.
    if(errno == ENOENT)
      return 0;

    // Throw an exception for general failures.
    ASTERIA_THROW("could remove directory '$2'\n"
                  "[`rmdir()` failed: $1]",
                  format_errno(errno), path);
  }

V_string
std_filesystem_file_read(V_string path, Opt_integer offset, Opt_integer limit)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW("negative file offset (offset `$1`)", *offset);

    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), O_RDONLY), ::close);
    if(!fd)
      ASTERIA_THROW("could not open file '$2'\n"
                    "[`open()` failed: $1]",
                    format_errno(errno), path);

    // We return data that have been read as a byte string.
    V_string data;
    int64_t roffset = offset.value_or(0);
    int64_t rlimit = limit.value_or(INT64_MAX);
    size_t nbatch = 0x100000;  // 1MiB

    for(;;) {
      // Don't read too many bytes at a time.
      if(rlimit <= 0)
        break;

      nbatch = ::std::min(nbatch * 2, ::rocket::clamp_cast<size_t>(rlimit, 0, INT_MAX));
      auto insert_pos = data.insert(data.end(), nbatch, '/');
      ::ssize_t nread;

      if(offset) {
        // Use `roffset`. The file must be seekable in this case.
        nread = ::pread(fd, &*insert_pos, nbatch, roffset);
        if(nread < 0)
          ASTERIA_THROW("error reading file '$2'\n"
                        "[`pread()` failed: $1]",
                        format_errno(errno), path);
      }
      else {
        // Use the internal file pointer.
        nread = ::read(fd, &*insert_pos, nbatch);
        if(nread < 0)
          ASTERIA_THROW("error reading file '$2'\n"
                        "[`read()` failed: $1]",
                        format_errno(errno), path);
      }
      data.erase(insert_pos + nread, data.end());
      if(nread == 0)
        break;

      roffset += nread;
      rlimit -= nread;
    }
    return data;
  }

V_integer
std_filesystem_file_stream(Global_Context& global, V_string path, V_function callback,
                           Opt_integer offset, Opt_integer limit)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW("negative file offset (offset `$1`)", *offset);

    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), O_RDONLY), ::close);
    if(!fd)
      ASTERIA_THROW("could not open file '$2'\n"
                    "[`open()` failed: $1]",
                    format_errno(errno), path);

    // We return data that have been read as a byte string.
    Reference self;
    Reference_Stack stack;
    V_string data;
    int64_t roffset = offset.value_or(0);
    int64_t rlimit = limit.value_or(INT64_MAX);
    size_t nbatch = 0x100000;  // 1MiB

    for(;;) {
      // Don't read too many bytes at a time.
      if(rlimit <= 0)
        break;

      nbatch = ::std::min(nbatch * 2, ::rocket::clamp_cast<size_t>(rlimit, 0, INT_MAX));
      data.resize(nbatch, '/');
      ::ssize_t nread;

      if(offset) {
        // Use `roffset`. The file must be seekable in this case.
        nread = ::pread(fd, data.mut_data(), nbatch, roffset);
        if(nread < 0)
          ASTERIA_THROW("error reading file '$2'\n"
                        "[`pread()` failed: $1]",
                        format_errno(errno), path);
      }
      else {
        // Use the internal file pointer.
        nread = ::read(fd, data.mut_data(), nbatch);
        if(nread < 0)
          ASTERIA_THROW("error reading file '$2'\n"
                        "[`read()` failed: $1]",
                        format_errno(errno), path);
      }
      data.erase(data.begin() + nread, data.end());
      if(nread == 0)
        break;

      // Call the function but discard its return value.
      stack.clear();
      stack.emplace_back_uninit().set_temporary(roffset);
      stack.emplace_back_uninit().set_temporary(::std::move(data));
      self.set_temporary(nullopt);
      callback.invoke(self, global, ::std::move(stack));

      roffset += nread;
      rlimit -= nread;
    }
    return roffset - offset.value_or(0);
  }

void
std_filesystem_file_write(V_string path, Opt_integer offset, V_string data)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW("negative file offset (offset `$1`)", *offset);

    // Calculate the `flags` argument.
    int flags = O_WRONLY | O_CREAT | O_APPEND;

    // If we are to write from the beginning, truncate the file at creation.
    int64_t roffset = offset.value_or(0);
    if(roffset == 0)
      flags |= O_TRUNC;

    // Open the file for writing.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), flags, 0666), ::close);
    if(!fd)
      ASTERIA_THROW("could not open file '$2'\n"
                    "[`open()` failed: $1]",
                    format_errno(errno), path);

    // Set the file pointer when an offset is specified, even when it is an explicit
    // zero. This ensures that the file is actually seekable (not a pipe or socket
    // whatsoever).
    if(offset && (::ftruncate(fd, roffset) != 0))
      ASTERIA_THROW("could not truncate file '$2'\n"
                    "[`ftruncate()` failed: $1]",
                    format_errno(errno), path);

    // Write all data.
    do_write_loop(fd, data.data(), data.size(), path);
  }

void
std_filesystem_file_append(V_string path, V_string data, Opt_boolean exclusive)
  {
    // Calculate the `flags` argument.
    int flags = O_WRONLY | O_CREAT | O_APPEND;

    // Treat `exclusive` as `false` if it is not specified at all.
    if(exclusive == true)
      flags |= O_EXCL;

    // Open the file for appending.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), flags, 0666), ::close);
    if(!fd)
      ASTERIA_THROW("could not open file '$2'\n"
                    "[`open()` failed: $1]",
                    format_errno(errno), path);

    // Append all data to the end.
    do_write_loop(fd, data.data(), data.size(), path);
  }

void
std_filesystem_file_copy_from(V_string path_new, V_string path_old)
  {
    // Open the old file.
    ::rocket::unique_posix_fd fd_old(::open(path_old.safe_c_str(), O_RDONLY), ::close);
    if(!fd_old)
      ASTERIA_THROW("could not open source file '$2'\n"
                    "[`open()` failed: $1]",
                    format_errno(errno), path_old);

    // Create the new file, discarding its contents.
    // The file is initially write-only.
    ::rocket::unique_posix_fd fd_new(::open(path_new.safe_c_str(),
                                            O_WRONLY | O_CREAT | O_TRUNC | O_APPEND,
                                            0200), ::close);
    if(!fd_new)
      ASTERIA_THROW("could not create destination file '$2'\n"
                    "[`open()` failed: $1]",
                    format_errno(errno), path_new);

    // Get the file mode and preferred I/O block size.
    struct ::stat stb_old;
    if(::fstat(fd_old, &stb_old) != 0)
      ASTERIA_THROW("could not get information about source file '$2'\n"
                    "[`fstat()` failed: $1]",
                    format_errno(errno), path_old);

    // Allocate the I/O buffer.
    size_t nbuf = static_cast<size_t>(stb_old.st_blksize | 0x1000);
    auto pbuf = ::rocket::make_unique_handle(static_cast<char*>(::operator new(nbuf)),
                                             static_cast<void (*)(void*)>(::operator delete));

    // Copy all contents.
    for(;;) {
      ::ssize_t nread = ::read(fd_old, pbuf, nbuf);
      if(nread < 0)
        ASTERIA_THROW("error reading file '$2'\n"
                      "[`read()` failed: $1]",
                      format_errno(errno), path_old);

      if(nread == 0)
        break;

      // Append all data to the end.
      do_write_loop(fd_new, pbuf, static_cast<size_t>(nread), path_new);
    }

    // Set the file mode. This must be the last operation.
    if(::fchmod(fd_new, stb_old.st_mode) != 0)
      ASTERIA_THROW("could not set permission of '$2'\n"
                    "[`fchmod()` failed: $1]",
                    format_errno(errno), path_new);
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
    ASTERIA_THROW("could not remove file '$2'\n"
                  "[`unlink()` failed: $1]",
                  format_errno(errno), path);
  }

void
create_bindings_filesystem(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("get_working_directory"),
      ASTERIA_BINDING_BEGIN("std.filesystem.get_working_directory", self, global, reader) {
        reader.start_overload();
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_get_working_directory);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("get_real_path"),
      ASTERIA_BINDING_BEGIN("std.filesystem.get_real_path", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);      // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_get_real_path, path);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("get_information"),
      ASTERIA_BINDING_BEGIN("std.filesystem.get_information", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);      // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_get_information, path);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("remove_recursive"),
      ASTERIA_BINDING_BEGIN("std.filesystem.remove_recursive", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);      // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_remove_recursive, path);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("move_from"),
      ASTERIA_BINDING_BEGIN("std.filesystem.move_from", self, global, reader) {
        V_string to;
        V_string from;

        reader.start_overload();
        reader.required(to);       // path_new
        reader.required(from);     // path_old
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_move_from, to, from);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("dir_list"),
      ASTERIA_BINDING_BEGIN("std.filesystem.dir_list", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);      // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_dir_list, path);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("dir_create"),
      ASTERIA_BINDING_BEGIN("std.filesystem.dir_create", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);      // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_dir_create, path);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("dir_remove"),
      ASTERIA_BINDING_BEGIN("std.filesystem.dir_remove", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);      // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_dir_remove, path);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("file_read"),
      ASTERIA_BINDING_BEGIN("std.filesystem.file_read", self, global, reader) {
        V_string path;
        Opt_integer off;
        Opt_integer lim;

        reader.start_overload();
        reader.required(path);     // path
        reader.optional(off);      // [offset]
        reader.optional(lim);      // [limit]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_file_read, path, off, lim);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("file_stream"),
      ASTERIA_BINDING_BEGIN("std.filesystem.file_stream", self, global, reader) {
        V_string path;
        V_function func;
        Opt_integer off;
        Opt_integer lim;

        reader.start_overload();
        reader.required(path);     // path
        reader.required(func);     // callback
        reader.optional(off);      // [offset]
        reader.optional(lim);      // [limit]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_file_stream, global, path, func, off, lim);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("file_write"),
      ASTERIA_BINDING_BEGIN("std.filesystem.file_write", self, global, reader) {
        V_string path;
        Opt_integer off;
        V_string data;

        reader.start_overload();
        reader.required(path);     // path
        reader.save_state(0);
        reader.required(data);     // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_file_write, path, nullopt, data);

        reader.load_state(0);      // path
        reader.optional(off);      // [offset]
        reader.required(data);     // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_file_write, path, off, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("file_append"),
      ASTERIA_BINDING_BEGIN("std.filesystem.file_append", self, global, reader) {
        V_string path;
        V_string data;
        Opt_boolean excl;

        reader.start_overload();
        reader.required(path);     // path
        reader.required(data);     // data
        reader.optional(excl);      // [exclusive]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_file_append, path, data, excl);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("file_copy_from"),
      ASTERIA_BINDING_BEGIN("std.filesystem.file_copy_from", self, global, reader) {
        V_string to;
        V_string from;

        reader.start_overload();
        reader.required(to);       // path_new
        reader.required(from);     // path_old
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_file_copy_from, to, from);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("file_remove"),
      ASTERIA_BINDING_BEGIN("std.filesystem.file_remove", self, global, reader) {
        V_string path;

        reader.start_overload();
        reader.required(path);      // path
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_filesystem_file_remove, path);
      }
      ASTERIA_BINDING_END);
  }

}  // namespace asteria
