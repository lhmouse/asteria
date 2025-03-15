// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      &__FILE__, __LINE__, &R"__(
///////////////////////////////////////////////////////////////////////////////

        const chars = "0123456789abcdefghijklmnopqrstuvwxyz";
        // We presume these random strings will never match any real files.
        var dname = ".filesystem-test_dir_" + std.string.implode(std.array.shuffle(std.string.explode(chars)));
        var fname = ".filesystem-test_file_" + std.string.implode(std.array.shuffle(std.string.explode(chars)));

        assert catch( std.filesystem.get_properties(dname) ) != null;
        assert catch( std.filesystem.get_properties(fname) ) != null;

        assert std.filesystem.create_directory(dname) == 1;
        assert std.filesystem.get_properties(dname).is_directory == true;
        assert std.filesystem.create_directory(dname) == 0;

        std.filesystem.append(dname + "/f1", "1");
        std.filesystem.append(dname + "/f2", "2");
        assert std.filesystem.create_directory(dname + "/f3") == 1;
        std.filesystem.append(dname + "/f3/a", "3");
        assert std.filesystem.create_directory(dname + "/f4") == 1;
        assert std.filesystem.create_directory(dname + "/f4/f5") == 1;
        std.filesystem.append(dname + "/f4/f5/a", "4");
        std.filesystem.append(dname + "/f4/f5/b", "5");
        assert std.array.sort(std.array.copy_keys(std.filesystem.list(dname))) == ["f1","f2","f3","f4"];

        assert std.filesystem.remove_recursive(dname + "/f1") == 1;
        assert std.filesystem.remove_recursive(dname + "/f1") == 0;

        std.filesystem.move(dname + "/f5", dname + "/f2");
        assert catch( std.filesystem.move(dname + "/f5", dname + "/f2") ) != null;
        assert std.array.sort(std.array.copy_keys(std.filesystem.list(dname))) == ["f3","f4","f5"];

        assert catch( std.filesystem.remove_file(dname) ) != null;
        assert catch( std.filesystem.remove_directory(dname) ) != null;
        assert std.filesystem.remove_recursive(dname) == 8;
        assert std.filesystem.remove_directory(dname) == 0;

        assert catch( std.filesystem.read("/nonexistent") == null ) != null;
        std.filesystem.append(fname, "@@@@$$", true);  // "@@@@$$"
        assert std.filesystem.get_properties(fname).is_directory == false;
        assert std.filesystem.get_properties(fname).size == 6;
        std.filesystem.write(fname, "hello");  // "hello"
        assert std.filesystem.get_properties(fname).size == 5;
        std.filesystem.write(fname, 3, "HELLO");  // "helHELLO"
        assert std.filesystem.get_properties(fname).size == 8;
        std.filesystem.write(fname, 5, "#");  // "helHE#"
        assert std.filesystem.get_properties(fname).size == 6;

        std.filesystem.append(fname, "??");  // "helHE#??"
        assert std.filesystem.get_properties(fname).size == 8;
        std.filesystem.append(fname, "!!");  // "helHE#??!!"
        assert std.filesystem.get_properties(fname).size == 10;
        assert catch( std.filesystem.append(fname, "!!", true) ) != null;

        assert std.filesystem.read(fname) == "helHE#??!!";
        assert std.filesystem.read(fname, 2) == "lHE#??!!";
        assert std.filesystem.read(fname, 1000) == "";
        assert std.filesystem.read(fname, 2, 1000) == "lHE#??!!";
        assert std.filesystem.read(fname, 2, 3) == "lHE";

        std.filesystem.copy_file(fname + ".2", fname);
        assert std.filesystem.read(fname + ".2") == "helHE#??!!";

        var data = "";
        var appender = func(off, str) { data += str;  };
        assert catch( std.filesystem.stream("/nonexistent", appender) ) != null;
        assert std.filesystem.stream(fname, appender) == 10;
        assert data == "helHE#??!!";
        data = "";
        assert std.filesystem.stream(fname, appender, 2) == 8;
        assert data == "lHE#??!!";
        data = "";
        assert std.filesystem.stream(fname, appender, 1000) == 0;
        assert data == "";
        data = "";
        assert std.filesystem.stream(fname, appender, 2, 1000) == 8;
        assert data == "lHE#??!!";
        data = "";
        assert std.filesystem.stream(fname, appender, 2, 3) == 3;
        assert data == "lHE";

        assert catch( std.filesystem.create_directory(fname) ) != null;
        assert std.filesystem.remove_file(fname) == 1;
        assert std.filesystem.remove_file(fname) == 0;
        assert std.filesystem.remove_file(fname + ".2") == 1;

        assert std.filesystem.create_directory(fname) == 1;
        assert catch( std.filesystem.remove_file(fname) ) != null;
        assert std.filesystem.remove_directory(fname) == 1;

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
