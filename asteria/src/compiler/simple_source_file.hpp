// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_SIMPLE_SOURCE_FILE_HPP_
#define ASTERIA_COMPILER_SIMPLE_SOURCE_FILE_HPP_

#include "../fwd.hpp"
#include "../runtime/rcbase.hpp"

namespace Asteria {

class Simple_Source_File
  {
  private:
    rcptr<Rcbase> m_cptr;  // note type erasure

  public:
    Simple_Source_File() noexcept
      {
      }
    Simple_Source_File(std::streambuf& sbuf, const cow_string& filename)
      {
        this->reload(sbuf, filename);
      }
    Simple_Source_File(std::istream& istrm, const cow_string& filename)
      {
        this->reload(istrm, filename);
      }
    Simple_Source_File(const cow_string& cstr, const cow_string& filename)
      {
        this->reload(cstr, filename);
      }
    explicit Simple_Source_File(const cow_string& filename)
      {
        this->open(filename);
      }

  public:
    explicit operator bool () const noexcept
      {
        return this->m_cptr != nullptr;
      }
    bool empty() const noexcept
      {
        return this->m_cptr == nullptr;
      }
    Simple_Source_File& clear() noexcept
      {
        return this->m_cptr.reset(), *this;
      }

    Simple_Source_File& reload(std::streambuf& sbuf, const cow_string& filename);
    Simple_Source_File& reload(std::istream& istrm, const cow_string& filename);
    Simple_Source_File& reload(const cow_string& cstr, const cow_string& filename);
    Simple_Source_File& open(const cow_string& filename);

    Reference execute(const Global_Context& global, cow_vector<Reference>&& args) const;
  };

inline std::istream& operator>>(std::istream& istrm, Simple_Source_File& file)
  {
    return file.reload(istrm, rocket::sref("<anonymous>")), istrm;
  }

}  // namespace Asteria

#endif
