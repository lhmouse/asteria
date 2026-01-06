// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "filesystem.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../runtime/global_context.hpp"
#include "../llds/reference_stack.hpp"
#include "../utils.hpp"
#include <sys/stat.h>  // ::stat(), ::fstat(), ::lstat(), ::mkdir(), ::fchmod()
#include <dirent.h>  // ::opendir(), ::closedir()
#include <fcntl.h>  // ::open()
#include <stdio.h>  // ::rename()
#include <glob.h>  // ::glob(), ::globfree();
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

const void*
do_write_loop(int fd, const void* data, size_t size, const V_string& path)
  {
    auto bp = static_cast<const char*>(data);
    auto ep = bp + size;

    while(bp < ep) {
      ::ssize_t nwrtn = ::write(fd, bp, static_cast<size_t>(ep - bp));
      if(nwrtn < 0) {
        ASTERIA_THROW((
            "Error writing file '$1'",
            "[`write()` failed: ${errno:full}]"),
            path);
      }
      bp += nwrtn;
    }
    return bp;
  }

}  // namespace

optV_string
std_filesystem_get_real_path(V_string path)
  {
    unique_ptr<char, void (void*)> str(::realpath(path.safe_c_str(), nullptr), ::free);
    if(!str) {
      if(errno == ENOENT)
        return nullopt;

      ASTERIA_THROW((
          "Could not resolve path '$1'",
          "[`realpath()` failed: ${errno:full}]"),
          path);
    }

    return V_string(str, ::strlen(str));
  }

optV_object
std_filesystem_get_properties(V_string path)
  {
    struct stat stb;
    if(::lstat(path.safe_c_str(), &stb) != 0) {
      if(errno == ENOENT)
        return nullopt;

      ASTERIA_THROW((
          "Could not get properties of file '$1'",
          "[`lstat()` failed: ${errno:full}]"),
          path);
    }

    // Convert the result to an `object`.
    V_object stat;

    stat.try_emplace(&"device",
      V_integer(
        stb.st_dev  // unique device ID on this machine
      ));

    stat.try_emplace(&"inode",
      V_integer(
        stb.st_ino  // unique file ID on this device
      ));

    stat.try_emplace(&"link_count",
      V_integer(
        stb.st_nlink  // number of hard links
      ));

    stat.try_emplace(&"is_directory",
      V_boolean(
        S_ISDIR(stb.st_mode)  // whether this is a directory
      ));

    stat.try_emplace(&"is_symlink",
      V_boolean(
        S_ISLNK(stb.st_mode)  // whether this is a symbolic link
      ));

    stat.try_emplace(&"size",
      V_integer(
        stb.st_size  // size of contents in bytes
      ));

    stat.try_emplace(&"size_on_disk",
      V_integer(
        (int64_t) stb.st_blocks * 512  // size of storage on disk in bytes
      ));

    stat.try_emplace(&"time_accessed",
      V_integer(
#ifdef __USE_XOPEN2K8
        (int64_t) stb.st_atim.tv_sec * 1000 + stb.st_atim.tv_nsec / 1000000  // timestamp of creation
#else
        (int64_t) stb.st_atime * 1000  // timestamp of creation
#endif
      ));

    stat.try_emplace(&"time_modified",
      V_integer(
#ifdef __USE_XOPEN2K8
        (int64_t) stb.st_mtim.tv_sec * 1000 + stb.st_mtim.tv_nsec / 1000000  // timestamp of modification
#else
        (int64_t) stb.st_mtime * 1000  // timestamp of modification
#endif
      ));

    return stat;
  }

void
std_filesystem_move(V_string path_new, V_string path_old)
  {
    if(::rename(path_old.safe_c_str(), path_new.safe_c_str()) != 0)
      ASTERIA_THROW((
          "Could not move file '$1' to '$2'",
          "[`rename()` failed: ${errno:full}]"),
          path_old, path_new);
  }

