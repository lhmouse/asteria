// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
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

V_string
std_filesystem_get_working_directory()
  {
    // Pass a null pointer to request dynamic allocation.
    // Note this behavior is an extension that exists almost everywhere.
    unique_ptr<char, void (void*)> cwd(::free);
    cwd.reset(::getcwd(nullptr, 0));
    if(!cwd)
      ASTERIA_THROW((
          "Could not get current working directory",
          "[`getcwd()` failed: ${errno:full}]"));

    V_string str(cwd.get());
    return str;
  }

V_string
std_filesystem_get_real_path(V_string path)
  {
    // Pass a null pointer to request dynamic allocation.
    unique_ptr<char, void (void*)> abspath(::free);
    abspath.reset(::realpath(path.safe_c_str(), nullptr));
    if(!abspath)
      ASTERIA_THROW((
          "Could not resolve path '$1'",
          "[`realpath()` failed: ${errno:full}]"),
          path);

    V_string str(abspath.get());
    return str;
  }

optV_object
std_filesystem_get_properties(V_string path)
  {
    struct ::stat stb;
    if(::lstat(path.safe_c_str(), &stb) != 0)
      return nullopt;

    // Convert the result to an `object`.
    V_object stat;

    stat.try_emplace(sref("device"),
      V_integer(
        stb.st_dev  // unique device ID on this machine
      ));

    stat.try_emplace(sref("inode"),
      V_integer(
        stb.st_ino  // unique file ID on this device
      ));

    stat.try_emplace(sref("link_count"),
      V_integer(
        stb.st_nlink  // number of hard links
      ));

    stat.try_emplace(sref("is_directory"),
      V_boolean(
        S_ISDIR(stb.st_mode)  // whether this is a directory
      ));

    stat.try_emplace(sref("is_symbolic"),
      V_boolean(
        S_ISLNK(stb.st_mode)  // whether this is a symbolic link
      ));

    stat.try_emplace(sref("size"),
      V_integer(
        stb.st_size  // size of contents in bytes
      ));

    stat.try_emplace(sref("size_on_disk"),
      V_integer(
        int64_t(stb.st_blocks) * 512  // size of storage on disk in bytes
      ));

    stat.try_emplace(sref("time_accessed"),
      V_integer(
        int64_t(stb.st_atim.tv_sec) * 1000 + stb.st_atim.tv_nsec / 1000000 // timestamp of creation
      ));

    stat.try_emplace(sref("time_modified"),
      V_integer(
        int64_t(stb.st_mtim.tv_sec) * 1000 + stb.st_mtim.tv_nsec / 1000000  // timestamp of last modification
      ));

    return stat;
  }

void
std_filesystem_move_from(V_string path_new, V_string path_old)
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

    if(errno != EISDIR)
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

          ASTERIA_THROW((
              "Could not remove directory '$1'",
              "[`rmdir()` failed: ${errno:full}]"),
              elem.path);
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

          ASTERIA_THROW((
              "Could not remove file '$1'",
              "[`unlink()` failed: ${errno:full}]"),
              elem.path);
        }

        case rm_disp_expand: {
          // This is a subdirectory that has not been expanded.
          // Push the directory itself.
          // Since elements are maintained in LIFO order, only when this element is encountered
          // for a second time, will all of its children have been removed.
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
                ASTERIA_THROW((
                    "Could not get information about '$1'",
                    "[`lstat()` failed: ${errno:full}]"),
                    child);

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
    ::rocket::unique_ptr<::glob_t, void (::glob_t*)> gp(&gl, ::globfree);
    paths.reserve(gl.gl_pathc);
    for(size_t k = 0;  k != gl.gl_pathc;  ++k)
      paths.emplace_back(V_string(gl.gl_pathv[k]));
    return paths;
  }

V_object
std_filesystem_dir_list(V_string path)
  {
    // Try opening the directory.
    ::rocket::unique_posix_dir dp(::opendir(path.safe_c_str()));
    if(!dp)
      ASTERIA_THROW((
          "Could not open directory '$1'",
          "[`opendir()` failed: ${errno:full}]"),
          path);

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

      entry.try_emplace(sref("inode"),
        V_integer(
          next->d_ino  // unique file ID on this device
        ));

      entry.try_emplace(sref("is_directory"),
        V_boolean(
          is_dir  // whether this is a directory
        ));

      entry.try_emplace(sref("is_symbolic"),
        V_boolean(
          is_sym  // whether this is a symbolic link
        ));

      entries.try_emplace(cow_string(next->d_name), ::std::move(entry));
    }
    return entries;
  }

