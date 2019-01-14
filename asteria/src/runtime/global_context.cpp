// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "global_context.hpp"
#include "generational_collector.hpp"
#include "variable.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    class Components : public Refcounted_base
      {
      private:
        rocket::refcounted_ptr<Generational_collector> m_coll;

      public:
        Components()
          {
            // Create the global garbage collector.
            this->m_coll = rocket::make_refcounted<Generational_collector>();
          }
        ROCKET_NONCOPYABLE_DESTRUCTOR(Components)
          {
          }

      public:
        // Implement the global garbage collector.
        rocket::refcounted_ptr<Variable> create_variable()
          {
            return this->m_coll->create_variable();
          }
        void perform_final_garbage_collection() noexcept
          try {
            this->m_coll->perform_garbage_collection(UINT_MAX);
          } catch(std::exception &e) {
            ASTERIA_DEBUG_LOG("An exception was thrown during the final garbage collection and some resources might have leaked: ", e.what());
          }
      };

    }

Global_context::~Global_context()
  {
    const auto impl = static_cast<Components *>(this->m_impl.get());
    if(ROCKET_UNEXPECT(!impl)) {
      return;
    }
    // Perform the final garbage collection.
    this->clear_named_references();
    impl->perform_final_garbage_collection();
  }

void Global_context::do_initialize()
  {
    // Create the object containing all components.
    const auto impl = rocket::make_refcounted<Components>();
    this->m_impl = impl;
    // Add standard library interfaces.
    D_object root;
    ASTERIA_DEBUG_LOG("TODO add std library");
    Reference_root::S_constant ref_c = { std::move(root) };
  }

rocket::refcounted_ptr<Variable> Global_context::create_variable()
  {
    ROCKET_ASSERT(this->m_impl);
    return static_cast<Components *>(this->m_impl.get())->create_variable();
  }

}
