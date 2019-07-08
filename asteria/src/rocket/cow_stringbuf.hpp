// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRINGBUF_HPP_
#define ROCKET_COW_STRINGBUF_HPP_

#include "cow_string.hpp"
#include <streambuf>  // std::ios_base, std::basic_streambuf<>, std::streamsize

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>, typename allocatorT = allocator<charT>> class basic_cow_stringbuf;

template<typename charT, typename traitsT, typename allocatorT> class basic_cow_stringbuf : public basic_streambuf<charT, traitsT>
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocatorT;

    using int_type   = typename traits_type::int_type;
    using pos_type   = typename traits_type::pos_type;
    using off_type   = typename traits_type::off_type;

    // N.B. These are non-standard extensions.
    using string_type      = basic_cow_string<char_type, traits_type, allocator_type>;
    using size_type        = typename string_type::size_type;
    using difference_type  = typename string_type::difference_type;

    static constexpr size_type npos = string_type::npos;

  private:
    string_type m_str;
    size_type m_caret;
    ios_base::openmode m_which;

  public:
    explicit basic_cow_stringbuf(string_type str, size_type caret = npos, ios_base::openmode which = ios_base::in | ios_base::out)
      : m_str(noadl::move(str)), m_caret(caret), m_which(which)
      {
      }
    basic_cow_stringbuf()
      : basic_cow_stringbuf(string_type())
      {
      }
    basic_cow_stringbuf(ios_base::openmode which)
      : basic_cow_stringbuf(string_type(), npos, which)
      {
      }
    ~basic_cow_stringbuf() override;

  protected:
    int sync() override
      {
        if(this->gptr() != nullptr) {
          // Empty the get area. If there are any characters read from it, remove them from the internal buffer.
          auto ngot = static_cast<size_type>(this->gptr() - this->eback());
          this->m_str.erase(0, ngot);
          this->setg(nullptr, nullptr, nullptr);
        }
        return basic_streambuf<charT, traitsT>::sync();
      }

    streamsize showmanyc() override
      {
        if(!(this->m_which & ios_base::in)) {
          // Non-input stream buffers can't be read from.
          return 0;
        }
        // Return the number of characters inside the internal buffer, minus those that have been read from it.
        auto ntotal = this->m_str.size();
        // N.B. This will yield the correct result (zero) even when both pointers are null.
        auto ngot = static_cast<size_type>(this->gptr() - this->eback());
        return static_cast<streamsize>(ntotal - ngot);
      }
    streamsize xsgetn(char_type* s, streamsize n) override
      {
        if(!(this->m_which & ios_base::in)) {
          // Non-input stream buffers can't be read from.
          return 0;
        }
        // Tidy the get area.
        this->sync();
        // Read and discard characters from the internal buffer directly.
        auto ngot = (static_cast<unsigned long long>(n) <= this->m_str.size()) ? static_cast<size_type>(n) : this->m_str.size();
        traits_type::copy(s, this->m_str.data(), ngot);
        return static_cast<streamsize>(ngot);
      }
    int_type underflow() override
      {
        if(!(this->m_which & ios_base::in)) {
          // Non-input stream buffers can't be read from.
          return traits_type::eof();
        }
        // Tidy the get area.
        this->sync();
        // Set the get area to the entire string.
        if(this->m_str.empty()) {
          return traits_type::eof();
        }
        auto gp = this->m_str.mut_data();
        this->setg(gp, gp, gp + this->m_str.size());
        return traits_type::to_int_type(*gp);
      }

    int_type pbackfail(int_type c) override
      {
        if(!(this->m_which & ios_base::out)) {
          // Non-output stream buffers can't be written to.
          return traits_type::eof();
        }
        if(traits_type::eq_int_type(c, traits_type::eof())) {
          return traits_type::eof();
        }
        // Tidy the get area, as the internal buffer is subject to reallocation.
        this->sync();
        // Put the character provided back into the front of the internal buffer. This is not meant to be efficient anyway.
        this->m_str.insert(0, 1, traits_type::to_char_type(c));
        return c;
      }

    streamsize xsputn(const char_type* s, streamsize n) override
      {
        if(!(this->m_which & ios_base::out)) {
          // Non-output stream buffers can't be written to.
          return 0;
        }
        // Tidy the get area, as the internal buffer is subject to reallocation.
        this->sync();
        // Write the string provided as a single operation.
        auto nput = (static_cast<unsigned long long>(n) <= this->m_str.max_size()) ? static_cast<size_type>(n) : this->m_str.max_size();
        if(this->m_caret == npos) {
          // Append the string provided to the internal buffer.
          this->m_str.insert(this->m_str.size(), s, nput);
        } else {
          // Insert the string provided at the specified position of the internal buffer.
          this->m_str.insert(this->m_caret, s, nput);
          this->m_caret += nput;
        }
        return static_cast<streamsize>(nput);
      }
    int_type overflow(int_type c) override
      {
        if(!(this->m_which & ios_base::out)) {
          // Non-output stream buffers can't be written to.
          return traits_type::eof();
        }
        if(traits_type::eq_int_type(c, traits_type::eof())) {
          return traits_type::not_eof(c);
        }
        // Tidy the get area, as the internal buffer is subject to reallocation.
        this->sync();
        // Write the character provided as a single operation.
        if(this->m_caret == npos) {
          // Append the character provided to the internal buffer.
          this->m_str.push_back(traits_type::to_char_type(c));
        } else {
          // Insert the character provided at the specified position of the internal buffer.
          this->m_str.insert(this->m_caret, static_cast<size_type>(1), traits_type::to_char_type(c));
          this->m_caret++;
        }
        return c;
      }

  public:
    string_type& get_string()
      {
        this->sync();
        return this->m_str;
      }
    size_type get_caret() const noexcept
      {
        return this->m_caret;
      }
    void set_string(string_type str, size_type caret = npos)
      {
        this->sync();
        this->m_str = noadl::move(str);
        this->m_caret = caret;
      }
    void set_caret(size_type caret) noexcept
      {
        this->m_caret = caret;
      }
    string_type extract_string()
      {
        string_type str;
        this->sync();
        this->m_str.swap(str);
        this->m_caret = npos;
        return str;
      }
  };

#if !(defined(__cpp_inline_variables) && (__cpp_inline_variables >= 201606))
template<typename charT, typename traitsT, typename allocatorT> constexpr typename basic_cow_stringbuf<charT, traitsT, allocatorT>::size_type basic_cow_stringbuf<charT, traitsT, allocatorT>::npos;
#endif

template<typename charT, typename traitsT, typename allocatorT> basic_cow_stringbuf<charT, traitsT, allocatorT>::~basic_cow_stringbuf()
  = default;

extern template class basic_cow_stringbuf<char>;
extern template class basic_cow_stringbuf<wchar_t>;

using cow_stringbuf = basic_cow_stringbuf<char>;
using cow_wstringbuf = basic_cow_stringbuf<wchar_t>;

}  // namespace rocket

#endif
