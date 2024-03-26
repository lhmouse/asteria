# Asteria Quick Guide

For this quick guide, we are using the REPL for demonstration. I hope you
will enjoy it.

Asteria is highly inspired by JavaScript and shares a similar syntax, as well
as other scripting languages. However there are some fundamental differences:

1. Arrays and objects/tables/dictionaries are passed by value, rather than by
   reference. Whenever the assignment operator `=` occurs, it makes a deep
   copy of a value.

2. There is a 64-bit integer type. Integers can be converted to real numbers
   implicitly. However there is no implicit conversion from real numbers to
   integers; it is always required to use one of the `__itrunc`, `__ifloor`,
   `__iceil` or `__iround` operators to cast them, with an explicit rounding
   direction.

3. Function parameters are all references. Whether to pass an argument by
   value or by reference is specified by the argument as part of the function
   call syntax. For example, `foo(arg1)` passes `arg1` by value, while
   `foo(ref arg1)` passes `arg1` by reference. If an argument is passed by
   value, its corresponding parameter will reference a temporary value;
   attempting to modify temporary values will effect runtime errors. This
   applies similarly to function results.

# Table of Contents

1. [Course 101](#course-101)
2. [Literals, Not To Be Confused with Constants](#literals-not-to-be-confused-with-constants)
3. [Variables and Functions](#variables-and-functions)
4. [Lambdas](#lambdas)
5. [Object-oriented Programming and the `this` Parameter](#object-oriented-programming-and-the-this-parameter)
6. [Ordering of Values](#ordering-of-values)
7. [Structured Bindings](#structured-bindings)
8. [Arguments and Results by Reference](#arguments-and-results-by-reference)
9. [Throwing and Catching Exceptions](#throwing-and-catching-exceptions)
10. [Integer Overflows](#integer-overflows)
11. [Bit-wise Operators on Strings](#bit-wise-operators-on-strings)
12. [Calling C++ Functions from Asteria](#calling-c-functions-from-asteria)
13. [Calling Asteria Functions from C++](#calling-asteria-functions-from-c)

## Course 101

In the first class of every programming language, we are taught to output the
magical string `"hello world!"`. This is achieved by calling the `putln()`
function from `std.io`, as in

```
#1:1> std.io.putln("hello world!");
* running 'snippet #1'...
hello world!
* result #1: void
```

Here is a program that prints the sum from 1 to 100. First, we need to define
a variable for the sum. Then, we iterate from 1 to 100 and add each number to
the sum. Finally, we print the sum with `putfln()`.

The `putfln()` function takes a _template string_ with _placeholders_; the
placeholder `$1` is to be replaced with the 1st argument that follows it, and
`$2` is to be replaced with the 2nd argument that follows it, and so on.

As our program no longer first into one line, we need to enable the _heredoc_
mode. The `:heredoc` command takes an arbitrary string that will mark the end
of a single snippet. In this example we use `@@`, so our program looks like

```
#2:1> :heredoc @@
* the next snippet will be terminated by `@@`

#3:1> var sum = 0;
   2> 
   3> for(var i = 1;  i <= 100;  ++i)
   4>   sum += i;
   5>   
   6> std.io.putfln("sum = $1", sum);
   7> @@
* running 'snippet #3'...
sum = 5050
* result #3: void
```

The last example in this section is a classical 'guess my number' game. Our
program generates a random number from 1 to 100, which we will have to guess.
Our program uses `getln()` to read our input, then converts it to an integer,
and checks whether it is correct, like

```
#4:1> :heredoc @@
* the next snippet will be terminated by `@@`

#5:1> var num = 1 + __ifloor std.numeric.random(100);  // 1 - 100
   2> std.io.putln("number generated; guess now!");
   3> 
   4> var input;
   5> while((input = std.io.getln()) != null) {  // get a line
   6>   var guess = std.numeric.parse(input);  // convert it to a number
   7> 
   8>   if(guess > num) {
   9>     std.io.putln("your answer was greater; try again.");
  10>     continue;
  11>   }
  12>   else if(guess < num) {
  13>     std.io.putln("your answer was less; try again.");
  14>     continue;
  15>   }
  16>   else {
  17>     std.io.putln("you got it!");
  18>     break;
  19>   }
  20> } 
  21> 
  22> std.io.putfln("my number was $1. have a nice day.", num);
  23> @@
* running 'snippet #5'...
number generated; guess now!
50
your answer was greater; try again.
23
your answer was less; try again.
28
your answer was less; try again.
35
your answer was greater; try again.
32
your answer was greater; try again.
30
you got it!
my number was 30. have a nice day.
* result #5: void
```

[back to table of contents](#table-of-contents)

## Literals, Not To Be Confused with Constants

A literal, such as the numeric literal `-42.5`, produces a single value. Some
people reference a literal as a 'constant', some people also consider an
immutable variable as a 'constant', and some others also think that anything
unmodifiable is a 'constant', such as a lambda. Due to potential confusion,
we tend to avoid the word 'constant' throughout this document.

Informally, literals are particular syntactical constructions, which produce
values that they 'literally' look like:

1. `null` denotes the unique null value.

2. `true` and `false` denote the two boolean values.

3. `infinity`/`Infinity` and `nan`/`NaN` denote the two special real numbers,
   positive infinity and (quiet) NaN, as their names suggest.

4. A _numeric literal_, which denotes a number, comprises an optional sign,
   an optional radix specifier, a non-empty series of significant figures,
   and an optional exponent. All components are case-insensitive, specified
   as follows:

   1) There shall be no space between individual components.
   2) The explicit sign is part of the literal, and is not to be interpreted
      as the unary negation operator.
   3) The radix specifier shall be either `0b` or `0x`, which denotes a
      binary literal or a hexadecimal literal, respectively. If there is no
      radix specifier, the literal is decimal, and significant figures shall
      start with a digit.
   4) Valid digits for a binary literal are either `0` or `1`. Valid digits
      for a hexadecimal literal are any of `0123456789abcdef`. Valid digits
      for a decimal literal are any of `0123456789`.
   5) If no radix point `.` exists amongst significant figures, the literal
      denotes an integer. If there is a radix point, the literal denotes a
      real number.
   6) The exponent for a binary or hexadecimal literal shall start with `p`,
      followed by the index in decimal which 2 is raised to. The exponent for
      a decimal literal shall start with `e`, followed by the index which 10
      is raised to.
   7) If the literal denotes an integer, it shall be exact. If the literal
      denotes a real number, it shall not overflow to infinity or underflow
      to zero.
   8) The digit separator `` ` `` can be inserted freely within a literal
      (except in the beginning, of course) without changing its value. For
      example, both ``123`456e1`0`` and `123456e10` produce the integer
      `1234560000000000`.

5. A _string literal_ denotes a string of bytes. There are two variants of
   string literals: A literal that starts and ends with a pair of single
   quotation marks `''` produces a verbatim copy of the UTF-8 string between
   them, which however prevents embedded single quotation marks; a literal
   that starts and ends with a pair of double quotation marks `""` is capable
   of encoding arbitrary bytes. Other than the above, all string literals are
   semantically equivalent. Consecutive string literals are concatenated to
   form a single, longer one before further processing. The following _escape
   sequences_ may be used within double-quotation literals:

   1) `\'`, `\"`, `\\`, `\?` and `\/` produce each character that follows the
      slash, respectively.
   2) `\0` produces the ASCII null character `U+0000`.
   3) `\a` produces the ASCII bell character `U+0007`.
   4) `\b` produces the ASCII backspace character `U+0008`.
   5) `\e` produces the ASCII escape character `U+001B`.
   6) `\f` produces the ASCII form feed character `U+000C`.
   7) `\n` produces the ASCII line feed character `U+000A`.
   8) `\r` produces the ASCII carriage return character `U+000D`.
   9) `\t` produces the ASCII horizontal tab character `U+0009`.
   10) `\U` which shall be followed by six hexadecimal digits, and `\u` which
       shall be followed by four hexadecimal digits, produce the UTF-8 byte
       sequence of their operand, which must be a valid UTF code point.
   11) `\v` produces the ASCII vertical tab character `U+000B`.
   12) `\x` which shall be followed by two hexadecimal digits, produces a
       single byte with that value.
   13) `\Z` produces the ASCII substitute character `U+001A`, which can be
       generated in a terminal by pressing Ctrl-V Ctrl-Z.

