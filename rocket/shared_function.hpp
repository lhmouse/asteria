// This file is part of Asteria.
// Copyleft 2024, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_SHARED_FUNCTION_
#define ROCKET_SHARED_FUNCTION_

#include "fwd.hpp"
#include "refcnt_ptr.hpp"
#include "xthrow.hpp"
namespace rocket {

template<typename xfuncT>
class shared_function;

template<typename xreturnT, typename... xparamsT>
class shared_function<xreturnT (xparamsT...)>
  {
  public:
    using function_type = xreturnT (xparamsT...);

    template<typename xfuncT>
    using is_viable = is_invocable_r<xreturnT,
                        typename decay<xfuncT>::type&, xparamsT&&...>;

  private:
    struct container : refcnt_base<container>
      {
        virtual ~container() { }
        virtual xreturnT forward_call(xparamsT&&...) = 0;
      };

    refcnt_ptr<container> m_pobj;
    function_type* m_pfn = nullptr;

  public:
    constexpr shared_function() noexcept = default;

    constexpr shared_function(function_type* pfn) noexcept
      : m_pfn(pfn)  { }

    template<typename xfuncT,
    ROCKET_ENABLE_IF(is_viable<xfuncT>::value),
    ROCKET_DISABLE_SELF(shared_function, xfuncT)>
    shared_function(xfuncT&& xfunc)
      {
        struct container_xfunc : container
          {
            typename decay<xfuncT>::type func;
            container_xfunc(xfuncT&& xf) : func(forward<xfuncT>(xf))  { }

            virtual xreturnT forward_call(xparamsT&&... params) override
              { return this->func(forward<xparamsT>(params)...);  }
          };

        // Set `m_pfn` to a non-null value.
        this->m_pobj.reset(new container_xfunc(forward<xfuncT>(xfunc)));
        this->m_pfn = reinterpret_cast<function_type*>(-2);
      }

    shared_function(const shared_function& other) noexcept
      : m_pobj(other.m_pobj),
        m_pfn(other.m_pfn)
      { }

    shared_function(shared_function&& other) noexcept
      : m_pobj(noadl::exchange(other.m_pobj)),
        m_pfn(noadl::exchange(other.m_pfn))
      { }

    shared_function&
    operator=(const shared_function& other) & noexcept
      {
        this->m_pobj = other.m_pobj;
        this->m_pfn = other.m_pfn;
        return *this;
      }

    shared_function&
    operator=(shared_function&& other) & noexcept
      {
        this->m_pobj = noadl::exchange(other.m_pobj);
        this->m_pfn = noadl::exchange(other.m_pfn);
        return *this;
      }

    shared_function&
    swap(shared_function& other) noexcept
      {
        this->m_pobj.swap(other.m_pobj);
        ::std::swap(this->m_pfn, other.m_pfn);
        return *this;
      }

  public:
    explicit constexpr operator bool() const noexcept
      {
        return this->m_pfn != nullptr;
      }

    xreturnT
    operator()(xparamsT... params) const
      {
        if(this->m_pfn == nullptr)
          noadl::sprintf_and_throw<invalid_argument>(
              "shared_function: Attempt to call a null function");

        if(this->m_pobj)
          return this->m_pobj->forward_call(forward<xparamsT>(params)...);
        else
          return this->m_pfn(forward<xparamsT>(params)...);
      }
  };

template<typename xfuncT>
inline
void
swap(shared_function<xfuncT>& lhs, shared_function<xfuncT>& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace rocket
#endif
