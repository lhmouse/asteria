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

namespace Asteria {
namespace {

[[noreturn]] ROCKET_NOINLINE void do_throw_system_error(const char* name, int err = errno)
  {
    char sbuf[256];
    const char* msg = ::strerror_r(err, sbuf, sizeof(sbuf));
    ASTERIA_THROW("`$1()` failed (errno was `$2`: $3)", name, err, msg);
  }

enum Rmdisp
  {
    rmdisp_rmdir,     // a subdirectory which should be empty and can be removed
    rmdisp_unlink,    // a plain file to be unlinked
    rmdisp_expand,    // a subdirectory to be expanded
  };

struct Rmelem
  {
    Rmdisp disp;
    cow_string path;
  };

int64_t do_remove_recursive(const Sval& path)
  {
    int64_t nremoved = 0;
    // Push the first element.
    cow_vector<Rmelem> stack;
    stack.push_back({ rmdisp_expand, path });
    // Expand non-empty directories and remove all contents.
    while(stack.size()) {
      auto elem = ::rocket::move(stack.mut_back());
      stack.pop_back();
      // Process this element.
      switch(elem.disp) {
      case rmdisp_rmdir: {
          // This is an empty directory. Remove it.
          if(::rmdir(elem.path.c_str()) != 0)
            do_throw_system_error("rmdir");
          // An element has been removed.
          nremoved++;
          break;
        }
      case rmdisp_unlink: {
          // This is a plain file. Unlink it.
          if(::unlink(elem.path.c_str()) != 0)
            do_throw_system_error("unlink");
          // An element has been removed.
          nremoved++;
          break;
        }
      case rmdisp_expand: {
          // This is a subdirectory that has not been expanded. Expand it.
          // Push the directory itself. Since elements are maintained in LIFO order, only when this
          // element is encountered for a second time, will all of its children have been removed.
          stack.push_back({ rmdisp_rmdir, elem.path });
          // Append all entries.
          ::rocket::unique_posix_dir dp(::opendir(elem.path.c_str()), ::closedir);
          if(!dp)
            do_throw_system_error("opendir");
          // Write entries.
          struct ::dirent* next;
          while((next = ::readdir(dp)) != nullptr) {
            // Skip special entries.
            if(next->d_name[0] == '.') {
              if(next->d_name[1] == 0)  // "."
                continue;
              if((next->d_name[1] == '.') && (next->d_name[2] == 0))  // ".."
                continue;
            }
            // Get the name and type of this entry.
            Rmdisp disp = rmdisp_unlink;
            cow_string child = elem.path + '/' + next->d_name;
#ifdef _DIRENT_HAVE_D_TYPE
            if(ROCKET_EXPECT(next->d_type != DT_UNKNOWN)) {
              // Get the file type if it is available immediately.
              if(next->d_type == DT_DIR)
                disp = rmdisp_expand;
            }
            else
#endif
            {
              // If the file type is unknown, ask for it.
              struct ::stat stb;
              if(::lstat(child.c_str(), &stb) != 0)
                do_throw_system_error("lstat");
              // Check whether the child path denotes a directory.
              if(S_ISDIR(stb.st_mode))
                disp = rmdisp_expand;
            }
            // Append the entry.
            stack.push_back({ disp, ::rocket::move(child) });
          }
          break;
        }
      default:
        ROCKET_ASSERT(false);
      }
    }
    return nremoved;
  }

::ssize_t loop_write(int fd, const void* buf, size_t count)
  {
    auto bp = static_cast<const char*>(buf);
    auto ep = bp + count;
    for(;;) {
      if(bp >= ep)
        break;
      ::ssize_t nwrtn = ::write(fd, bp, static_cast<size_t>(ep - bp));
      if(nwrtn < 0)
        break;
      bp += nwrtn;
    }
    return bp - static_cast<const char*>(buf);
  }

}  // namespace

Sval std_filesystem_get_working_directory()
  {
    // Get the current directory, resizing the buffer as needed.
    Sval cwd;
#ifdef ROCKET_DEBUG
    cwd.append(1, '*');
#else
    cwd.append(PATH_MAX, '*');
#endif
    while(::getcwd(cwd.mut_data(), cwd.size()) == nullptr) {
      // Resize the buffer if it isn't large enough.
      if(errno != ERANGE)
        do_throw_system_error("getcwd");
#ifdef ROCKET_DEBUG
      cwd.append(1, '*');
#else
      cwd.append(cwd.size() / 2, '*');
#endif
    }
    cwd.erase(cwd.find('\0'));
    return cwd;
  }

Oopt std_filesystem_get_information(Sval path)
  {
    struct ::stat stb;
    if(::lstat(path.c_str(), &stb) != 0) {
      return nullopt;
    }
    // Convert the result to an `object`.
    Oval stat;
    stat.try_emplace(::rocket::sref("i_dev"),
      Ival(
        stb.st_dev  // unique device id on this machine
      ));
    stat.try_emplace(::rocket::sref("i_file"),
      Ival(
        stb.st_ino  // unique file id on this device
      ));
    stat.try_emplace(::rocket::sref("n_ref"),
      Ival(
        stb.st_nlink  // number of hard links to this file
      ));
    stat.try_emplace(::rocket::sref("b_dir"),
      Bval(
        S_ISDIR(stb.st_mode)  // whether this is a directory
      ));
    stat.try_emplace(::rocket::sref("b_sym"),
      Bval(
        S_ISLNK(stb.st_mode)  // whether this is a symbolic link
      ));
    stat.try_emplace(::rocket::sref("n_size"),
      Ival(
        stb.st_size  // number of bytes this file contains
      ));
    stat.try_emplace(::rocket::sref("n_ocup"),
      Ival(
        int64_t(stb.st_blocks) * 512  // number of bytes this file occupies
      ));
    stat.try_emplace(::rocket::sref("t_accs"),
      Ival(
        int64_t(stb.st_atim.tv_sec) * 1000 + stb.st_atim.tv_nsec / 1000000  // timestamp of last access
      ));
    stat.try_emplace(::rocket::sref("t_mod"),
      Ival(
        int64_t(stb.st_mtim.tv_sec) * 1000 + stb.st_mtim.tv_nsec / 1000000  // timestamp of last modification
      ));
    return ::rocket::move(stat);
  }

void std_filesystem_move_from(Sval path_new, Sval path_old)
  {
    if(::rename(path_old.c_str(), path_new.c_str()) != 0)
      do_throw_system_error("rename");
  }

Ival std_filesystem_remove_recursive(Sval path)
  {
    if(::rmdir(path.c_str()) == 0) {
      // An empty directory has been removed.
      return 1;
    }
    switch(errno) {
    case ENOENT: {
        // The path does not denote an existent file or directory.
        return 0;
      }
    case ENOTDIR: {
        // This is something not a directory.
        if(::unlink(path.c_str()) != 0)
          do_throw_system_error("unlink");
        // A file has been removed.
        return 1;
      }
    case EEXIST:
    case ENOTEMPTY: {
        // Remove contents first.
        return do_remove_recursive(path);
      }
    default:
      do_throw_system_error("rmdir");
    }
  }

Oopt std_filesystem_directory_list(Sval path)
  {
    ::rocket::unique_posix_dir dp(::opendir(path.c_str()), closedir);
    if(!dp) {
      if(errno != ENOENT)
        do_throw_system_error("opendir");
      // The path denotes a non-existent directory.
      return nullopt;
    }
    // Write entries.
    Oval entries;
    cow_string child;
    struct ::dirent* next;
    while((next = ::readdir(dp)) != nullptr) {
      // Assume the name is in UTF-8.
      phsh_string name = cow_string(next->d_name);
      Oval entry;
      // Compose the full path of the child.
      // We want to reuse the storage so don't just assign to `child` here.
      child.clear();
      child += path;
      child += '/';
      child += next->d_name;
#ifdef _DIRENT_HAVE_D_TYPE
      if(next->d_type != DT_UNKNOWN) {
        // Get the file type if it is available immediately.
        entry.try_emplace(::rocket::sref("b_dir"),
          Bval(
            next->d_type == DT_DIR
          ));
        entry.try_emplace(::rocket::sref("b_sym"),
          Bval(
            next->d_type == DT_LNK
          ));
      }
      else
#endif
      {
        // If the file type is unknown, ask for it.
        // Identify the entry.
        struct ::stat stb;
        if(::lstat(child.c_str(), &stb) != 0)
          do_throw_system_error("lstat");
        // Check whether the child path denotes a directory or symlink.
        entry.try_emplace(::rocket::sref("b_dir"),
          Bval(
            S_ISDIR(stb.st_mode)
          ));
        entry.try_emplace(::rocket::sref("b_sym"),
          Bval(
            S_ISLNK(stb.st_mode)
          ));
      }
      // Insert the entry.
      entries.try_emplace(::rocket::move(name), ::rocket::move(entry));
    }
    return ::rocket::move(entries);
  }

Bval std_filesystem_directory_create(Sval path)
  {
    if(::mkdir(path.c_str(), 0777) == 0) {
      // A new directory has been created.
      return true;
    }
    if(errno != EEXIST)
      do_throw_system_error("mkdir");

    // Check whether a directory or a symlink to a directory already exists.
    struct ::stat stb;
    if(::stat(path.c_str(), &stb) != 0)
      do_throw_system_error("stat");
    if(!S_ISDIR(stb.st_mode))
      do_throw_system_error("mkdir", EEXIST);

    // A directory already exists.
    return false;
  }

Bval std_filesystem_directory_remove(Sval path)
  {
    if(::rmdir(path.c_str()) == 0) {
      // The directory has been removed.
      return true;
    }
    if(errno != ENOENT)
      do_throw_system_error("rmdir");

    // The directory does not exist.
    return false;
  }

Sopt std_filesystem_file_read(Sval path, Iopt offset, Iopt limit)
  {
    if(offset && (*offset < 0)) {
      ASTERIA_THROW("negative file offset (offset `$1`)", *offset);
    }
    int64_t roffset = offset.value_or(0);
    int64_t rlimit = ::rocket::clamp(limit.value_or(INT32_MAX), 0, 0x10'00000);
    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.c_str(), O_RDONLY), ::close);
    if(!fd) {
      if(errno != ENOENT)
        do_throw_system_error("open");
      // The path denotes a non-existent file.
      return nullopt;
    }