6. An _array literal_ starts with an open bracket `[`, followed by a sequence
   of expressions, and ends with a closing bracket `]`. Expressions may be
   separated by a comma `,` or a semicolon `;`. A trailing separator between
   the last expression and the closing bracket is allowed. All expressions
   are evaluated from left to right, and the literal produces a temporary
   array of those values.

7. An _object literal_ starts with an open brace `{`, followed by a sequence
   of key-expression pairs, and ends with a closing brace `}`. As with JSON5,
   a key may be an identifier, keyword or string literal. A key and its
   corresponding expression may be separated by a colon `:` or an equals sign
   `=`. Key-expression pairs may be separated by a comma `,` or a semicolon
   `;`. A trailing separator between the last expression and the closing
   brace is allowed. There shall be no duplicate keys. All expressions are
   evaluated from left to right, and the literal produces a temporary object
   of those key-value pairs.

[back to table of contents](#table-of-contents)

## Variables and Functions

By definition, a _variable_ is a named container that is capable of storing a
value. Usually we declare variables with `var`, however that's not the only
way. Sometimes, people find it helpful to forbid unintentional modification
to a variable. A variable can be declared with `const` to prevent itself from
being modified; such a variable is called an _immutable variable_, as in

```
#1:1> :heredoc @@
* the next snippet will be terminated by `@@`

#2:1> const temp_value = 42;
   2> temp_value = 100;
   3> @@
* running 'snippet #2'...
! exception: runtime error: Attempt to modify a `const` variable
[backtrace frames:
  1) native code at '[unknown]:-1:-1': "Attempt to modify a `const` variable"
  2)   expression at 'snippet #2:2:12': ""
  3)   function at 'snippet #2:0:0': "[file scope]"
  -- end of backtrace frames]
```

Plain variables can be declared without initialization, in which case they
are initialized to `null`. Immutable variables are required to be initialized
as soon as they are defined; otherwise syntax errors are raised.

A _function_ can be defined with `func`, and by its nature, it is also an
immutable variable, whose value is a function.

Each variable has a _scope_ where it is declared, and can only be referenced
within its scope. Multiple variables with the same name can be declared, in
the same scope or nested scopes; in this way earlier variables are _shadowed_
by the latest one, and become invisible, as in

```
#3:1> :heredoc @@
* the next snippet will be terminated by `@@`

#4:1> var a = 42;
   2> {
   3>   func a() { std.io.putln("the first `a` is shadowed"); }
   4>   var b = a;  // another reference to `a()`
   5>   var a = "shadow-it-again";
   6>   
   7>   std.io.putfln("`a` denotes $1", a);
   8>   std.io.putfln("`b` denotes $1", b);  // access function via `b`
   9> } 
  10> std.io.putfln("left scope; `a` now denotes $1", a);
  11> @@
* running 'snippet #4'...
`a` denotes shadow-it-again
`b` denotes (function) [[`a()` at 'snippet #4:3:3']]
left scope; `a` now denotes 42
```

[back to table of contents](#table-of-contents)

## Lambdas

Lambdas are anonymous functions. The most common scenario where lambdas are
useful is probably sorting and searching. For example, in order to sort an
array of integers in ascending order, we do

```
#1:1> std.array.sort([8,4,6,9,2,5,3,0,1,7])
* running 'expression #1'...
* result #1: array(10) [
  0 = integer 0;
  1 = integer 1;
  2 = integer 2;
  3 = integer 3;
  4 = integer 4;
  5 = integer 5;
  6 = integer 6;
  7 = integer 7;
  8 = integer 8;
  9 = integer 9;
];
```

But what if we want to sort it in descending order instead? First, let's take
a look at the prototype of `std.array.sort`, like

```
#2:1> std.array.sort
* running 'expression #2'...
* result #2: function [[`std.array.sort(data, [comparator])` at 'asteria/library/array.cpp:1058']];
```

So it can take an optional `comparator` function. Every standard function
which takes a `comparator` has the same requirement:

1. `comparator` takes two arguments, namely `x` and `y`.
2. If `x` is less than `y`, `comparator(x, y)` returns a negative number.
3. If `x` is greater than `y`, `comparator(x, y)` returns a positive number.
4. If `x` is equivalent to `y`, `comparator(x, y)` returns zero.

Other results usually indicate that `x` and `y` are unordered, and are likely
to cause exceptions to be thrown. This specification matches the built-in
_spaceship operator_ `<=>`. Therefore, in order to sort an array in reverse
order, we need to define a lambda which returns the opposite of the default
comparison result. There are actually two ways to achieve this, either
`func(x, y) { return -(x <=> y); }` or `func(x, y) { return y <=> x; }`. We
use the second form, and it looks like

```
#3:1> std.array.sort([8,4,6,9,2,5,3,0,1,7], \
   2>                func(x, y) { return y <=> x; })
* running 'expression #3'...
* result #3: array(10) [
  0 = integer 9;
  1 = integer 8;
  2 = integer 7;
  3 = integer 6;
  4 = integer 5;
  5 = integer 4;
  6 = integer 3;
  7 = integer 2;
  8 = integer 1;
  9 = integer 0;
];
```

Another example is about how to sort an array of integers by their final
digits, as in

```
#4:1> std.array.sort([58,34,16,89,62,95,73,41,27],  \
   2>                func(x, y) { return x % 10 <=> y % 10; })
* running 'expression #4'...
* result #4: array(9) [
  0 = integer 41;
  1 = integer 62;
  2 = integer 73;
  3 = integer 34;
  4 = integer 95;
  5 = integer 16;
  6 = integer 27;
  7 = integer 58;
  8 = integer 89;
];
```

Lambdas are (sub-)expressions. A lambda that returns something has a short
form. For example, `func(x, y) { return x + y; }` can be abbreviated as just
`func(x, y) = x + y`, without `{ return` or `; }`. Likewise, `func(x, y) {
return ref x.foo(y); }` can be abbreviated as `func(x, y) -> x.foo(y)`, also
without `{ return ref` or `; }`.

[back to table of contents](#table-of-contents)

## Object-oriented Programming and the `this` Parameter

In many other programming languages, there is a concept about non-static
member functions. A non-static member function receives an implicit `this`
parameter that references the object which is 'intuitively' used to call the
function.

Our definition of the argument for `this` is based on this 'intuitive' idea.
The _argument_ for `this` is the reference that denotes the target function
with the final subscript removed. For example, in

```
#4:1> :heredoc @@
* the next snippet will be terminated by `@@`

#5:1> var my_obj = {
   2>   value = 1;
   3>   get = func() { return this.value; };
   4>   set = func(x) { this.value = x; };
   5> };
   6> 
   7> std.io.putfln("my_obj.value = $1", my_obj.value);
   8> std.io.putfln("my_obj.get() = $1", my_obj.get());
   9> 
  10> my_obj.set(42);
  11> std.io.putfln("after set(), my_obj.value = $1", my_obj.value);
  12> std.io.putfln("and my_obj.get() = $1", my_obj.get());
  13> @@
* running 'snippet #5'...
my_obj.value = 1
my_obj.get() = 1
after set(), my_obj.value = 42
and my_obj.get() = 42
* result #5: void
```

The argument for `this` is determined by the expression before the function
call operator `()`, and it is always passed by reference. Although `this`
references mostly objects, when a call to a function within an array is made,
`this` will reference the enclosing array. For a non-member function call,
`this` is uninitialized, and any attempt to reference `this` will cause an
error.

[back to table of contents](#table-of-contents)

## Ordering of Values

Values can be compared and form a _partial order_. Ordering of two values is
specified in the following table, where the first column denotes the type of
the first operand, and the first row denotes the type of the second operand.
Blank cells indicate that the operands are always unordered; types not listed
in this table (function, opaque and object) are unordered with everything,
including themselves:

|             |   null    | boolean | integer |  real   | string |  array  |
|:-----------:|:---------:|:-------:|:-------:|:-------:|:------:|:-------:|
|  **null**   | identical |         |         |         |        |         |
| **boolean** |           |  total  |         |         |        |         |
| **integer** |           |         |  total  | partial |        |         |
|  **real**   |           |         | partial | partial |        |         |
| **string**  |           |         |         |         | total  |         |
|  **array**  |           |         |         |         |        | partial |

Real numbers may be unordered due to special values called not-a-number (NaN)
which can be produced by an invalid arithmetic operation, such as `0.0 / 0.0`
or `__sqrt -1`. A NaN does not equal anything, even with itself.

There are eight operators about comparison, categorized as two ranks: the
_comparison operators_, `<`, `<=`, `>`, `>=`; and the _equality operators_,
`==`, `!=`, `<=>`, `</>`. Their results are:

|           | if less | if equal | if greater |    if unordered     |
|:---------:|:-------:|:--------:|:----------:|:-------------------:|
|  **`<`**  | `true`  | `false`  |  `false`   | throws an exception |
| **`<=`**  | `true`  |  `true`  |  `false`   | throws an exception |
|  **`>`**  | `false` | `false`  |   `true`   | throws an exception |
| **`>=`**  | `false` |  `true`  |   `true`   | throws an exception |
| **`==`**  | `false` |  `true`  |  `false`   |       `false`       |
| **`!=`**  | `true`  | `false`  |   `true`   |       `true`        |
| **`<=>`** |  `-1`   |   `0`    |    `+1`    |   `"[unordered]"`   |
| **`</>`** | `false` | `false`  |  `false`   |       `true`        |

Comparison operators have higher precedence than equality operators, so `a <
b == c < d` is interpreted as `(a < b) == (c < d)`, instead of `((a < b) ==
c) < d`.

[back to table of contents](#table-of-contents)

## Structured Bindings

Sometimes it makes sense for a function to return multiple values, such as
`std.numeric.frexp()`. Although it is impossible to have multiple expressions
in a _return statement_, it is fairly possible to return an array or object
of multiple values. Here we take `std.numeric.frexp()` as an example, which
decomposes a real number into fractional and exponential parts, such as

```
#1:1> std.numeric.frexp(6.375)
* running 'expression #1'...
* result #1: array(2) [
  0 = real 0.796875;
  1 = integer 3;
];
```

This gives the equation `6.375 = 0.796875 × 2³`. As a good habit, we may wish
to have names for its two results, so we can do

```
#2:1> :heredoc @@
* the next snippet will be terminated by `@@`

#3:1> var [ frac, exp ] = std.numeric.frexp(6.375);
   2> std.io.putfln("fraction = $1", frac);
   3> std.io.putfln("exponent = $1", exp);
   4> @@
* running 'snippet #3'...
fraction = 0.796875
exponent = 3
* result #3: void
```

The `var [ ... ] = ...` syntax is called a _structured binding for arrays_.
The declared variables at the first ellipsis are initialized by the elements
of the initializer at the second ellipsis in ascending order. The initializer
must yield an array or `null`. Variables that correspond to no elements are
initialized to `null`.

Similarly, the `var { ... } = ...` syntax is called a _structure binding for
objects_. The declared variables at the first ellipsis are initialized by the
elements of the initializer at the second ellipsis according to their names.
The initializer must yield an object or `null`. Variables that correspond to
no elements are initialized to `null`.

[back to table of contents](#table-of-contents)

## Arguments and Results by Reference

Some people consider passing by reference to be a source of issues about
maintainability, and in addition, in many languages there is absolutely no
way to pass variables or scalar values (boolean values, integers or strings
for example) by reference.

We deem passing by reference an important feature; however the uniform syntax
about arguments, by reference and those by value, causes a huge amount of
confusion. Therefore, references have to passed with special syntax.

Let's take the `exchange(slot, new_value)` function as an example. It writes
`new_value` to `slot` and returns its old value. It is implemented as

```
#3:1> :heredoc @@
* the next snippet will be terminated by `@@`

#4:1> func exchange(slot, new_value) {
   2>   var old_value = slot;
   3>   slot = new_value;
   4>   return old_value;
   5> }
   6> 
   7> var foo = 42;
   8> var old_foo = exchange(ref foo, "meow");
   9> std.io.putfln("`foo` has been changed from $1 to $2", old_foo, foo);
  10> @@
* running 'snippet #4'...
`foo` has been changed from 42 to meow
* result #4: void
```

Likewise, a function can return a result by reference with `return ref ...`.
These `ref` keywords may also be written equivalently as arrow specifiers
`->`. They are parts of the function call expressions and return statements,
and shall not be specified arbitrarily elsewhere.

[back to table of contents](#table-of-contents)

## Throwing and Catching Exceptions

Exceptions are used extensively in the runtime and the standard library for
error handling, as in

```
#1:1> 1 / 0
* running 'expression #1'...
! exception: runtime error: Zero as divisor (operands were `1` and `0`)
[backtrace frames:
  1) native code at '[unknown]:-1:-1': "Zero as divisor (operands were `1` and `0`)"
  2)   expression at 'expression #1:1:3': ""
  3)   function at 'expression #1:0:0': "[file scope]"
  -- end of backtrace frames]

#2:1> std.numeric.parse("not a valid number")
* running 'expression #2'...
! exception: runtime error: String not convertible to a number (text was `not a valid number`)
[thrown from `std_numeric_parse(...)` at 'asteria/library/numeric.cpp:500']
[exception class `St13runtime_error`]
[backtrace frames:
  1) native code at '[unknown]:-1:-1': "String not convertible to a number (text was `not a valid number`)\n[thrown from `s ... (102 characters omitted)
  2)   expression at 'expression #2:1:18': "[proper tail call]"
  3)   function at 'expression #2:0:0': "[file scope]"
  -- end of backtrace frames]
```

Throwing an exception transfers execution to the nearest enclosing `catch`.
There are two forms of `catch`. One is the well-known _try-catch statement_
like in JavaScript. We do

```
#5:1> :heredoc @@
* the next snippet will be terminated by `@@`

#6:1> var i = "unknown";
   2> try {
   3>   std.io.putln("try");
   4>   i = 1 / 0;
   5>   std.io.putln("should never arrive here");
   6> } 
   7> catch(e) {
   8>   std.io.putfln(
   9>     "caught exception: $1\nwith backtrace:\n$2",
  10>     e, std.json.format(__backtrace, 2));
  11> }
  12> std.io.putfln("after try, `i` is $1", i);
  13> @@
* running 'snippet #6'...
try
caught exception: Zero as divisor (operands were `1` and `0`)
with backtrace:
[
  {
    "file": "[unknown]",
    "column": -1,
    "frame": "native code",
    "line": -1,
    "value": "Zero as divisor (operands were `1` and `0`)"
  },
  {
    "file": "snippet #6",
    "column": 9,
    "frame": "  expression",
    "line": 4,
    "value": ""
  },
  {
    "file": "snippet #6",
    "column": 1,
    "frame": "  try clause",
    "line": 2,
    "value": "Zero as divisor (operands were `1` and `0`)"
  }
]
after try, `i` is unknown
* result #6: void
```

The `{` and `}` are not required when there is only a single statement that
follows `try` or `catch`.

The other form is that `catch` is also an operator. `catch` evaluates its
operand, and if an exception is thrown during evaluation, it evaluates to a
copy of the exception, otherwise it evaluates to `null`, as in

```
#7:1> :heredoc @@
* the next snippet will be terminated by `@@`

#8:1> var i = "unknown";
   2> var e = catch(i = 1 / 0);
   3> std.io.putfln("caught exception: $1", e);
   4> std.io.putfln("after catch, `i` is $1", i);
   5> @@
* running 'snippet #8'...
caught exception: Zero as divisor (operands were `1` and `0`)
after catch, `i` is unknown
* result #8: void
```

In this example `1 / 0` throws an exception, so the assignment operator isn't
evaluated, leaving the value of `i` intact.

[back to table of contents](#table-of-contents)

## Integer Overflows

The integer type is 64-bit and signed, capable of representing values from
`-9223372036854775808` to `+9223372036854775807`. The elementary arithmetic
operators on integers also produce integer results. If such a result cannot
be represented as an integer, an exception is thrown, as in

```
#1:1> 100 / 0
* running 'expression #1'...
! exception: runtime error: Zero as divisor (operands were `100` and `0`)
[backtrace frames:
  1) native code at '[unknown]:-1:-1': "Zero as divisor (operands were `100` and `0`)"
  2)   expression at 'expression #1:1:5': ""
  3)   function at 'expression #1:0:0': "[file scope]"
  -- end of backtrace frames]

#2:1> 1000000000000 * 1000000000000
* running 'expression #2'...
! exception: runtime error: Integer multiplication overflow (operands were `1000000000000` and `1000000000000`)
[backtrace frames:
  1) native code at '[unknown]:-1:-1': "Integer multiplication overflow (operands were `1000000000000` and `1000000000000`)"
  2)   expression at 'expression #2:1:15': ""
  3)   function at 'expression #2:0:0': "[file scope]"
  -- end of backtrace frames]
