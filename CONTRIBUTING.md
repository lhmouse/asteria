# Coding Standard (C++)

### Preamble

This document describes the coding style requirements of **Asteria**.

These rules cover all files in the `asteria/src` directory, save for those in `asteria/src/rocket`.

### Standard Conformance

1. Code shall conform to _C++11_. _C++14_ or above is permitted with proper checks of [feature test macros](https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations).

2. These specifications are mandated and can be assumed to be always true:

    1. `char` has _exactly_ 8 bits; `int` has _at least_ 32 bits.
    2. All integral types are represented in [two's complement](https://en.wikipedia.org/wiki/Two%27s_complement); `int{8,16,32,64,ptr}_t` and their unsigned counterparts are always available.
    3. `ptrdiff_t` and `size_t` only differ in signness.
    4. `float` and `double` conform to the _single precision_ and _double precision_ formats in [IEEE 754-1985](https://en.wikipedia.org/wiki/IEEE_754-1985).

3. `_Names_that_begin_with_an_underscore_followed_by_an_uppercase_letter` and `__those_contain_consecutive_underscores` are reserved by the implementation and shall not be used.

### Line Length Limit

1. A full-width character counts as two half-width ones.

2. The _suggested_ maximum number of half-width characters in a line is **160** characters. This means that you should keep you lines shorter than this where appropriate.

3. The maximum number of half-width characters in a line is **200** characters. Lines longer than this shall be split into multiple lines, unless otherwise specified.

4. Function and function template declarations, especially those in public headers, are exempt from this limit. Each such declaration must consist of a single line. The rationale is that, when a call to such function or function template contains a syntax error, GCC only prints one contextual line about the target, which would not include the entire declaration if it straddled multiple lines.

### Usage and Formatting of Namespaces

1. Each header and source file is assigned a _root namespace_. The root namespace may be a nested namespace, an unnamed namespace or the global namespace.

2. Root namespaces are not indented. Nested namespaces are indented relatively to their direct parent namespaces by **4 spaces**. Namespace members that are not namespaces are not indented relatively to the enclosing namespaces.

3. There shall be one blank line following the initiation of the root namespace and all other namespaces, as well as preceding the termination of the root namespace and all other namespaces.

4. An example is as follows:

```c++
    namespace Some_Project {
    namespace Some_Component {
    
        namespace Nested {
        
        extern int some_integer;
        
        }
        
    inline int get_some_integer() noexcept
      {
        return Nested::some_integer;
      }
    
    }
    }
```

### Formatting of Function and Function Templates

1. Everything other than the function body (including but not limited to: template parameters, attributes, storage duration and virtual specifiers, the return type, the function name, parameters, exception specifications, the trailing return type, `override` and `final`) shall consist of a single line where possible.

2. If the declaration doesn't contain a function body (i.e. it is not a definition), it is terminated by a `;` within the last line. Specifically, if it declares a pure virtual function, it is terminated by `= 0;` within the last line.

3. When the function prototype needs to be split into multiple lines, the first line must end with a (function or template) parameter or an argument. Consequent lines, beginning with a sibling parameter, argument or closing bracket, shall be aligned with the aforementioned parameter or argument. If a declaration is split multiple times, this rule applies recursively.

4. An example is as follows:

```c++
    template<typename xelementT, typename xdeleterT,
             typename yelementT, typename ydeleterT> inline bool operator==(const unique_ptr<xelementT, xdeleterT> &lhs,
                                                                            const unique_ptr<yelementT, ydeleterT> &rhs) noexcept
      {
        return lhs.get() == rhs.get();
      }
```

5. For a constructor or constructor template, the member initializer list starts at the next line following the prototype, indented by **2 spaces**. A line may contain purely base initializers or non-base member initializers, but not a mixture of both.

6. Function-try-blocks are forbidden on constructors completely.

7. If a function body, a `= default` or `= delete` definition is part of the declaration, it starts at the next line following the prototype, indented by **2 spaces**; or if there is a member initializer list, it starts at the next line following the member initializer list, indented to the same level with the `:` starting the initializer list. The `{` that initiates the function definition and the `}` that terminates the function definition shall not appear on the same line, and shall be indented to the same level.

8. Function-try-blocks for non-constructors are indented as plain blocks.

9. An example is as follows:

```c++
    template<typename stringT, typename hashT> class string_storage : private allocator_wrapper_base_for<hashT>::type
      {
      public:
        using string_type  = stringT;
        using hasher       = hashT;
    
      private:
        using hasher_base   = typename allocator_wrapper_base_for<hasher>::type;
    
      private:
        string_type m_str;
        size_t m_hval;
    
      public:
        template<typename ...paramsT> explicit string_storage(const hasher &hf, paramsT &&...params)
          : hasher_base(hf),
            m_str(::std::forward<paramsT>(params)...), m_hval(this->as_hasher()(this->m_str))
          {
          }
    
        string_storage(const string_storage &)
          = delete;
        string_storage & operator=(const string_storage &)
          = delete;
    
      public:
        const hasher & as_hasher() const noexcept
          {
            return static_cast<const hasher_base &>(*this);
          }
        hasher & as_hasher() noexcept
          {
            return static_cast<hasher_base &>(*this);
          }
    
        const string_type & str() const noexcept
          {
            return this->m_str;
          }
        size_t hval() const noexcept
          {
            return this->m_hval;
          }
      };
```