    // Don't read too many bytes at a time.
    ::ssize_t nread;
    Sval data;
    data.resize(static_cast<size_t>(rlimit));
    if(offset) {
      nread = ::pread(fd, data.mut_data(), data.size(), roffset);
      if(nread < 0)
        do_throw_system_error("pread");
    }
    else {
      nread = ::read(fd, data.mut_data(), data.size());
      if(nread < 0)
        do_throw_system_error("read");
    }
    data.erase(static_cast<size_t>(nread));
    return ::rocket::move(data);
  }

Iopt std_filesystem_file_stream(Global& global, Sval path, Fval callback, Iopt offset, Iopt limit)
  {
    if(offset && (*offset < 0)) {
      ASTERIA_THROW("negative file offset (offset `$1`)", *offset);
    }
    int64_t roffset = offset.value_or(0);
    int64_t rlimit = ::rocket::clamp(limit.value_or(INT32_MAX), 0, 0x10'00000);
    int64_t ntotal = 0;
    int64_t ntlimit = ::rocket::max(limit.value_or(INT64_MAX), 0);
    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.c_str(), O_RDONLY), ::close);
    if(!fd) {
      if(errno != ENOENT)
        do_throw_system_error("open");
      // The path denotes a non-existent file.
      return nullopt;
    }

    // Read and process all data in blocks.
    cow_vector<Reference> args;
    ::ssize_t nread;
    Sval data;
    for(;;) {
      // Check for the read limit.
      if(ntotal >= ntlimit)
        break;

      // Don't read too many bytes at a time.
      data.resize(static_cast<size_t>(rlimit));
      if(offset) {
        nread = ::pread(fd, data.mut_data(), data.size(), roffset);
        if(nread < 0)
          do_throw_system_error("pread");
      }
      else {
        nread = ::read(fd, data.mut_data(), data.size());
        if(nread < 0)
          do_throw_system_error("read");
      }
      data.erase(static_cast<size_t>(nread));
      // Check for EOF.
      if(data.empty())
        break;

      // Prepare arguments for the user-defined function.
      args.clear().reserve(2);
      Reference_root::S_temporary xref_offset = { roffset };
      args.emplace_back(::rocket::move(xref_offset));
      Reference_root::S_temporary xref_data = { ::rocket::move(data) };
      args.emplace_back(::rocket::move(xref_data));
      // Call the function but discard its return value.
      callback.invoke(global, ::rocket::move(args));
      // Read the next block.
      roffset += nread;
      ntotal += nread;
    }
    return ntotal;
  }

