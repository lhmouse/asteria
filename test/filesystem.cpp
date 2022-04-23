// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"

using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        const chars = "0123456789abcdefghijklmnopqrstuvwxyz";
        // We presume these random strings will never match any real files.
        var dname = ".filesystem-test_dir_" + std.string.implode(std.array.shuffle(std.string.explode(chars)));
        var fname = ".filesystem-test_file_" + std.string.implode(std.array.shuffle(std.string.explode(chars)));

        assert std.filesystem.get_properties(dname) == null;
        assert std.filesystem.get_properties(fname) == null;

        assert std.filesystem.dir_create(dname) == 1;
        assert std.filesystem.get_properties(dname).is_directory == true;
        assert std.filesystem.dir_create(dname) == 0;

        std.filesystem.file_append(dname + "/f1", "1");
        std.filesystem.file_append(dname + "/f2", "2");
        assert std.filesystem.dir_create(dname + "/f3") == 1;
        std.filesystem.file_append(dname + "/f3/a", "3");
        assert std.filesystem.dir_create(dname + "/f4") == 1;
        assert std.filesystem.dir_create(dname + "/f4/f5") == 1;
        std.filesystem.file_append(dname + "/f4/f5/a", "4");
        std.filesystem.file_append(dname + "/f4/f5/b", "5");
        assert std.array.sort(std.array.copy_keys(std.filesystem.dir_list(dname))) == ["f1","f2","f3","f4"];

        assert std.filesystem.remove_recursive(dname + "/f1") == 1;
        assert std.filesystem.remove_recursive(dname + "/f1") == 0;

        std.filesystem.move_from(dname + "/f5", dname + "/f2");
        assert catch( std.filesystem.move_from(dname + "/f5", dname + "/f2") ) != null;
        assert std.array.sort(std.array.copy_keys(std.filesystem.dir_list(dname))) == ["f3","f4","f5"];

        assert catch( std.filesystem.file_remove(dname) ) != null;
        assert catch( std.filesystem.dir_remove(dname) ) != null;
        assert std.filesystem.remove_recursive(dname) == 8;
        assert std.filesystem.dir_remove(dname) == 0;

        assert catch( std.filesystem.file_read("/nonexistent") == null ) != null;
        std.filesystem.file_append(fname, "@@@@$$", true);  // "@@@@$$"
        assert std.filesystem.get_properties(fname).is_directory == false;
        assert std.filesystem.get_properties(fname).size == 6;
        std.filesystem.file_write(fname, "hello");  // "hello"
        assert std.filesystem.get_properties(fname).size == 5;
        std.filesystem.file_write(fname, 3, "HELLO");  // "helHELLO"
        assert std.filesystem.get_properties(fname).size == 8;
        std.filesystem.file_write(fname, 5, "#");  // "helHE#"
        assert std.filesystem.get_properties(fname).size == 6;

        std.filesystem.file_append(fname, "??");  // "helHE#??"
        assert std.filesystem.get_properties(fname).size == 8;
        std.filesystem.file_append(fname, "!!");  // "helHE#??!!"
        assert std.filesystem.get_properties(fname).size == 10;
        assert catch( std.filesystem.file_append(fname, "!!", true) ) != null;

        assert std.filesystem.file_read(fname) == "helHE#??!!";
        assert std.filesystem.file_read(fname, 2) == "lHE#??!!";
        assert std.filesystem.file_read(fname, 1000) == "";
        assert std.filesystem.file_read(fname, 2, 1000) == "lHE#??!!";
        assert std.filesystem.file_read(fname, 2, 3) == "lHE";

        std.filesystem.file_copy_from(fname + ".2", fname);
        assert std.filesystem.file_read(fname + ".2") == "helHE#??!!";

        var data = "";
        var appender = func(off, str) { data += str;  };
        assert catch( std.filesystem.file_stream("/nonexistent", appender) ) != null;
        assert std.filesystem.file_stream(fname, appender) == 10;
        assert data == "helHE#??!!";
        data = "";
        assert std.filesystem.file_stream(fname, appender, 2) == 8;
        assert data == "lHE#??!!";
        data = "";
        assert std.filesystem.file_stream(fname, appender, 1000) == 0;
        assert data == "";
        data = "";
        assert std.filesystem.file_stream(fname, appender, 2, 1000) == 8;
        assert data == "lHE#??!!";
        data = "";
        assert std.filesystem.file_stream(fname, appender, 2, 3) == 3;
        assert data == "lHE";

        assert catch( std.filesystem.dir_create(fname) ) != null;
        assert std.filesystem.file_remove(fname) == 1;
        assert std.filesystem.file_remove(fname) == 0;
        assert std.filesystem.file_remove(fname + ".2") == 1;

        assert std.filesystem.dir_create(fname) == 1;
        assert catch( std.filesystem.file_remove(fname) ) != null;
        assert std.filesystem.dir_remove(fname) == 1;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