V_integer
std_filesystem_dir_create(V_string path)
  {
    if(::mkdir(path.safe_c_str(), 0777) == 0)
      return 1;

    struct ::stat stb;
    if((errno == EEXIST) && (::stat(path.c_str(), &stb) == 0) && S_ISDIR(stb.st_mode))
      return 0;

    ASTERIA_THROW((
        "Could not create directory '$1'",
        "[`mkdir()` failed: ${errno:full}]"),
        path);
  }

V_integer
std_filesystem_dir_remove(V_string path)
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
std_filesystem_file_read(V_string path, optV_integer offset, optV_integer limit)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW((
          "Negative file offset (offset `$1`)"), *offset);

    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), O_RDONLY));
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

      nbatch = ::std::min(nbatch * 2, ::rocket::clamp_cast<size_t>(rlimit, 0, INT_MAX));
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
std_filesystem_file_stream(Global_Context& global, V_string path, V_function callback,
                           optV_integer offset, optV_integer limit)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW((
          "Negative file offset (offset `$1`)"), *offset);

    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), O_RDONLY));
    if(!fd)
      ASTERIA_THROW((
          "Could not open file '$1'",
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

      nbatch = ::std::min(nbatch * 2, ::rocket::clamp_cast<size_t>(rlimit, 0, INT_MAX));
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
      stack.push().set_temporary(::std::move(data));
      self.set_temporary(nullopt);
      callback.invoke(self, global, ::std::move(stack));

      roffset += nread;
      rlimit -= nread;
    }
    return roffset - offset.value_or(0);
  }

void
std_filesystem_file_write(V_string path, optV_integer offset, V_string data)
  {
    if(offset && (*offset < 0))
      ASTERIA_THROW((
          "Negative file offset (offset `$1`)"), *offset);

    // Calculate the `flags` argument.
    int flags = O_WRONLY | O_CREAT | O_APPEND;

    // If we are to write from the beginning, truncate the file at creation.
    int64_t roffset = offset.value_or(0);
    if(roffset == 0)
      flags |= O_TRUNC;

    // Open the file for writing.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), flags, 0666));
    if(!fd)
      ASTERIA_THROW((
          "Could not open file '$1'",
          "[`open()` failed: ${errno:full}]"),
          path);

    // Set the file pointer when an offset is specified, even when it is an explicit
    // zero. This ensures that the file is actually seekable (not a pipe or socket
    // whatsoever).
    if(offset && (::ftruncate(fd, roffset) != 0))
      ASTERIA_THROW((
          "Could not truncate file '$1'",
          "[`ftruncate()` failed: ${errno:full}]"),
          path);

    // Write all data.
    do_write_loop(fd, data.data(), data.size(), path);
  }

void
std_filesystem_file_append(V_string path, V_string data, optV_boolean exclusive)
  {
    // Calculate the `flags` argument.
    int flags = O_WRONLY | O_CREAT | O_APPEND;

    // Treat `exclusive` as `false` if it is not specified at all.
    if(exclusive == true)
      flags |= O_EXCL;

    // Open the file for appending.
    ::rocket::unique_posix_fd fd(::open(path.safe_c_str(), flags, 0666));
    if(!fd)
      ASTERIA_THROW((
          "Could not open file '$1'",
          "[`open()` failed: ${errno:full}]"),
          path);

    // Append all data to the end.
    do_write_loop(fd, data.data(), data.size(), path);
  }