V_integer
std_filesystem_remove_recursive(V_string path)
  {
    // Try removing a regular file.
    if(::unlink(path.safe_c_str()) == 0)
      return 1;

    if(errno == ENOENT)
      return 0;

    if((errno != EISDIR) && (errno != EPERM))
      ASTERIA_THROW((
          "Could not remove file '$1'",
          "[`unlink()` failed: ${errno:full}]"),
          path);

    // Try removing an empty directory.
    if(::rmdir(path.safe_c_str()) == 0)
      return 1;

    // Push the first element.
    cow_vector<RM_Element> stack;
    stack.push_back({ rm_disp_expand, path });
    int64_t nremoved = 0;

    // Expand non-empty directories and remove all contents.
    while(stack.size()) {
      auto elem = move(stack.mut_back());
      stack.pop_back();

      // Process this element.
      switch(elem.disp)
        {
        case rm_disp_rmdir:
          {
            // This is an empty directory. Remove it.
            if(::rmdir(elem.path.c_str()) == 0) {
              nremoved++;
              break;
            }

            // Hmm... avoid TOCTTOU errors.
            if(errno == ENOENT)
              break;

            ASTERIA_THROW((
                "Could not remove directory '$1'",
                "[`rmdir()` failed: ${errno:full}]"),
                elem.path);
          }

        case rm_disp_unlink:
          {
            // This is a non-directory. Unlink it.
            if(::unlink(elem.path.c_str()) == 0) {
              nremoved++;
              break;
            }

            // Hmm... avoid TOCTTOU errors.
            if(errno == ENOENT)
              break;

            ASTERIA_THROW((
                "Could not remove file '$1'",
                "[`unlink()` failed: ${errno:full}]"),
                elem.path);
          }

        case rm_disp_expand:
          {
            // This is a subdirectory that has not been expanded.
            // Push the directory itself. Since elements are maintained in LIFO order,
            // only when this element is encountered for a second time, will all of its
            // children have been removed.
            stack.push_back({ rm_disp_rmdir, elem.path });

            // Open the directory for listing.
            ::rocket::unique_posix_dir dp(::opendir(elem.path.c_str()));
            if(!dp)
              ASTERIA_THROW((
                  "Could not open directory '$1'",
                  "[`opendir()` failed: ${errno:full}]"),
                  elem.path);

            // Append all entries.
            while(auto next = ::readdir(dp)) {
              // Skip special entries.
              if(::strcmp(next->d_name, ".") == 0)
                continue;

              if(::strcmp(next->d_name, "..") == 0)
                continue;

              // Get the name and type of this entry.
              RM_Element enext = { rm_disp_unlink, elem.path + '/' + next->d_name };
  #ifdef _DIRENT_HAVE_D_TYPE
              if(next->d_type != DT_UNKNOWN) {
                // Get the file type if it is available immediately.
                if(next->d_type == DT_DIR)
                  enext.disp = rm_disp_expand;
              }
              else
  #endif
              {
                // If the file type is unknown, ask for it.
                struct stat stb;
                if(::lstat(enext.path.c_str(), &stb) != 0)
                  ASTERIA_THROW((
                      "Could not get information about '$1'",
                      "[`lstat()` failed: ${errno:full}]"),
                      enext.path);

                // Check whether the child path denotes a directory.
                if(S_ISDIR(stb.st_mode))
                  enext.disp = rm_disp_expand;
              }

              // Append this entry.
              stack.push_back(move(enext));
            }
            break;
          }

        default:
          ROCKET_ASSERT(false);
      }
    }
    return nremoved;
  }

V_array
std_filesystem_glob(V_string pattern)
  {
    // Get all paths into an array.
    V_array paths;
    ::glob_t gl = { };
    int gr = ::glob(pattern.safe_c_str(), GLOB_MARK | GLOB_NOSORT, nullptr, &gl);
    if(gr == GLOB_NOMATCH)
      return paths;
    else if(gr != 0)
      ASTERIA_THROW((
          "Could not find paths according to '$1'",
          "[`glob()` failed: ${errno:full}]"),
          pattern);

    // Convert them to strings.
    unique_ptr<::glob_t, void (::glob_t*)> gp(&gl, ::globfree);
    paths.reserve(gl.gl_pathc);
    for(size_t k = 0;  k != gl.gl_pathc;  ++k)
      paths.emplace_back(V_string(gl.gl_pathv[k]));
    return paths;
  }