```

In some other programming languages, there can be two kinds of right-shift
operators: `>>` aka. the 'arithmetic right-shift operator', which duplicates
the sign bit in the left; and `>>>` aka. the 'logical right-shift operator',
which fills zeroes instead. There is usually only one left-shift operator,
`<<`, which always fills zeroes in the right.

Given the fact that integer overflows shall not go unnoticed, we have four
shift operators: `<<` and `>>` aka. the 'arithmetic shift operators', which
treat their operands as 64-bit signed integers; and `<<<` and `>>>` aka. the
'logical shift operators', which treat their operands as series of bits:

```
#3:1> 1 << 63
* running 'expression #3'...
! exception: runtime error: Arithmetic left shift overflow (operands were `1` and `63`)
[backtrace frames:
  1) native code at '[unknown]:-1:-1': "Arithmetic left shift overflow (operands were `1` and `63`)"
  2)   expression at 'expression #3:1:3': ""
  3)   function at 'expression #3:0:0': "[file scope]"
  -- end of backtrace frames]

#4:1> 1 <<< 63
* running 'expression #4'...
* result #4: integer -9223372036854775808;

#5:1> 5 << 62
* running 'expression #5'...
! exception: runtime error: Arithmetic left shift overflow (operands were `5` and `62`)
[backtrace frames:
  1) native code at '[unknown]:-1:-1': "Arithmetic left shift overflow (operands were `5` and `62`)"
  2)   expression at 'expression #5:1:3': ""
  3)   function at 'expression #5:0:0': "[file scope]"
  -- end of backtrace frames]

