// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TRANSPARENT_COMPARATORS_HPP_
#define ROCKET_TRANSPARENT_COMPARATORS_HPP_

#include "compatibility.h"
#include <type_traits>
#include <utility>

namespace rocket
{

using ::std::true_type;

struct transparent_equal_to
{
	using is_transparent = true_type;

	template<typename selfT, typename otherT>
	constexpr auto operator()(selfT &&self, otherT &&other) const -> decltype(::std::forward<selfT>(self) == ::std::forward<otherT>(other))
	{
		return ::std::forward<selfT>(self) == ::std::forward<otherT>(other);
	}
};

struct transparent_not_equal_to
{
	using is_transparent = true_type;

	template<typename selfT, typename otherT>
	constexpr auto operator()(selfT &&self, otherT &&other) const -> decltype(::std::forward<selfT>(self) != ::std::forward<otherT>(other))
	{
		return ::std::forward<selfT>(self) != ::std::forward<otherT>(other);
	}
};

struct transparent_less
{
	using is_transparent = true_type;

	template<typename selfT, typename otherT>
	constexpr auto operator()(selfT &&self, otherT &&other) const -> decltype(::std::forward<selfT>(self) < ::std::forward<otherT>(other))
	{
		return ::std::forward<selfT>(self) < ::std::forward<otherT>(other);
	}
};

struct transparent_greater
{
	using is_transparent = true_type;

	template<typename selfT, typename otherT>
	constexpr auto operator()(selfT &&self, otherT &&other) const -> decltype(::std::forward<selfT>(self) > ::std::forward<otherT>(other))
	{
		return ::std::forward<selfT>(self) > ::std::forward<otherT>(other);
	}
};

struct transparent_less_equal
{
	using is_transparent = true_type;

	template<typename selfT, typename otherT>
	constexpr auto operator()(selfT &&self, otherT &&other) const -> decltype(::std::forward<selfT>(self) <= ::std::forward<otherT>(other))
	{
		return ::std::forward<selfT>(self) <= ::std::forward<otherT>(other);
	}
};

struct transparent_greater_equal
{
	using is_transparent = true_type;

	template<typename selfT, typename otherT>
	constexpr auto operator()(selfT &&self, otherT &&other) const -> decltype(::std::forward<selfT>(self) >= ::std::forward<otherT>(other))
	{
		return ::std::forward<selfT>(self) >= ::std::forward<otherT>(other);
	}
};

}

#endif