optV_object
std_filesystem_list(V_string path)
  {
    // Try opening the directory.
    ::rocket::unique_posix_dir dp(::opendir(path.safe_c_str()));
    if(!dp) {
      if(errno == ENOENT)
        return nullopt;

      ASTERIA_THROW((
          "Could not open directory '$1'",
          "[`opendir()` failed: ${errno:full}]"),
          path);
    }

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
        struct stat stb;
        if(::lstat(child.c_str(), &stb) != 0)
          ASTERIA_THROW((
              "Could not get information about '$1'",
              "[`lstat()` failed: ${errno:full}]"),
              child);

        // Check whether the child path denotes a directory.
        is_dir = S_ISDIR(stb.st_mode);
        is_sym = S_ISLNK(stb.st_mode);
      }

      // Append this entry, assuming the name is in UTF-8.
      V_object entry;

      entry.try_emplace(&"inode",
        V_integer(
          next->d_ino  // unique file ID on this device
        ));

      entry.try_emplace(&"is_directory",
        V_boolean(
          is_dir  // whether this is a directory
        ));

      entry.try_emplace(&"is_symlink",
        V_boolean(
          is_sym  // whether this is a symbolic link
        ));

      entries.try_emplace(cow_string(next->d_name), move(entry));
    }
    return entries;
  }

V_integer
std_filesystem_create_directory(V_string path)
  {
    if(::mkdir(path.safe_c_str(), 0777) == 0)
      return 1;

    if(errno == EEXIST) {
      struct stat stb;
      if((::stat(path.c_str(), &stb) == 0) && S_ISDIR(stb.st_mode))
        return 0;
    }

    ASTERIA_THROW((
        "Could not create directory '$1'",
        "[`mkdir()` failed: ${errno:full}]"),
        path);
  }

V_integer
std_filesystem_remove_directory(V_string path)
  {
    if(::rmdir(path.safe_c_str()) == 0)
      return 1;

    if(errno == ENOENT)
      return 0;

    ASTERIA_THROW((
        "Could remove directory '$1'",
        "[`rmdir()` failed: ${errno:full}]"),
        path);
  }

V_string
std_filesystem_read(V_string path, optV_integer offset, optV_integer limit)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW((
          "Negative file offset (offset `$1`)"), *offset);

    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), O_RDONLY | O_NOCTTY));
    if(!fd)
      ASTERIA_THROW((
          "Could not open file '$1'",
          "[`open()` failed: ${errno:full}]"),
          path);

    // We return data that have been read as a byte string.
    V_string data;
    int64_t roffset = offset.value_or(0);
    int64_t rlimit = limit.value_or(INT64_MAX);
    size_t nbatch = 0x100000;  // 1MiB

    for(;;) {
      // Don't read too many bytes at a time.
      if(rlimit <= 0)
        break;

      nbatch = ::rocket::min(nbatch * 2, ::rocket::clamp_cast<size_t>(rlimit, 0, INT_MAX));
      auto insert_pos = data.insert(data.end(), nbatch, '/');
      ::ssize_t nread;

      if(offset) {
        // Use `roffset`. The file must be seekable in this case.
        nread = ::pread(fd, &*insert_pos, nbatch, roffset);
        if(nread < 0)
          ASTERIA_THROW((
              "Error reading file '$1'",
              "[`pread()` failed: ${errno:full}]"),
              path);
      }
      else {
        // Use the internal file pointer.
        nread = ::read(fd, &*insert_pos, nbatch);
        if(nread < 0)
          ASTERIA_THROW((
              "Error reading file '$1'",
              "[`read()` failed: ${errno:full}]"),
              path);
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
std_filesystem_stream(Global_Context& global, V_string path, V_function callback,
                      optV_integer offset, optV_integer limit)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW((
          "Negative file offset (offset `$1`)"), *offset);

    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), O_RDONLY | O_NOCTTY));
    if(!fd)
      ASTERIA_THROW((
          "Could not open file '$1' for reading",
          "[`open()` failed: ${errno:full}]"),
          path);

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

      nbatch = ::rocket::min(nbatch * 2, ::rocket::clamp_cast<size_t>(rlimit, 0, INT_MAX));
      data.resize(nbatch, '/');
      ::ssize_t nread;

      if(offset) {
        // Use `roffset`. The file must be seekable in this case.
        nread = ::pread(fd, data.mut_data(), nbatch, roffset);
        if(nread < 0)
          ASTERIA_THROW((
              "Error reading file '$1'",
              "[`pread()` failed: ${errno:full}]"),
              path);
      }
      else {
        // Use the internal file pointer.
        nread = ::read(fd, data.mut_data(), nbatch);
        if(nread < 0)
          ASTERIA_THROW((
              "Error reading file '$1'",
              "[`read()` failed: ${errno:full}]"),
              path);
      }
      data.erase(data.begin() + nread, data.end());
      if(nread == 0)
        break;

      // Call the function but discard its return value.
      stack.clear();
      stack.push().set_temporary(roffset);
      stack.push().set_temporary(move(data));
      self.clear();
      callback.invoke(self, global, move(stack));

      roffset += nread;
      rlimit -= nread;
    }
    return roffset - offset.value_or(0);
  }