#6:1> 5 <<< 62
* running 'expression #6'...
* result #6: integer 4611686018427387904;

#7:1> -1 << 63
* running 'expression #7'...
* result #7: integer -9223372036854775808;

#8:1> -3 << 63
* running 'expression #8'...
! exception: runtime error: Arithmetic left shift overflow (operands were `-3` and `63`)
[backtrace frames:
  1) native code at '[unknown]:-1:-1': "Arithmetic left shift overflow (operands were `-3` and `63`)"
  2)   expression at 'expression #8:1:4': ""
  3)   function at 'expression #8:0:0': "[file scope]"
  -- end of backtrace frames]

#9:1> -3 <<< 63
* running 'expression #9'...
* result #9: integer -9223372036854775808;
```

There are additional addition, subtraction and multiplication operators that
do not effect exceptions on overflows:

1. `__addm`, `__subm` and `__mulm` are called the _modular arithmetic
   operators_. They perform calculations with infinite precision, and then
   truncate ('wrap') their results to 64 bits, producing the same values as
   reinterpreting the results of their corresponding unsigned operations in
   two's complement.

2. `__adds`, `__subs` and `__muls` are called the _saturating arithmetic
   operators_. If their results are greater than the maximum integer value,
   they produce the maximum integer value; and if their results are less than
   the minimum integer value, they produce the minimum integer value.

These are not infix operators, but look like function calls, as in

```
#10:1> __addm(2, 9223372036854775807)
* running 'expression #10'...
* result #10: integer -9223372036854775807;

