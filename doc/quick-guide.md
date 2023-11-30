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

## Course 101

In the first class of every programming language, we are taught to output the
magic string `"hello world!"`. This can be done by calling the `putln()`
function in `std.io`:

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
of a single snippet. In this example we use `@@`, so our program looks like:

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
and checks whether it is correct:

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

## Lambdas

The most common scenario where lambdas are useful is probably sorting. For
example, this sorts an array of integers in ascending order:

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
a look at the prototype of `std.array.sort`:

```
#2:1> std.array.sort
* running 'expression #2'...
* result #2: function [[`std.array.sort(data, [comparator])` at 'asteria/library/array.cpp:1058']];
```

So it can take an optional `comparator` function. Every standard function
which takes a `comparator` function has the same requirement on it, which is

1. It takes two arguments, namely `x` and `y`.
2. If `x` is less than `y`, `comparator(x, y)` returns a negative number.
3. If `x` is greater than `y`, `comparator(x, y)` returns a positive number.
4. If `x` is equivalent to `y`, `comparator(x, y)` returns zero.

Other results usually indicate that `x` and `y` are unordered, and are likely
to cause exceptions to be thrown. This specification matches the built-in
'spaceship' operator `<=>`. Therefore, in order to sort an array in reverse
order, we need to define a lambda, i.e. an anonymous function, that returns
the opposite of the default comparison result. There are actually two ways to
do this, either `func(x, y) { return -(x <=> y); }` or `func(x, y) { return y
<=> x; }`. We use the second form, and it looks like

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

## Exceptions and Error Handling

Exceptions are used extensively in the runtime and standard library for error
handling, as in

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
! exception: runtime error: String not convertible to a number (text `not a valid number`)
[thrown from `std_numeric_parse(...)` at 'asteria/library/numeric.cpp:500']
[exception class `St13runtime_error`]
[backtrace frames:
  1) native code at '[unknown]:-1:-1': "String not convertible to a number (text `not a valid number`)\n[thrown from `s ... (102 characters omitted)
  2)   expression at 'expression #2:1:18': "[proper tail call]"
  3)   function at 'expression #2:0:0': "[file scope]"
  -- end of backtrace frames]
```

Throwing an exception transfers execution to the nearest enclosing `catch`.
There are two forms of `catch`. One is the well-known _try-catch statement_:

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
   8>   std.io.putfln("caught exception: $1", e);
   9> } 
  10> std.io.putfln("after try, `i` is $1", i);
  11> @@
* running 'snippet #6'...
try
caught exception: Zero as divisor (operands were `1` and `0`)
after try, `i` is unknown
* result #6: void
```

This looks like JavaScript, except that the `{` and `}` are not required when
there is only a single statement that follows `try` or `catch`.

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

## Object-oriented Programming and the `this` Parameter

In many other programming languages, there is a concept about (non-static)
member functions. A member function receives an implicit `this` parameter
that references the object which is 'intuitively' used to call the function.

Our definition of the argument for `this` is based on this 'intuitive' idea:
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