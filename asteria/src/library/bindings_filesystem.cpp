// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_filesystem.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/global_context.hpp"
#include "../utilities.hpp"
#include <sys/stat.h>  // ::stat(), ::fstat(), ::lstat(), ::mkdir(), ::fchmod()
#include <dirent.h>  // ::opendir(), ::closedir()
#include <fcntl.h>  // ::open()
#include <unistd.h>  // ::rmdir(), ::close(), ::read(), ::write(), ::unlink()
#include <stdio.h>  // ::rename()
#include <errno.h>  // errno

namespace Asteria {

Sval std_filesystem_get_working_directory()
  {
    // Get the current directory, resizing the buffer as needed.
    Sval cwd(PATH_MAX, '*');
    while(::getcwd(cwd.mut_data(), cwd.size()) == nullptr) {
      int err = errno;
      if(err != ERANGE) {
        ASTERIA_THROW("`getcwd()` failed (errno `$1`)", err);
      }
      cwd.append(cwd.size() / 2, '*');
    }
    cwd.erase(cwd.find('\0'));
    return cwd;
  }

Oopt std_filesystem_get_information(Sval path)
  {
    struct ::stat stb;
    if(::lstat(path.c_str(), &stb) != 0) {
      return clear;
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

bool std_filesystem_move_from(Sval path_new, Sval path_old)
  {
    return ::rename(path_old.c_str(), path_new.c_str()) == 0;
  }

Iopt std_filesystem_remove_recursive(Sval path)
  {
    if(::rmdir(path.c_str()) == 0) {
      // An empty directory has been removed.
      // Succeed.
      return Ival(1);
    }
    int err = errno;
    if(err == ENOTDIR) {
      // This is something not a directory.
      if(::unlink(path.c_str()) != 0) {
        return clear;
      }
      // Succeed.
      return Ival(1);
    }
    // Remove the directory recursively.
    enum Rmlist
      {
        rmlist_rmdir,     // a subdirectory which should be empty and can be removed
        rmlist_unlink,    // a plain file to be unlinked
        rmlist_expand,    // a subdirectory to be expanded
      };
    int64_t count = 0;
    // This is the list of files and directories to be removed.
    cow_bivector<Rmlist, cow_string> stack;
    stack.emplace_back(rmlist_expand, path);
    while(!stack.empty()) {
      // Pop an element off the stack.
      auto pair = ::rocket::move(stack.mut_back());
      stack.pop_back();
      // Do something.
      if(pair.first == rmlist_rmdir) {
        // This is an empty directory. Remove it.
        if(::rmdir(pair.second.c_str()) != 0) {
          return clear;
        }
        count++;
        continue;
      }
      if(pair.first == rmlist_unlink) {
        // This is a plain file. Remove it.
        if(::unlink(pair.second.c_str()) != 0) {
          return clear;
        }
        count++;
        continue;
      }
      // This is a subdirectory that has not been expanded. Expand it.
      // Push the directory itself. Since elements are maintained in LIFO order, only when this element
      // is encountered for a second time, will all of its children have been removed.
      stack.emplace_back(rmlist_rmdir, pair.second);
      // Append all entries.
      ::rocket::unique_posix_dir dp(::opendir(pair.second.c_str()), ::closedir);
      if(!dp) {
        return clear;
      }
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
        auto child = pair.second + '/' + next->d_name;
        bool is_dir;
#ifdef _DIRENT_HAVE_D_TYPE
        if(ROCKET_EXPECT(next->d_type != DT_UNKNOWN)) {
          // Get the file type if it is available immediately.
          is_dir = next->d_type == DT_DIR;
        }
        else
#endif
        {
          // If the file type is unknown, ask for it.
          struct ::stat stb;
          if(::lstat(child.c_str(), &stb) != 0) {
            return clear;
          }
          is_dir = S_ISDIR(stb.st_mode);
        }
        // Append the entry.
        stack.emplace_back(is_dir ? rmlist_expand : rmlist_unlink, ::rocket::move(child));
      }
    }
    return count;
  }

Oopt std_filesystem_directory_list(Sval path)
  {
    ::rocket::unique_posix_dir dp(::opendir(path.c_str()), closedir);
    if(!dp) {
      return clear;
    }
    // Write entries.
    Oval entries;
    struct ::dirent* next;
    while((next = ::readdir(dp)) != nullptr) {
      cow_string name;
      Oval entry;
      // Assume the name is in UTF-8.
      name.assign(next->d_name);
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
        // Compose the path.
        auto child = path + '/' + name;
        // Identify the entry.
        struct ::stat stb;
        if(::lstat(child.c_str(), &stb) != 0) {
          return clear;
        }
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

Iopt std_filesystem_directory_create(Sval path)
  {
    if(::mkdir(path.c_str(), 0777) == 0) {
      // A new directory has been created.
      return Ival(1);
    }
    int err = errno;
    if(err != EEXIST) {
      return clear;
    }
    // Fail only if it is not a directory that exists.
    struct ::stat stb;
    if(::stat(path.c_str(), &stb) != 0) {
      return clear;
    }
    if(!S_ISDIR(stb.st_mode)) {
      return clear;
    }
    // The directory already exists.
    return Ival(0);
  }

Iopt std_filesystem_directory_remove(Sval path)
  {
    if(::rmdir(path.c_str()) == 0) {
      // The directory has been removed.
      return Ival(1);
    }
    int err = errno;
    if((err != ENOTEMPTY) && (err != EEXIST)) {
      return clear;
    }
    // The directory is not empty.
    return Ival(0);
  }

Sopt std_filesystem_file_read(Sval path, Iopt offset, Iopt limit)
  {
    if(offset && (*offset < 0)) {
      ASTERIA_THROW("negative file offset (offset `$1`)", *offset);
    }
    int64_t roffset = offset.value_or(0);
    int64_t rlimit = ::rocket::clamp(limit.value_or(INT32_MAX), 0, 16777216);
    Sval data;
    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.c_str(), O_RDONLY), ::close);
    if(!fd) {
      return clear;
    }
    // Don't read too many bytes at a time.
    data.resize(static_cast<size_t>(rlimit));
    ::ssize_t nread;
    if(offset) {
      // Read data from the offset specified.
      nread = ::pread(fd, data.mut_data(), data.size(), roffset);
    }
    else {
      // Read data from the beginning.
      nread = ::read(fd, data.mut_data(), data.size());
    }
    if(nread < 0) {
      return clear;
    }
    data.erase(static_cast<size_t>(nread));
    return ::rocket::move(data);
  }

bool std_filesystem_file_stream(Global& global, Sval path, Fval callback,
                                Iopt offset, Iopt limit)
  {
    if(offset && (*offset < 0)) {
      ASTERIA_THROW("negative file offset (offset `$1`)", *offset);
    }
    int64_t roffset = offset.value_or(0);
    int64_t rlimit = ::rocket::clamp(limit.value_or(INT32_MAX), 0, 16777216);
    int64_t nremaining = ::rocket::max(limit.value_or(INT64_MAX), 0);
    Sval data;
    // Open the file for reading.
    ::rocket::unique_posix_fd fd(::open(path.c_str(), O_RDONLY), ::close);
    if(!fd) {
      return false;
    }
    cow_vector<Reference> args;
    for(;;) {
      // Has the read limit been reached?
      if(nremaining <= 0) {
        break;
      }
      // Don't read too many bytes at a time.
      data.resize(static_cast<size_t>(rlimit));
      ::ssize_t nread;
      if(offset) {
        // Read data from the offset specified.
        nread = ::pread(fd, data.mut_data(), data.size(), roffset);
      }
      else {
        // Read data from the beginning.
        nread = ::read(fd, data.mut_data(), data.size());
      }
      if(nread < 0) {
        return false;
      }
      if(nread == 0) {
        break;
      }
      data.erase(static_cast<size_t>(nread));
      // Prepare arguments for the user-defined function.
      args.clear().reserve(2);
      Reference_Root::S_temporary xref_offset = { roffset };
      args.emplace_back(::rocket::move(xref_offset));
      Reference_Root::S_temporary xref_data = { ::rocket::move(data) };
      args.emplace_back(::rocket::move(xref_data));
      // Call the function but discard its return value.
      Reference self = Reference_Root::S_constant();
      callback->invoke(self, global, ::rocket::move(args));
      self.finish_call(global);
      // Read the next block.
      nremaining -= nread;
      roffset += nread;
    }
    return true;
  }

bool std_filesystem_file_write(Sval path, Sval data, Iopt offset)
  {
    if(offset && (*offset < 0)) {
      ASTERIA_THROW("negative file offset (offset `$1`)", *offset);
    }
    int64_t roffset = offset.value_or(0);
    int64_t nremaining = static_cast<int64_t>(data.size());
    // Calculate the `flags` argument.
    // If we are to write from the beginning, truncate the file at creation.
    // This saves us two syscalls to truncate the file below.
    int flags = O_WRONLY | O_CREAT | O_APPEND;
    if(roffset == 0) {
      flags |= O_TRUNC;
    }
    // Open the file for writing.
    ::rocket::unique_posix_fd fd(::open(path.c_str(), flags, 0666), ::close);
    if(!fd) {
      return false;
    }
    // Set the file pointer when an offset is specified, even when it is an explicit zero.
    if(offset) {
      // If `roffset` is not zero, truncate the file there.
      // This also ensures it is a normal file (not a pipe or socket whatsoever).
      // Otherwise, the file will have been truncate at creation.
      if(::ftruncate(fd, roffset) != 0)
        return false;
    }
    for(;;) {
      // Have all data been written successfully?
      if(nremaining <= 0) {
        break;
      }
      // Write data to the end.
      ::ssize_t nwritten = ::write(fd, data.data() + data.size() - nremaining, static_cast<size_t>(nremaining));
      if(nwritten < 0) {
        return false;
      }
      nremaining -= nwritten;
    }
    return true;
  }

bool std_filesystem_file_append(Sval path, Sval data, Bopt exclusive)
  {
    int64_t nremaining = static_cast<int64_t>(data.size());
    // Calculate the `flags` argument.
    // If we are to write from the beginning, truncate the file at creation.
    // This saves us two syscalls to truncate the file below.
    int flags = O_WRONLY | O_CREAT | O_APPEND;
    if(exclusive == true) {
      flags |= O_EXCL;
    }
    // Open the file for writing.
    ::rocket::unique_posix_fd fd(::open(path.c_str(), flags, 0666), ::close);
    if(!fd) {
      return false;
    }
    for(;;) {
      // Have all data been written successfully?
      if(nremaining <= 0) {
        break;
      }
      // Write data to the end.
      ::ssize_t nwritten = ::write(fd, data.data() + data.size() - nremaining, static_cast<size_t>(nremaining));
      if(nwritten < 0) {
        return false;
      }
      nremaining -= nwritten;
    }
    return true;
  }

bool std_filesystem_file_copy_from(Sval path_new, Sval path_old)
  {
    // Open the old file.
    ::rocket::unique_posix_fd hf_old(::open(path_old.c_str(), O_RDONLY), ::close);
    if(!hf_old) {
      return false;
    }
    // Get the file mode and preferred I/O block size.
    struct ::stat stb_old;
    if(::fstat(hf_old, &stb_old) != 0) {
      return false;
    }
    // Create the new file, discarding its contents.
    ::rocket::unique_posix_fd hf_new(::open(path_new.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0200), ::close);
    if(!hf_new) {
      // If the file cannot be opened, unlink it and try again.
      if((errno == EISDIR) || (::unlink(path_new.c_str()) != 0)) {
        return false;
      }
      hf_new.reset(::open(path_new.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0200));
      if(!hf_new) {
        return false;
      }
    }
    // Allocate the I/O buffer.
    Sval buff;
    buff.resize(static_cast<size_t>(stb_old.st_blksize));
    for(;;) {
      // Read some bytes.
      ::ssize_t nread = ::read(hf_old, buff.mut_data(), buff.size());
      if(nread < 0) {
        return false;
      }
      if(nread == 0) {
        break;
      }
      // Write them all.
      ::ssize_t ntotal = 0;
      do {
        ::ssize_t nwritten = ::write(hf_new, buff.mut_data() + ntotal, static_cast<size_t>(nread - ntotal));
        if(nwritten < 0) {
          return false;
        }
        ntotal += nwritten;
      } while(ntotal < nread);
    }
    // Set the file mode. This must be at the last.
    if(::fchmod(hf_new, stb_old.st_mode) != 0) {
      return false;
    }
    return true;
  }

bool std_filesystem_file_remove(Sval path)
  {
    return ::unlink(path.c_str()) == 0;
  }

void create_bindings_filesystem(Oval& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.filesystem.get_working_directory()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("get_working_directory"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.get_working_directory()`\n"
          "\n"
          "  * Gets the absolute path of the current working directory.\n"
          "\n"
          "  * Returns a `string` containing the path to the current working\n"
          "    directory.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.get_working_directory"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            return std_filesystem_get_working_directory();
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.get_information()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("get_information"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.get_information(path)`\n"
          "\n"
          "  * Retrieves information of the file or directory designated by\n"
          "    `path`.\n"
          "\n"
          "  * Returns an `object` consisting of the following members (names\n"
          "    that start with `b_` are `boolean` flags; names that start with\n"
          "    `i_` are IDs as `integer`s; names that start with `n_` are\n"
          "    plain `integer`s; names that start with `t_` are timestamps in\n"
          "    UTC as `integer`s):\n"
          "\n"
          "    * `i_dev`   unique device id on this machine.\n"
          "    * `i_file`  unique file id on this device.\n"
          "    * `n_ref`   number of hard links to this file.\n"
          "    * `b_dir`   whether this is a directory.\n"
          "    * `b_sym`   whether this is a symbolic link.\n"
          "    * `n_size`  number of bytes this file contains.\n"
          "    * `n_ocup`  number of bytes this file occupies.\n"
          "    * `t_accs`  timestamp of last access.\n"
          "    * `t_mod`   timestamp of last modification.\n"
          "\n"
          "    On failure, `null` is returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.get_information"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          if(reader.I().g(path).F()) {
            // Call the binding function.
            return std_filesystem_get_information(::rocket::move(path));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.remove_recursive()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("remove_recursive"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.remove_recursive(path)`\n"
          "\n"
          "  * Removes the file or directory at `path`. If `path` designates\n"
          "    a directory, all of its contents are removed recursively.\n"
          "\n"
          "  * Returns the number of files and directories that have been\n"
          "    successfully removed in total, or `null` on failure.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.remove_recursive"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          if(reader.I().g(path).F()) {
            // Call the binding function.
            return std_filesystem_remove_recursive(::rocket::move(path));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.move_from(path_new, path_old)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("move_from"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.move_from(path_new, path_old)`\n"
          "\n"
          "  * Moves (renames) the file or directory at `path_old` to\n"
          "    `path_new`.\n"
          "\n"
          "  * Returns `true` on success, or `null` on failure.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.move_from"), ::rocket::ref(args));
          // Parse arguments.
          Sval path_new;
          Sval path_old;
          if(reader.I().g(path_new).g(path_old).F()) {
            // Call the binding function.
            return std_filesystem_move_from(path_new, path_old) ? true : null_value;
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.directory_list()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("directory_list"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.directory_list(path)`\n"
          "\n"
          "  * Lists the contents of the directory at `path`.\n"
          "\n"
          "  * Returns an `object` containing all entries of the directory at\n"
          "    `path`, including the special subdirectories '.' and '..'. For\n"
          "    each element, its key specifies the filename and the value is\n"
          "    an `object` consisting of the following members (names that\n"
          "    start with `b_` are `boolean` flags; names that start with `i_`\n"
          "    are IDs as `integer`s):\n"
          "\n"
          "    * `b_dir`   whether this is a directory.\n"
          "    * `b_sym`   whether this is a symbolic link.\n"
          "\n"
          "    On failure, `null` is returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.directory_list"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          if(reader.I().g(path).F()) {
            // Call the binding function.
            return std_filesystem_directory_list(::rocket::move(path));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.directory_create()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("directory_create"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.directory_create(path)`\n"
          "\n"
          "  * Creates a directory at `path`. Its parent directory must exist\n"
          "    and must be accessible. This function does not fail if either\n"
          "    a directory or a symbolic link to a directory already exists on\n"
          "    `path`.\n"
          "\n"
          "  * Returns `1` if a new directory has been created successfully,\n"
          "    `0` if the directory already exists, or `null` on failure.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.directory_create"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          if(reader.I().g(path).F()) {
            // Call the binding function.
            return std_filesystem_directory_create(::rocket::move(path));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.directory_remove()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("directory_remove"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.directory_remove(path)`\n"
          "\n"
          "  * Removes the directory at `path`. The directory must be empty.\n"
          "    This function fails if `path` does not designate a directory.\n"
          "\n"
          "  * Returns `1` if the directory has been removed successfully, `0`\n"
          "    if it is not empty, or `null` on failure.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.directory_remove"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          if(reader.I().g(path).F()) {
            // Call the binding function.
            return std_filesystem_directory_remove(::rocket::move(path));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.file_read()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_read"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.file_read(path, [offset], [limit])`\n"
          "\n"
          "  * Reads the file at `path` in binary mode. The read operation\n"
          "    starts from the byte offset that is denoted by `offset` if it\n"
          "    is specified, or from the beginning of the file otherwise. If\n"
          "    `limit` is specified, no more than this number of bytes will be\n"
          "    read.\n"
          "\n"
          "  * Returns the bytes that have been read as a `string`, or `null`\n"
          "    on failure.\n"
          "\n"
          "  * Throws an exception if `offset` is negative.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.file_read"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          Iopt offset;
          Iopt limit;
          if(reader.I().g(path).g(offset).g(limit).F()) {
            // Call the binding function.
            return std_filesystem_file_read(::rocket::move(path), ::rocket::move(offset), ::rocket::move(limit));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.file_stream()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_stream"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.file_stream(path, callback, [offset], [limit])`\n"
          "\n"
          "  * Reads the file at `path` in binary mode and invokes `callback`\n"
          "    with the data read repeatedly. `callback` shall be a binary\n"
          "    `function` whose first argument is the absolute offset of the\n"
          "    data block that has been read, and whose second argument is the\n"
          "    bytes read and stored in a `string`. Data may be transferred in\n"
          "    multiple blocks of variable sizes; the caller shall make no\n"
          "    assumption about the number of times that `callback` will be\n"
          "    called or the size of each individual block. The read operation\n"
          "    starts from the byte offset that is denoted by `offset` if it\n"
          "    is specified, or from the beginning of the file otherwise. If\n"
          "    `limit` is specified, no more than this number of bytes will be\n"
          "    read.\n"
          "\n"
          "  * Returns `true` if all data have been processed successfully, or\n"
          "    `null` on failure.\n"
          "\n"
          "  * Throws an exception if `offset` is negative.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.file_stream"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          Fval callback;
          Iopt offset;
          Iopt limit;
          if(reader.I().g(path).g(callback).g(offset).g(limit).F()) {
            // Call the binding function.
            return std_filesystem_file_stream(global, path, callback, offset, limit) ? true : null_value;
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.file_write()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_write"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.file_write(path, data, [offset])`\n"
          "\n"
          "  * Writes the file at `path` in binary mode. The write operation\n"
          "    starts from the byte offset that is denoted by `offset` if it\n"
          "    is specified, or from the beginning of the file otherwise. The\n"
          "    file is truncated to this length before the write operation;\n"
          "    any existent contents after the write point are discarded. This\n"
          "    function fails if the data can only be written partially.\n"
          "\n"
          "  * Returns `true` if all data have been written successfully, or\n"
          "    `null` on failure.\n"
          "\n"
          "  * Throws an exception if `offset` is negative.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.file_write"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          Sval data;
          Iopt offset;
          if(reader.I().g(path).g(data).g(offset).F()) {
            // Call the binding function.
            return std_filesystem_file_write(path, data, offset) ? true : null_value;
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.file_append()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_append"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.file_append(path, data)`\n"
          "\n"
          "  * Writes the file at `path` in binary mode. The write operation\n"
          "    starts from the end of the file; existent contents of the file\n"
          "    are left intact. If `exclusive` is `true` and a file exists on\n"
          "    `path`, this function fails. This function also fails if the\n"
          "    data can only be written partially.\n"
          "\n"
          "  * Returns `true` if all data have been written successfully, or\n"
          "    `null` on failure.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.file_append"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          Sval data;
          Bopt exclusive;
          if(reader.I().g(path).g(data).g(exclusive).F()) {
            // Call the binding function.
            return std_filesystem_file_append(path, data, exclusive) ? true : null_value;
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.file_copy_from(path_new, path_old)`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_copy_from"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.file_copy_from(path_new, path_old)`\n"
          "\n"
          "  * Copies the file `path_old` to `path_new`. If `path_old` is a\n"
          "    symbolic link, it is the target that will be copied, rather\n"
          "    than the symbolic link itself. This function fails if\n"
          "    `path_old` designates a directory.\n"
          "\n"
          "  * Returns `true` on success, or `null` on failure.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.file_copy_from"), ::rocket::ref(args));
          // Parse arguments.
          Sval path_new;
          Sval path_old;
          if(reader.I().g(path_new).g(path_old).F()) {
            // Call the binding function.
            return std_filesystem_file_copy_from(path_new, path_old) ? true : null_value;
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.filesystem.file_remove()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("file_remove"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.filesystem.file_remove(path)`\n"
          "\n"
          "  * Removes the file at `path`. This function fails if `path`\n"
          "    designates a directory.\n"
          "\n"
          "  * Returns `true` if the file has been removed successfully, or\n"
          "    `null` on failure.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.filesystem.file_remove"), ::rocket::ref(args));
          // Parse arguments.
          Sval path;
          if(reader.I().g(path).F()) {
            // Call the binding function.
            return std_filesystem_file_remove(path) ? true : null_value;
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // End of `std.filesystem`
    //===================================================================
  }

}  // namespace Asteria
