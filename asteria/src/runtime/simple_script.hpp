// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_SIMPLE_SCRIPT_HPP_
#define ASTERIA_RUNTIME_SIMPLE_SCRIPT_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"
#include "global_context.hpp"
#include "reference.hpp"

namespace Asteria {

class Simple_Script
  {
  private:
    Global_Context m_global;
    rcptr<Rcbase> m_cptr;  // note type erasure

  public:
    explicit Simple_Script(API_Version version = api_version_latest)
      : m_global(version)
      {
      }
    Simple_Script(std::streambuf& sbuf, const cow_string& filename, API_Version version = api_version_latest)
      : m_global(version)
      {
        this->reload(sbuf, filename);
      }
    Simple_Script(std::istream& istrm, const cow_string& filename, API_Version version = api_version_latest)
      : m_global(version)
      {
        this->reload(istrm, filename);
      }
    Simple_Script(const cow_string& cstr, const cow_string& filename, API_Version version = api_version_latest)
      : m_global(version)
      {
        this->reload(cstr, filename);
      }
    explicit Simple_Script(const cow_string& filename, API_Version version = api_version_latest)
      : m_global(version)
      {
        this->reload_file(filename);
      }

  public:
    bool empty() const noexcept
      {
        return this->m_cptr == nullptr;
      }
    Simple_Script& clear() noexcept
      {
        return this->m_cptr.reset(), *this;
      }

    explicit operator bool () const noexcept
      {
        return this->m_cptr != nullptr;
      }

    Simple_Script& reload(std::streambuf& sbuf, const cow_string& filename);
    Simple_Script& reload(std::istream& istrm, const cow_string& filename);
    Simple_Script& reload(const cow_string& cstr, const cow_string& filename);
    Simple_Script& reload_file(const cow_string& filename);

    Reference execute() const;
    Reference execute(cow_vector<Reference>&& args) const;
    Reference execute(cow_vector<Value>&& vals) const;
  };

inline std::istream& operator>>(std::istream& istrm, Simple_Script& file)
  {
    return file.reload(istrm, rocket::sref("<anonymous>")), istrm;
  }

}  // namespace Asteria

#endif