void
std_filesystem_write(V_string path, optV_integer offset, V_string data)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW((
          "Negative file offset (offset `$1`)"), *offset);

    // Open the file for writing, truncating it at `offset`.
    ::rocket::unique_posix_fd fd;
    if(!offset) {
      fd.reset(::open(path.safe_c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_APPEND | O_NOCTTY, 0666));
      if(!fd)
        ASTERIA_THROW((
            "Could not open file '$1' for writing",
            "[`open()` failed: ${errno:full}]"),
            path);
    }
    else {
      fd.reset(::open(path.safe_c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NOCTTY, 0666));
      if(!fd)
        ASTERIA_THROW((
            "Could not open file '$1' for writing",
            "[`open()` failed: ${errno:full}]"),
            path);

      // Set the file pointer when an offset is specified, even when it is an
      // explicit zero. This ensures that the file is actually seekable (not a
      // pipe or socket whatsoever).
      if(::ftruncate(fd, *offset) != 0)
        ASTERIA_THROW((
            "Could not truncate file '$1'",
            "[`ftruncate()` failed: ${errno:full}]"),
            path);
    }

    // Write all data.
    do_write_loop(fd, data.data(), data.size(), path);
  }

void
std_filesystem_append(V_string path, V_string data, optV_boolean exclusive)
  {
    // Open the file for appending.
    ::rocket::unique_posix_fd fd;
    if(exclusive != true) {
      fd.reset(::open(path.safe_c_str(), O_WRONLY | O_CREAT | O_APPEND | O_NOCTTY, 0666));
      if(!fd)
        ASTERIA_THROW((
            "Could not open file '$1' for appending",
            "[`open()` failed: ${errno:full}]"),
            path);
    }
    else {
      fd.reset(::open(path.safe_c_str(), O_WRONLY | O_CREAT | O_APPEND | O_EXCL | O_NOCTTY, 0666));
      if(!fd)
        ASTERIA_THROW((
            "Could not exclusive file '$1' for appending",
            "[`open()` failed: ${errno:full}]"),
            path);
    }

    // Append all data to the end.
    do_write_loop(fd, data.data(), data.size(), path);
  }

void
std_filesystem_copy(V_string path_new, V_string path_old)
  {
    // Open the old file.
    ::rocket::unique_posix_fd fd_old(::open(path_old.safe_c_str(), O_RDONLY | O_NOCTTY));
    if(!fd_old)
      ASTERIA_THROW((
          "Could not open source file '$1'",
          "[`open()` failed: ${errno:full}]"),
          path_old);

    // Create the new file, discarding its contents. The file is initially
    // write-only.
    ::rocket::unique_posix_fd fd_new(::open(path_new.safe_c_str(),
                                            O_WRONLY | O_CREAT | O_TRUNC | O_APPEND | O_NOCTTY,
                                            0200));
    if(!fd_new)
      ASTERIA_THROW((
          "Could not create destination file '$1'",
          "[`open()` failed: ${errno:full}]"),
          path_new);

    // Get the file mode and preferred I/O block size.
    struct stat stb_old;
    if(::fstat(fd_old, &stb_old) != 0)
      ASTERIA_THROW((
          "Could not get information about source file '$1'",
          "[`fstat()` failed: ${errno:full}]"),
          path_old);

    // Allocate the I/O buffer.
    size_t nbuf = static_cast<size_t>(stb_old.st_blksize | 0x1000);
    unique_ptr<char, void (void*)> pbuf(static_cast<char*>(::operator new(nbuf)), ::operator delete);

    // Copy all contents.
    for(;;) {
      ::ssize_t nread = ::read(fd_old, pbuf, nbuf);
      if(nread < 0)
        ASTERIA_THROW((
            "Error reading file '$1'",
            "[`read()` failed: ${errno:full}]"),
            path_old);

      if(nread == 0)
        break;

      // Append all data to the end.
      do_write_loop(fd_new, pbuf, static_cast<size_t>(nread), path_new);
    }

    // Set the file mode. This must be the last operation.
    if(::fchmod(fd_new, stb_old.st_mode) != 0)
      ASTERIA_THROW((
          "Could not set permission of '$1'",
          "[`fchmod()` failed: ${errno:full}]"),
          path_new);
  }