#11:1> __subm(2, -9223372036854775808)
* running 'expression #11'...
* result #11: integer -9223372036854775806;

#12:1> __mulm(2, 9223372036854775807)
* running 'expression #12'...
* result #12: integer -2;

#13:1> __adds(2, 9223372036854775807)
* running 'expression #13'...
* result #13: integer 9223372036854775807;

#14:1> __subs(2, -9223372036854775808)
* running 'expression #14'...
* result #14: integer 9223372036854775807;

#15:1> __muls(2, 9223372036854775807)
* running 'expression #15'...
* result #15: integer 9223372036854775807;
```

[back to table of contents](#table-of-contents)

## Bit-wise Operators on Strings

The shift and bitwise operators also apply to strings, where they perform
byte-wise operations.

Shifting a string to the left has the effect to fill zero bytes in the right,
and shifting a string to the right has the effect to remove characters from
the right:

```
#1:1> "12345678" >> 1
* running 'expression #1'...
* result #1: string(7) "1234567";

#2:1> "12345678" << 1
* running 'expression #2'...
* result #2: string(9) "12345678\0";
```

Logical shift operators `>>>` and `<<<` also fill zero bytes in the other end
so the length of their operand is unchanged:

```
#3:1> "12345678" <<< 1
* running 'expression #3'...
* result #3: string(8) "2345678\0";

