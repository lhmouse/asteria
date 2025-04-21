# The Asteria Programming Language

|Compiler     |Category                   |
|:------------|:--------------------------|
|**GCC 7**    |:1st_place_medal:Primary   |
|**Clang 11** |:2nd_place_medal:Secondary |

![asteria](asteria.png)

**Asteria** is a procedural, dynamically typed programming language that is
highly inspired by JavaScript.

The most notable difference between Asteria and other languages is that the
so-called 'objects' have _value semantics_. That is, an object is copied when
it is passed to a function by value or is assigned to a variable, just like
values of primitive types.

* [Quick Guide](doc/quick-guide.md)
* [Grammar](doc/grammar.md)
* [Standard Library](doc/standard-library.md)
* [Highlighting Rules for GNU nano](doc/asteria.nanorc)

# How to Build

First, you need to install some dependencies and an appropriate compiler,
which can be done with

```sh
# For Debian, Ubuntu, Linux Mint:
# There is usually an outdated version of meson in the system APT source. Do
# not use it; instead, install the latest one from pip.
sudo apt-get install ninja-build python3 python3-pip pkgconf g++  \
        libpcre2-dev libssl-dev zlib1g-dev libedit-dev
sudo pip3 install meson
```

```sh
# For MSYS2 on Windows:
# The `iconv_open()` etc. functions are provided by libiconv. Only the MSYS
# shell is supported. Do not try building in the MINGW64 or UCRT64 shell.
pacman -S meson gcc pkgconf pcre2-devel openssl-devel zlib-devel  \
        libiconv-devel libedit-devel
```

```sh
# For macOS:
# The `gcc` command actually denotes Clang, so ask for a specific version
# explicitly.
brew install meson pkgconf gcc@10 pcre2 openssl@3 zlib libedit
export CXX='g++-10'
```

Then we can build as usual

```sh
meson setup build_debug
meson compile -Cbuild_debug
```

Finally we launch the REPL, as

```sh
./build_debug/asteria
```

![README](README.png)

If you need only the library and don't want to build the REPL, you may omit
`libedit` from the dependencies above, and pass `-Denable-repl=false` to `meson`.

# License

BSD 3-Clause License