void
std_filesystem_file_copy_from(V_string path_new, V_string path_old)
  {
    // Open the old file.
    ::rocket::unique_posix_fd fd_old(::open(path_old.safe_c_str(), O_RDONLY));
    if(!fd_old)
      ASTERIA_THROW((
          "Could not open source file '$1'",
          "[`open()` failed: ${errno:full}]"),
          path_old);

    // Create the new file, discarding its contents.
    // The file is initially write-only.
    int flags = O_WRONLY | O_CREAT | O_TRUNC | O_APPEND;
    ::rocket::unique_posix_fd fd_new(::open(path_new.safe_c_str(), flags, 0200));
    if(!fd_new)
      ASTERIA_THROW((
          "Could not create destination file '$1'",
          "[`open()` failed: ${errno:full}]"),
          path_new);

    // Get the file mode and preferred I/O block size.
    struct ::stat stb_old;
    if(::fstat(fd_old, &stb_old) != 0)
      ASTERIA_THROW((
          "Could not get information about source file '$1'",
          "[`fstat()` failed: ${errno:full}]"),
          path_old);

    // Allocate the I/O buffer.
    unique_ptr<char, void (void*)> pbuf(::operator delete);
    size_t nbuf = static_cast<size_t>(stb_old.st_blksize | 0x1000);
    pbuf.reset(static_cast<char*>(::operator new(nbuf)));

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

V_integer
std_filesystem_file_remove(V_string path)
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
    result.insert_or_assign(sref("get_working_directory"),
      ASTERIA_BINDING(
        "std.filesystem.get_working_directory", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_filesystem_get_working_directory();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("get_real_path"),
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

    result.insert_or_assign(sref("get_properties"),
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

    result.insert_or_assign(sref("move_from"),
      ASTERIA_BINDING(
        "std.filesystem.move_from", "path_new, path_old",
        Argument_Reader&& reader)
      {
        V_string to, from;

        reader.start_overload();
        reader.required(to);
        reader.required(from);
        if(reader.end_overload())
          return (void) std_filesystem_move_from(to, from);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("remove_recursive"),
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

    result.insert_or_assign(sref("glob"),
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

    result.insert_or_assign(sref("dir_list"),
      ASTERIA_BINDING(
        "std.filesystem.dir_list", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_dir_list(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("dir_create"),
      ASTERIA_BINDING(
        "std.filesystem.dir_create", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_dir_create(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("dir_remove"),
      ASTERIA_BINDING(
        "std.filesystem.dir_remove", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_dir_remove(path);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("file_read"),
      ASTERIA_BINDING(
        "std.filesystem.file_read", "path, [offset, [limit]]",
        Argument_Reader&& reader)
      {
        V_string path;
        optV_integer off, lim;

        reader.start_overload();
        reader.required(path);
        reader.optional(off);
        reader.optional(lim);
        if(reader.end_overload())
          return (Value) std_filesystem_file_read(path, off, lim);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("file_stream"),
      ASTERIA_BINDING(
        "std.filesystem.file_stream", "path, callback, [offset, [limit]]",
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
          return (Value) std_filesystem_file_stream(global, path, func, off, lim);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("file_write"),
      ASTERIA_BINDING(
        "std.filesystem.file_write", "path, data",
        Argument_Reader&& reader)
      {
        V_string path, data;
        optV_integer off;

        reader.start_overload();
        reader.required(path);
        reader.save_state(0);
        reader.required(data);
        if(reader.end_overload())
          return (void) std_filesystem_file_write(path, nullopt, data);

        reader.load_state(0);
        reader.optional(off);
        reader.required(data);
        if(reader.end_overload())
          return (void) std_filesystem_file_write(path, off, data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("file_append"),
      ASTERIA_BINDING(
        "std.filesystem.file_append", "path, data, [exclusive]",
        Argument_Reader&& reader)
      {
        V_string path, data;
        optV_boolean excl;

        reader.start_overload();
        reader.required(path);
        reader.required(data);
        reader.optional(excl);
        if(reader.end_overload())
          return (void) std_filesystem_file_append(path, data, excl);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("file_copy_from"),
      ASTERIA_BINDING(
        "std.filesystem.file_copy_from", "path_new, path_old",
        Argument_Reader&& reader)
      {
        V_string to, from;

        reader.start_overload();
        reader.required(to);
        reader.required(from);
        if(reader.end_overload())
          return (void) std_filesystem_file_copy_from(to, from);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("file_remove"),
      ASTERIA_BINDING(
        "std.filesystem.file_remove", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_filesystem_file_remove(path);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