#4:1> "12345678" >>> 1
* running 'expression #4'...
* result #4: string(8) "\01234567";
```

The bit-wise AND operator `&`, the bit-wise OR operator `|`, and the bit-wise
XOR operator `^` are intuitively defined on two strings of the same length.
If one string is longer than the other, its longer part corresponds to the
'missing information' of the other. The bit-wise AND operator truncates the
longer string, and produces a string of the same length as the shorter one.
On the other hand, the bit-wise OR and XOR operators treat 'missing
information' as zeroes (which means to copy from the other) and produce a
string of the same length as the longer one:

```
#6:1> "1234" & "12345678"
* running 'expression #6'...
* result #6: string(4) "1234";

#7:1> "2345" & "1234"
* running 'expression #7'...
* result #7: string(4) "0204";

#8:1> "1234" | "12345678"
* running 'expression #8'...
* result #8: string(8) "12345678";

#9:1> "23450000" | "12345678"
* running 'expression #9'...
* result #9: string(8) "33755678";

#10:1> "1234" ^ "12345678"
* running 'expression #10'...
* result #10: string(8) "\0\0\0\05678";

#11:1> "2345" ^ "1234"
* running 'expression #11'...
* result #11: string(4) "\x03\x01\a\x01";

#12:1> "2345" ^ "12345678"
* running 'expression #12'...
* result #12: string(8) "\x03\x01\a\x015678";
```

[back to table of contents](#table-of-contents)

## Calling C++ Functions from Asteria

Functions are reference-counting pointers, and can be passed like any other
values. The simplest but still powerful way to create a function value is the
`ASTERIA_BINDING()` macro, which is defined as

```c++
ASTERIA_BINDING(
  "display-name",            // display name, when converted to a string
  "parameter-list",          // parameter list, when converted to a string
  Global_Context& global,    // (optional) global context from caller
  Reference&& self,          // (optional) `this` argument from caller
  Argument_Reader&& reader)  // arguments
{
  // function body
}
```

Like in C++, it is generally not recommended to define functions directly in
the global namespace. Instead, we define functions in custom namespaces. This
example defines a global object `my`, which contains two member `add` and
`sub`, which can be referenced as `my.add` and `my.sub`.

```c++
#include <asteria/simple_script.hpp>
#include <asteria/runtime/binding_generator.hpp>
#include <asteria/runtime/argument_reader.hpp>
#include <asteria/runtime/garbage_collector.hpp>
#include <asteria/runtime/variable.hpp>
using namespace rocket;
using namespace asteria;

