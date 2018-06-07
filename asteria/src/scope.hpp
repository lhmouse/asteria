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

	Vp_string_map<Reference> m_named_references;
	Sp_vector<const Function_base> m_deferred_callbacks;

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
	const Sp<const Scope> & get_parent_opt() const noexcept {
		return m_parent_opt;
	}

	Sp<const Reference> get_named_reference_opt(const Cow_string &id) const noexcept;
	std::reference_wrapper<Vp<Reference>> drill_for_named_reference(const Cow_string &id);

	void defer_callback(Sp<const Function_base> &&callback);
};

extern void prepare_function_scope(Spr<Scope> scope, Spr<Recycler> recycler_inout, const Cow_string &source_location, const Sp_vector<const Parameter> &parameters_opt, Vp<Reference> &&this_opt, Vp_vector<Reference> &&arguments_opt);
extern void prepare_function_scope_lexical(Spr<Scope> scope, const Cow_string &source_location, const Sp_vector<const Parameter> &parameters_opt);

}

#endif
