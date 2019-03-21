// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COMPILER_SIMPLE_SOURCE_FILE_HPP_
#define ASTERIA_COMPILER_SIMPLE_SOURCE_FILE_HPP_

#include "../fwd.hpp"
#include "parser_error.hpp"

namespace Asteria {

class Simple_Source_File
  {
  private:
    // We want type erasure.
    Cow_Vector<Instantiated_Function> m_codev;

  public:
    Simple_Source_File() noexcept
      {
      }
    explicit Simple_Source_File(const Cow_String& filename)
      {
        this->do_reload_file(true, filename);
      }
    Simple_Source_File(std::istream& cstrm, const Cow_String& filename)
      {
        this->do_reload_stream(true, cstrm, filename);
      }

  private:
    // For these two functions, if `error_opt` is null, an exception is thrown.
    Parser_Error do_reload_file(bool throw_on_failure, const Cow_String& filename);
    Parser_Error do_reload_stream(bool throw_on_failure, std::istream& cstrm, const Cow_String& filename);

  public:
    bool empty() const noexcept
      {
        return this->m_codev.empty();
      }
    explicit operator bool () const noexcept
      {
        return !this->m_codev.empty();
      }
    Parser_Error load_file(const Cow_String& filename)
      {
        return this->do_reload_file(false, filename);
      }
    Parser_Error load_stream(std::istream& cstrm, const Cow_String& filename)
      {
        return this->do_reload_stream(false, cstrm, filename);
      }
    void clear() noexcept;

    Reference execute(const Global_Context& global, Cow_Vector<Reference>&& args) const;
  };

}  // namespace Asteria

#endif
