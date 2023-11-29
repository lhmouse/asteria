# The Asteria Programming Language

|Compiler     |Category                   |Remarks          |
|:------------|:--------------------------|:----------------|
|**GCC 9**    |:1st_place_medal:Primary   |                 |
|**Clang 11** |:2nd_place_medal:Secondary |A number of meaningless warnings.  |

![asteria](asteria.png)

**Asteria** is a procedural, dynamically typed programming language that is highly inspired by JavaScript but has been designed to address its issues.

* Manuals
    * [Quick guide](doc/quick-guide.md)
    * [Standard library reference](doc/standard-library.md)

* Formal grammar
    * [Production rules](doc/grammar.txt)
    * [Operator precedence](doc/operator-precedence.txt)

# Characteristics

1. First-class functions and closures.
2. Exceptions.
3. Flexible syntax, similar to JavaScript.
4. Copy-on-write values.
5. Garbage collection, similar to Python.
6. Structured binding.
7. Native exception handling, able to catch C++ exceptions as strings.

# How to Build

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
