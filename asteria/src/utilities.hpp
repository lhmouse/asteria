// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILITIES_HPP_
#define ASTERIA_UTILITIES_HPP_

#include "rocket/compatibility.h"
#include "rocket/utilities.hpp"
#include "rocket/insertable_ostream.hpp"
#include "rocket/unique_ptr.hpp"
#include <iomanip>
#include <exception>
#include <cstdint>

namespace Asteria {

///////////////////////////////////////////////////////////////////////////////
// Formatter
///////////////////////////////////////////////////////////////////////////////

class Formatter
  {
  private:
    rocket::unique_ptr<rocket::insertable_ostream> m_strm;

  public:
    Formatter() noexcept
      : m_strm()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Formatter);

  private:
    std::ostream & do_open_stream();
    rocket::cow_string do_extract_string() noexcept;

    template<typename ValueT>
      void do_put(const ValueT &value)
      {
        this->do_open_stream() << value;
      }
    void do_put(bool value)
      {
        this->do_open_stream() << value;
      }
    void do_put(char value)
      {
        this->do_open_stream() << value;
      }
    void do_put(signed char value)
      {
        this->do_open_stream() << static_cast<int>(value);
      }
    void do_put(unsigned char value)
      {
        this->do_open_stream() << static_cast<unsigned>(value);
      }
    void do_put(short value)
      {
        this->do_open_stream() << static_cast<int>(value);
      }
    void do_put(unsigned short value)
      {
        this->do_open_stream() << static_cast<unsigned>(value);
      }
    void do_put(int value)
      {
        this->do_open_stream() << value;
      }
    void do_put(unsigned value)
      {
        this->do_open_stream() << value;
      }
    void do_put(long value)
      {
        this->do_open_stream() << value;
      }
    void do_put(unsigned long value)
      {
        this->do_open_stream() << value;
      }
    void do_put(long long value)
      {
        this->do_open_stream() << value;
      }
    void do_put(unsigned long long value)
      {
        this->do_open_stream() << value;
      }
    void do_put(const char *value)
      {
        this->do_open_stream() << value;
      }
    void do_put(const signed char *value)
      {
        this->do_open_stream() << static_cast<const void *>(value);
      }
    void do_put(const unsigned char *value)
      {
        this->do_open_stream() << static_cast<const void *>(value);
      }
    void do_put(const void *value)
      {
        this->do_open_stream() << value;
      }

  public:
    template<typename ValueT>
      Formatter & operator,(const ValueT &value) noexcept
      try {
        this->do_put(value);
        return *this;
      } catch(...) {
        return *this;
      }
    rocket::cow_string extract_string() noexcept
      {
        return this->do_extract_string();
      }
  };

#define ASTERIA_FORMAT(fmt_, ...)        (static_cast<::Asteria::Formatter &&>(fmt_), __VA_ARGS__)
#define ASTERIA_CREATE_FORMATTER(...)    (::std::move(ASTERIA_FORMAT(::Asteria::Formatter(), __VA_ARGS__)))
#define ASTERIA_FORMAT_STRING(...)       (ASTERIA_CREATE_FORMATTER(__VA_ARGS__).extract_string())

ROCKET_PURE_FUNCTION extern bool are_debug_logs_enabled() noexcept;
extern bool write_log_to_stderr(const char *file, unsigned long line, Formatter &&fmt) noexcept;

#define ASTERIA_DEBUG_LOG(...)           (ROCKET_UNEXPECT(::Asteria::are_debug_logs_enabled()) && ::Asteria::write_log_to_stderr(__FILE__, __LINE__, ASTERIA_CREATE_FORMATTER(__VA_ARGS__)))
#define ASTERIA_TERMINATE(...)           (::Asteria::write_log_to_stderr(__FILE__, __LINE__, ASTERIA_CREATE_FORMATTER("FATAL ERROR: ", __VA_ARGS__)), ::std::terminate())

class Runtime_error : public virtual std::exception
  {
  private:
    rocket::cow_string m_msg;

  public:
    explicit Runtime_error(rocket::cow_string msg) noexcept
      : m_msg(std::move(msg))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Runtime_error);

  public:
    const char * what() const noexcept override;
  };

[[noreturn]] extern void throw_runtime_error(const char *funcsig, Formatter &&fmt);

#define ASTERIA_THROW_RUNTIME_ERROR(...)      (::Asteria::throw_runtime_error(ROCKET_FUNCSIG, ASTERIA_CREATE_FORMATTER(__VA_ARGS__)))

///////////////////////////////////////////////////////////////////////////////
// Indent
///////////////////////////////////////////////////////////////////////////////

class Indent
  {
  private:
    char m_head;
    std::size_t m_count;

  public:
    constexpr Indent(char xhead, std::size_t xcount) noexcept
      : m_head(xhead), m_count(xcount)
      {
      }

  public:
    char head() const noexcept
      {
        return this->m_head;
      }
    std::size_t count() const noexcept
      {
        return this->m_count;
      }
  };

constexpr Indent indent(char head, std::size_t count) noexcept
  {
    return Indent(head, count);
  }

extern std::ostream & operator<<(std::ostream &os, const Indent &n);

///////////////////////////////////////////////////////////////////////////////
// Quote
///////////////////////////////////////////////////////////////////////////////

class Quote
  {
  private:
    const char *m_data;
    std::size_t m_size;

  public:
    constexpr Quote(const char *xdata, std::size_t xsize) noexcept
      : m_data(xdata), m_size(xsize)
      {
      }

  public:
    const char * data() const noexcept
      {
        return this->m_data;
      }
    std::size_t size() const noexcept
      {
        return this->m_size;
      }
  };

constexpr Quote quote(const char *data, std::size_t size) noexcept
  {
    return Quote(data, size);
  }
inline Quote quote(const char *str) noexcept
  {
    return Quote(str, std::char_traits<char>::length(str));
  }
inline Quote quote(const rocket::cow_string &str) noexcept
  {
    return Quote(str.data(), str.size());
  }

extern std::ostream & operator<<(std::ostream &os, const Quote &q);

///////////////////////////////////////////////////////////////////////////////
// Doubly Linked List
///////////////////////////////////////////////////////////////////////////////

template<typename NodeT>
  inline void list_attach(NodeT &pos, NodeT &node) noexcept
  {
    NodeT *const prev = pos.prev;
    NodeT *const next = std::addressof(pos);
    // Set pointers.
    node.prev = prev;
    prev->next = std::addressof(node);
    node.next = next;
    next->prev = std::addressof(node);
  }

template<typename NodeT>
  inline void list_detach(NodeT &node) noexcept
  {
    NodeT *const prev = node.prev;
    NodeT *const next = node.next;
    // Set pointers.
    prev->next = next;
    next->prev = prev;
  }

///////////////////////////////////////////////////////////////////////////////
// Wrappable Index
///////////////////////////////////////////////////////////////////////////////

struct Wrapped_index
  {
    std::uint64_t index;
    std::uint64_t front_fill;
    std::uint64_t back_fill;
  };

extern Wrapped_index wrap_index(std::int64_t index, std::size_t size) noexcept;

}

#endif
