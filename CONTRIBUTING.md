# Coding Standard (C++)

### Preamble

This document describes the coding style requirements of **Asteria**.

These rules cover all files in the `src` directory, save for those in `src/rocket`.

### Standard Conformance

1. Code shall conform to _C++14_. _C++17_ or above is permitted with proper checks of [feature test macros](https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations).

2. These specifications are mandated and can be assumed to be always true:

    1. `char` has _exactly_ 8 bits; `int` has _at least_ 32 bits.
    2. All integral types are represented in [two's complement](https://en.wikipedia.org/wiki/Two%27s_complement); `int{8,16,32,64,ptr}_t` and their unsigned counterparts are always available.
    3. `ptrdiff_t` and `size_t` only differ in signness.
    4. `float` and `double` conform to the _single precision_ and _double precision_ formats in [IEEE 754-1985](https://en.wikipedia.org/wiki/IEEE_754-1985).

3. `_Names_that_begin_with_an_underscore_followed_by_an_uppercase_letter` and `__those_contain_consecutive_underscores` are reserved by the implementation and shall not be used.

### Line Length Limit

1. A full-width character counts as two half-width ones.

2. The _suggested_ maximum number of half-width characters in a line is **100** characters. This means that you should keep you lines shorter than this where appropriate.

3. The maximum number of half-width characters in a line is **124** characters, so the total number of characters including line numbers will be a bit below **132**, which is about half the width of a 1920x1080 screen. Lines longer than this shall be split into multiple lines, unless specified otherwise.

4. Function and function template declarations, especially those in public headers, are exempt from this limit. Each such declaration must consist of a single line. The rationale is that, when a call to such function or function template contains a syntax error, GCC only prints one contextual line about the target, which would not include the entire declaration if it straddled multiple lines.

### Usage and Formatting of Namespaces

1. Each header and source file is assigned a _root namespace_. The root namespace may be a nested namespace, an unnamed namespace or the global namespace.

2. The root namespaces is not indented. A nested namespaces is indented relatively to it direct parent namespace by **4 spaces**. A namespace member that is not a namespace is not indented relatively to its enclosing namespace.

3. There shall be one blank line following the initiation of a namespace, as well as preceding the termination of it.

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
    template<typename stringT, typename hashT> class string_storage : private ebo_wrapper<hashT>::type
      {
      public:
        using string_type  = stringT;
        using hasher       = hashT;
    
      private:
        using hasher_base   = typename ebo_wrapper<hasher>::type;
    
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
