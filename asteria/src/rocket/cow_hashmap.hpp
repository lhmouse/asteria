// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_HASHMAP_HPP_
#define ROCKET_COW_HASHMAP_HPP_

#include <memory> // std::allocator<>, std::allocator_traits<>
#include <atomic> // std::atomic<>
#include <type_traits> // so many...
#include <iterator> // std::iterator_traits<>, std::forward_iterator_tag
#include <initializer_list> // std::initializer_list<>
#include <utility> // std::move(), std::forward(), std::declval(), std::pair<>
#include <cstddef> // std::size_t, std::ptrdiff_t, std::nullptr_t
#include <cstring> // std::memset()
#include "compatibility.h"
#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
#include "allocator_utilities.hpp"
#include "transparent_comparators.hpp"

/* Differences from `std::unordered_map`:
 * 1. `begin()` and `end()` always return `const_iterator`s. `at()`, `front()` and `back()` always return `const_reference`s.
 * 2. No default hash function is provided. You must provide your own hasher or write `std::hash` explicitly.
 * 3. The copy constructor and copy assignment operator will not throw exceptions.
 * 4. Comparison operators are not provided.
 * 5. `emplace()` and `emplace_hint()` functions are not provided. `try_emplace()` is recommended as an alternative.
 * 6. There are no buckets. Bucket lookups and local iterators are not provided. The non-unique (`unordered_multimap`) equivalent cannot be implemented.
 * 7. `equal_range()` functions are not provided.
 * 8. The key type may be incomplete.
 * 9. The value type may be incomplete. It need be neither copy-assignable nor move-assignable.
 */

namespace rocket
{

using ::std::allocator;
using ::std::allocator_traits;
using ::std::atomic;
using ::std::is_same;
using ::std::decay;
using ::std::remove_reference;
using ::std::is_array;
using ::std::is_trivial;
using ::std::is_const;
using ::std::enable_if;
using ::std::is_convertible;
using ::std::is_copy_constructible;
using ::std::is_nothrow_copy_constructible;
using ::std::is_nothrow_move_constructible;
using ::std::remove_const;
using ::std::conditional;
using ::std::integral_constant;
using ::std::iterator_traits;
using ::std::initializer_list;
using ::std::pair;
using ::std::size_t;
using ::std::ptrdiff_t;
using ::std::nullptr_t;

template<typename keyT, typename mappedT, typename hasherT, typename equalT = transparent_equal_to, typename allocatorT = allocator<pair<const keyT, mappedT>>>
class cow_hashmap;

namespace details_cow_hashmap
{
	template<typename allocatorT>
	class value_handle
	{
	public:
		using allocator_type   = allocatorT;
		using value_type       = typename allocatorT::value_type;
		using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
		using pointer          = typename allocator_traits<allocator_type>::pointer;

	private:
		pointer m_ptr;

	public:
		value_handle() noexcept
			: m_ptr()
		{
		}

		value_handle(const value_handle &) = delete;
		value_handle & operator=(const value_handle &) = delete;

	public:
		const_pointer get() const noexcept
		{
			return this->m_ptr;
		}
		pointer get() noexcept
		{
			return this->m_ptr;
		}
		pointer exchange(pointer ptr) noexcept
		{
			return noadl::exchange(this->m_ptr, ptr);
		}
	};

	template<typename allocatorT>
	struct handle_storage
	{
		using allocator_type   = allocatorT;
		using handle_type      = value_handle<allocator_type>;
		using size_type        = typename allocator_traits<allocator_type>::size_type;

		static constexpr size_type min_nblk_for_nelem(size_type nelem) noexcept
		{
			return (nelem * sizeof(handle_type) + sizeof(handle_storage) - 1) / sizeof(handle_storage) + 1;
		}
		static constexpr size_type max_nelem_for_nblk(size_type nblk) noexcept
		{
			return (nblk - 1) * sizeof(handle_storage) / sizeof(handle_type);
		}

		atomic<long> nref;
		allocator_type alloc;
		size_type nblk;
		size_type nelem;
		union { handle_type data[0]; };

		handle_storage(const allocator_type &xalloc, size_type xnblk) noexcept
			: alloc(xalloc), nblk(xnblk)
		{
			const auto nslot = this->max_nelem_for_nblk(this->nblk);
			for(size_type i = 0; i < nslot; ++i) {
				allocator_traits<allocator_type>::construct(this->alloc, this->data + i);
			}
			this->nelem = 0;
			this->nref.store(1, ::std::memory_order_release);
		}
		~handle_storage()
		{
			const auto nslot = this->max_nelem_for_nblk(this->nblk);
			for(size_type i = 0; i < nslot; ++i) {
				const auto eptr = this->data[i].get();
				if(eptr == nullptr) {
					continue;
				}
				allocator_traits<allocator_type>::destroy(this->alloc, noadl::unfancy(eptr));
				allocator_traits<allocator_type>::deallocate(this->alloc, eptr, size_type(1));
			}
			for(size_type i = 0; i < nslot; ++i) {
				allocator_traits<allocator_type>::destroy(this->alloc, this->data + i);
			}
#ifdef ROCKET_DEBUG
			this->nelem = 0xEECD;
#endif
		}

		handle_storage(const handle_storage &) = delete;
		handle_storage & operator=(const handle_storage &) = delete;
	};

	template<typename typeT, typename otherT>
	struct copy_const_from
		: conditional<is_const<typename remove_reference<otherT>::type>::value, const typeT, typeT>
	{
	};

