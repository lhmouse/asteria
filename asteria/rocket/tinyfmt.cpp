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
        constexpr operator char* () const noexcept
          {
            return this->m_str + this->m_off;
          }
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
        int shr = numeric_limits<uintptr_t>::digits;
        while((shr -= 4) >= 0)
          xapp << "0123456789ABCDEF"[(reg >> shr) & 0xF];
        return xapp;
      }

    }

    namespace {

    template<typename valueT> xappender& do_xformat_Xu(xappender& xapp, const valueT& value) noexcept
      {
        static_assert(std::is_unsigned<valueT>::value, "??");
        // Record the beginning of the put area.
        char* bp = xapp;
        // Write all digits in reverse order.
        auto reg = value;
        while(reg != 0)
          xapp << "0123456789"[reg % 10], reg /= 10;
        // Get the end of the put area.
        char* ep = xapp;
        if(bp != ep)
          // Reverse the string.
          while(bp < --ep)
            ::std::iter_swap(bp++, ep);
        else
          // Ensure there is at least a digit.
          xapp << '0';
        return xapp;
      }

    }

    namespace details_tinyfmt {

    long xformat_Di(char* str, signed value) noexcept
      {
        xappender xapp(str);
        // Write the integer in decimal.
        // Prepend a minus sign if the number is negative.
        if(value < 0)
          xapp << '-', do_xformat_Xu(xapp, -static_cast<unsigned>(value));
        else
          do_xformat_Xu(xapp, static_cast<unsigned>(value));
        return xapp;
      }

    long xformat_Du(char* str, unsigned value) noexcept
      {
        xappender xapp(str);
        // Write the integer in decimal.
        do_xformat_Xu(xapp, value);
        return xapp;
      }

    long xformat_Qi(char* str, signed long long value) noexcept
      {
        xappender xapp(str);
        // Write the integer in decimal.
        // Prepend a minus sign if the number is negative.
        if(value < 0)
          xapp << '-', do_xformat_Xu(xapp, -static_cast<unsigned long long>(value));
        else
          do_xformat_Xu(xapp, static_cast<unsigned long long>(value));
        return xapp;
      }

    long xformat_Qu(char* str, unsigned long long value) noexcept
      {
        xappender xapp(str);
        // Write the integer in decimal.
        do_xformat_Xu(xapp, value);
        return xapp;
      }

    long xformat_Qf(char* str, double value) noexcept
      {
return std::sprintf(str, "%g", value);
      }

    long xformat_Tf(char* str, long double value) noexcept
      {
return std::sprintf(str, "%Lg", value);
      }

    }

template class basic_tinyfmt<char>;
template class basic_tinyfmt<wchar_t>;

}  // namespace rocket
