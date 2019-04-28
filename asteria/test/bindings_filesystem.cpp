// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/compiler/simple_source_file.hpp"
#include "../src/runtime/global_context.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    static constexpr char s_source[] =
      R"__(
        const chars = "0123456789abcdefghijklmnopqrstuvwxyz";
        // We presume these random strings will never match any real files.
        var dname = ".filesystem-test_dir_" + std.string.implode(std.array.shuffle(std.string.explode(chars)));
        var fname = ".filesystem-test_file_" + std.string.implode(std.array.shuffle(std.string.explode(chars)));

        assert std.filesystem.get_information(dname) == null;
        assert std.filesystem.get_information(fname) == null;

        assert std.filesystem.directory_create(dname) == 1;
        assert std.filesystem.get_information(dname).b_dir == true;
        assert std.filesystem.directory_create(dname) == 0;

        assert std.filesystem.file_append(dname + "/f1", "1") == true;
        assert std.filesystem.file_append(dname + "/f2", "2") == true;
        assert std.filesystem.directory_create(dname + "/f3") == 1;
        assert std.filesystem.file_append(dname + "/f3/a", "3") == true;
        assert std.filesystem.directory_create(dname + "/f4") == 1;
        assert std.filesystem.directory_create(dname + "/f4/f5") == 1;
        assert std.filesystem.file_append(dname + "/f4/f5/a", "4") == true;
        assert std.filesystem.file_append(dname + "/f4/f5/b", "5") == true;
        assert std.array.sort(std.array.copy_keys(std.filesystem.directory_list(dname))) == [".","..","f1","f2","f3","f4"];

        assert std.filesystem.remove_recursive(dname + "/f1") == 1;
        assert std.filesystem.remove_recursive(dname + "/f1") == null;
        assert std.filesystem.move_from(dname + "/f5", dname + "/f2") == true;
        assert std.filesystem.move_from(dname + "/f5", dname + "/f2") == null;
        assert std.array.sort(std.array.copy_keys(std.filesystem.directory_list(dname))) == [".","..","f3","f4","f5"];

        assert std.filesystem.file_remove(dname) == null;
        assert std.filesystem.directory_remove(dname) == 0;
        assert std.filesystem.remove_recursive(dname) == 8;
        assert std.filesystem.directory_remove(dname) == null;

        assert std.filesystem.file_read(fname) == null;
        assert std.filesystem.file_append(fname, "@@@@$$", true) == true; // "@@@@$$"
        assert std.filesystem.get_information(fname).b_dir == false;
        assert std.filesystem.get_information(fname).n_size == 6;
        assert std.filesystem.file_write(fname, "hello") == true;  // "hello"
        assert std.filesystem.get_information(fname).n_size == 5;
        assert std.filesystem.file_write(fname, "HELLO", 3) == true; // "helHELLO"
        assert std.filesystem.get_information(fname).n_size == 8;
        assert std.filesystem.file_write(fname, "#", 5) == true; // "helHE#"
        assert std.filesystem.get_information(fname).n_size == 6;

        assert std.filesystem.file_append(fname, "??") == true; // "helHE#??"
        assert std.filesystem.get_information(fname).n_size == 8;
        assert std.filesystem.file_append(fname, "!!") == true; // "helHE#??!!"
        assert std.filesystem.get_information(fname).n_size == 10;
        assert std.filesystem.file_append(fname, "!!", true) == null;

        assert std.filesystem.file_read(fname) == "helHE#??!!";
        assert std.filesystem.file_read(fname, 2) == "lHE#??!!";
        assert std.filesystem.file_read(fname, 1000) == "";
        assert std.filesystem.file_read(fname, 2, 1000) == "lHE#??!!";
        assert std.filesystem.file_read(fname, 2, 3) == "lHE";

        assert std.filesystem.file_copy_from(fname + ".2", fname) == true;
        assert std.filesystem.file_read(fname + ".2") == "helHE#??!!";

        var data = "";
        var appender = func(off, str) { data += str;  };
        assert std.filesystem.file_stream(fname, appender) == true;
        assert data == "helHE#??!!";
        data = "";
        assert std.filesystem.file_stream(fname, appender, 2) == true;
        assert data == "lHE#??!!";
        data = "";
        assert std.filesystem.file_stream(fname, appender, 1000) == true;
        assert data == "";
        data = "";
        assert std.filesystem.file_stream(fname, appender, 2, 1000) == true;
        assert data == "lHE#??!!";
        data = "";
        assert std.filesystem.file_stream(fname, appender, 2, 3) == true;
        assert data == "lHE";

        assert std.filesystem.directory_create(fname) == null;
        assert std.filesystem.file_remove(fname) == true;
        assert std.filesystem.file_remove(fname) == null;
        assert std.filesystem.file_remove(fname + ".2") == true;

        assert std.filesystem.directory_create(fname) == 1;
        assert std.filesystem.file_remove(fname) == null;
        assert std.filesystem.directory_remove(fname) == 1;
      )__";

    std::istringstream iss(s_source);
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global, { });
  }