	ROCKET_SELECTANY extern constexpr unsigned short step_table[] = {
		1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097,
		1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223,
		1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321,
		1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459,
		1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511, 1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571,
		1579, 1583, 1597, 1601, 1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693,
		1697, 1699, 1709, 1721, 1723, 1733, 1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811,
		1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949,
		1951, 1973, 1979, 1987, 1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053, 2063, 2069,
		2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129, 2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203,
		2207, 2213, 2221, 2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287, 2293, 2297, 2309, 2311,
		2333, 2339, 2341, 2347, 2351, 2357, 2371, 2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417, 2423,
		2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 2531, 2539, 2543, 2549, 2551, 2557, 2579,
		2591, 2593, 2609, 2617, 2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677, 2683, 2687, 2689, 2693,
		2699, 2707, 2711, 2713, 2719, 2729, 2731, 2741, 2749, 2753, 2767, 2777, 2789, 2791, 2797, 2801,
		2803, 2819, 2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 2903, 2909, 2917, 2927, 2939,
		2953, 2957, 2963, 2969, 2971, 2999, 3001, 3011, 3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079,
		3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 3181, 3187, 3191, 3203, 3209, 3217, 3221,
		3229, 3251, 3253, 3257, 3259, 3271, 3299, 3301, 3307, 3313, 3319, 3323, 3329, 3331, 3343, 3347,
		3359, 3361, 3371, 3373, 3389, 3391, 3407, 3413, 3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491,
		3499, 3511, 3517, 3527, 3529, 3533, 3539, 3541, 3547, 3557, 3559, 3571, 3581, 3583, 3593, 3607,
		3613, 3617, 3623, 3631, 3637, 3643, 3659, 3671, 3673, 3677, 3691, 3697, 3701, 3709, 3719, 3727,
		3733, 3739, 3761, 3767, 3769, 3779, 3793, 3797, 3803, 3821, 3823, 3833, 3847, 3851, 3853, 3863,
		3877, 3881, 3889, 3907, 3911, 3917, 3919, 3923, 3929, 3931, 3943, 3947, 3967, 3989, 4001, 4003,
		4007, 4013, 4019, 4021, 4027, 4049, 4051, 4057, 4073, 4079, 4091, 4093, 4099, 4111, 4127, 4129,
		4133, 4139, 4153, 4157, 4159, 4177, 4201, 4211, 4217, 4219, 4229, 4231, 4241, 4243, 4253, 4259,
		4261, 4271, 4273, 4283, 4289, 4297, 4327, 4337, 4339, 4349, 4357, 4363, 4373, 4391, 4397, 4409,
		4421, 4423, 4441, 4447, 4451, 4457, 4463, 4481, 4483, 4493, 4507, 4513, 4517, 4519, 4523, 4547,
		4549, 4561, 4567, 4583, 4591, 4597, 4603, 4621, 4637, 4639, 4643, 4649, 4651, 4657, 4663, 4673,
		4679, 4691, 4703, 4721, 4723, 4729, 4733, 4751, 4759, 4783, 4787, 4789, 4793, 4799, 4801, 4813,
		4817, 4831, 4861, 4871, 4877, 4889, 4903, 4909, 4919, 4931, 4933, 4937, 4943, 4951, 4957, 4967,
		4969, 4973, 4987, 4993, 4999, 5003, 5009, 5011, 5021, 5023, 5039, 5051, 5059, 5077, 5081, 5087,
		5099, 5101, 5107, 5113, 5119, 5147, 5153, 5167, 5171, 5179, 5189, 5197, 5209, 5227, 5231, 5233,
		5237, 5261, 5273, 5279, 5281, 5297, 5303, 5309, 5323, 5333, 5347, 5351, 5381, 5387, 5393, 5399,
		5407, 5413, 5417, 5419, 5431, 5437, 5441, 5443, 5449, 5471, 5477, 5479, 5483, 5501, 5503, 5507,
		5519, 5521, 5527, 5531, 5557, 5563, 5569, 5573, 5581, 5591, 5623, 5639, 5641, 5647, 5651, 5653,
		5657, 5659, 5669, 5683, 5689, 5693, 5701, 5711, 5717, 5737, 5741, 5743, 5749, 5779, 5783, 5791,
		5801, 5807, 5813, 5821, 5827, 5839, 5843, 5849, 5851, 5857, 5861, 5867, 5869, 5879, 5881, 5897,
		5903, 5923, 5927, 5939, 5953, 5981, 5987, 6007, 6011, 6029, 6037, 6043, 6047, 6053, 6067, 6073,
		6079, 6089, 6091, 6101, 6113, 6121, 6131, 6133, 6143, 6151, 6163, 6173, 6197, 6199, 6203, 6211,
		6217, 6221, 6229, 6247, 6257, 6263, 6269, 6271, 6277, 6287, 6299, 6301, 6311, 6317, 6323, 6329,
		6337, 6343, 6353, 6359, 6361, 6367, 6373, 6379, 6389, 6397, 6421, 6427, 6449, 6451, 6469, 6473,
		6481, 6491, 6521, 6529, 6547, 6551, 6553, 6563, 6569, 6571, 6577, 6581, 6599, 6607, 6619, 6637,
		6653, 6659, 6661, 6673, 6679, 6689, 6691, 6701, 6703, 6709, 6719, 6733, 6737, 6761, 6763, 6779,
		6781, 6791, 6793, 6803, 6823, 6827, 6829, 6833, 6841, 6857, 6863, 6869, 6871, 6883, 6899, 6907,
		6911, 6917, 6947, 6949, 6959, 6961, 6967, 6971, 6977, 6983, 6991, 6997, 7001, 7013, 7019, 7027,
		7039, 7043, 7057, 7069, 7079, 7103, 7109, 7121, 7127, 7129, 7151, 7159, 7177, 7187, 7193, 7207,
		7211, 7213, 7219, 7229, 7237, 7243, 7247, 7253, 7283, 7297, 7307, 7309, 7321, 7331, 7333, 7349,
		7351, 7369, 7393, 7411, 7417, 7433, 7451, 7457, 7459, 7477, 7481, 7487, 7489, 7499, 7507, 7517,
		7523, 7529, 7537, 7541, 7547, 7549, 7559, 7561, 7573, 7577, 7583, 7589, 7591, 7603, 7607, 7621,
		7639, 7643, 7649, 7669, 7673, 7681, 7687, 7691, 7699, 7703, 7717, 7723, 7727, 7741, 7753, 7757,
		7759, 7789, 7793, 7817, 7823, 7829, 7841, 7853, 7867, 7873, 7877, 7879, 7883, 7901, 7907, 7919,
	};
	static_assert(sizeof(step_table) / sizeof(step_table[0]) >= 512 + 64, "?");

	template<typename allocatorT>
	struct find_slot_helper
	{
		// This function may return a null pointer if the table is full.
		template<typename xpointerT, typename predT>
		typename copy_const_from<value_handle<allocatorT>, decltype(*(::std::declval<xpointerT &>()))>::type * operator()(xpointerT ptr, size_t hval, predT &&pred) const
		{
			const auto nslot = ptr->max_nelem_for_nblk(ptr->nblk);
			if(nslot == 0) {
				// There is no element.
				return nullptr;
			}
			// Check the first element.
			const auto seed = static_cast<::std::uint64_t>(hval) * 0xA17870F5D4F51B49;
			// Conversion between an unsigned integer type and a floating point type results in performance penalty.
			// For values known to be non-negative, an intermediate cast to a signed integer type will mitigate this.
			const auto ratio = static_cast<double>(static_cast<::std::int_fast64_t>(seed / 2)) / 0x1p63;
			ROCKET_ASSERT((0.0 <= ratio) && (ratio < 1.0));
			const auto roffset = static_cast<double>(static_cast<::std::int_fast64_t>(nslot)) * ratio;
			ROCKET_ASSERT((0.0 <= roffset) && (roffset < static_cast<double>(nslot)));
			const auto origin = static_cast<typename allocator_traits<allocatorT>::size_type>(static_cast<::std::int64_t>(roffset));
			auto cur = origin;
			// Stop when a null slot is encountered, or when the predicator function returns `true`.
			const auto stop_here = [&] { return (ptr->data[cur].get() == nullptr) || ::std::forward<predT>(pred)(ptr->data[cur].get()->first); };
			if(stop_here() == false) {
				// A hash collision has been detected.
				if(nslot == 1) {
					// There is no more element to check.
					return nullptr;
				}
				// Iterate the entire table using double hashing scheme.
				// `step` and `nslot` shall be mutually prime.
				size_t step;
				cur = (seed >> 32) % 512;
				for(;;) {
					// Choose a prime number for `step`.
					step = step_table[cur];
					if(step > nslot) {
						// Note that `(X + step) % nslot` is identically equal to `(X + step % nslot) % nslot`.
						step %= nslot;
						// `step` was a prime number before the modulus operation, so it cannot be zero now.
						break;
					}
					// Ensure that `step` is not a divisor of `nslot`.
					if(nslot % step != 0) {
						// Here `step` cannot be equal to `nslot`, so it must be less than `nslot`.
						break;
					}
					++cur;
				}
				ROCKET_ASSERT((0 < step) && (step < nslot));
				// Iterate the table, wrapping around as needed.
				cur = origin;
				do {
					cur = (step < nslot - cur) ? (cur + step) : (cur - (nslot - step));
					if(cur == origin) {
						// No null slot or desired element has been found so far.
						return nullptr;
					}
					ROCKET_ASSERT(cur < nslot);
				} while(stop_here() == false);
			}
			return ptr->data + cur;
		}
	};