void std_filesystem_file_write(Sval path, Sval data, Iopt offset)
  {
    if(offset && (*offset < 0)) {
      ASTERIA_THROW("negative file offset (offset `$1`)", *offset);
    }
    int64_t roffset = offset.value_or(0);
    // Calculate the `flags` argument.
    // If we are to write from the beginning, truncate the file at creation.
    int flags = O_WRONLY | O_CREAT | O_APPEND;
    if(roffset == 0)
      flags |= O_TRUNC;
    // Open the file for writing.
    ::rocket::unique_posix_fd fd(::open(path.c_str(), flags, 0666), ::close);
    if(!fd)
      do_throw_system_error("open");
    // Set the file pointer when an offset is specified, even when it is an explicit zero.
    if(offset) {
      // If `roffset` is not zero, truncate the file there.
      // This also ensures it is a normal file (not a pipe or socket whatsoever).
      // Otherwise, the file will have been truncate at creation.
      if(::ftruncate(fd, roffset) != 0)
        do_throw_system_error("ftruncate");
    }
    // Write all data.
    ::ssize_t nwrtn = loop_write(fd, data.data(), data.size());
    if(nwrtn < data.ssize())
      do_throw_system_error("write");
  }

void std_filesystem_file_append(Sval path, Sval data, Bopt exclusive)
  {
    int flags = O_WRONLY | O_CREAT | O_APPEND;
    if(exclusive == true)
      flags |= O_EXCL;
    // Open the file for writing.
    ::rocket::unique_posix_fd fd(::open(path.c_str(), flags, 0666), ::close);
    if(!fd)
      do_throw_system_error("open");
    // Write all data.
    ::ssize_t nwrtn = loop_write(fd, data.data(), data.size());
    if(nwrtn < data.ssize())
      do_throw_system_error("write");
  }

