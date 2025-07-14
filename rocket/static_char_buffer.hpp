// This file is part of Asteria.
// Copyright (C) 2024-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_STATIC_CHAR_BUFFER_
#define ROCKET_STATIC_CHAR_BUFFER_

#include "fwd.hpp"
#include "xthrow.hpp"
namespace rocket {

template<size_t capacityT>
class static_char_buffer;

template<size_t capacityT>
class static_char_buffer
  {
  private:
    union {
      char m_init[1];
      char m_data[capacityT];
    };

  public:
    constexpr
    static_char_buffer()
      noexcept
      : m_init()
      { }

    static_char_buffer(const char* str)
      {
        this->assign(str);
      }

    static_char_buffer(const static_char_buffer& other)
      noexcept
      {
        ::strcpy(this->m_data, other.m_data);
      }

    static_char_buffer&
    operator=(const char* str)
      &
      {
        this->assign(str);
        return *this;
      }

    static_char_buffer&
    operator=(const static_char_buffer& other)
      & noexcept
      {
        ::strcpy(this->m_data, other.m_data);
        return *this;
      }

  public:
    constexpr operator
    const char*()
      const noexcept
      { return this->m_data;  }

    const char*
    c_str()
      const noexcept
      { return this->m_data;  }

    const char*
    data()
      const noexcept
      { return this->m_data;  }

    char*
    mut_data()
      noexcept
      { return this->m_data;  }

    void
    assign(const char* str)
      {
        size_t len = ::strlen(str);
        if(len + 1 >= capacityT)
          noadl::sprintf_and_throw<length_error>(
                   "static_char_buffer<%d>: string `%s` too long",
                   static_cast<int>(capacityT), str);

        // Do we allow overlapped buffers?
        ::memmove(this->m_data, str, len + 1);
      }
  };

template<size_t M, size_t N>
constexpr
bool
operator==(const static_char_buffer<M>& lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs.c_str()) == 0;  }

template<size_t M>
constexpr
bool
operator==(const static_char_buffer<M>& lhs, const char* rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs) == 0;  }

template<size_t N>
constexpr
bool
operator==(const char* lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs, rhs.c_str()) == 0;  }

template<size_t M, size_t N>
constexpr
bool
operator!=(const static_char_buffer<M>& lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs.c_str()) != 0;  }

template<size_t M>
constexpr
bool
operator!=(const static_char_buffer<M>& lhs, const char* rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs) != 0;  }

template<size_t N>
constexpr
bool
operator!=(const char* lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs, rhs.c_str()) != 0;  }

template<size_t M, size_t N>
constexpr
bool
operator<(const static_char_buffer<M>& lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs.c_str()) < 0;  }

template<size_t M>
constexpr
bool
operator<(const static_char_buffer<M>& lhs, const char* rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs) < 0;  }

template<size_t N>
constexpr
bool
operator<(const char* lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs, rhs.c_str()) < 0;  }

template<size_t M, size_t N>
constexpr
bool
operator>(const static_char_buffer<M>& lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs.c_str()) > 0;  }

template<size_t M>
constexpr
bool
operator>(const static_char_buffer<M>& lhs, const char* rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs) > 0;  }

template<size_t N>
constexpr
bool
operator>(const char* lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs, rhs.c_str()) > 0;  }

template<size_t M, size_t N>
constexpr
bool
operator<=(const static_char_buffer<M>& lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs.c_str()) <= 0;  }

template<size_t M>
constexpr
bool
operator<=(const static_char_buffer<M>& lhs, const char* rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs) <= 0;  }

template<size_t N>
constexpr
bool
operator<=(const char* lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs, rhs.c_str()) <= 0;  }

template<size_t M, size_t N>
constexpr
bool
operator>=(const static_char_buffer<M>& lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs.c_str()) >= 0;  }

template<size_t M>
constexpr
bool
operator>=(const static_char_buffer<M>& lhs, const char* rhs)
  noexcept
  { return ::strcmp(lhs.c_str(), rhs) >= 0;  }

template<size_t N>
constexpr
bool
operator>=(const char* lhs, const static_char_buffer<N>& rhs)
  noexcept
  { return ::strcmp(lhs, rhs.c_str()) >= 0;  }

}  // namespace rocket
#endif