	template<typename allocatorT, typename hasherT, bool copyableT = is_copy_constructible<typename allocatorT::value_type>::value>
	struct copy_storage_helper
	{
		// This is the generic version.
		template<typename xpointerT, typename ypointerT>
		void operator()(xpointerT ptr, const hasherT &hf, ypointerT ptr_old, size_t off, size_t cnt) const
		{
			for(auto i = off; i != off + cnt; ++i) {
				const auto eptr_old = ptr_old->data[i].get();
				if(eptr_old == nullptr) {
					continue;
				}
				// Find a slot for the new element.
				const auto slot = find_slot_helper<allocatorT>()(ptr, hf(eptr_old->first), [](const typename allocatorT::value_type::first_type &) { return false; });
				ROCKET_ASSERT(slot);
				// Allocate a new element by copy-constructing from the old one.
				const auto eptr = allocator_traits<allocatorT>::allocate(ptr->alloc, static_cast<typename allocator_traits<allocatorT>::size_type>(1));
				try {
					allocator_traits<allocatorT>::construct(ptr->alloc, noadl::unfancy(eptr), *eptr_old);
				} catch(...) {
					allocator_traits<allocatorT>::deallocate(ptr->alloc, eptr, static_cast<typename allocator_traits<allocatorT>::size_type>(1));
					throw;
				}
				// Insert it at the new slot.
				ROCKET_ASSERT(slot->get() == nullptr);
				slot->exchange(eptr);
				ptr->nelem += 1;
			}
		}
	};
	template<typename allocatorT, typename hasherT>
	struct copy_storage_helper<allocatorT, hasherT, false>
	{
		// This specialization is used when `allocatorT::value_type` is not copy-constructible.
		template<typename xpointerT, typename ypointerT>
		ROCKET_NORETURN void operator()(xpointerT /*ptr*/, const hasherT & /*hf*/, ypointerT /*ptr_old*/, size_t /*off*/, size_t /*cnt*/) const
		{
			// `allocatorT::value_type` is not copy-constructible.
			// Throw an exception unconditionally, even when there is nothing to copy.
			noadl::throw_domain_error("cow_hashmap: The `keyT` or `mappedT` of this `cow_hashmap` is not copy-constructible.");
		}
	};

	template<typename allocatorT, typename hasherT>
	struct move_storage_helper
	{
		// This is the generic version.
		template<typename xpointerT, typename ypointerT>
		void operator()(xpointerT ptr, const hasherT &hf, ypointerT ptr_old, size_t off, size_t cnt) const
		{
			for(auto i = off; i != off + cnt; ++i) {
				const auto eptr_old = ptr_old->data[i].get();
				if(eptr_old == nullptr) {
					continue;
				}
				// Find a slot for the new element.
				const auto slot = find_slot_helper<allocatorT>()(ptr, hf(eptr_old->first), [](const typename allocatorT::value_type::first_type &) { return false; });
				ROCKET_ASSERT(slot);
				// Allocate a new element by copy-constructing from the old one.
				const auto eptr = ptr_old->data[i].exchange(nullptr);
				ROCKET_ASSERT(eptr);
				ptr_old->nelem -= 1;
				// Insert it at the new slot.
				ROCKET_ASSERT(slot->get() == nullptr);
				slot->exchange(eptr);
				ptr->nelem += 1;
			}
		}
	};

	// This struct is used as placeholders for EBO'd bases that would otherwise be duplicate, in order to prevent ambiguity.
	template<int indexT>
	struct ebo_placeholder
	{
		template<typename anythingT>
		explicit constexpr ebo_placeholder(anythingT &&) noexcept
		{
		}
	};

