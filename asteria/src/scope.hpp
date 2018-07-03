// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SCOPE_HPP_
#define ASTERIA_SCOPE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Scope {
public:
	enum Purpose : std::uint8_t {
		purpose_lexical   = 0,
		purpose_plain     = 1,
		purpose_function  = 2,
		purpose_file      = 3,
	};

private:
	const Purpose m_purpose;
	const Sp<const Scope> m_parent_opt;

	Dictionary<Vp<Reference>> m_named_references;
	Vector<Sp<const Function_base>> m_deferred_callbacks;

public:
	Scope(Purpose purpose, Sp<const Scope> parent_opt)
		: m_purpose(std::move(purpose)), m_parent_opt(std::move(parent_opt))
		, m_named_references(), m_deferred_callbacks()
	{ }
	~Scope();

	Scope(const Scope &) = delete;
	Scope & operator=(const Scope &) = delete;

private:
	void do_dispose_deferred_callbacks() noexcept;

public:
	Purpose get_purpose() const noexcept {
		return m_purpose;
	}
	Sp_cref<const Scope> get_parent_opt() const noexcept {
		return m_parent_opt;
	}

	Sp<const Reference> get_named_reference_opt(Cow_string_cref id) const noexcept;
	std::reference_wrapper<Vp<Reference>> mutate_named_reference(Cow_string_cref id);

	void defer_callback(Sp<const Function_base> &&callback);
};

extern void prepare_function_scope(Sp_cref<Scope> scope, Sp_cref<Recycler> recycler_out, Cow_string_cref source, const Vector<Cow_string> &params, Vp<Reference> &&this_opt, Vector<Vp<Reference>> &&args);
extern void prepare_function_scope_lexical(Sp_cref<Scope> scope, Cow_string_cref source, const Vector<Cow_string> &params);

}

#endif