void std_filesystem_file_copy_from(Sval path_new, Sval path_old)
  {
    // Open the old file.
    ::rocket::unique_posix_fd fd_old(::open(path_old.c_str(), O_RDONLY), ::close);
    if(!fd_old)
      do_throw_system_error("open");
    // Get the file mode and preferred I/O block size.
    struct ::stat stb_old;
    if(::fstat(fd_old, &stb_old) != 0)
      do_throw_system_error("fstat");
    // We always overwrite the destination file.
    int flags = O_WRONLY | O_CREAT | O_TRUNC | O_APPEND;
    // Create the new file, discarding its contents.
    ::rocket::unique_posix_fd fd_new(::open(path_new.c_str(), flags, 0200), ::close);
    if(!fd_new)
      do_throw_system_error("open");

    // Copy data in blocks.
    ::ssize_t nread, nwrtn;
    Sval buff;
    buff.resize(static_cast<size_t>(stb_old.st_blksize));
    for(;;) {
      // Read some bytes.
      nread = ::read(fd_old, buff.mut_data(), buff.size());
      if(nread < 0)
        do_throw_system_error("read");
      // Check for EOF.
      if(nread == 0)
        break;
      // Write them all.
      nwrtn = loop_write(fd_new, buff.data(), static_cast<size_t>(nread));
      if(nwrtn < nread)
        do_throw_system_error("write");
    }
    // Set the file mode. This must be at the last.
    if(::fchmod(fd_new, stb_old.st_mode) != 0)
      do_throw_system_error("fchmod");
  }