int
main(void)
  {
    V_object my;

    // `my.add(x, y)`
    my.try_emplace(&"add",
      ASTERIA_BINDING("my.add", "x, y",
        Argument_Reader&& reader)
        -> Value
      {
        // Try overload `integer, integer`.
        V_integer ix, iy;
        reader.start_overload();
        reader.required(ix);
        reader.required(iy);
        if(reader.end_overload())
          return ix + iy;

        // Try overload `real, real`.
        V_real fx, fy;
        reader.start_overload();
        reader.required(fx);
        reader.required(fy);
        if(reader.end_overload())
          return fx + fy;

        // Try overload `string, string`.
        V_string sx, sy;
        reader.start_overload();
        reader.required(sx);
        reader.required(sy);
        if(reader.end_overload())
          return sx + sy;

        // Fail.
        reader.throw_no_matching_function_call();
      });

    // `my.sub(x, y)`
    my.try_emplace(&"sub",
      ASTERIA_BINDING("my.sub", "x, y",
        Argument_Reader&& reader)
        -> Value
      {
        // Try overload `integer, integer`.
        V_integer ix, iy;
        reader.start_overload();
        reader.required(ix);
        reader.required(iy);
        if(reader.end_overload())
          return ix - iy;

        // Try overload `real, real`.
        V_real fx, fy;
        reader.start_overload();
        reader.required(fx);
        reader.required(fy);
        if(reader.end_overload())
          return fx - fy;

        // Fail.
        reader.throw_no_matching_function_call();
      });

    // Create a script and install `my` into its global context as
    // an immutable variable.
    Simple_Script script;
    auto v = script.mut_global().garbage_collector()->create_variable();
    v->initialize(my);
    v->set_immutable(true);
    script.mut_global().insert_named_reference(&"my").set_variable(v);

    // Load a script which calls those functions. If there is a parser
    // error, an exception is thrown.
    script.reload_string(
      &"script name",
      &R"##(
        std.io.putfln('my.add(123, 654) = $1', my.add(123, 654));
        std.io.putfln('my.add("a", "b") = $1', my.add("a", "b"));
        std.io.putfln('my.sub(123, 654) = $1', my.sub(123, 654));
      )##");

    // Execute the script. The result is ignored.
    script.execute();
  }
