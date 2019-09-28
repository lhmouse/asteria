// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "tinyfmt.hpp"

namespace rocket {

    namespace {

    class xappender
      {
      private:
        char* m_str;
        long m_off;

      public:
        explicit constexpr xappender(char* str) noexcept
          : m_str(str), m_off(0)
          {
          }

        xappender(const xappender&)
          = delete;
        xappender& operator=(const xappender&)
          = delete;

      public:
        constexpr operator long () const noexcept
          {
            return this->m_off;
          }
        xappender& operator<<(char c) noexcept
          {
            return this->m_str[this->m_off++] = c, *this;
          }
      };

    }

    namespace details_tinyfmt {

    long xformat_Fb(char* str, bool value) noexcept
      {
        xappender xapp(str);
        // Write `"true"` for TRUE and `"false"` for FALSE.
        if(value)
          xapp << 't' << 'r' << 'u' << 'e';
        else
          xapp << 'f' << 'a' << 'l' << 's' << 'e';
        return xapp;
      }

    long xformat_Zp(char* str, const void* value) noexcept
      {
        xappender xapp(str);
        // Write a `0x` prefix.
        xapp << '0' << 'x';
        // Write the pointer as a hexadecimal string;
        auto reg = reinterpret_cast<uintptr_t>(value);
        int shr = numeric_limits<uintptr_t>::digits - 4;
        while(shr >= 0)
          xapp << "0123456789ABCDEF"[(reg >> shr) % 16], shr -= 4;
        return xapp;
      }

    long xformat_Di(char* str, signed value) noexcept
      {
        xappender xapp(str);
        // Write the integer in decimal.
        // Prepend a minus sign if the number is negative.
        auto reg = static_cast<unsigned>(value);
        if(value < 0)
          xapp << '-', reg *= -1;
        long bp = xapp;
        while(reg != 0)
          xapp << "0123456789"[reg % 10], reg /= 10;
        long ep = xapp;
        // Reverse the string if there are any digits.
        // Ensure there is an explicit zero otherwise.
        if(bp != ep)
          while(bp != --ep)
            ::std::swap(str[bp++], str[ep]);
        else
          xapp << '0';
        return xapp;
      }

    long xformat_Du(char* str, unsigned value) noexcept
      {
        xappender xapp(str);
        // Write the integer in decimal.
        // Prepend a minus sign if the number is negative.
        auto reg = static_cast<unsigned>(value);
        long bp = xapp;
        while(reg != 0)
          xapp << "0123456789"[reg % 10], reg /= 10;
        long ep = xapp;
        // Reverse the string if there are any digits.
        // Ensure there is an explicit zero otherwise.
        if(bp != ep)
          while(bp != --ep)
            ::std::swap(str[bp++], str[ep]);
        else
          xapp << '0';
        return xapp;
      }

    long xformat_Qi(char* str, signed long long value) noexcept
      {
        xappender xapp(str);
        // Write the integer in decimal.
        // Prepend a minus sign if the number is negative.
        auto reg = static_cast<unsigned long long>(value);
        if(value < 0)
          xapp << '-', reg *= -1;
        long bp = xapp;
        while(reg != 0)
          xapp << "0123456789"[reg % 10], reg /= 10;
        long ep = xapp;
        // Reverse the string if there are any digits.
        // Ensure there is an explicit zero otherwise.
        if(bp != ep)
          while(bp != --ep)
            ::std::swap(str[bp++], str[ep]);
        else
          xapp << '0';
        return xapp;
      }

    long xformat_Qu(char* str, unsigned long long value) noexcept
      {
        xappender xapp(str);
        // Write the integer in decimal.
        // Prepend a minus sign if the number is negative.
        auto reg = static_cast<unsigned long long>(value);
        long bp = xapp;
        while(reg != 0)
          xapp << "0123456789"[reg % 10], reg /= 10;
        long ep = xapp;
        // Reverse the string if there are any digits.
        // Ensure there is an explicit zero otherwise.
        if(bp != ep)
          while(bp != --ep)
            ::std::swap(str[bp++], str[ep]);
        else
          xapp << '0';
        return xapp;
      }

    long xformat_Qf(char* str, double value) noexcept
      {
return std::sprintf(str, "%f", value);
      }

    long xformat_Tf(char* str, long double value) noexcept
      {
return std::sprintf(str, "%Lf", value);
      }

    }

template class basic_tinyfmt<char>;
template class basic_tinyfmt<wchar_t>;

}  // namespace rocket
