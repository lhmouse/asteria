# The Asteria Programming Language

|Compiler     |Category                   |
|:------------|:--------------------------|
|**GCC 7**    |:1st_place_medal:Primary   |
|**Clang 11** |:2nd_place_medal:Secondary |

![asteria](asteria.png)

**Asteria** is a procedural, dynamically typed programming language that is
highly inspired by JavaScript but has been designed to address its issues.

* Manuals
    * [Quick Guide](doc/quick-guide.md)
    * [Standard Library Reference](doc/standard-library.md)
    * [Highlighting Rules for GNU nano](doc/asteria.nanorc)

* Formal grammar
    * [Production Rules](doc/grammar.txt)
    * [Operator Precedence](doc/operator-precedence.txt)

# How to Build

Install `libedit-dev` by yourself.

```text
$ autoreconf -i    # requires autoconf, automake and libtool.
$ ./configure
$ make -j$(nproc)
```

# The REPL

```text
$ ./bin/asteria
```

![README](README.png)

# License

BSD 3-Clause License
