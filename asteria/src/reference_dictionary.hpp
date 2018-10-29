// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_DICTIONARY_HPP_
#define ASTERIA_REFERENCE_DICTIONARY_HPP_

#include "fwd.hpp"
#include "reference.hpp"

namespace Asteria {

class Reference_dictionary
  {
  private:
    rocket::cow_hashmap<rocket::cow_string, Reference, rocket::cow_string::hash, rocket::cow_string::equal_to> m_dict;

  public:
    Reference_dictionary() noexcept
      : m_dict()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Reference_dictionary);

  public:
    bool empty() const noexcept
      {
        return this->m_dict.empty();
      }
    std::size_t size() const noexcept
      {
        return this->m_dict.size();
      }
    void clear() noexcept
      {
        this->m_dict.clear();
      }

    const Reference * get_opt(const rocket::cow_string &name) const noexcept
      {
        const auto it = this->m_dict.find(name);
        if(it == this->m_dict.end()) {
          return nullptr;
        }
        return &(it->second);
      }
    Reference * get_opt(const rocket::cow_string &name) noexcept
      {
        const auto it = this->m_dict.find_mut(name);
        if(it == this->m_dict.end()) {
          return nullptr;
        }
        return &(it->second);
      }

    template<typename ParamT>
      Reference & set(const rocket::cow_string &name, ParamT &&param)
      {
        return this->m_dict.insert_or_assign(name, std::forward<ParamT>(param)).first->second;
      }
    bool unset(const rocket::cow_string &name) noexcept
      {
        return this->m_dict.erase(name);
      }
  };

}

#endif