	template<typename allocatorT, typename hasherT, typename equalT>
	class storage_handle
		: private allocator_wrapper_base_for<allocatorT>::type
		, private conditional<is_same<hasherT, allocatorT>::value,
		                      ebo_placeholder<0>, typename allocator_wrapper_base_for<hasherT>::type>::type
		, private conditional<is_same<equalT, allocatorT>::value || is_same<equalT, hasherT>::value,
		                      ebo_placeholder<1>, typename allocator_wrapper_base_for<equalT>::type>::type
	{
	public:
		using allocator_type   = allocatorT;
		using value_type       = typename allocator_type::value_type;
		using hasher           = hasherT;
		using key_equal        = equalT;
		using handle_type      = value_handle<allocator_type>;
		using size_type        = typename allocator_traits<allocator_type>::size_type;
		using difference_type  = typename allocator_traits<allocator_type>::difference_type;

		enum : size_type { max_load_factor_reciprocal = 2 };

	private:
		using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;
		using hasher_base       = typename allocator_wrapper_base_for<hasher>::type;
		using key_equal_base    = typename allocator_wrapper_base_for<key_equal>::type;
		using storage           = handle_storage<allocator_type>;
		using storage_allocator = typename allocator_traits<allocator_type>::template rebind_alloc<storage>;
		using storage_pointer   = typename allocator_traits<storage_allocator>::pointer;

	private:
		storage_pointer m_ptr;

	public:
		storage_handle(const allocator_type &alloc, const hasher &hf, const key_equal &eq)
			: allocator_base(alloc)
			, conditional<is_same<hasherT, allocatorT>::value,
			              ebo_placeholder<0>, hasher_base>::type(hf)
			, conditional<is_same<equalT, allocatorT>::value || is_same<equalT, hasherT>::value,
			              ebo_placeholder<1>, key_equal_base>::type(eq)
			, m_ptr(nullptr)
		{
		}
		storage_handle(allocator_type &&alloc, const hasher &hf, const key_equal &eq)
			: allocator_base(::std::move(alloc))
			, conditional<is_same<hasherT, allocatorT>::value,
			              ebo_placeholder<0>, hasher_base>::type(hf)
			, conditional<is_same<equalT, allocatorT>::value || is_same<equalT, hasherT>::value,
			              ebo_placeholder<1>, key_equal_base>::type(eq)
			, m_ptr(nullptr)
		{
		}
		~storage_handle()
		{
			this->do_reset(nullptr);
		}

		storage_handle(const storage_handle &) = delete;
		storage_handle & operator=(const storage_handle &) = delete;

	private:
		void do_reset(storage_pointer ptr_new) noexcept
		{
			const auto ptr = noadl::exchange(this->m_ptr, ptr_new);
			if(ptr == nullptr) {
				return;
			}
			// Decrement the reference count with acquire-release semantics to prevent races on `ptr->alloc`.
			const auto old = ptr->nref.fetch_sub(1, ::std::memory_order_acq_rel);
			ROCKET_ASSERT(old > 0);
			if(old > 1) {
				return;
			}
			// If it has been decremented to zero, deallocate the block.
			auto st_alloc = storage_allocator(ptr->alloc);
			const auto nblk = ptr->nblk;
			allocator_traits<storage_allocator>::destroy(st_alloc, noadl::unfancy(ptr));
#ifdef ROCKET_DEBUG
			::std::memset(static_cast<void *>(noadl::unfancy(ptr)), '~', sizeof(storage) * nblk);
#endif
			allocator_traits<storage_allocator>::deallocate(st_alloc, ptr, nblk);
		}

	public:
		const hasher & as_hasher() const noexcept
		{
			return static_cast<const hasher_base &>(*this);
		}
		hasher & as_hasher() noexcept
		{
			return static_cast<hasher_base &>(*this);
		}

		const key_equal & as_key_equal() const noexcept
		{
			return static_cast<const key_equal_base &>(*this);
		}
		key_equal & as_key_equal() noexcept
		{
			return static_cast<key_equal_base &>(*this);
		}

		const allocator_type & as_allocator() const noexcept
		{
			return static_cast<const allocator_base &>(*this);
		}
		allocator_type & as_allocator() noexcept
		{
			return static_cast<allocator_base &>(*this);
		}

		bool unique() const noexcept
		{
			const auto ptr = this->m_ptr;
			if(ptr == nullptr) {
				return false;
			}
			return ptr->nref.load(::std::memory_order_relaxed) == 1;
		}
		size_type slot_count() const noexcept
		{
			const auto ptr = this->m_ptr;
			if(ptr == nullptr) {
				return 0;
			}
			return storage::max_nelem_for_nblk(ptr->nblk);
		}
		size_type capacity() const noexcept
		{
			const auto ptr = this->m_ptr;
			if(ptr == nullptr) {
				return 0;
			}
			return storage::max_nelem_for_nblk(ptr->nblk) / max_load_factor_reciprocal;
		}
		size_type max_size() const noexcept
		{
			auto st_alloc = storage_allocator(this->as_allocator());
			const auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
			return storage::max_nelem_for_nblk(max_nblk / 2) / max_load_factor_reciprocal;
		}
		size_type check_size_add(size_type base, size_type add) const
		{
			const auto cap_max = this->max_size();
			ROCKET_ASSERT(base <= cap_max);
			if(cap_max - base < add) {
				noadl::throw_length_error("cow_hashmap: Increasing `%lld` by `%lld` would exceed the max size `%lld`.",
				                          static_cast<long long>(base), static_cast<long long>(add), static_cast<long long>(cap_max));
			}
			return base + add;
		}
		size_type round_up_capacity(size_type res_arg) const
		{
			const auto cap = this->check_size_add(0, res_arg);
			const auto nblk = storage::min_nblk_for_nelem(cap * max_load_factor_reciprocal);
			return storage::max_nelem_for_nblk(nblk) / max_load_factor_reciprocal;
		}
		const handle_type * data() const noexcept
		{
			const auto ptr = this->m_ptr;
			if(ptr == nullptr) {
				return nullptr;
			}
			return ptr->data;
		}
		size_type element_count() const noexcept
		{
			const auto ptr = this->m_ptr;
			if(ptr == nullptr) {
				return 0;
			}
			return ptr->nelem;
		}
		handle_type * reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
		{
			if(res_arg == 0) {
				// Deallocate the block.
				this->do_reset(nullptr);
				return nullptr;
			}
			const auto cap = this->check_size_add(0, res_arg);
			// Allocate an array of `storage` large enough for a header + `cap` instances of pointers.
			const auto nblk = storage::min_nblk_for_nelem(cap * max_load_factor_reciprocal);
			auto st_alloc = storage_allocator(this->as_allocator());
			const auto ptr = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
#ifdef ROCKET_DEBUG
			::std::memset(static_cast<void *>(noadl::unfancy(ptr)), '*', sizeof(storage) * nblk);
#endif
			allocator_traits<storage_allocator>::construct(st_alloc, noadl::unfancy(ptr), this->as_allocator(), nblk);
			const auto ptr_old = this->m_ptr;
			if(ptr_old) {
				try {
					// Copy or move elements into the new block.
					// Moving is only viable if the old and new allocators compare equal and the old block is owned exclusively.
					if((ptr_old->alloc != ptr->alloc) || (ptr_old->nref.load(::std::memory_order_relaxed) != 1)) {
						copy_storage_helper<allocator_type, hasher>()(ptr, this->as_hasher(), ptr_old,       0, cnt_one);
						copy_storage_helper<allocator_type, hasher>()(ptr, this->as_hasher(), ptr_old, off_two, cnt_two);
					} else {
						move_storage_helper<allocator_type, hasher>()(ptr, this->as_hasher(), ptr_old,       0, cnt_one);
						move_storage_helper<allocator_type, hasher>()(ptr, this->as_hasher(), ptr_old, off_two, cnt_two);
					}
				} catch(...) {
					// If an exception is thrown, deallocate the new block, then rethrow the exception.
					allocator_traits<storage_allocator>::destroy(st_alloc, noadl::unfancy(ptr));
					allocator_traits<storage_allocator>::deallocate(st_alloc, ptr, nblk);
					throw;
				}
			}
			// Replace the current block.
			this->do_reset(ptr);
			return ptr->data;
		}
		void deallocate() noexcept
		{
			this->do_reset(nullptr);
		}

		constexpr operator const storage_handle * () const noexcept
		{
			return this;
		}
		operator storage_handle * () noexcept
		{
			return this;
		}

		void share_with(const storage_handle &other) noexcept
		{
			const auto ptr = other.m_ptr;
			if(ptr) {
				// Increment the reference count.
				const auto old = ptr->nref.fetch_add(1, ::std::memory_order_relaxed);
				ROCKET_ASSERT(old > 0);
			}
			this->do_reset(ptr);
		}
		void share_with(storage_handle &&other) noexcept
		{
			const auto ptr = other.m_ptr;
			if(ptr) {
				// Detach the block.
				other.m_ptr = nullptr;
			}
			this->do_reset(ptr);
		}
		void exchange_with(storage_handle &other) noexcept
		{
			::std::swap(this->m_ptr, other.m_ptr);
		}

		handle_type * mut_data_unchecked() noexcept
		{
			const auto ptr = this->m_ptr;
			if(ptr == nullptr) {
				return nullptr;
			}
			ROCKET_ASSERT(this->unique());
			return ptr->data;
		}
		template<typename ykeyT>
		difference_type index_of_unchecked(const ykeyT &ykey) const
		{
			const auto ptr = this->m_ptr;
			if(ptr == nullptr) {
				return -1;
			}
			const auto slot = find_slot_helper<allocator_type>()(ptr, this->as_hasher()(ykey), [&](const typename value_type::first_type &xkey) { return this->as_key_equal()(xkey, ykey); });
			if((slot == nullptr) || (slot->get() == nullptr)) {
				return -1;
			}
			const auto toff = slot - ptr->data;
			ROCKET_ASSERT(toff >= 0);
			return toff;
		}
		template<typename ykeyT, typename ...paramsT>
		pair<handle_type *, bool> keyed_emplace_unchecked(const ykeyT &ykey, paramsT &&...params)
		{
			ROCKET_ASSERT(this->unique());
			ROCKET_ASSERT(this->element_count() < this->capacity());
			const auto ptr = this->m_ptr;
			ROCKET_ASSERT(ptr);
			// Find a slot for the new element.
			const auto slot = find_slot_helper<allocator_type>()(ptr, this->as_hasher()(ykey), [&](const typename value_type::first_type &xkey) { return this->as_key_equal()(xkey, ykey); });
			ROCKET_ASSERT(slot);
			if(slot->get() != nullptr) {
				// A duplicate key has been found.
				return ::std::make_pair(slot, false);
			}
			// Allocate a new element.
			const auto eptr = allocator_traits<allocator_type>::allocate(ptr->alloc, size_type(1));
			try {
				allocator_traits<allocator_type>::construct(ptr->alloc, noadl::unfancy(eptr), ::std::forward<paramsT>(params)...);
			} catch(...) {
				allocator_traits<allocatorT>::deallocate(ptr->alloc, eptr, size_type(1));
				throw;
			}
			// Insert it at the new slot.
			ROCKET_ASSERT(slot->get() == nullptr);
			slot->exchange(eptr);
			ptr->nelem += 1;
			return ::std::make_pair(slot, true);
		}
		void erase_range_unchecked(size_type tpos, size_type tn) noexcept
		{
			ROCKET_ASSERT(this->unique());
			ROCKET_ASSERT(tpos <= this->slot_count());
			ROCKET_ASSERT(tn <= this->slot_count() - tpos);
			if(tn == 0) {
				return;
			}
			const auto ptr = this->m_ptr;
			ROCKET_ASSERT(ptr);
			for(auto i = tpos; i != tpos + tn; ++i) {
				const auto eptr = ptr->data[i].exchange(nullptr);
				if(eptr == nullptr) {
					continue;
				}
				// Destroy the element and deallicate its storage.
				allocator_traits<allocator_type>::destroy(ptr->alloc, noadl::unfancy(eptr));
				allocator_traits<allocator_type>::deallocate(ptr->alloc, eptr, size_type(1));
			}
		}
	};