```

This gives

```text
$ g++ -std=c++17 -W{all,extra,pedantic} test.cpp -lasteria
$ ./a.out
my.add(123, 654) = 777
my.add("a", "b") = ab
my.sub(123, 654) = -531
```

[back to table of contents](#table-of-contents)

## Calling Asteria Functions from C++

Calling an Asteria function from C++ is a bit complex. First, we pass a
function value to C++, either via a static variable or a return value. Then,
we define a special `Reference` which passes the `this` reference into the
function and passes the result from the function. Then, we push positional
arguments from left to right onto a `Reference_Stack`. Finally, we invoke the
function.

```c++
#include <asteria/simple_script.hpp>
#include <asteria/runtime/binding_generator.hpp>
#include <asteria/runtime/argument_reader.hpp>
using namespace rocket;
using namespace asteria;

int
main(void)
  {
    static V_function my_add_and_print;

    // Create a script and install a helper function to its global context,
    // which is used to set `my_add_and_print`.
    Simple_Script script;
    script.mut_global()
      .insert_named_reference(&"set_my_add_and_print")
      .set_temporary(
        ASTERIA_BINDING("my_add_and_print", "fn",
          Argument_Reader&& reader)
          -> void
        {
          // Try overload `function`.
          V_function fn;
          reader.start_overload();
          reader.required(fn);
          if(reader.end_overload()) {
            my_add_and_print = fn;
            return;
          }

          // Fail.
          reader.throw_no_matching_function_call();
        });

    // Load a script which defines the actual function and sets it into
    // `my_add_and_print` for later use.
    script.reload_string(
      &"script name",
      &R"##(
        // define the actual function.
        func add_and_print(x, y) {
          var sum = x + y;
          std.io.putfln("add_and_print($1, $2) = $3", x, y, sum);
          return sum;
        }

        // set it.
        set_my_add_and_print(add_and_print);
      )##");

    // Execute the script. The result is ignored.
    script.execute();

    // Now `my_add_and_print` points to a function. Use it.
    Reference result;
    Reference_Stack args;
    args.push().set_temporary(123);
    args.push().set_temporary(678);
    my_add_and_print.invoke(result, script.mut_global(), move(args));
    long res1 = result.dereference_readonly().as_integer();
    ::printf("my_add_and_print(%d, %d) = %ld\n",
             123, 678, res1);

    result.clear();
    args.clear();
    args.push().set_temporary(&"hello ");
    args.push().set_temporary(&"world!");
    my_add_and_print.invoke(result, script.mut_global(), move(args));
    cow_string res2 = result.dereference_readonly().as_string();
    ::printf("my_add_and_print(%s, %s) = %s\n",
             "hello ", "world!", res2.c_str());
  }
```

This gives

```text
$ g++ -std=c++17 -W{all,extra,pedantic} test.cpp -lasteria
$ ./a.out
add_and_print(123, 678) = 801
add_and_print(hello , world!) = hello world!
```

[back to table of contents](#table-of-contents)
