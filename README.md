# The Asteria Programming Language

![README](https://raw.githubusercontent.com/lhmouse/asteria/master/README.png)

|CI            |Category                   |Build on OS     |Host OS         |Build Status     |Remarks          |
|:-------------|:--------------------------|:---------------|:---------------|:----------------|:----------------|
|[**Travis CI**](https://travis-ci.org/lhmouse/asteria) |:1st_place_medal:Primary   |Linux (Ubuntu)  |Linux (Ubuntu)  |[![Build Status](https://travis-ci.org/lhmouse/asteria.svg?branch=master)](https://travis-ci.org/lhmouse/asteria) |       |
|[**AppVeyor**](https://ci.appveyor.com/project/lhmouse/asteria) |:2nd_place_medal:Secondary |Windows (MSYS2) |Windows (MSYS2) |[![Build Status](https://ci.appveyor.com/api/projects/status/github/lhmouse/asteria?branch=master&svg=true)](https://ci.appveyor.com/project/lhmouse/asteria) |Standard I/O not in **UTF-32**.  |

|Compiler    |Category                   |Remarks          |
|:-----------|:--------------------------|:----------------|
|**GCC 10**  |:1st_place_medal:Primary   |                 |
|**GCC 9**   |:1st_place_medal:Primary   |                 |
|**GCC 8**   |:1st_place_medal:Primary   |                 |
|**GCC 7**   |:2nd_place_medal:Secondary |Faulty strict overflow warnings.    |
|**GCC 6**   |:1st_place_medal:Primary   |                 |
|**GCC 5**   |:2nd_place_medal:Secondary |Only **5.5** is supported due to faulty `std::regex` constructor.   |
|**Clang 8** |:2nd_place_medal:Secondary |Unknown warning options. A number of meaningless warnings.  |
|**Clang 7** |:2nd_place_medal:Secondary |Unknown warning options. A number of meaningless warnings.  |
|**MSVC 19** |:no_entry:Abandoned        |Little build system support. Internal compiler errors.   |

![GNU nano for the win!](https://raw.githubusercontent.com/lhmouse/asteria/master/GNU-nano-FTW.png)

# Concepts

I started learning programming from **ActionScript** since **Flash 5** days. It was no sooner (and not strange, either) than I learned **JavaScript**, because they are so similar. I admit that **JavaScript** is a fascinating language, but it also has quite a few drawbacks when we look at it today:

1. There is no 64-bit integer type. While some data exchange formats (such as **Protocol Buffers**) do have 64-bit integers, they cannot be stored as `Number`s safely. Some implementations split 64-bit integers into higher and lower parts, which is not very handy, as the lower part suddenly becomes signed and may be negative.
2. There are no binary strings. `String`s are in **UCS-2** (rather than **UTF-16**), while `ArrayBuffer`s are not resizable.
3. `NaN` and `Infinity` are neither keywords nor constants. They are properties of the global object and may be overwritten. Moreover, `Boolean(NaN)` yields `false` unlike other languages.

Here is an issue that is shared by almost all common languages, including **C** and **Python**:

```javascript
let a = [ ];
let b = [ ];

function test(x, y) {
  x.length = 0, x[0] = "hello";  // modifies the array that `x` would reference
  y = [ "hello" ];               // modifies `y` instead of the argument
}
test(a, b);  // arguments are in effect pointers

console.log("a = ", a);   // [ 'hello' ]
console.log("b = ", b);   // []
```

However, compared with typed languages such as **Java**, **JavaScript** has a few advantages:

1. A number is always a `Number`. There are no integer types of varieties of widths, which simplifies programming. Unlike integers, `Number`s never overflow.
2. Being untyped, a function can be passed around like objects without knowing its parameters.

**Asteria** has been highly inspired by **JavaScript** but has been designed to address such issues.

# Features

1. Sane and clean.
2. Self-consistent.
3. Simple to use.
4. Lightweight.
5. Procedural.
6. Dynamically typed.
7. Easy to integrate in a C++ project. (C++14 and hexadecimal floating-point literals are required.)
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

# Data Types

**Asteria** is _untyped_. _Variables_ do not have types. Only _values_ (which can be stored in in variables) do. In this sense functions are considered opaque data.

There are 9 data types:

|**Asteria**  |**JavaScript**  |**Java**   |**C++**                       |**Remarks**                                        |
|:------------|:---------------|:----------|:-----------------------------|:--------------------------------------------------|
|`null`       |`undefined`     |N/A        |`std::nullptr_t`              |                                                   |
|`boolean`    |`Boolean`       |`boolean`  |`bool`                        |                                                   |
|`integer`    |N/A             |`long`     |`std::int64_t`                |signed 64-bit integer in two's complement          |
|`real`       |`Number`        |`double`   |`double`                      |IEEE-754 double-precision floating-point number    |
|`string`     |N/A             |`byte[]`   |`std::string`                 |octet string, commonly refered as byte string      |
|`opaque`     |N/A             |`Object`   |`std::any`                    |opaque value used by bindings                      |
|`function`   |`Function`      |N/A        |N/A                           |functions and closures                             |
|`array`      |`Array`         |N/A        |`std::vector<`<br/>&emsp;`std::any>`       |                                                   |
|`object`     |`Object`        |N/A        |`std::unordered_map<`<br/>&emsp;`std::string,`<br/>&emsp;`std::any>`  |order of elements not preserved                   |

# Expression Categories

```go
var foo;
// `foo` refers to a "variable" holding `null`.

const inc = 42;
// `inc` refers to an "immutable variable" holding an `integer` of `42`.

var bar = func() { return& inc;  };      // return by reference
// `bar` refers to an "immutable variable" holding a function.
// `bar()` refers to the same "variable" as `inc`.

func add(x) { return x + inc;  };   // return by value
// `add` refers to an "immutable variable" holding a function.
// `add(5)` refers to a "temporary" holding an `integer` of `47`.
```

# How to Build

```sh
$ autoreconf -i  # requires autoconf, automake and libtool
$ ./configure
$ make -j$(nproc)
```

# The REPL

```sh
$ ./bin/asteria
```

# WIP

# License

BSD 3-Clause License