	// This informs the constructor of an iterator that the `slot` parameter might point to an empty slot.
	constexpr struct need_adjust_tag { } need_adjust;

	template<typename hashmapT, typename valueT>
	class hashmap_iterator
	{
		friend hashmapT;

	public:
		using iterator_category  = ::std::forward_iterator_tag;
		using value_type         = valueT;
		using pointer            = value_type *;
		using reference          = value_type &;
		using difference_type    = ptrdiff_t;

		using parent_type   = storage_handle<typename hashmapT::allocator_type, typename hashmapT::hasher, typename hashmapT::key_equal>;
		using handle_type   = typename copy_const_from<typename parent_type::handle_type, value_type>::type;

	private:
		const parent_type *m_ref;
		handle_type *m_slot;

	private:
		constexpr hashmap_iterator(const parent_type *ref, handle_type *slot) noexcept
			: m_ref(ref), m_slot(slot)
		{
		}
		hashmap_iterator(const parent_type *ref, need_adjust_tag, handle_type *hint) noexcept
			: m_ref(ref), m_slot(this->do_adjust_forwards(hint))
		{
		}

	public:
		constexpr hashmap_iterator() noexcept
			: hashmap_iterator(nullptr, nullptr)
		{
		}
		template<typename yvalueT, typename enable_if<is_convertible<yvalueT *, valueT *>::value>::type * = nullptr>
		constexpr hashmap_iterator(const hashmap_iterator<hashmapT, yvalueT> &other) noexcept
			: hashmap_iterator(other.m_ref, other.m_slot)
		{
		}

	private:
		handle_type * do_assert_valid_slot(handle_type *slot, bool to_dereference) const noexcept
		{
			const auto ref = this->m_ref;
			ROCKET_ASSERT_MSG(ref, "This iterator has not been initialized.");
			const auto dist = static_cast<size_t>(slot - ref->data());
			ROCKET_ASSERT_MSG(dist <= ref->slot_count(), "This iterator has been invalidated.");
			ROCKET_ASSERT_MSG(!((dist < ref->slot_count()) && (slot->get() == nullptr)), "The element referenced by this iterator no longer exists.");
			ROCKET_ASSERT_MSG(!(to_dereference && (dist == ref->slot_count())), "This iterator contains a past-the-end value and cannot be dereferenced.");
			return slot;
		}
		handle_type * do_adjust_forwards(handle_type *hint) const noexcept
		{
			if(hint == nullptr) {
				return nullptr;
			}
			const auto ref = this->m_ref;
			ROCKET_ASSERT_MSG(ref, "This iterator has not been initialized.");
			const auto end = ref->data() + ref->slot_count();
			auto slot = hint;
			while((slot != end) && (slot->get() == nullptr)) {
				++slot;
			}
			return slot;
		}

	public:
		const parent_type * parent() const noexcept
		{
			return this->m_ref;
		}

		handle_type * tell() const noexcept
		{
			return this->do_assert_valid_slot(this->m_slot, false);
		}
		handle_type * tell_owned_by(const parent_type *ref) const noexcept
		{
			ROCKET_ASSERT_MSG(this->m_ref == ref, "This iterator does not refer to an element in the same container.");
			return this->tell();
		}
		hashmap_iterator & seek_next() noexcept
		{
			auto slot = this->do_assert_valid_slot(this->m_slot, false);
			ROCKET_ASSERT_MSG(slot != this->m_ref->data() + this->m_ref->slot_count(), "The past-the-end iterator cannot be incremented.");
			slot = this->do_adjust_forwards(slot + 1);
			this->m_slot = this->do_assert_valid_slot(slot, false);
			return *this;
		}

		reference operator*() const noexcept
		{
			const auto slot = this->do_assert_valid_slot(this->m_slot, true);
			const auto eptr = slot->get();
			return *eptr;
		}
		pointer operator->() const noexcept
		{
			const auto slot = this->do_assert_valid_slot(this->m_slot, true);
			const auto eptr = slot->get();
			return noadl::unfancy(eptr);
		}
	};

	template<typename hashmapT, typename valueT>
	inline hashmap_iterator<hashmapT, valueT> & operator++(hashmap_iterator<hashmapT, valueT> &rhs) noexcept
	{
		return rhs.seek_next();
	}

	template<typename hashmapT, typename valueT>
	inline hashmap_iterator<hashmapT, valueT> operator++(hashmap_iterator<hashmapT, valueT> &lhs, int) noexcept
	{
		auto res = lhs;
		lhs.seek_next();
		return res;
	}

	template<typename hashmapT, typename xvalueT, typename yvalueT>
	inline bool operator==(const hashmap_iterator<hashmapT, xvalueT> &lhs, const hashmap_iterator<hashmapT, yvalueT> &rhs) noexcept
	{
		return lhs.tell() == rhs.tell();
	}
	template<typename hashmapT, typename xvalueT, typename yvalueT>
	inline bool operator!=(const hashmap_iterator<hashmapT, xvalueT> &lhs, const hashmap_iterator<hashmapT, yvalueT> &rhs) noexcept
	{
		return lhs.tell() != rhs.tell();
	}
}

template<typename keyT, typename mappedT, typename hasherT, typename equalT, typename allocatorT>
class cow_hashmap
{
	static_assert(is_array<keyT>::value == false, "`keyT` must not be an array type.");
	static_assert(is_array<mappedT>::value == false, "`mappedT` must not be an array type.");
	static_assert(is_same<typename allocatorT::value_type, pair<const keyT, mappedT>>::value, "`allocatorT::value_type` must denote the same type as `pair<const keyT, mappedT>`.");

public:
	// types
	using key_type        = keyT;
	using mapped_type     = mappedT;
	using value_type      = pair<const key_type, mapped_type>;
	using hasher          = hasherT;
	using key_equal       = equalT;
	using allocator_type  = allocatorT;

