|CI            |Category                   |Build on OS  |Host OS  |Build Status     |
|:-------------|:--------------------------|:------------|:--------|:----------------|
|**Travis CI** |:1st_place_medal:Primary   |Ubuntu       |Ubuntu   |[![Build Status](https://travis-ci.org/lhmouse/asteria.svg?branch=master)](https://travis-ci.org/lhmouse/asteria) |
|**AppVeyor**  |:2nd_place_medal:Secondary |Windows      |Windows  |[![Build Status](https://ci.appveyor.com/api/projects/status/github/lhmouse/asteria?branch=master&svg=true)](https://ci.appveyor.com/project/lhmouse/asteria) |

|Compiler    |Category                   |Comments                                                    |
|:-----------|:--------------------------|:-----------------------------------------------------------|
|**GCC 9**   |:1st_place_medal:Primary   |                                                            |
|**GCC 8**   |:1st_place_medal:Primary   |                                                            |
|**Clang 7** |:2nd_place_medal:Secondary |Unknown warning options. A number of meaningless warnings.  |
|**GCC 7**   |:2nd_place_medal:Secondary |Faulty strict overflow warnings.                            |
|**GCC 6**   |:1st_place_medal:Primary   |                                                            |
|**GCC 5**   |:1st_place_medal:Primary   |                                                            |
|**GCC 4.9** |:warning:Unknown           |Lack of test environments.                                  |
|**MSVC 19** |:no_entry:Abandoned        |Little build system support. Internal compiler errors.      |

![GNU nano for the win!](https://raw.githubusercontent.com/lhmouse/poseidon/master/gnu-nano-ftw.png)

# The Asteria Programming Language

1. Sane and clean.
2. Self-consistent.
3. Simple to use.
4. Lightweight.
5. Procedural.
6. Dynamically typed.
7. Easy to integrate in a C++ project. (C++14 is required.)
8. Native to C++ exceptions, particularly `std::bad_alloc`.

# Characteristics

1. First-class functions.
2. Closure functions.
3. Exceptions.
4. Flexible syntax similar to **C++** and **JavaScript**.
5. Regular grammar.
6. Argument passing (by value or reference) determined by the **argument** rather than the **parameter**.
7. Idempotently copyable values basing on copy-on-write.
8. Minimal garbage collection support.
9. Structured binding similar to **C++17**.

# License

BSD 3-Clause License