bool std_filesystem_file_remove(Sval path)
  {
    if(::unlink(path.c_str()) == 0) {
      // The file has been removed.
      return true;
    }
    if(errno != ENOENT)
      do_throw_system_error("unlink");

    // The file does not exist.
    return false;
  }

void create_bindings_filesystem(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.filesystem.get_working_directory()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("get_working_directory"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.get_working_directory"));
    // Parse arguments.
    if(reader.I().F()) {
      return std_filesystem_get_working_directory();
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.get_working_directory()`

  * Gets the absolute path of the current working directory.

  * Returns a string containing the path to the current working
    directory.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.get_information()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("get_information"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.get_information"));
    // Parse arguments.
    Sval path;
    if(reader.I().v(path).F()) {
      return std_filesystem_get_information(::rocket::move(path));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.remove_recursive()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("remove_recursive"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.remove_recursive"));
    // Parse arguments.
    Sval path;
    if(reader.I().v(path).F()) {
      return std_filesystem_remove_recursive(::rocket::move(path));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.move_from(path_new, path_old)`

  * Moves (renames) the file or directory at `path_old` to
    `path_new`.

  * Returns `true`.

  * Throws an exception on failure.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.move_from(path_new, path_old)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("move_from"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.move_from"));
    // Parse arguments.
    Sval path_new;
    Sval path_old;
    if(reader.I().v(path_new).v(path_old).F()) {
      std_filesystem_move_from(path_new, path_old);
      return true;
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.remove_recursive(path)`

  * Removes the file or directory at `path`. If `path` designates a
    directory, all of its contents are removed recursively.

  * Returns the number of files and directories that have been
    successfully removed in total. If `path` does not reference an
    existent file or directory, `0` is returned.

  * Throws an exception if the file or directory at `path` cannot
    be removed.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.directory_list()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("directory_list"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.directory_list"));
    // Parse arguments.
    Sval path;
    if(reader.I().v(path).F()) {
      return std_filesystem_directory_list(::rocket::move(path));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.directory_list(path)`

  * Lists the contents of the directory at `path`.

  * Returns an object containing all entries of the directory at
    `path`, including the special subdirectories '.' and '..'. For
    each element, its key specifies the filename and the value is
    an object consisting of the following members (names that
    start with `b_` are boolean values; names that start with `i_`
    are IDs as integers):

    * `b_dir`   whether this is a directory.
    * `b_sym`   whether this is a symbolic link.

    If `path` references a non-existent directory, `null` is
    returned.

  * Throws an exception if `path` designates a non-directory, or
    some other errors occur.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.directory_create()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("directory_create"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.directory_create"));
    // Parse arguments.
    Sval path;
    if(reader.I().v(path).F()) {
      return std_filesystem_directory_create(::rocket::move(path));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.directory_create(path)`

  * Creates a directory at `path`. Its parent directory must exist
    and must be accessible. This function does not fail if either a
    directory or a symbolic link to a directory already exists on
    `path`.

  * Returns `true` if a new directory has been created, or `false`
    if a directory already exists.

  * Throws an exception if `path` designates a non-directory, or
    some other errors occur.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.directory_remove()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("directory_remove"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.directory_remove"));
    // Parse arguments.
    Sval path;
    if(reader.I().v(path).F()) {
      return std_filesystem_directory_remove(::rocket::move(path));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.directory_remove(path)`

  * Removes the directory at `path`. The directory must be empty.
    This function fails if `path` does not designate a directory.

  * Returns `true` if a directory has been removed successfully, or
    `false` if no such directory exists.

  * Throws an exception if `path` designates a non-directory, or
    some other errors occur.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.file_read()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_read"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.file_read"));
    // Parse arguments.
    Sval path;
    Iopt offset;
    Iopt limit;
    if(reader.I().v(path).o(offset).o(limit).F()) {
      return std_filesystem_file_read(::rocket::move(path), ::rocket::move(offset), ::rocket::move(limit));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_read(path, [offset], [limit])`

  * Reads the file at `path` in binary mode. The read operation
    starts from the byte offset that is denoted by `offset` if it
    is specified, or from the beginning of the file otherwise. If
    `limit` is specified, no more than this number of bytes will be
    read.

  * Returns the bytes that have been read as a string, or `null` if
    the file does not exist.

  * Throws an exception if `offset` is negative, or a read error
    occurs.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.file_stream()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_stream"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.file_stream"));
    // Parse arguments.
    Sval path;
    Fval callback;
    Iopt offset;
    Iopt limit;
    if(reader.I().v(path).v(callback).o(offset).o(limit).F()) {
      return std_filesystem_file_stream(global, path, callback, offset, limit);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
    as an integer, or `null` if the file does not exist.

  * Throws an exception if `offset` is negative, or a read error
    occurs.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.file_write()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_write"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.file_write"));
    // Parse arguments.
    Sval path;
    Sval data;
    Iopt offset;
    if(reader.I().v(path).v(data).o(offset).F()) {
      std_filesystem_file_write(path, data, offset);
      return true;
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_write(path, data, [offset])`

  * Writes the file at `path` in binary mode. The write operation
    starts from the byte offset that is denoted by `offset` if it
    is specified, or from the beginning of the file otherwise. The
    file is truncated to this length before the write operation;
    any existent contents after the write point are discarded. This
    function fails if the data can only be written partially.

  * Returns `true` if all data have been written successfully.

  * Throws an exception if `offset` is negative, or a write error
    occurs.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.file_append()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_append"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.file_append"));
    // Parse arguments.
    Sval path;
    Sval data;
    Bopt exclusive;
    if(reader.I().v(path).v(data).o(exclusive).F()) {
      std_filesystem_file_append(path, data, exclusive);
      return true;
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_append(path, data, [exclusive])`

  * Writes the file at `path` in binary mode. The write operation
    starts from the end of the file; existent contents of the file
    are left intact. If `exclusive` is `true` and a file exists on
    `path`, this function fails. This function also fails if the
    data can only be written partially.

  * Returns `true` if all data have been written successfully.

  * Throws an exception if `offset` is negative, or a write error
    occurs.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.file_copy_from(path_new, path_old)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_copy_from"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.file_copy_from"));
    // Parse arguments.
    Sval path_new;
    Sval path_old;
    if(reader.I().v(path_new).v(path_old).F()) {
      std_filesystem_file_copy_from(path_new, path_old);
      return true;
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_copy_from(path_new, path_old)`

  * Copies the file `path_old` to `path_new`. If `path_old` is a
    symbolic link, it is the target that will be copied, rather
    than the symbolic link itself. This function fails if
    `path_old` designates a directory.

  * Returns `true`.

  * Throws an exception on failure.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.filesystem.file_remove()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_remove"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.filesystem.file_remove"));
    // Parse arguments.
    Sval path;
    if(reader.I().v(path).F()) {
      return std_filesystem_file_remove(path);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.filesystem.file_remove(path)`

  * Removes the file at `path`. This function fails if `path`
    designates a directory.

  * Returns `true` if a file has been removed successfully, or
    `false` if no such file exists.

  * Throws an exception if `path` designates a directory, or some
    other errors occur.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // End of `std.filesystem`
    //===================================================================
  }

}  // namespace Asteria
