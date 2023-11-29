# Asteria Quick Guide

For this quick guide, we are using the REPL for demonstration. I hope you
will enjoy it.

Asteria is highly inspired by JavaScript and shares a similar syntax, as well
as other scripting languages. However there are some fundamental differences:

1. Arrays and objects/tables/dictionaries are passed by value, rather than by
   reference. Whenever the assignment operator `=` occurs, it makes a deep
   copy of a value.

2. There is a 64-bit integer type. The conversion from integers to real
   numbers is implicit, but there is no implicit conversion from real numbers
   to integers. Instead, it is required to use one of `__itrunc`, `__ifloor`,
   `__iceil` or `__iround` operators to cast them, with an explicit rounding
   direction.

3. Function parameters are all references. Whether to pass an argument by
   value or by reference is specified by the argument as part of the function
   call syntax. For example, `foo(arg1)` passes `arg1` by value, while
   `foo(ref arg1)` passes `arg1` by reference. If an argument is passed by
   value, its corresponding parameter will reference a temporary value;
   attempting to modify temporary values will effect runtime errors. This
   applies similarly to function results.

## 101

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
of a single snippet. In this example we use `end_of_my_code`, so our program
looks like:

```
#2:1> :heredoc end_of_my_code
* the next snippet will be terminated by `end_of_my_code`

#3:1> var sum = 0;
   2> 
   3> for(var i = 1;  i <= 100;  ++i)
   4>   sum += i;
   5>   
   6> std.io.putfln("sum = $1", sum);
   7> 
   8> end_of_my_code
* running 'snippet #3'...
sum = 5050
* result #3: void
```

The last example in this section is a classical 'guess my number' game. Our
program generates a random number from 1 to 100, which we will have to guess.
Our program uses `getln()` to read our input, then converts it to an integer,
and checks whether it is correct:

```
#4:1> :heredoc end_of_my_code
* the next snippet will be terminated by `end_of_my_code`

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
  23> 
  24> end_of_my_code
* running 'snippet #2'...
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
* result #2: void
```
