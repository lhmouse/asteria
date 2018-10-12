|CI            |Category  |Host OS       |Build for OS        |Build Status     |
|:-------------|:---------|:-------------|:-------------------|:----------------|
|**Travis CI** |Primary   |Ubuntu Trusty |Ubuntu Trusty       |[![Build Status](https://travis-ci.org/lhmouse/asteria.svg?branch=master)](https://travis-ci.org/lhmouse/asteria) |
|**Tea CI**    |Secondary |Wine          |Windows Server 2003 |[![Build Status](https://tea-ci.org/api/badges/lhmouse/asteria/status.svg)](https://tea-ci.org/lhmouse/asteria) |

![GNU nano for the win!](https://raw.githubusercontent.com/lhmouse/poseidon/master/gnu-nano-ftw.png)

# The Asteria Programming Language

1. Sane and clean.
2. Self-consistent.
3. Simple to use.
4. Lightweight.
5. Procedural.
6. Dynamically typed.
7. Easy to integrate in a C++ project. (C++11 is required.)
8. Native to C++ exceptions, particularly `std::bad_alloc`.

# Characteristics

1. First-class functions.
2. Closure functions (lambda expressions).
3. Exceptions.
4. Flexible syntax similar to **C++** and **JavaScript**.
5. Regular grammar.
6. Pass-by-reference function arguments.  
Return values are passed by value by default, but can also be passed by reference using the `return&` syntax.
7. Minimal garbage collection support.
Objects are managed using referencing counting. Primitive types are copy-on-write hence circular reference is impossible.

# License

BSD 3-Clause License