void
std_filesystem_symlink(V_string path_new, V_string target)
  {
    if(::symlink(target.safe_c_str(), path_new.safe_c_str()) != 0)
      ASTERIA_THROW((
          "Could not create symbolic link '$2' to '$1'",
          "[`symlink()` failed: ${errno:full}]"),
          target, path_new);
  }

V_integer
std_filesystem_remove(V_string path)
  {
    if(::unlink(path.safe_c_str()) == 0)
      return 1;

    if(errno == ENOENT)
      return 0;

    ASTERIA_THROW((
        "Could not remove file '$1'",
        "[`unlink()` failed: ${errno:full}]"),
        path);
  }

void
create_bindings_filesystem(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(&"get_real_path",
      ASTERIA_BINDING(
        "std.filesystem.get_real_path", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_get_real_path(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"get_properties",
      ASTERIA_BINDING(
        "`std.filesystem.get_properties", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_get_properties(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"move",
      ASTERIA_BINDING(
        "std.filesystem.move", "path_new, path_old",
        Argument_Reader&& reader)
      {
        V_string path_new, path_old;

        reader.start_overload();
        reader.required(path_new);
        reader.required(path_old);
        if(reader.end_overload())
          return (void) std_filesystem_move(path_new, path_old);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"remove_recursive",
      ASTERIA_BINDING(
        "std.filesystem.remove_recursive", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_remove_recursive(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"glob",
      ASTERIA_BINDING(
        "std.filesystem.glob", "pattern",
        Argument_Reader&& reader)
      {
        V_string pattern;

        reader.start_overload();
        reader.required(pattern);
        if(reader.end_overload())
          return (Value) std_filesystem_glob(pattern);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"list",
      ASTERIA_BINDING(
        "std.filesystem.list", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_list(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"create_directory",
      ASTERIA_BINDING(
        "std.filesystem.create_directory", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_create_directory(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"remove_directory",
      ASTERIA_BINDING(
        "std.filesystem.remove_directory", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_remove_directory(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"read",
      ASTERIA_BINDING(
        "std.filesystem.read", "path, [offset, [limit]]",
        Argument_Reader&& reader)
      {
        V_string path;
        optV_integer off, lim;

        reader.start_overload();
        reader.required(path);
        reader.optional(off);
        reader.optional(lim);
        if(reader.end_overload())
          return (Value) std_filesystem_read(path, off, lim);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"stream",
      ASTERIA_BINDING(
        "std.filesystem.stream", "path, callback, [offset, [limit]]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_string path;
        V_function func;
        optV_integer off, lim;

        reader.start_overload();
        reader.required(path);
        reader.required(func);
        reader.optional(off);
        reader.optional(lim);
        if(reader.end_overload())
          return (Value) std_filesystem_stream(global, path, func, off, lim);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"write",
      ASTERIA_BINDING(
        "std.filesystem.write", "path, data",
        Argument_Reader&& reader)
      {
        V_string path, data;
        optV_integer off;

        reader.start_overload();
        reader.required(path);
        reader.save_state(0);
        reader.required(data);
        if(reader.end_overload())
          return (void) std_filesystem_write(path, nullopt, data);

        reader.load_state(0);
        reader.optional(off);
        reader.required(data);
        if(reader.end_overload())
          return (void) std_filesystem_write(path, off, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"append",
      ASTERIA_BINDING(
        "std.filesystem.append", "path, data, [exclusive]",
        Argument_Reader&& reader)
      {
        V_string path, data;
        optV_boolean excl;

        reader.start_overload();
        reader.required(path);
        reader.required(data);
        reader.optional(excl);
        if(reader.end_overload())
          return (void) std_filesystem_append(path, data, excl);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"copy",
      ASTERIA_BINDING(
        "std.filesystem.copy", "path_new, path_old",
        Argument_Reader&& reader)
      {
        V_string path_new, path_old;

        reader.start_overload();
        reader.required(path_new);
        reader.required(path_old);
        if(reader.end_overload())
          return (void) std_filesystem_copy(path_new, path_old);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"symlink",
      ASTERIA_BINDING(
        "std.filesystem.symlink", "path_new, target",
        Argument_Reader&& reader)
      {
        V_string path_new, target;

        reader.start_overload();
        reader.required(path_new);
        reader.required(target);
        if(reader.end_overload())
          return (void) std_filesystem_symlink(path_new, target);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"remove",
      ASTERIA_BINDING(
        "std.filesystem.remove", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_remove(path);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
