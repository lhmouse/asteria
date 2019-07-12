// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "exception.hpp"
#include "../utilities.hpp"

namespace Asteria {

Exception::~Exception()
  {
  }

Exception& Exception::dynamic_copy(const std::exception& other)
  {
    auto qother = dynamic_cast<const Exception*>(&other);
    if(!qother) {
      // Say the exception was thrown native code.
      this->m_value = G_string(other.what());
      this->m_frames.emplace_back(rocket::sref("<native code>"), -1, Backtrace_Frame::ftype_native, G_null());
      return *this;
    }
    // Copy frames from `other`.
    this->m_value = qother->m_value;
    this->m_frames = qother->m_frames;
    return *this;
  }

}  // namespace Asteria