	using size_type        = typename allocator_traits<allocator_type>::size_type;
	using difference_type  = typename allocator_traits<allocator_type>::difference_type;
	using const_pointer    = typename allocator_traits<allocator_type>::const_pointer;
	using pointer          = typename allocator_traits<allocator_type>::pointer;
	using const_reference  = const value_type &;
	using reference        = value_type &;

	using const_iterator          = details_cow_hashmap::hashmap_iterator<cow_hashmap, const value_type>;
	using iterator                = details_cow_hashmap::hashmap_iterator<cow_hashmap, value_type>;

private:
	details_cow_hashmap::storage_handle<allocator_type, hasher, key_equal> m_sth;

public:
	// 26.5.4.2, construct/copy/destroy
	explicit cow_hashmap(size_type res_arg, const hasher &hf = hasher(), const key_equal &eq = key_equal(), const allocator_type &alloc = allocator_type())
		: m_sth(alloc, hf, eq)
	{
		this->m_sth.reallocate(0, 0, 0, res_arg);
	}
	cow_hashmap()
		: cow_hashmap(0, hasher(), key_equal(), allocator_type())
	{
	}
	explicit cow_hashmap(const allocator_type &alloc) noexcept
		: cow_hashmap(0, hasher(), key_equal(), alloc)
	{
	}
	cow_hashmap(size_type res_arg, const allocator_type &alloc)
		: cow_hashmap(res_arg, hasher(), key_equal(), alloc)
	{
	}
	cow_hashmap(size_type res_arg, const hasher &hf, const allocator_type &alloc)
		: cow_hashmap(res_arg, hf, key_equal(), alloc)
	{
	}
	cow_hashmap(const cow_hashmap &other) noexcept(is_nothrow_copy_constructible<hasher>::value && is_nothrow_copy_constructible<key_equal>::value)
		: cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
	{
		this->assign(other);
	}
	cow_hashmap(const cow_hashmap &other, const allocator_type &alloc) noexcept(is_nothrow_copy_constructible<hasher>::value && is_nothrow_copy_constructible<key_equal>::value)
		: cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), alloc)
	{
		this->assign(other);
	}
	cow_hashmap(cow_hashmap &&other) noexcept(is_nothrow_copy_constructible<hasher>::value && is_nothrow_copy_constructible<key_equal>::value)
		: cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), allocator_traits<allocator_type>::select_on_container_copy_construction(other.m_sth.as_allocator()))
	{
		this->assign(::std::move(other));
	}
	cow_hashmap(cow_hashmap &&other, const allocator_type &alloc) noexcept(is_nothrow_copy_constructible<hasher>::value && is_nothrow_copy_constructible<key_equal>::value)
		: cow_hashmap(0, other.m_sth.as_hasher(), other.m_sth.as_key_equal(), alloc)
	{
		this->assign(::std::move(other));
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	cow_hashmap(inputT first, inputT last, size_type res_arg = 0, const hasher &hf = hasher(), const key_equal &eq = key_equal(), const allocator_type &alloc = allocator_type())
		: cow_hashmap(res_arg, hf, eq, alloc)
	{
		this->assign(::std::move(first), ::std::move(last));
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	cow_hashmap(inputT first, inputT last, size_type res_arg, const allocator_type &alloc = allocator_type())
		: cow_hashmap(res_arg, hasher(), key_equal(), alloc)
	{
		this->assign(::std::move(first), ::std::move(last));
	}
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	cow_hashmap(inputT first, inputT last, size_type res_arg, const hasher &hf = hasher(), const allocator_type &alloc = allocator_type())
		: cow_hashmap(res_arg, hf, key_equal(), alloc)
	{
		this->assign(::std::move(first), ::std::move(last));
	}
	cow_hashmap(initializer_list<value_type> init, size_type res_arg = 0, const hasher &hf = hasher(), const key_equal &eq = key_equal(), const allocator_type &alloc = allocator_type())
		: cow_hashmap(res_arg, hf, eq, alloc)
	{
		this->assign(init);
	}
	cow_hashmap(initializer_list<value_type> init, size_type res_arg, const allocator_type &alloc = allocator_type())
		: cow_hashmap(res_arg, hasher(), key_equal(), alloc)
	{
		this->assign(init);
	}
	cow_hashmap(initializer_list<value_type> init, size_type res_arg, const hasher &hf = hasher(), const allocator_type &alloc = allocator_type())
		: cow_hashmap(res_arg, hf, key_equal(), alloc)
	{
		this->assign(init);
	}
	cow_hashmap & operator=(const cow_hashmap &other) noexcept
	{
		allocator_copy_assigner<allocator_type>()(this->m_sth.as_allocator(), other.m_sth.as_allocator());
		return this->assign(other);
	}
	cow_hashmap & operator=(cow_hashmap &&other) noexcept
	{
		allocator_move_assigner<allocator_type>()(this->m_sth.as_allocator(), ::std::move(other.m_sth.as_allocator()));
		return this->assign(::std::move(other));
	}
	cow_hashmap & operator=(initializer_list<value_type> init)
	{
		return this->assign(init);
	}

private:
	// Reallocate the storage to `res_arg` elements.
	// The storage is owned by the current hashmap exclusively after this function returns normally.
	details_cow_hashmap::value_handle<allocator_type> * do_reallocate(size_type cnt_one, size_type off_two, size_type cnt_two, size_type res_arg)
	{
		ROCKET_ASSERT(cnt_one <= off_two);
		ROCKET_ASSERT(off_two <= this->m_sth.slot_count());
		ROCKET_ASSERT(cnt_two <= this->m_sth.slot_count() - off_two);
		const auto ptr = this->m_sth.reallocate(cnt_one, off_two, cnt_two, res_arg);
		if(ptr == nullptr) {
			// The storage has been deallocated.
			return nullptr;
		}
		ROCKET_ASSERT(this->m_sth.unique());
		return ptr;
	}
	// Deallocate any dynamic storage.
	void do_deallocate() noexcept
	{
		this->m_sth.deallocate();
	}

	// Reallocate more storage as needed, without shrinking.
	void do_reserve_more(size_type cap_add)
	{
		const auto cnt = this->size();
		auto cap = this->m_sth.check_size_add(cnt, cap_add);
		if((this->unique() == false) || (this->capacity() < cap)) {
#ifndef ROCKET_DEBUG
			// Reserve more space for non-debug builds.
			cap = noadl::max(cap, cnt + cnt / 2 + 7);
#endif
			this->do_reallocate(0, 0, this->slot_count(), cap);
		}
		ROCKET_ASSERT(this->capacity() >= cap);
	}

	const details_cow_hashmap::value_handle<allocator_type> * do_get_table() const noexcept
	{
		return this->m_sth.data();
	}
	details_cow_hashmap::value_handle<allocator_type> * do_mut_table()
	{
		if(this->empty()) {
			return nullptr;
		}
		if(this->unique() == false) {
			return this->do_reallocate(0, 0, this->slot_count(), this->size());
		}
		return this->m_sth.mut_data_unchecked();
	}

	details_cow_hashmap::value_handle<allocator_type> * do_erase_no_bound_check(size_type tpos, size_type tn)
	{
		const auto cnt_old = this->size();
		const auto slot_cnt_old = this->slot_count();
		ROCKET_ASSERT(tpos <= slot_cnt_old);
		ROCKET_ASSERT(tn <= slot_cnt_old - tpos);
		if(this->unique() == false) {
			const auto ptr = this->do_reallocate(tpos, tpos + tn, slot_cnt_old - (tpos + tn), cnt_old);
			return ptr + tn;
		}
		const auto ptr = this->m_sth.mut_data_unchecked();
		this->m_sth.erase_range_unchecked(tpos, tn);
		return ptr + tn;
	}

public:
	// iterators
	const_iterator begin() const noexcept
	{
		return const_iterator(this->m_sth, details_cow_hashmap::need_adjust, this->do_get_table());
	}
	const_iterator end() const noexcept
	{
		return const_iterator(this->m_sth, this->do_get_table() + this->slot_count());
	}

	const_iterator cbegin() const noexcept
	{
		return this->begin();
	}
	const_iterator cend() const noexcept
	{
		return this->end();
	}

	// N.B. This function may throw `std::bad_alloc()`.
	// N.B. This is a non-standard extension.
	iterator mut_begin()
	{
		return iterator(this->m_sth, details_cow_hashmap::need_adjust, this->do_mut_table());
	}
	// N.B. This function may throw `std::bad_alloc()`.
	// N.B. This is a non-standard extension.
	iterator mut_end()
	{
		return iterator(this->m_sth, this->do_mut_table() + this->slot_count());
	}

	// capacity
	bool empty() const noexcept
	{
		return this->m_sth.element_count() == 0;
	}
	size_type size() const noexcept
	{
		return this->m_sth.element_count();
	}
	size_type max_size() const noexcept
	{
		return this->m_sth.max_size();
	}
	size_type capacity() const noexcept
	{
		return this->m_sth.capacity();
	}
	void reserve(size_type res_arg)
	{
		const auto cnt = this->size();
		const auto cap_new = this->m_sth.round_up_capacity(noadl::max(cnt, res_arg));
		// If the storage is shared with other hashmaps, force rellocation to prevent copy-on-write upon modification.
		if((this->unique() != false) && (this->capacity() >= cap_new)) {
			return;
		}
		this->do_reallocate(0, 0, this->slot_count(), cap_new);
		ROCKET_ASSERT(this->capacity() >= res_arg);
	}
	void shrink_to_fit()
	{
		const auto cnt = this->size();
		const auto cap_min = this->m_sth.round_up_capacity(cnt);
		// Don't increase memory usage.
		if((this->unique() == false) || (this->capacity() <= cap_min)) {
			return;
		}
		this->do_reallocate(0, 0, cnt, cnt);
		ROCKET_ASSERT(this->capacity() <= cap_min);
	}
	void clear() noexcept
	{
		if(this->unique() == false) {
			this->do_deallocate();
			return;
		}
		this->m_sth.erase_range_unchecked(0, this->slot_count());
	}
	// N.B. This is a non-standard extension.
	bool unique() const noexcept
	{
		return this->m_sth.unique();
	}

	// hash policy
	// N.B. This is a non-standard extension.
	size_type slot_count() const noexcept
	{
		return this->m_sth.slot_count();
	}
	float load_factor() const noexcept
	{
		return static_cast<float>(static_cast<difference_type>(this->size())) / static_cast<float>(static_cast<difference_type>(this->slot_count()));
	}
	// N.B. The `constexpr` specifier is a non-standard extension.
	constexpr float max_load_factor() const noexcept
	{
		return 1.0f / static_cast<float>(static_cast<difference_type>(this->m_sth.max_load_factor_reciprocal));
	}
	void rehash(size_type n)
	{
		const auto cnt = this->size();
		this->do_reallocate(0, 0, this->slot_count(), noadl::max(cnt, n));
	}

	// 26.5.4.4, modifiers
	pair<iterator, bool> insert(const value_type &value)
	{
		this->do_reserve_more(1);
		const auto result = this->m_sth.keyed_emplace_unchecked(value.first, value);
		return ::std::make_pair(iterator(this->m_sth, result.first), false);
	}
	pair<iterator, bool> insert(value_type &&value)
	{
		this->do_reserve_more(1);
		const auto result = this->m_sth.keyed_emplace_unchecked(value.first, ::std::move(value));
		return ::std::make_pair(iterator(this->m_sth, result.first), false);
	}
	// N.B. The return type is a non-standard extension.
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	cow_hashmap & insert(inputT first, inputT last)
	{
		if(first == last) {
			return *this;
		}
		const auto dist = noadl::estimate_distance(first, last);
		if(dist == 0) {
			auto it = ::std::move(first);
			do {
				this->insert(*it);
			} while(++it != last);
			return *this;
		}
		this->do_reserve_more(dist);
		auto it = ::std::move(first);
		do {
			this->m_sth.keyed_emplace_unchecked(it->first, *it);
		} while(++it != last);
		return *this;
	}
	// N.B. The return type is a non-standard extension.
	cow_hashmap & insert(initializer_list<value_type> init)
	{
		return this->insert(init.begin(), init.end());
	}
	// N.B. The hint is ignored.
	iterator insert(const_iterator /*hint*/, const value_type &value)
	{
		return this->insert(value).first;
	}
	// N.B. The hint is ignored.
	iterator insert(const_iterator /*hint*/, value_type &&value)
	{
		return this->insert(::std::move(value)).first;
	}

	template<typename ...paramsT>
	pair<iterator, bool> try_emplace(const key_type &key, paramsT &&...params)
	{
		this->do_reserve_more(1);
		const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::piecewise_construct,
		                                                        ::std::forward_as_tuple(key), ::std::forward_as_tuple(::std::forward<paramsT>(params)...));
		return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
	}
	template<typename ...paramsT>
	pair<iterator, bool> try_emplace(key_type &&key, paramsT &&...params)
	{
		this->do_reserve_more(1);
		const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::piecewise_construct,
		                                                        ::std::forward_as_tuple(::std::move(key)), ::std::forward_as_tuple(::std::forward<paramsT>(params)...));
		return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
	}
	// N.B. The hint is ignored.
	template<typename ...paramsT>
	iterator try_emplace(const_iterator /*hint*/, const key_type &key, paramsT &&...params)
	{
		return this->try_emplace(key, ::std::forward<paramsT>(params)...).first;
	}
	// N.B. The hint is ignored.
	template<typename ...paramsT>
	iterator try_emplace(const_iterator /*hint*/, key_type &&key, paramsT &&...params)
	{
		return this->try_emplace(::std::move(key), ::std::forward<paramsT>(params)...).first;
	}

	template<typename yvalueT>
	pair<iterator, bool> insert_or_assign(const key_type &key, yvalueT &&yvalue)
	{
		this->do_reserve_more(1);
		const auto result = this->m_sth.keyed_emplace_unchecked(key, key, ::std::forward<yvalueT>(yvalue));
		if(result.second == false) {
			result.first->get()->second = ::std::forward<yvalueT>(yvalue);
		}
		return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
	}
	template<typename yvalueT>
	pair<iterator, bool> insert_or_assign(key_type &&key, yvalueT &&yvalue)
	{
		this->do_reserve_more(1);
		const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::move(key), ::std::forward<yvalueT>(yvalue));
		if(result.second == false) {
			result.first->get()->second = ::std::forward<yvalueT>(yvalue);
		}
		return ::std::make_pair(iterator(this->m_sth, result.first), result.second);
	}
	// N.B. The hint is ignored.
	template<typename yvalueT>
	pair<iterator, bool> insert_or_assign(const_iterator /*hint*/, const key_type &key, yvalueT &&yvalue)
	{
		return this->insert_or_assign(key, ::std::forward<yvalueT>(yvalue)).first;
	}
	// N.B. The hint is ignored.
	template<typename yvalueT>
	pair<iterator, bool> insert_or_assign(const_iterator /*hint*/, key_type &&key, yvalueT &&yvalue)
	{
		return this->insert_or_assign(::std::move(key), ::std::forward<yvalueT>(yvalue)).first;
	}

	// N.B. This function may throw `std::bad_alloc()`.
	iterator erase(const_iterator tfirst, const_iterator tlast)
	{
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->do_get_table());
		const auto tn = static_cast<size_type>(tlast.tell_owned_by(this->m_sth) - tfirst.tell());
		const auto slot = this->do_erase_no_bound_check(tpos, tn);
		return iterator(this->m_sth, slot);
	}
	// N.B. This function may throw `std::bad_alloc()`.
	iterator erase(const_iterator tfirst)
	{
		const auto tpos = static_cast<size_type>(tfirst.tell_owned_by(this->m_sth) - this->do_get_table());
		const auto slot = this->do_erase_no_bound_check(tpos, 1);
		return iterator(this->m_sth, details_cow_hashmap::need_adjust, slot);
	}
	// N.B. This function may throw `std::bad_alloc()`.
	// N.B. The return type differs from `std::unordered_map`.
	bool erase(const key_type &key)
	{
		const auto toff = this->m_sth.index_of_unchecked(key);
		if(toff < 0) {
			return false;
		}
		this->do_erase_no_bound_check(static_cast<size_type>(toff), 1);
		return true;
	}

	// map operations
	const_iterator find(const key_type &key) const
	{
		const auto toff = this->m_sth.index_of_unchecked(key);
		if(toff < 0) {
			return this->end();
		}
		const auto slot = this->do_get_table() + toff;
		return const_iterator(this->m_sth, slot);
	}
	iterator find(const key_type &key)
	{
		const auto toff = this->m_sth.index_of_unchecked(key);
		if(toff < 0) {
			return this->mut_end();
		}
		const auto slot = this->do_mut_table() + toff;
		return iterator(this->m_sth, slot);
	}
	// N.B. The return type differs from `std::unordered_map`.
	bool count(const key_type &key) const
	{
		const auto toff = this->m_sth.index_of_unchecked(key);
		if(toff < 0) {
			return false;
		}
		return true;
	}

	// N.B. This is a non-standard extension.
	const mapped_type * get(const key_type &key) const
	{
		const auto toff = this->m_sth.index_of_unchecked(key);
		if(toff < 0) {
			return nullptr;
		}
		const auto slot = this->do_get_table() + toff;
		const auto eptr = slot->get();
		return ::std::addressof(eptr->second);
	}
	// N.B. This is a non-standard extension.
	mapped_type * get(const key_type &key)
	{
		const auto toff = this->m_sth.index_of_unchecked(key);
		if(toff < 0) {
			return nullptr;
		}
		const auto slot = this->do_mut_table() + toff;
		const auto eptr = slot->get();
		return ::std::addressof(eptr->second);
	}

	// 26.5.4.3, element access
	mapped_type & operator[](const key_type &key)
	{
		this->do_reserve_more(1);
		const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::piecewise_construct,
		                                                        ::std::forward_as_tuple(key), ::std::forward_as_tuple());
		const auto slot = result.first;
		const auto eptr = slot->get();
		return eptr->second;
	}
	mapped_type & operator[](key_type &&key)
	{
		this->do_reserve_more(1);
		const auto result = this->m_sth.keyed_emplace_unchecked(key, ::std::piecewise_construct,
		                                                        ::std::forward_as_tuple(::std::move(key)), ::std::forward_as_tuple());
		const auto slot = result.first;
		const auto eptr = slot->get();
		return eptr->second;
	}
	const mapped_type & at(const key_type &key) const
	{
		const auto toff = this->m_sth.index_of_unchecked(key);
		if(toff < 0) {
			noadl::throw_out_of_range("cow_hashmap: The specified key does not exist in this hashmap.");
		}
		const auto slot = this->do_get_table() + toff;
		const auto eptr = slot->get();
		return eptr->second;
	}
	mapped_type & at(const key_type &key)
	{
		const auto toff = this->m_sth.index_of_unchecked(key);
		if(toff < 0) {
			noadl::throw_out_of_range("cow_hashmap: The specified key does not exist in this hashmap.");
		}
		const auto slot = this->do_mut_table() + toff;
		const auto eptr = slot->get();
		return eptr->second;
	}

	// N.B. This function is a non-standard extension.
	cow_hashmap & assign(const cow_hashmap &other) noexcept
	{
		this->m_sth.share_with(other.m_sth);
		return *this;
	}
	// N.B. This function is a non-standard extension.
	cow_hashmap & assign(cow_hashmap &&other) noexcept
	{
		this->m_sth.share_with(::std::move(other.m_sth));
		return *this;
	}
	// N.B. This function is a non-standard extension.
	cow_hashmap & assign(initializer_list<value_type> init)
	{
		this->clear();
		this->insert(init);
		return *this;
	}
	// N.B. This function is a non-standard extension.
	template<typename inputT, typename iterator_traits<inputT>::iterator_category * = nullptr>
	cow_hashmap & assign(inputT first, inputT last)
	{
		this->clear();
		this->insert(::std::move(first), ::std::move(last));
		return *this;
	}

	void swap(cow_hashmap &other) noexcept
	{
		allocator_swapper<allocator_type>()(this->m_sth.as_allocator(), other.m_sth.as_allocator());
		this->m_sth.exchange_with(other.m_sth);
	}

	// N.B. The return type differs from `std::unordered_map`.
	const allocator_type & get_allocator() const noexcept
	{
		return this->m_sth.as_allocator();
	}
	allocator_type & get_allocator() noexcept
	{
		return this->m_sth.as_allocator();
	}
	const hasher & hash_function() const noexcept
	{
		return this->m_sth.as_hasher();
	}
	hasher & hash_function() noexcept
	{
		return this->m_sth.as_hasher();
	}
	const key_equal & key_eq() const noexcept
	{
		return this->m_sth.as_key_equal();
	}
	key_equal & key_eq() noexcept
	{
		return this->m_sth.as_key_equal();
	}
};

template<typename ...paramsT>
inline void swap(cow_hashmap<paramsT...> &lhs, cow_hashmap<paramsT...> &rhs) noexcept
{
	lhs.swap(rhs);
}

template<typename ...paramsT>
inline typename cow_hashmap<paramsT...>::const_iterator begin(const cow_hashmap<paramsT...> &rhs) noexcept
{
	return rhs.begin();
}
template<typename ...paramsT>
inline typename cow_hashmap<paramsT...>::const_iterator end(const cow_hashmap<paramsT...> &rhs) noexcept
{
	return rhs.end();
}

template<typename ...paramsT>
inline typename cow_hashmap<paramsT...>::const_iterator cbegin(const cow_hashmap<paramsT...> &rhs) noexcept
{
	return rhs.cbegin();
}
template<typename ...paramsT>
inline typename cow_hashmap<paramsT...>::const_iterator cend(const cow_hashmap<paramsT...> &rhs) noexcept
{
	return rhs.cend();
}

}

#endif
