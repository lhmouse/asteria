// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_TRACEABLE_EXCEPTION_HPP_
#define ASTERIA_RUNTIME_TRACEABLE_EXCEPTION_HPP_

#include "../fwd.hpp"
#include "value.hpp"
#include "../syntax/source_location.hpp"

namespace Asteria {

class Traceable_Exception : public virtual std::exception
  {
  private:
    Value m_value;
    CoW_Vector<Source_Location> m_frames;

  public:
    template<typename XvalueT, ROCKET_ENABLE_IF(std::is_constructible<Value, XvalueT &&>::value)>
      explicit Traceable_Exception(XvalueT &&xvalue)
      {
        this->m_value = std::forward<XvalueT>(xvalue);
      }
    // This constructor does not accept lvalues.
    template<typename ExceptT, ROCKET_ENABLE_IF(std::is_base_of<std::exception, ExceptT>::value)>
      explicit Traceable_Exception(ExceptT &&except)
      {
        const auto other = dynamic_cast<Traceable_Exception *>(std::addressof(except));
        if(!other) {
          this->m_value = D_string(except.what());
          return;
        }
        this->m_value = std::move(other->m_value);
        this->m_frames = std::move(other->m_frames);
      }
    ~Traceable_Exception() override;

  public:
    const char * what() const noexcept override
      {
        return "Asteria::Traceable_Exception";
      }

    const Value & get_value() const noexcept
      {
        return this->m_value;
      }
    Value & mut_value() noexcept
      {
        return this->m_value;
      }

    std::size_t get_frame_count() const noexcept
      {
        return this->m_frames.size();
      }
    const Source_Location & get_frame(std::size_t index) const
      {
        return this->m_frames.at(index);
      }
    void append_frame(const Source_Location &sloc)
      {
        this->m_frames.emplace_back(sloc);
      }
  };

}

#endif
