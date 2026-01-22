#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <bit>
#include <ios>
#include <tuple>
#include <memory>
#include <vector>
#include <variant>
#include <utility>
#include <ostream>
#include <fstream>
#include <iterator>
#include <optional>
#include <algorithm>
#include <filesystem>
#include <type_traits>

//┌────────────────────────────────────────────────────────────────────────────────┐
//│         _          _            _            _          _             _        │
//│        / /\       /\ \         /\ \         /\ \       /\ \     _    /\ \      │
//│       / /  \      \_\ \       /  \ \        \ \ \     /  \ \   /\_\ /  \ \     │
//│      / / /\ \__   /\__ \     / /\ \ \       /\ \_\   / /\ \ \_/ / // /\ \_\    │
//│     / / /\ \___\ / /_ \ \   / / /\ \_\     / /\/_/  / / /\ \___/ // / /\/_/    │
//│     \ \ \ \/___// / /\ \ \ / / /_/ / /    / / /    / / /  \/____// / / ______  │
//│      \ \ \     / / /  \/_// / /__\/ /    / / /    / / /    / / // / / /\_____\ │
//│  _    \ \ \   / / /      / / /_____/    / / /    / / /    / / // / /  \/____ / │
//│ /_/\__/ / /  / / /      / / /\ \ \  ___/ / /__  / / /    / / // / /_____/ / /  │
//│ \ \/___/ /  /_/ /      / / /  \ \ \/\__\/_/___\/ / /    / / // / /______\/ /   │
//│  \_____\/   \_\/       \/_/    \_\/\/_________/\/_/     \/_/ \/___________/    │
//│                                                                                │
//└────────────────────────────────────────────────────────────────────────────────┘

// assumption; utf-8 terminal code page
inline auto operator<<(std::ostream& os, char32_t code) noexcept -> decltype(os)
{
	char out[4]; short unit {0};

	if (code <= 0x00007F)
	{
		out[unit++] = static_cast<char>(code /* safe to truncate */);
	}
	else if (code <= 0x0007FF)
	{
		out[unit++] = static_cast<char>(0xC0 | ((code >> 06) & 0x1F));
		out[unit++] = static_cast<char>(0x80 | ((code >> 00) & 0x3F));
	}
	else if (code <= 0x00FFFF)
	{
		out[unit++] = static_cast<char>(0xE0 | ((code >> 12) & 0x0F));
		out[unit++] = static_cast<char>(0x80 | ((code >> 06) & 0x3F));
		out[unit++] = static_cast<char>(0x80 | ((code >> 00) & 0x3F));
	}
	else if (code <= 0x10FFFF)
	{
		out[unit++] = static_cast<char>(0xF0 | ((code >> 18) & 0x07));
		out[unit++] = static_cast<char>(0x80 | ((code >> 12) & 0x3F));
		out[unit++] = static_cast<char>(0x80 | ((code >> 06) & 0x3F));
		out[unit++] = static_cast<char>(0x80 | ((code >> 00) & 0x3F));
	}
	return os.write(out, unit);
}

namespace utf
{

template <typename T, size_t N> using get_arg_t // extracts nth template argument from given T; amazingly useful
=
typename decltype([]<template <typename...> typename Type, typename... Args> (Type<Args...>*) consteval noexcept
->
std::type_identity<std::tuple_element_t<N, std::tuple<Args...>>>{ return {}; } (static_cast<T*>(nullptr)))::type;

//┌──────────────────────────────────────────────────────────────┐
//│ special thanks to facebook's folly::FBString.                │
//│                                                              │
//│ SSO mode uses every bytes of heap string struct using union  │
//│ this was achievable thanks to the very clever memory layout. │
//│                                                              │
//│ for more, watch https://www.youtube.com/watch?v=kPR8h4-qZdk. │
//└──────────────────────────────────────────────────────────────┘

#define COPY_ASSIGNMENT(T) constexpr auto operator=(const T& rhs) noexcept -> T&
#define MOVE_ASSIGNMENT(T) constexpr auto operator=(T&& rhs) noexcept -> T&

#define COPY_CONSTRUCTOR(T) constexpr T(const T& other) noexcept
#define MOVE_CONSTRUCTOR(T) constexpr T(T&& other) noexcept

//┌───────┬───────┬────────────┬─────────────────┐
//│ class │ owns? │ null-term? │ use-after-free? │
//├───────┼───────┼────────────┼─────────────────┤
//│ [str] │   T   │   always   │      safe       │
//├───────┼───────┼────────────┼─────────────────┤
//│ [txt] │   F   │   maybe?   │      [UB]       │
//└───────┴───────┴────────────┴─────────────────┘

/* owns str */ template <typename Codec, typename Alloc = std::allocator<typename Codec::T>> class str;
/* str view */ template <typename Codec /* slice is a not-owning view of ptr<const unit> */> class txt;

#define __OWNED__(name) const str<Other, Arena>& name
#define __SLICE__(name) const txt<Other /*##*/>  name
#define __EQSTR__(name) const T        (&name)[N]
#define __08STR__(name) const char8_t  (&name)[N]
#define __16STR__(name) const char16_t (&name)[N]
#define __32STR__(name) const char32_t (&name)[N]

template <size_t N> struct label { char str[N];
auto operator<=>(const label&) const = default;
constexpr label(const char (&str)[N]) noexcept
{ for (size_t i {0}; i < N; ++i) { this->str[i]
= str[i]; } assert(this->str[N - 1] == 0); } };

enum class range : uint8_t {N};
struct clamp { const size_t _;
inline constexpr /**/ operator
size_t() const { return _; } };
inline constexpr auto operator-
(range, size_t offset) noexcept
-> clamp { return { offset }; }

template <label> struct codec
{
	static_assert(false, "?");
};

// https://en.wikipedia.org/wiki/ASCII
template <> struct codec<"ASCII">
{
	static constexpr const bool variable {false};
	static constexpr const bool stateful {false};

	typedef char T;

	codec() = delete;

	static constexpr auto size(char32_t code) noexcept -> int8_t;
	static constexpr auto next(const T* data) noexcept -> int8_t;
	static constexpr auto back(const T* data) noexcept -> int8_t;

	static constexpr auto // transform a code point into code units.
	encode(const char32_t in, T* out, int8_t step) noexcept -> void;
	static constexpr auto // transform code units into a code point.
	decode(const T* in, char32_t& out, int8_t step) noexcept -> void;
};

// https://en.wikipedia.org/wiki/UTF-8
template <> struct codec<"UTF-8">
{
	static constexpr const bool variable {true};
	static constexpr const bool stateful {false};

	typedef char8_t T;

	codec() = delete;

	static constexpr auto size(char32_t code) noexcept -> int8_t;
	static constexpr auto next(const T* data) noexcept -> int8_t;
	static constexpr auto back(const T* data) noexcept -> int8_t;

	static constexpr auto // transform a code point into code units.
	encode(const char32_t in, T* out, int8_t step) noexcept -> void;
	static constexpr auto // transform code units into a code point.
	decode(const T* in, char32_t& out, int8_t step) noexcept -> void;
};

// https://en.wikipedia.org/wiki/UTF-16
template <> struct codec<"UTF-16">
{
	static constexpr const bool variable {true};
	static constexpr const bool stateful {false};

	typedef char16_t T;

	codec() = delete;

	static constexpr auto size(char32_t code) noexcept -> int8_t;
	static constexpr auto next(const T* data) noexcept -> int8_t;
	static constexpr auto back(const T* data) noexcept -> int8_t;

	static constexpr auto // transform a code point into code units.
	encode(const char32_t in, T* out, int8_t step) noexcept -> void;
	static constexpr auto // transform code units into a code point.
	decode(const T* in, char32_t& out, int8_t step) noexcept -> void;
};

// https://en.wikipedia.org/wiki/UTF-32
template <> struct codec<"UTF-32">
{
	static constexpr const bool variable {false};
	static constexpr const bool stateful {false};

	typedef char32_t T;

	codec() = delete;

	static constexpr auto size(char32_t code) noexcept -> int8_t;
	static constexpr auto next(const T* data) noexcept -> int8_t;
	static constexpr auto back(const T* data) noexcept -> int8_t;

	static constexpr auto // transform a code point into code units.
	encode(const char32_t in, T* out, int8_t step) noexcept -> void;
	static constexpr auto // transform code units into a code point.
	decode(const T* in, char32_t& out, int8_t step) noexcept -> void;
};

template <typename Class /* CRTP core */> class API
{
	template <typename> friend class API;

	   using Codec = get_arg_t<Class, 0>;
	// using Alloc = get_arg_t<Class, 1>;

	using T = typename Codec::T;

	//┌────────────────────────────────────────────────┐
	//│ note: helper funcs are not encapsulated within │
	//│       and it was deliberate; to cut bin bloat. │
	//└────────────────────────────────────────────────┘

	template <typename LHS, typename RHS> class concat;

	class const_forward_iterator;
	class const_reverse_iterator;

	constexpr auto head() const noexcept -> const T*;
	constexpr auto tail() const noexcept -> const T*;

public:
	
	// returns the number of code units, excluding NULL-TERMINATOR.
	constexpr auto size() const noexcept -> size_t;
	// returns the number of code points, excluding NULL-TERMINATOR.
	constexpr auto length() const noexcept -> size_t;

	// *self explanatory* returns whether or not it starts with *parameter*.
	template <typename Other, typename Arena>
	constexpr auto starts_with(__OWNED__(value)) const noexcept -> bool;
	template <typename Other /* can't own */>
	constexpr auto starts_with(__SLICE__(value)) const noexcept -> bool;
	template <size_t                       N>
	constexpr auto starts_with(__EQSTR__(value)) const noexcept -> bool requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto starts_with(__08STR__(value)) const noexcept -> bool /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto starts_with(__16STR__(value)) const noexcept -> bool /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto starts_with(__32STR__(value)) const noexcept -> bool /* encoding of char32_t is trivial */;

	// *self explanatory* returns whether or not it ends with *parameter*.
	template <typename Other, typename Arena>
	constexpr auto ends_with(__OWNED__(value)) const noexcept -> bool;
	template <typename Other /* can't own */>
	constexpr auto ends_with(__SLICE__(value)) const noexcept -> bool;
	template <size_t                       N>
	constexpr auto ends_with(__EQSTR__(value)) const noexcept -> bool requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto ends_with(__08STR__(value)) const noexcept -> bool /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto ends_with(__16STR__(value)) const noexcept -> bool /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto ends_with(__32STR__(value)) const noexcept -> bool /* encoding of char32_t is trivial */;

	// *self explanatory* returns whether or not it contains *parameter*.
	template <typename Other, typename Arena>
	constexpr auto contains(__OWNED__(value)) const noexcept -> size_t;
	template <typename Other /* can't own */>
	constexpr auto contains(__SLICE__(value)) const noexcept -> size_t;
	template <size_t                       N>
	constexpr auto contains(__EQSTR__(value)) const noexcept -> size_t requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto contains(__08STR__(value)) const noexcept -> size_t /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto contains(__16STR__(value)) const noexcept -> size_t /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto contains(__32STR__(value)) const noexcept -> size_t /* encoding of char32_t is trivial */;

	// returns a list of string slice, of which is a product of split aka division.
	template <typename Other, typename Arena>
	constexpr auto split(__OWNED__(value)) const noexcept -> std::vector<txt<Codec>>;
	template <typename Other /* can't own */>
	constexpr auto split(__SLICE__(value)) const noexcept -> std::vector<txt<Codec>>;
	template <size_t                       N>
	constexpr auto split(__EQSTR__(value)) const noexcept -> std::vector<txt<Codec>> requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto split(__08STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto split(__16STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto split(__32STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char32_t is trivial */;

	// returns a list of string slice, of which is a product of search occurrence.
	template <typename Other, typename Arena>
	constexpr auto match(__OWNED__(value)) const noexcept -> std::vector<txt<Codec>>;
	template <typename Other /* can't own */>
	constexpr auto match(__SLICE__(value)) const noexcept -> std::vector<txt<Codec>>;
	template <size_t                       N>
	constexpr auto match(__EQSTR__(value)) const noexcept -> std::vector<txt<Codec>> requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto match(__08STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto match(__16STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto match(__32STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char32_t is trivial */;

	// returns a slice, of which is a product of substring. N is a sentinel value.
	constexpr auto substr(clamp  start, clamp  until) const noexcept -> txt<Codec>;
	constexpr auto substr(clamp  start, range  until) const noexcept -> txt<Codec>;
	constexpr auto substr(size_t start, clamp  until) const noexcept -> txt<Codec>;
	constexpr auto substr(size_t start, range  until) const noexcept -> txt<Codec>;
	constexpr auto substr(size_t start, size_t until) const noexcept -> txt<Codec>;

	// iterator

	constexpr auto begin() const noexcept -> const_forward_iterator;
	constexpr auto end() const noexcept -> const_forward_iterator;

	constexpr auto rbegin() const noexcept -> const_reverse_iterator;
	constexpr auto rend() const noexcept -> const_reverse_iterator;

	// operators

	constexpr auto operator[](size_t value) const noexcept -> decltype(auto);
	constexpr auto operator[](size_t value) /*&*/ noexcept -> decltype(auto);

	template <typename Other, typename Arena>
	constexpr auto operator==(__OWNED__(rhs)) const noexcept -> bool;
	template <typename Other /* can't own */>
	constexpr auto operator==(__SLICE__(rhs)) const noexcept -> bool;
	template <size_t                       N>
	constexpr auto operator==(__EQSTR__(rhs)) const noexcept -> bool requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto operator==(__08STR__(rhs)) const noexcept -> bool /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto operator==(__16STR__(rhs)) const noexcept -> bool /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto operator==(__32STR__(rhs)) const noexcept -> bool /* encoding of char32_t is trivial */;

	template <typename Other, typename Arena>
	constexpr auto operator!=(__OWNED__(rhs)) const noexcept -> bool;
	template <typename Other /* can't own */>
	constexpr auto operator!=(__SLICE__(rhs)) const noexcept -> bool;
	template <size_t                       N>
	constexpr auto operator!=(__EQSTR__(rhs)) const noexcept -> bool requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto operator!=(__08STR__(rhs)) const noexcept -> bool /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto operator!=(__16STR__(rhs)) const noexcept -> bool /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto operator!=(__32STR__(rhs)) const noexcept -> bool /* encoding of char32_t is trivial */;

	template <typename Other, typename Arena>
	constexpr auto operator+(__OWNED__(rhs)) const noexcept -> concat<txt<Codec>, txt<Other>>;
	template <typename Other /* can't own */>
	constexpr auto operator+(__SLICE__(rhs)) const noexcept -> concat<txt<Codec>, txt<Other>>;
	template <size_t                       N>
	constexpr auto operator+(__EQSTR__(rhs)) const noexcept -> concat<txt<Codec>, txt<Codec>> requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto operator+(__08STR__(rhs)) const noexcept -> concat<txt<Codec>, txt<codec<"UTF-8">>> /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto operator+(__16STR__(rhs)) const noexcept -> concat<txt<Codec>, txt<codec<"UTF-16">>> /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto operator+(__32STR__(rhs)) const noexcept -> concat<txt<Codec>, txt<codec<"UTF-32">>> /* encoding of char32_t is trivial */;

	// reverse operators

	template <size_t                       N,
	          typename Other, typename Arena>
	friend constexpr auto operator+(__08STR__(lhs), __OWNED__(rhs)) noexcept -> concat<txt<codec<"UTF-8">>, txt<Other>>
	{
		return {{lhs}, {rhs}};
	}

	template <size_t                       N,
	          typename Other, typename Arena>
	friend constexpr auto operator+(__16STR__(lhs), __OWNED__(rhs)) noexcept -> concat<txt<codec<"UTF-16">>, txt<Other>>
	{
		return {{lhs}, {rhs}};
	}

	template <size_t                       N,
	          typename Other, typename Arena>
	friend constexpr auto operator+(__32STR__(lhs), __OWNED__(rhs)) noexcept -> concat<txt<codec<"UTF-32">>, txt<Other>>
	{
		return {{lhs}, {rhs}};
	}

	template <size_t                       N,
	          typename Other /* can't own */>
	friend constexpr auto operator+(__08STR__(lhs), __SLICE__(rhs)) noexcept -> concat<txt<codec<"UTF-8">>, txt<Other>>
	{
		return {{lhs}, {rhs}};
	}

	template <size_t                       N,
	          typename Other /* can't own */>
	friend constexpr auto operator+(__16STR__(lhs), __SLICE__(rhs)) noexcept -> concat<txt<codec<"UTF-16">>, txt<Other>>
	{
		return {{lhs}, {rhs}};
	}

	template <size_t                       N,
	          typename Other /* can't own */>
	friend constexpr auto operator+(__32STR__(lhs), __SLICE__(rhs)) noexcept -> concat<txt<codec<"UTF-32">>, txt<Other>>
	{
		return {{lhs}, {rhs}};
	}

	template <size_t                       N,
	          typename   LHS, typename   RHS>
	friend constexpr auto operator+(__08STR__(lhs), const concat<LHS, RHS>& rhs) noexcept -> concat<txt<codec<"UTF-8">>, concat<LHS, RHS>>
	{
		return {{lhs}, {rhs}};
	}

	template <size_t                       N,
	          typename   LHS, typename   RHS>
	friend constexpr auto operator+(__16STR__(lhs), const concat<LHS, RHS>& rhs) noexcept -> concat<txt<codec<"UTF-16">>, concat<LHS, RHS>>
	{
		return {{lhs}, {rhs}};
	}

	template <size_t                       N,
	          typename   LHS, typename   RHS>
	friend constexpr auto operator+(__32STR__(lhs), const concat<LHS, RHS>& rhs) noexcept -> concat<txt<codec<"UTF-32">>, concat<LHS, RHS>>
	{
		return {{lhs}, {rhs}};
	}

private:

	template <typename LHS, typename RHS> class concat
	{
		using blank_t = txt<codec<"ASCII">>;

		const LHS lhs;
		const RHS rhs;
		
		constexpr auto __for_each__(const auto&& fun) const noexcept -> void;

	public:

		constexpr concat
		(
			decltype(lhs) lhs,
			decltype(rhs) rhs
		)
		noexcept : lhs {lhs},
		           rhs {rhs}
		{
			// nothing to do...
		}

		template <typename Other, typename Arena>
		constexpr operator str<Other, Arena>() const noexcept;

		// operators

		template <typename Other, typename Arena>
		constexpr auto operator=(__OWNED__(rhs)) noexcept -> concat<blank_t, txt<Other>>;

		template <typename Other /* can't own */>
		constexpr auto operator=(__SLICE__(rhs)) noexcept -> concat<blank_t, txt<Other>>;

		template <size_t                       N>
		constexpr auto operator=(__08STR__(rhs)) noexcept -> concat<blank_t, txt<codec<"UTF-8">>>;

		template <size_t                       N>
		constexpr auto operator=(__16STR__(rhs)) noexcept -> concat<blank_t, txt<codec<"UTF-16">>>;

		template <size_t                       N>
		constexpr auto operator=(__32STR__(rhs)) noexcept -> concat<blank_t, txt<codec<"UTF-32">>>;

		template <typename Other, typename Arena>
		constexpr auto operator+(__OWNED__(rhs)) noexcept -> concat<concat, txt<Other>>;

		template <typename Other /* can't own */>
		constexpr auto operator+(__SLICE__(rhs)) noexcept -> concat<concat, txt<Other>>;

		template <size_t                       N>
		constexpr auto operator+(__08STR__(rhs)) noexcept -> concat<concat, txt<codec<"UTF-8">>>;

		template <size_t                       N>
		constexpr auto operator+(__16STR__(rhs)) noexcept -> concat<concat, txt<codec<"UTF-16">>>;

		template <size_t                       N>
		constexpr auto operator+(__32STR__(rhs)) noexcept -> concat<concat, txt<codec<"UTF-32">>>;
	};

	class const_forward_iterator
	{
		const T* ptr;

	public:

		using iterator_category = std::bidirectional_iterator_tag;
		using iterator_concept = std::bidirectional_iterator_tag;
		// for STL algorithms; itrator trait
		using difference_type = ptrdiff_t;
		using value_type = char32_t;
		using reference = char32_t;

		constexpr const_forward_iterator
		(
			decltype(ptr) ptr
		)
		noexcept : ptr {ptr} {}

		// stl compat; must be default constructible
		constexpr  const_forward_iterator() noexcept = default;
		constexpr ~const_forward_iterator() noexcept = default;

		constexpr auto operator*() const noexcept -> value_type;

		constexpr auto operator++(   ) noexcept -> const_forward_iterator&;
		constexpr auto operator++(int) noexcept -> const_forward_iterator;

		constexpr auto operator--(   ) noexcept -> const_forward_iterator&;
		constexpr auto operator--(int) noexcept -> const_forward_iterator;

		constexpr auto operator+(size_t value) noexcept -> const_forward_iterator;
		constexpr auto operator-(size_t value) noexcept -> const_forward_iterator;

		constexpr auto operator+=(size_t value) noexcept -> const_forward_iterator&;
		constexpr auto operator-=(size_t value) noexcept -> const_forward_iterator&;

		constexpr auto operator==(const const_forward_iterator& rhs) const noexcept -> bool = default;
		constexpr auto operator!=(const const_forward_iterator& rhs) const noexcept -> bool = default;
	};

	class const_reverse_iterator
	{
		const T* ptr;

	public:

		using iterator_category = std::bidirectional_iterator_tag;
		using iterator_concept = std::bidirectional_iterator_tag;
		// for STL algorithms; itrator trait
		using difference_type = ptrdiff_t;
		using value_type = char32_t;
		using reference = char32_t;

		constexpr const_reverse_iterator
		(
			decltype(ptr) ptr
		)
		noexcept : ptr {ptr} {}

		// stl compat; must be default constructible
		constexpr  const_reverse_iterator() noexcept = default;
		constexpr ~const_reverse_iterator() noexcept = default;

		constexpr auto operator*() const noexcept -> value_type;

		constexpr auto operator++(   ) noexcept -> const_reverse_iterator&;
		constexpr auto operator++(int) noexcept -> const_reverse_iterator;

		constexpr auto operator--(   ) noexcept -> const_reverse_iterator&;
		constexpr auto operator--(int) noexcept -> const_reverse_iterator;

		constexpr auto operator+(size_t value) noexcept -> const_reverse_iterator;
		constexpr auto operator-(size_t value) noexcept -> const_reverse_iterator;

		constexpr auto operator+=(size_t value) noexcept -> const_reverse_iterator&;
		constexpr auto operator-=(size_t value) noexcept -> const_reverse_iterator&;

		constexpr auto operator==(const const_reverse_iterator& rhs) const noexcept -> bool = default;
		constexpr auto operator!=(const const_reverse_iterator& rhs) const noexcept -> bool = default;
	};
};

template <typename Codec, typename Alloc> class str : public API<str<Codec, Alloc>>
{
	template <typename /*none*/> friend class API;
	template <typename,typename> friend class str;
	template <typename /*none*/> friend class txt;

	using allocator = std::allocator_traits<Alloc>;

	using T = typename Codec::T;

	#define IS_BIG          \
	(                       \
	    std::endian::native \
	             !=         \
	    std::endian::little \
	)                       \

	enum mode_t : uint8_t
	{
		SMALL = IS_BIG ? 0b0000000'0 : 0b0'0000000,
		LARGE = IS_BIG ? 0b0000000'1 : 0b1'0000000,
	};

	struct buffer
	{
		T* head;
		T* tail;
		size_t size : (sizeof(size_t) * 8) - (sizeof(mode_t) * 8);
		size_t meta : (sizeof(mode_t) * 8) - (sizeof(mode_t) * 0);

		constexpr operator const T*() const noexcept;
		constexpr operator /*&*/ T*() /*&*/ noexcept;
	};

	static constexpr const uint8_t MAX {(sizeof(buffer) - 1) / (sizeof(T))};
	static constexpr const uint8_t RMB {(sizeof(buffer) - 1) * (    1    )};
	static constexpr const uint8_t SFT {IS_BIG ? (    1    ) : (    0    )};
	static constexpr const uint8_t MSK {IS_BIG ? 0b0000000'1 : 0b1'0000000};

	#undef IS_BIG

	//┌───────────────────────────┐
	//│           small           │
	//├──────┬──────┬──────┬──────┤
	//│ head │ tail │ size │ meta │
	//├──────┴──────┴──────┴──────┤
	//│           bytes           │
	//└───────────────────────────┘

	struct storage : public Alloc
	{
		union
		{
			typedef T chunk_t;
			
			buffer large;

			chunk_t small
			[sizeof(buffer) / sizeof(chunk_t)];

			uint8_t bytes
			[sizeof(buffer) / sizeof(uint8_t)];
		}
		__union__ { .bytes {} };

		constexpr  storage() noexcept;
		constexpr ~storage() noexcept;

		// single source of truth; category.
		constexpr auto mode() const noexcept -> mode_t;
		// single source of truth; category.
		constexpr auto mode() /*&*/ noexcept -> mode_t;
	};

	// returns ptr to the first element.
	constexpr auto __head__() const noexcept -> const T*;
	// returns ptr to the first element.
	constexpr auto __head__() /*&*/ noexcept -> /*&*/ T*;

	// returns ptr to the one-past-the-last element.
	constexpr auto __tail__() const noexcept -> const T*;
	// returns ptr to the one-past-the-last element.
	constexpr auto __tail__() /*&*/ noexcept -> /*&*/ T*;

	// fixes invariant; use it after internal manipulation.
	constexpr auto __size__(size_t value) noexcept -> void;

	storage store;

	class reader;
	friend reader;
	class writer;
	friend writer;

	class forward_iterator;
	friend forward_iterator;
	class reverse_iterator;
	friend reverse_iterator;

	static_assert(sizeof(storage) == sizeof(buffer));
	static_assert(std::is_standard_layout_v<buffer>);
	static_assert(std::is_trivially_copyable_v<buffer>);
	static_assert(sizeof(buffer) == sizeof(size_t) * 3);
	static_assert(alignof(buffer) == alignof(size_t) * 1);
	static_assert(offsetof(buffer, head) == sizeof(size_t) * 0);
	static_assert(offsetof(buffer, tail) == sizeof(size_t) * 1);

	template <typename Other>
	constexpr auto __assign__(const typename Other::T* rhs_0,
	                          const typename Other::T* rhs_N) noexcept -> void;

	template <typename Other>
	constexpr auto __concat__(const typename Other::T* rhs_0,
	                          const typename Other::T* rhs_N) noexcept -> void;

	// 2x capacity growth strategy; returns next ptr
	constexpr auto __insert__(T* dest, char32_t code, int8_t step) noexcept -> T*;

public:

	// optional; returns the content of a file with CRLF/CR to LF normalization.
	template <typename STRING> friend auto fileof(const STRING& path) noexcept
	->
	std::optional<std::variant
	<
		str<codec<"UTF-8">>
		,
		str<codec<"UTF-16">>
		,
		str<codec<"UTF-32">>
	>>;

	[[deprecated]] constexpr operator const T*() const noexcept;
	[[deprecated]] constexpr operator /*&*/ T*() /*&*/ noexcept;

	// rule of 5

	COPY_CONSTRUCTOR(str);
	MOVE_CONSTRUCTOR(str);

	COPY_ASSIGNMENT(str);
	MOVE_ASSIGNMENT(str);

	// constructors

	constexpr  str() noexcept = default;
	constexpr ~str() noexcept = default;

	template <typename Other, typename Arena>
	constexpr str(__OWNED__(str)) noexcept;
	template <typename Other /* can't own */>
	constexpr str(__SLICE__(str)) noexcept;
	template <size_t                       N>
	constexpr str(__EQSTR__(str)) noexcept requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr str(__08STR__(str)) noexcept /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr str(__16STR__(str)) noexcept /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr str(__32STR__(str)) noexcept /* encoding of char32_t is trivial */;

	// returns the number of code units it can hold, excluding NULL-TERMINATOR.
	constexpr auto capacity(/* getter */) const noexcept -> size_t;
	// changes the number of code units it can hold, excluding NULL-TERMINATOR.
	constexpr auto capacity(size_t value) /*&*/ noexcept -> void;

	// iterator

	using API<str<Codec, Alloc>>::begin; // fix; name hiding
	using API<str<Codec, Alloc>>::end; // fix; name hiding

	constexpr auto begin() /*&*/ noexcept -> forward_iterator;
	constexpr auto end() /*&*/ noexcept -> forward_iterator;

	using API<str<Codec, Alloc>>::rbegin; // fix; name hiding
	using API<str<Codec, Alloc>>::rend; // fix; name hiding

	constexpr auto rbegin() /*&*/ noexcept -> reverse_iterator;
	constexpr auto rend() /*&*/ noexcept -> reverse_iterator;

	// operators

	template <typename Other, typename Arena>
	constexpr auto operator=(__OWNED__(rhs))& noexcept -> str&;
	template <typename Other /* can't own */>
	constexpr auto operator=(__SLICE__(rhs))& noexcept -> str&;
	template <size_t                       N>
	constexpr auto operator=(__EQSTR__(rhs))& noexcept -> str& requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto operator=(__08STR__(rhs))& noexcept -> str& /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto operator=(__16STR__(rhs))& noexcept -> str& /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto operator=(__32STR__(rhs))& noexcept -> str& /* encoding of char32_t is trivial */;

	template <typename Other, typename Arena>
	constexpr auto operator=(__OWNED__(rhs))&& noexcept -> str&&;
	template <typename Other /* can't own */>
	constexpr auto operator=(__SLICE__(rhs))&& noexcept -> str&&;
	template <size_t                       N>
	constexpr auto operator=(__EQSTR__(rhs))&& noexcept -> str&& requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto operator=(__08STR__(rhs))&& noexcept -> str&& /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto operator=(__16STR__(rhs))&& noexcept -> str&& /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto operator=(__32STR__(rhs))&& noexcept -> str&& /* encoding of char32_t is trivial */;

	template <typename Other, typename Arena>
	constexpr auto operator+=(__OWNED__(rhs))& noexcept -> str&;
	template <typename Other /* can't own */>
	constexpr auto operator+=(__SLICE__(rhs))& noexcept -> str&;
	template <size_t                       N>
	constexpr auto operator+=(__EQSTR__(rhs))& noexcept -> str& requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto operator+=(__08STR__(rhs))& noexcept -> str& /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto operator+=(__16STR__(rhs))& noexcept -> str& /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto operator+=(__32STR__(rhs))& noexcept -> str& /* encoding of char32_t is trivial */;

	template <typename Other, typename Arena>
	constexpr auto operator+=(__OWNED__(rhs))&& noexcept -> str&&;
	template <typename Other /* can't own */>
	constexpr auto operator+=(__SLICE__(rhs))&& noexcept -> str&&;
	template <size_t                       N>
	constexpr auto operator+=(__EQSTR__(rhs))&& noexcept -> str&& requires (std::is_same_v<T, char>);
	template <size_t                       N>
	constexpr auto operator+=(__08STR__(rhs))&& noexcept -> str&& /* encoding of char8_t is trivial */;
	template <size_t                       N>
	constexpr auto operator+=(__16STR__(rhs))&& noexcept -> str&& /* encoding of char16_t is trivial */;
	template <size_t                       N>
	constexpr auto operator+=(__32STR__(rhs))&& noexcept -> str&& /* encoding of char32_t is trivial */;

private:

	class reader
	{
		const str* src;
		const size_t arg;

	public:

		constexpr reader
		(
			decltype(src) src,
			decltype(arg) arg
		)
		noexcept : src {src},
		           arg {arg}
		{
			// nothing to do...
		}

		[[nodiscard]] constexpr operator char32_t() const noexcept;

		constexpr auto operator==(char32_t code) const noexcept -> bool;
		constexpr auto operator!=(char32_t code) const noexcept -> bool;
	};

	class writer
	{
		/*&*/ str* src;
		const size_t arg;

	public:

		constexpr writer
		(
			decltype(src) src,
			decltype(arg) arg
		)
		noexcept : src {src},
		           arg {arg}
		{
			// nothing to do...
		}

		constexpr auto operator=(char32_t code) noexcept -> writer&;
		
		[[nodiscard]] constexpr operator char32_t() const noexcept;

		constexpr auto operator==(char32_t code) const noexcept -> bool;
		constexpr auto operator!=(char32_t code) const noexcept -> bool;
	};

	class forward_iterator
	{
		using S = str<Codec, Alloc>;

		/*&&&*/ S*        src; // ← cmp
		mutable T*        ptr;
		mutable ptrdiff_t dif; // ← cmp

		class proxy
		{
			using S = forward_iterator;

			const S* const src;
			/*&*/ T* /*&*/ ptr;

		public:

			constexpr proxy
			(
				decltype(src) src,
				decltype(ptr) ptr
			)
			noexcept : src {src},
			           ptr {ptr} {}

			[[nodiscard]] constexpr operator char32_t() const noexcept;
			constexpr auto operator=(char32_t code) noexcept -> proxy&;

			constexpr auto operator==(char32_t code) const noexcept -> bool;
			constexpr auto operator!=(char32_t code) const noexcept -> bool;
		};

	public:

		using iterator_category = std::bidirectional_iterator_tag;
		using iterator_concept = std::bidirectional_iterator_tag;
		using difference_type = ptrdiff_t;
		using value_type = proxy;
		using reference = proxy;

		constexpr forward_iterator
		(
			decltype(src) src,
			decltype(ptr) ptr,
			decltype(dif) dif
		)
		noexcept : src {src},
		           ptr {ptr},
		           dif {dif} {}

		// stl compat; must be default constructible
		constexpr  forward_iterator() noexcept = default;
		constexpr ~forward_iterator() noexcept = default;

		constexpr auto operator*() const noexcept -> value_type;

		constexpr auto operator++(   ) noexcept -> forward_iterator&;
		constexpr auto operator++(int) noexcept -> forward_iterator;

		constexpr auto operator--(   ) noexcept -> forward_iterator&;
		constexpr auto operator--(int) noexcept -> forward_iterator;

		constexpr auto operator+(size_t value) noexcept -> forward_iterator;
		constexpr auto operator-(size_t value) noexcept -> forward_iterator;

		constexpr auto operator+=(size_t value) noexcept -> forward_iterator&;
		constexpr auto operator-=(size_t value) noexcept -> forward_iterator&;

		constexpr auto operator==(const forward_iterator& rhs) const noexcept -> bool;
		constexpr auto operator!=(const forward_iterator& rhs) const noexcept -> bool;
	};

	class reverse_iterator
	{
		using S = str<Codec, Alloc>;

		/*&&&*/ S*        src; // ← cmp
		mutable T*        ptr;
		mutable ptrdiff_t dif; // ← cmp

		class proxy
		{
			using S = reverse_iterator;

			const S* const src;
			/*&*/ T* /*&*/ ptr;

		public:

			constexpr proxy
			(
				decltype(src) src,
				decltype(ptr) ptr
			)
			noexcept : src {src},
			           ptr {ptr} {}

			[[nodiscard]] constexpr operator char32_t() const noexcept;
			constexpr auto operator=(char32_t code) noexcept -> proxy&;

			constexpr auto operator==(char32_t code) const noexcept -> bool;
			constexpr auto operator!=(char32_t code) const noexcept -> bool;
		};

	public:

		using iterator_category = std::bidirectional_iterator_tag;
		using iterator_concept = std::bidirectional_iterator_tag;
		using difference_type = ptrdiff_t;
		using value_type = proxy;
		using reference = proxy;

		constexpr reverse_iterator
		(
			decltype(src) src,
			decltype(ptr) ptr,
			decltype(dif) dif
		)
		noexcept : src {src},
		           ptr {ptr},
		           dif {dif} {}

		// stl compat; must be default constructible
		constexpr  reverse_iterator() noexcept = default;
		constexpr ~reverse_iterator() noexcept = default;

		constexpr auto operator*() const noexcept -> value_type;

		constexpr auto operator++(   ) noexcept -> reverse_iterator&;
		constexpr auto operator++(int) noexcept -> reverse_iterator;

		constexpr auto operator--(   ) noexcept -> reverse_iterator&;
		constexpr auto operator--(int) noexcept -> reverse_iterator;

		constexpr auto operator+(size_t value) noexcept -> reverse_iterator;
		constexpr auto operator-(size_t value) noexcept -> reverse_iterator;

		constexpr auto operator+=(size_t value) noexcept -> reverse_iterator&;
		constexpr auto operator-=(size_t value) noexcept -> reverse_iterator&;

		constexpr auto operator==(const reverse_iterator& rhs) const noexcept -> bool;
		constexpr auto operator!=(const reverse_iterator& rhs) const noexcept -> bool;
	};
};

template <typename Codec /* can't own */> class txt : public API<txt<Codec /*##*/>>
{
	template <typename /*none*/> friend class API;
	template <typename,typename> friend class str;
	template <typename /*none*/> friend class txt;

	using T = typename Codec::T;

	const T* __head__;
	const T* __tail__;

	class reader;
	friend reader;
	class writer;
	friend writer;

public:

	constexpr txt
	(
		decltype(__head__) head,
		decltype(__tail__) tail
	)
	noexcept : __head__ {head},
	           __tail__ {tail}
	{
		// nothing to do...
	}

	template <size_t N>
	constexpr txt
	(
		const T (&str)[N]
	)
	noexcept : __head__ {&str[N - N]},
	           __tail__ {&str[N - 1]}
	{
		// nothing to do...
	}

	template <size_t N>
	constexpr txt
	(
		/*&*/ T (&str)[N]
	)
	noexcept : __head__ {&str[N - N]},
	           __tail__ {&str[N - 1]}
	{
		// maybe mark it as unsafe..?
	}

	template <typename Other,
	          typename Arena>
	constexpr txt
	(
		const str<Other, Arena>& str
	)
	noexcept : __head__ {str.__head__()},
	           __tail__ {str.__tail__()}
	{
		// nothing to do...
	}

	template <typename Other,
	          typename Arena>
	constexpr txt
	(
		/*&*/ str<Other, Arena>& str
	)
	noexcept : __head__ {str.__head__()},
	           __tail__ {str.__tail__()}
	{
		// maybe mark it as unsafe..?
	}

	COPY_CONSTRUCTOR(txt) = default;
	MOVE_CONSTRUCTOR(txt) = default;

	COPY_ASSIGNMENT(txt) = default;
	MOVE_ASSIGNMENT(txt) = default;

	constexpr  txt() noexcept = delete;
	constexpr ~txt() noexcept = default;

private:

	class reader
	{
		const txt* src;
		const size_t arg;

	public:

		constexpr reader
		(
			decltype(src) src,
			decltype(arg) arg
		)
		noexcept : src {src},
		           arg {arg}
		{
			// nothing to do...
		}

		[[nodiscard]] constexpr operator char32_t() const noexcept;

		constexpr auto operator==(char32_t code) const noexcept -> bool;
		constexpr auto operator!=(char32_t code) const noexcept -> bool;
	};

	class writer
	{
		/*&*/ txt* src;
		const size_t arg;

	public:

		constexpr writer
		(
			decltype(src) src,
			decltype(arg) arg
		)
		noexcept : src {src},
		           arg {arg}
		{
			// nothing to do...
		}

		// constexpr auto operator=(char32_t code) noexcept -> writer&;
		
		[[nodiscard]] constexpr operator char32_t() const noexcept;

		constexpr auto operator==(char32_t code) const noexcept -> bool;
		constexpr auto operator!=(char32_t code) const noexcept -> bool;
	};
};

namespace detail
{
	template <typename Codec>
	static constexpr auto __difcu__(const typename Codec::T* head, const typename Codec::T* tail) noexcept -> size_t;

	template <typename Codec>
	static constexpr auto __difcp__(const typename Codec::T* head, const typename Codec::T* tail) noexcept -> size_t;

	template <typename Codec,
	          typename Other>
	static constexpr auto __fcopy__(const typename Other::T* head, const typename Other::T* tail,
	                                                               /*&*/ typename Codec::T* dest) noexcept -> size_t;

	template <typename Codec,
	          typename Other>
	static constexpr auto __rcopy__(const typename Other::T* head, const typename Other::T* tail,
	                                                               /*&*/ typename Codec::T* dest) noexcept -> size_t;

	template <typename Codec,
	          typename Other>
	static constexpr auto __equal__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool;

	template <typename Codec,
	          typename Other>
	static constexpr auto __nqual__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool;

	template <typename Codec,
	          typename Other>
	static constexpr auto __swith__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool;

	template <typename Codec,
	          typename Other>
	static constexpr auto __ewith__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool;

	template <typename Codec,
	          typename Other>
	static constexpr auto __scan__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                               const typename Other::T* rhs_0, const typename Other::T* rhs_N,
	                                                               const auto& fun /* lambda E */) noexcept -> void;

	template <typename Codec,
	          typename Other>
	static constexpr auto __split__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> std::vector<txt<Codec>>;

	template <typename Codec,
	          typename Other>
	static constexpr auto __match__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> std::vector<txt<Codec>>;

	template <typename Codec>
	static constexpr auto __substr__(const typename Codec::T* head, const typename Codec::T* tail, clamp  start, clamp  until) noexcept -> txt<Codec>;

	template <typename Codec>
	static constexpr auto __substr__(const typename Codec::T* head, const typename Codec::T* tail, clamp  start, range  until) noexcept -> txt<Codec>;

	template <typename Codec>
	static constexpr auto __substr__(const typename Codec::T* head, const typename Codec::T* tail, size_t start, clamp  until) noexcept -> txt<Codec>;

	template <typename Codec>
	static constexpr auto __substr__(const typename Codec::T* head, const typename Codec::T* tail, size_t start, range  until) noexcept -> txt<Codec>;

	template <typename Codec>
	static constexpr auto __substr__(const typename Codec::T* head, const typename Codec::T* tail, size_t start, size_t until) noexcept -> txt<Codec>;
};

#pragma region iostream

template <typename Other, typename Arena> inline auto operator<<(std::ostream& os, __OWNED__(str)) noexcept -> decltype(os)
{
	for (const auto code : str) { ::operator<<(os, code); } return os;
}

template <typename Other /* can't own */> inline auto operator<<(std::ostream& os, __SLICE__(str)) noexcept -> decltype(os)
{
	for (const auto code : str) { ::operator<<(os, code); } return os;
}

#pragma endregion iostream
#pragma region codec<"ASCII">

constexpr auto codec<"ASCII">::size([[maybe_unused]] char32_t code) noexcept -> int8_t
{
	return 1;
}

constexpr auto codec<"ASCII">::next([[maybe_unused]] const T* data) noexcept -> int8_t
{
	return +1;
}

constexpr auto codec<"ASCII">::back([[maybe_unused]] const T* data) noexcept -> int8_t
{
	return -1;
}

constexpr auto codec<"ASCII">::encode(const char32_t in, T* out, int8_t step) noexcept -> void
{
	switch (step)
	{
		case +1:
		{
			out[+0] = static_cast<T>(in);
			break;
		}
		case -1:
		{
			out[-1] = static_cast<T>(in);
			break;
		}
		// invalid size; please check out the caller
		default: { assert(false); std::unreachable(); }
	}
}

constexpr auto codec<"ASCII">::decode(const T* in, char32_t& out, int8_t step) noexcept -> void
{
	switch (step)
	{
		case +1:
		{
			out = static_cast<char32_t>(in[+0]);
			break;
		}
		case -1:
		{
			out = static_cast<char32_t>(in[-1]);
			break;
		}
		// invalid size; please check out the caller
		default: { assert(false); std::unreachable(); }
	}
}

#pragma endregion codec<"ASCII">
#pragma region codec<"UTF-8">

constexpr auto codec<"UTF-8">::size([[maybe_unused]] char32_t code) noexcept -> int8_t
{
	const size_t N (std::bit_width
	(static_cast<uint32_t>(code)));

	//┌───────────────────────┐
	//│ U+000000 ... U+00007F │ -> 1 code unit
	//│ U+000080 ... U+0007FF │ -> 2 code unit
	//│ U+000800 ... U+00FFFF │ -> 3 code unit
	//│ U+010000 ... U+10FFFF │ -> 4 code unit
	//└───────────────────────┘

	return 1 + (8 <= N) + (12 <= N) + (17 <= N);
}

constexpr auto codec<"UTF-8">::next([[maybe_unused]] const T* data) noexcept -> int8_t
{
	constexpr static const int8_t TABLE[]
	{
		/*┌─────┬────────┬─────┬────────┐*/
		/*│ 0x0 │*/ 1, /*│ 0x1 │*/ 1, /*│*/
		/*│ 0x2 │*/ 1, /*│ 0x3 │*/ 1, /*│*/
		/*│ 0x4 │*/ 1, /*│ 0x5 │*/ 1, /*│*/
		/*│ 0x6 │*/ 1, /*│ 0x7 │*/ 1, /*│*/
		/*│ 0x8 │*/ 1, /*│ 0x9 │*/ 1, /*│*/
		/*│ 0xA │*/ 1, /*│ 0xB │*/ 1, /*│*/
		/*│ 0xC │*/ 2, /*│ 0xD │*/ 2, /*│*/
		/*│ 0xE │*/ 3, /*│ 0xF │*/ 4, /*│*/
		/*└─────┴────────┴─────┴────────┘*/
	};

	return TABLE[(data[0] >> 0x4) & 0x0F];
}

constexpr auto codec<"UTF-8">::back([[maybe_unused]] const T* data) noexcept -> int8_t
{
	int8_t i {-1};
	
	for (; (data[i] & 0xC0) == 0x80; --i) {}
	
	return i;
}

constexpr auto codec<"UTF-8">::encode(const char32_t in, T* out, int8_t step) noexcept -> void
{
	switch (step)
	{
		case +1:
		{
			out[+0] = static_cast<T>(in);
			break;
		}
		case -1:
		{
			out[-1] = static_cast<T>(in);
			break;
		}
		case +2:
		{
			out[+0] = 0xC0 | ((in >> 06) & 0x1F);
			out[+1] = 0x80 | ((in >> 00) & 0x3F);
			break;
		}
		case -2:
		{
			out[-2] = 0xC0 | ((in >> 06) & 0x1F);
			out[-1] = 0x80 | ((in >> 00) & 0x3F);
			break;
		}
		case +3:
		{
			out[+0] = 0xE0 | ((in >> 12) & 0x0F);
			out[+1] = 0x80 | ((in >> 06) & 0x3F);
			out[+2] = 0x80 | ((in >> 00) & 0x3F);
			break;
		}
		case -3:
		{
			out[-3] = 0xE0 | ((in >> 12) & 0x0F);
			out[-2] = 0x80 | ((in >> 06) & 0x3F);
			out[-1] = 0x80 | ((in >> 00) & 0x3F);
			break;
		}
		case +4:
		{
			out[+0] = 0xF0 | ((in >> 18) & 0x07);
			out[+1] = 0x80 | ((in >> 12) & 0x3F);
			out[+2] = 0x80 | ((in >> 06) & 0x3F);
			out[+3] = 0x80 | ((in >> 00) & 0x3F);
			break;
		}
		case -4:
		{
			out[-4] = 0xF0 | ((in >> 18) & 0x07);
			out[-3] = 0x80 | ((in >> 12) & 0x3F);
			out[-2] = 0x80 | ((in >> 06) & 0x3F);
			out[-1] = 0x80 | ((in >> 00) & 0x3F);
			break;
		}
		// invalid size; please check out the caller
		default: { assert(false); std::unreachable(); }
	}
}

constexpr auto codec<"UTF-8">::decode(const T* in, char32_t& out, int8_t step) noexcept -> void
{
	switch (step)
	{
		case +1:
		{
			out = static_cast<char32_t>(in[+0]);
			break;
		}
		case -1:
		{
			out = static_cast<char32_t>(in[-1]);
			break;
		}
		case +2:
		{
			out = ((in[+0] & 0x1F) << 06)
			      |
			      ((in[+1] & 0x3F) << 00);
			break;
		}
		case -2:
		{
			out = ((in[-2] & 0x1F) << 06)
			      |
			      ((in[-1] & 0x3F) << 00);
			break;
		}
		case +3:
		{
			out = ((in[+0] & 0x0F) << 12)
			      |
			      ((in[+1] & 0x3F) << 06)
			      |
			      ((in[+2] & 0x3F) << 00);
			break;
		}
		case -3:
		{
			out = ((in[-3] & 0x0F) << 12)
			      |
			      ((in[-2] & 0x3F) << 06)
			      |
			      ((in[-1] & 0x3F) << 00);
			break;
		}
		case +4:
		{
			out = ((in[+0] & 0x07) << 18)
			      |
			      ((in[+1] & 0x3F) << 12)
			      |
			      ((in[+2] & 0x3F) << 06)
			      |
			      ((in[+3] & 0x3F) << 00);
			break;
		}
		case -4:
		{
			out = ((in[-4] & 0x07) << 18)
			      |
			      ((in[-3] & 0x3F) << 12)
			      |
			      ((in[-2] & 0x3F) << 06)
			      |
			      ((in[-1] & 0x3F) << 00);
			break;
		}
		// invalid size; please check out the caller
		default: { assert(false); std::unreachable(); }
	}
}

#pragma endregion codec<"UTF-8">
#pragma region codec<"UTF-16">

constexpr auto codec<"UTF-16">::size([[maybe_unused]] char32_t code) noexcept -> int8_t
{
	//┌───────────────────────┐
	//│ U+000000 ... U+00D7FF │ -> 1 code unit
	//│ U+00E000 ... U+00FFFF │ -> 1 code unit
	//│ U+000000 ... U+10FFFF │ -> 2 code unit
	//└───────────────────────┘

	return 1 + (0xFFFF /* pair? */ < code);
}

constexpr auto codec<"UTF-16">::next([[maybe_unused]] const T* data) noexcept -> int8_t
{
	return +1 + ((data[0] >> 0xA) == 0x36);
}

constexpr auto codec<"UTF-16">::back([[maybe_unused]] const T* data) noexcept -> int8_t
{
	int8_t i {-1};
	
	for (; (data[i] >> 0xA) == 0x37; --i) {}
	
	return i;
}

constexpr auto codec<"UTF-16">::encode(const char32_t in, T* out, int8_t step) noexcept -> void
{
	switch (step)
	{
		case +1:
		{
			out[+0] = static_cast<T>(in);
			break;
		}
		case -1:
		{
			out[-1] = static_cast<T>(in);
			break;
		}
		case +2:
		{
			const char32_t code {in - 0x10000};
			out[+0] = 0xD800 | (code / 0x400);
			out[+1] = 0xDC00 | (code & 0x3FF);
			break;
		}
		case -2:
		{
			const char32_t code {in - 0x10000};
			out[-2] = 0xD800 | (code / 0x400);
			out[-1] = 0xDC00 | (code & 0x3FF);
			break;
		}
		default: std::unreachable();
	}
}

constexpr auto codec<"UTF-16">::decode(const T* in, char32_t& out, int8_t step) noexcept -> void
{
	switch (step)
	{
		case +1:
		{
			out = static_cast<char32_t>(in[+0]);
			break;
		}
		case -1:
		{
			out = static_cast<char32_t>(in[-1]);
			break;
		}
		case +2:
		{
			out = 0x10000 // supplymentary
			      |
			      ((in[+0] - 0xD800) << 10)
			      |
			      ((in[+1] - 0xDC00) << 00);
			break;
		}
		case -2:
		{
			out = 0x10000 // supplymentary
			      |
			      ((in[-2] - 0xD800) << 10)
			      |
			      ((in[-1] - 0xDC00) << 00);
			break;
		}
		// invalid size; please check out the caller
		default: { assert(false); std::unreachable(); }
	}
}

#pragma endregion codec<"UTF-16">
#pragma region codec<"UTF-32">

constexpr auto codec<"UTF-32">::size([[maybe_unused]] char32_t code) noexcept -> int8_t
{
	return 1;
}

constexpr auto codec<"UTF-32">::next([[maybe_unused]] const T* data) noexcept -> int8_t
{
	return +1;
}

constexpr auto codec<"UTF-32">::back([[maybe_unused]] const T* data) noexcept -> int8_t
{
	return -1;
}

constexpr auto codec<"UTF-32">::encode(const char32_t in, T* out, int8_t step) noexcept -> void
{
	switch (step)
	{
		case +1:
		{
			out[+0] = static_cast<T>(in);
			break;
		}
		case -1:
		{
			out[-1] = static_cast<T>(in);
			break;
		}
		// invalid size; please check out the caller
		default: { assert(false); std::unreachable(); }
	}
}

constexpr auto codec<"UTF-32">::decode(const T* in, char32_t& out, int8_t step) noexcept -> void
{
	switch (step)
	{
		case +1:
		{
			out = static_cast<char32_t>(in[+0]);
			break;
		}
		case -1:
		{
			out = static_cast<char32_t>(in[-1]);
			break;
		}
		// invalid size; please check out the caller
		default: { assert(false); std::unreachable(); }
	}
}

#pragma endregion codec<"UTF-32">
#pragma region CRTP

template <typename Class /* CRTP core */> constexpr auto API<Class>::head() const noexcept -> const T*
{
	if constexpr (requires { static_cast<const Class*>(this)->__head__(); })
	     return static_cast<const Class*>(this)->__head__();
	else return static_cast<const Class*>(this)->__head__  ;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::tail() const noexcept -> const T*
{
	if constexpr (requires { static_cast<const Class*>(this)->__tail__(); })
	     return static_cast<const Class*>(this)->__tail__();
	else return static_cast<const Class*>(this)->__tail__  ;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::size() const noexcept -> size_t
{
	return detail::__difcu__<Codec /*&*/>(this->head(), this->tail());
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::length() const noexcept -> size_t
{
	return detail::__difcp__<Codec /*&*/>(this->head(), this->tail());
}

template <typename Class /* CRTP core */>
template <typename Other, typename Arena> constexpr auto API<Class>::starts_with(__OWNED__(value)) const noexcept -> bool
{
	return detail::__swith__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Class /* CRTP core */>
template <typename Other /* can't own */> constexpr auto API<Class>::starts_with(__SLICE__(value)) const noexcept -> bool
{
	return detail::__swith__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::starts_with(__EQSTR__(value)) const noexcept -> bool requires (std::is_same_v<T, char>)
{
	return detail::__swith__<Codec, Codec>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::starts_with(__08STR__(value)) const noexcept -> bool /* encoding of char8_t is trivial */
{
	return detail::__swith__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::starts_with(__16STR__(value)) const noexcept -> bool /* encoding of char16_t is trivial */
{
	return detail::__swith__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::starts_with(__32STR__(value)) const noexcept -> bool /* encoding of char32_t is trivial */
{
	return detail::__swith__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <typename Other, typename Arena> constexpr auto API<Class>::ends_with(__OWNED__(value)) const noexcept -> bool
{
	return detail::__ewith__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Class /* CRTP core */>
template <typename Other /* can't own */> constexpr auto API<Class>::ends_with(__SLICE__(value)) const noexcept -> bool
{
	return detail::__ewith__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::ends_with(__EQSTR__(value)) const noexcept -> bool requires (std::is_same_v<T, char>)
{
	return detail::__ewith__<Codec, Codec>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::ends_with(__08STR__(value)) const noexcept -> bool /* encoding of char8_t is trivial */
{
	return detail::__ewith__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::ends_with(__16STR__(value)) const noexcept -> bool /* encoding of char16_t is trivial */
{
	return detail::__ewith__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::ends_with(__32STR__(value)) const noexcept -> bool /* encoding of char32_t is trivial */
{
	return detail::__ewith__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <typename Other, typename Arena> constexpr auto API<Class>::contains(__OWNED__(value)) const noexcept -> size_t
{
	return detail::__match__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail()).size();
}

template <typename Class /* CRTP core */>
template <typename Other /* can't own */> constexpr auto API<Class>::contains(__SLICE__(value)) const noexcept -> size_t
{
	return detail::__match__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail()).size();
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::contains(__EQSTR__(value)) const noexcept -> size_t requires (std::is_same_v<T, char>)
{
	return detail::__match__<Codec, Codec>(this->head(), this->tail(), &value[N - N], &value[N - 1]).size();
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::contains(__08STR__(value)) const noexcept -> size_t /* encoding of char8_t is trivial */
{
	return detail::__match__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &value[N - N], &value[N - 1]).size();
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::contains(__16STR__(value)) const noexcept -> size_t /* encoding of char16_t is trivial */
{
	return detail::__match__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &value[N - N], &value[N - 1]).size();
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::contains(__32STR__(value)) const noexcept -> size_t /* encoding of char32_t is trivial */
{
	return detail::__match__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &value[N - N], &value[N - 1]).size();
}

template <typename Class /* CRTP core */>
template <typename Other, typename Arena> constexpr auto API<Class>::split(__OWNED__(value)) const noexcept -> std::vector<txt<Codec>>
{
	return detail::__split__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Class /* CRTP core */>
template <typename Other /* can't own */> constexpr auto API<Class>::split(__SLICE__(value)) const noexcept -> std::vector<txt<Codec>>
{
	return detail::__split__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::split(__EQSTR__(value)) const noexcept -> std::vector<txt<Codec>> requires (std::is_same_v<T, char>)
{
	return detail::__split__<Codec, Codec>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::split(__08STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char8_t is trivial */
{
	return detail::__split__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::split(__16STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char16_t is trivial */
{
	return detail::__split__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::split(__32STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char32_t is trivial */
{
	return detail::__split__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <typename Other, typename Arena> constexpr auto API<Class>::match(__OWNED__(value)) const noexcept -> std::vector<txt<Codec>>
{
	return detail::__match__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Class /* CRTP core */>
template <typename Other /* can't own */> constexpr auto API<Class>::match(__SLICE__(value)) const noexcept -> std::vector<txt<Codec>>
{
	return detail::__match__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::match(__EQSTR__(value)) const noexcept -> std::vector<txt<Codec>> requires (std::is_same_v<T, char>)
{
	return detail::__match__<Codec, Codec>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::match(__08STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char8_t is trivial */
{
	return detail::__match__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::match(__16STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char16_t is trivial */
{
	return detail::__match__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::match(__32STR__(value)) const noexcept -> std::vector<txt<Codec>> /* encoding of char32_t is trivial */
{
	return detail::__match__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::substr(clamp  start, clamp  until) const noexcept -> txt<Codec>
{
	return detail::__substr__<Codec /*&*/>(this->head(), this->tail(), start, until);
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::substr(clamp  start, range  until) const noexcept -> txt<Codec>
{
	return detail::__substr__<Codec /*&*/>(this->head(), this->tail(), start, until);
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::substr(size_t start, clamp  until) const noexcept -> txt<Codec>
{
	return detail::__substr__<Codec /*&*/>(this->head(), this->tail(), start, until);
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::substr(size_t start, range  until) const noexcept -> txt<Codec>
{
	return detail::__substr__<Codec /*&*/>(this->head(), this->tail(), start, until);
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::substr(size_t start, size_t until) const noexcept -> txt<Codec>
{
	return detail::__substr__<Codec /*&*/>(this->head(), this->tail(), start, until);
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::begin() const noexcept -> const_forward_iterator
{
	return {this->head()};
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::end() const noexcept -> const_forward_iterator
{
	return {this->tail()};
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::rbegin() const noexcept -> const_reverse_iterator
{
	return {this->tail()};
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::rend() const noexcept -> const_reverse_iterator
{
	return {this->head()};
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::operator[](size_t value) const noexcept -> decltype(auto)
{
	return typename Class::reader {static_cast<Class*>(this), value};
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::operator[](size_t value) /*&*/ noexcept -> decltype(auto)
{
	return typename Class::writer {static_cast<Class*>(this), value};
}

template <typename Class /* CRTP core */>
template <typename Other, typename Arena> constexpr auto API<Class>::operator==(__OWNED__(rhs)) const noexcept -> bool
{
	return detail::__equal__<Codec, Other>(this->head(), this->tail(), rhs.head(), rhs.tail());
}

template <typename Class /* CRTP core */>
template <typename Other /* can't own */> constexpr auto API<Class>::operator==(__SLICE__(rhs)) const noexcept -> bool
{
	return detail::__equal__<Codec, Other>(this->head(), this->tail(), rhs.head(), rhs.tail());
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator==(__EQSTR__(rhs)) const noexcept -> bool requires (std::is_same_v<T, char>)
{
	return detail::__equal__<Codec, Codec>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator==(__08STR__(rhs)) const noexcept -> bool /* encoding of char8_t is trivial */
{
	return detail::__equal__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator==(__16STR__(rhs)) const noexcept -> bool /* encoding of char16_t is trivial */
{
	return detail::__equal__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator==(__32STR__(rhs)) const noexcept -> bool /* encoding of char32_t is trivial */
{
	return detail::__equal__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Class /* CRTP core */>
template <typename Other, typename Arena> constexpr auto API<Class>::operator!=(__OWNED__(rhs)) const noexcept -> bool
{
	return detail::__nqual__<Codec, Other>(this->head(), this->tail(), rhs.head(), rhs.tail());
}

template <typename Class /* CRTP core */>
template <typename Other /* can't own */> constexpr auto API<Class>::operator!=(__SLICE__(rhs)) const noexcept -> bool
{
	return detail::__nqual__<Codec, Other>(this->head(), this->tail(), rhs.head(), rhs.tail());
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator!=(__EQSTR__(rhs)) const noexcept -> bool requires (std::is_same_v<T, char>)
{
	return detail::__nqual__<Codec, Codec>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator!=(__08STR__(rhs)) const noexcept -> bool /* encoding of char8_t is trivial */
{
	return detail::__nqual__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator!=(__16STR__(rhs)) const noexcept -> bool /* encoding of char16_t is trivial */
{
	return detail::__nqual__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator!=(__32STR__(rhs)) const noexcept -> bool /* encoding of char32_t is trivial */
{
	return detail::__nqual__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Class /* CRTP core */>
template <typename Other, typename Arena> constexpr auto API<Class>::operator+(__OWNED__(rhs)) const noexcept -> concat<txt<Codec>, txt<Other>>
{
	return {{this->head(), this->tail()}, {rhs.head(), rhs.tail()}};
}

template <typename Class /* CRTP core */>
template <typename Other /* can't own */> constexpr auto API<Class>::operator+(__SLICE__(rhs)) const noexcept -> concat<txt<Codec>, txt<Other>>
{
	return {{this->head(), this->tail()}, {rhs.head(), rhs.tail()}};
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator+(__EQSTR__(rhs)) const noexcept -> concat<txt<Codec>, txt<Codec>> requires (std::is_same_v<T, char>)
{
	return {{this->head(), this->tail()}, {&rhs[N - N], &rhs[N - 1]}};
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator+(__08STR__(rhs)) const noexcept -> concat<txt<Codec>, txt<codec<"UTF-8">>> /* encoding of char8_t is trivial */
{
	return {{this->head(), this->tail()}, {&rhs[N - N], &rhs[N - 1]}};
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator+(__16STR__(rhs)) const noexcept -> concat<txt<Codec>, txt<codec<"UTF-16">>> /* encoding of char16_t is trivial */
{
	return {{this->head(), this->tail()}, {&rhs[N - N], &rhs[N - 1]}};
}

template <typename Class /* CRTP core */>
template <size_t                       N> constexpr auto API<Class>::operator+(__32STR__(rhs)) const noexcept -> concat<txt<Codec>, txt<codec<"UTF-32">>> /* encoding of char32_t is trivial */
{
	return {{this->head(), this->tail()}, {&rhs[N - N], &rhs[N - 1]}};
}

#pragma endregion CRTP
#pragma region CRTP::concat

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <typename Other, typename Arena> constexpr API<Class>::concat<LHS, RHS>::operator str<Other, Arena>() const noexcept
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	size_t size {0};

	this->__for_each__([&](auto&& chunk)
	{
		[&]<typename 𝒞𝑜𝒹𝑒𝒸>(const txt<𝒞𝑜𝒹𝑒𝒸>& slice)
		{
			if constexpr (std::is_same_v<Other, 𝒞𝑜𝒹𝑒𝒸>)
			{
				size += detail::__difcu__<Other>(slice.head(), slice.tail());
			}
			if constexpr (!std::is_same_v<Other, 𝒞𝑜𝒹𝑒𝒸>)
			{
				for (const auto code : slice) { size += Other::size(code); }
			}
		}
		(chunk);
	});

	str<Other, Arena> out;

	out.capacity(size);
	out.__size__(size);

	T* ptr {out.__head__()};

	this->__for_each__([&](auto&& chunk)
	{
		[&]<typename 𝒞𝑜𝒹𝑒𝒸>(const txt<𝒞𝑜𝒹𝑒𝒸>& slice)
		{
			if constexpr (std::is_same_v<Other, 𝒞𝑜𝒹𝑒𝒸>)
			{
				ptr += detail::__fcopy__<Other, 𝒞𝑜𝒹𝑒𝒸>(slice.head(), slice.tail(), ptr);
			}
			if constexpr (!std::is_same_v<Other, 𝒞𝑜𝒹𝑒𝒸>)
			{
				ptr += detail::__fcopy__<Other, 𝒞𝑜𝒹𝑒𝒸>(slice.head(), slice.tail(), ptr);
			}
		}
		(chunk);
	});

	return out;
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS> constexpr auto API<Class>::concat<LHS, RHS>::__for_each__(const auto&& fun) const noexcept -> void
{
	if constexpr (requires(LHS l) { l.__for_each__(fun); })
	{ this->lhs.__for_each__(fun); } else { fun(this->lhs); }

	if constexpr (requires(RHS r) { r.__for_each__(fun); })
	{ this->rhs.__for_each__(fun); } else { fun(this->rhs); }
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <typename Other, typename Arena> constexpr auto API<Class>::concat<LHS, RHS>::operator=(__OWNED__(rhs)) noexcept -> concat<blank_t, txt<Other>>
{
	return {{""}, {rhs}};
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <typename Other /* can't own */> constexpr auto API<Class>::concat<LHS, RHS>::operator=(__SLICE__(rhs)) noexcept -> concat<blank_t, txt<Other>>
{
	return {{""}, {rhs}};
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <size_t                       N> constexpr auto API<Class>::concat<LHS, RHS>::operator=(__08STR__(rhs)) noexcept -> concat<blank_t, txt<codec<"UTF-8">>>
{
	return {{""}, {rhs}};
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <size_t                       N> constexpr auto API<Class>::concat<LHS, RHS>::operator=(__16STR__(rhs)) noexcept -> concat<blank_t, txt<codec<"UTF-16">>>
{
	return {{""}, {rhs}};
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <size_t                       N> constexpr auto API<Class>::concat<LHS, RHS>::operator=(__32STR__(rhs)) noexcept -> concat<blank_t, txt<codec<"UTF-32">>>
{
	return {{""}, {rhs}};
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <typename Other, typename Arena> constexpr auto API<Class>::concat<LHS, RHS>::operator+(__OWNED__(rhs)) noexcept -> concat<concat, txt<Other>>
{
	return {*this, {rhs}};
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <typename Other /* can't own */> constexpr auto API<Class>::concat<LHS, RHS>::operator+(__SLICE__(rhs)) noexcept -> concat<concat, txt<Other>>
{
	return {*this, {rhs}};
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <size_t                       N> constexpr auto API<Class>::concat<LHS, RHS>::operator+(__08STR__(rhs)) noexcept -> concat<concat, txt<codec<"UTF-8">>>
{
	return {*this, {rhs}};
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <size_t                       N> constexpr auto API<Class>::concat<LHS, RHS>::operator+(__16STR__(rhs)) noexcept -> concat<concat, txt<codec<"UTF-16">>>
{
	return {*this, {rhs}};
}

template <typename Class /* CRTP core */>
template <typename   LHS, typename   RHS>
template <size_t                       N> constexpr auto API<Class>::concat<LHS, RHS>::operator+(__32STR__(rhs)) noexcept -> concat<concat, txt<codec<"UTF-32">>>
{
	return {*this, {rhs}};
}

#pragma endregion CRTP::concat
#pragma region CRTP::const_forward_iterator

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_forward_iterator::operator*() const noexcept -> value_type
{
	char32_t code;

	Codec::decode(this->ptr, code, Codec::next(this->ptr));

	return code;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_forward_iterator::operator++(   ) noexcept -> const_forward_iterator&
{
	this->ptr += Codec::next(this->ptr);

	return *this;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_forward_iterator::operator++(int) noexcept -> const_forward_iterator
{
	const auto clone {*this};

	operator++();
	return clone;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_forward_iterator::operator--(   ) noexcept -> const_forward_iterator&
{
	this->ptr += Codec::back(this->ptr);

	return *this;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_forward_iterator::operator--(int) noexcept -> const_forward_iterator
{
	const auto clone {*this};
	
	operator--();
	return clone;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_forward_iterator::operator+(size_t value) noexcept -> const_forward_iterator
{
	const auto clone {*this};

	for (size_t i {0}; i < value; ++i)
	{
		++clone;
	}
	return clone;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_forward_iterator::operator-(size_t value) noexcept -> const_forward_iterator
{
	const auto clone {*this};

	for (size_t i {0}; i < value; ++i)
	{
		--clone;
	}
	return clone;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_forward_iterator::operator+=(size_t value) noexcept -> const_forward_iterator&
{
	for (size_t i {0}; i < value; ++i)
	{
		++(*this);
	}
	return *this;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_forward_iterator::operator-=(size_t value) noexcept -> const_forward_iterator&
{
	for (size_t i {0}; i < value; ++i)
	{
		--(*this);
	}
	return *this;
}

#pragma endregion CRTP::const_forward_iterator
#pragma region CRTP::const_reverse_iterator

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_reverse_iterator::operator*() const noexcept -> value_type
{
	char32_t code;

	Codec::decode(this->ptr, code, Codec::back(this->ptr));

	return code;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_reverse_iterator::operator++(   ) noexcept -> const_reverse_iterator&
{
	this->ptr += Codec::back(this->ptr);

	return *this;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_reverse_iterator::operator++(int) noexcept -> const_reverse_iterator
{
	const auto clone {*this};

	operator++();
	return clone;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_reverse_iterator::operator--(   ) noexcept -> const_reverse_iterator&
{
	this->ptr += Codec::next(this->ptr);

	return *this;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_reverse_iterator::operator--(int) noexcept -> const_reverse_iterator
{
	const auto clone {*this};

	operator--();
	return clone;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_reverse_iterator::operator+(size_t value) noexcept -> const_reverse_iterator
{
	const auto clone {*this};

	for (size_t i {0}; i < value; ++i)
	{
		++clone;
	}
	return clone;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_reverse_iterator::operator-(size_t value) noexcept -> const_reverse_iterator
{
	const auto clone {*this};

	for (size_t i {0}; i < value; ++i)
	{
		--clone;
	}
	return clone;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_reverse_iterator::operator+=(size_t value) noexcept -> const_reverse_iterator&
{
	for (size_t i {0}; i < value; ++i)
	{
		++(*this);
	}
	return *this;
}

template <typename Class /* CRTP core */> constexpr auto API<Class>::const_reverse_iterator::operator-=(size_t value) noexcept -> const_reverse_iterator&
{
	for (size_t i {0}; i < value; ++i)
	{
		--(*this);
	}
	return *this;
}

#pragma endregion CRTP::const_reverse_iterator
#pragma region CRTP::detail

template <typename Codec> constexpr auto detail::__difcu__(const typename Codec::T* head, const typename Codec::T* tail) noexcept -> size_t
{
	typedef typename Codec::T T;

	if constexpr (Codec::variable)
	{
		return tail - head;
	}

	if constexpr (!Codec::variable)
	{
		return tail - head;
	}
}

template <typename Codec> constexpr auto detail::__difcp__(const typename Codec::T* head, const typename Codec::T* tail) noexcept -> size_t
{
	typedef typename Codec::T T;

	if constexpr (Codec::variable)
	{
		size_t out {0};

		for (const T* ptr {head}; ptr < tail; ++out, ptr += Codec::next(ptr)) {}

		return out;
	}

	if constexpr (!Codec::variable)
	{
		size_t out {0};

		for (const T* ptr {head}; ptr < tail; ++out, ptr += Codec::next(ptr)) {}

		return out;
	}
}

template <typename Codec,
          typename Other> constexpr auto detail::__fcopy__(const typename Other::T* head, const typename Other::T* tail,
                                                                                          /*&*/ typename Codec::T* dest) noexcept -> size_t
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		const U* ptr {head};
		/*&*/ T* out {dest};

		for (; ptr <= tail; ++ptr, ++out)
		{
			*out = *ptr;
		}

		return __difcu__<Codec>(head, tail);
	}

	if constexpr (!std::is_same_v<Codec, Other>)
	{
		T* out {dest};

		// for (const U* ptr {head}; ptr < tail; )
		// {
		// 	char32_t code;

		// 	const auto U_step {Other::next(ptr)};
		// 	Other::decode(ptr, code, U_step);
		// 	const auto T_step {Codec::size(code)};

		// 	ptr += U_step;
		// 	out += T_step;
		// }

		for (const U* ptr {head}; ptr < tail; )
		{
			char32_t code;

			const auto U_step {Other::next(ptr)};
			Other::decode(ptr, code, U_step);
			const auto T_step {Codec::size(code)};
			Codec::encode(code, out, T_step);

			ptr += U_step;
			out += T_step;
		}

		return __difcu__<Codec>(dest, out);
	}
}

template <typename Codec,
          typename Other> constexpr auto detail::__rcopy__(const typename Other::T* head, const typename Other::T* tail,
                                                                                          /*&*/ typename Codec::T* dest) noexcept -> size_t
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		const U* ptr {head + (tail - head)};
		/*&*/ T* out {dest + (tail - head)};

		for (; head <= ptr; --ptr, --out)
		{
			*out = *ptr;
		}

		return __difcu__<Codec>(head, tail);
	}

	if constexpr (!std::is_same_v<Codec, Other>)
	{
		T* out {dest};

		for (const U* ptr {head}; ptr < tail; )
		{
			char32_t code;

			const auto U_step {Other::next(ptr)};
			Other::decode(ptr, code, U_step);
			const auto T_step {Codec::size(code)};

			ptr += U_step;
			out += T_step;
		}

		for (const U* ptr {tail}; head < ptr; )
		{
			char32_t code;

			const auto U_step {Other::back(ptr)};
			Other::decode(ptr, code, U_step);
			const auto T_step {Codec::size(code)};
			Codec::encode(code, out, T_step);

			ptr += U_step;
			out += T_step;
		}

		return __difcu__<Codec>(dest, out);
	}
}

template <typename Codec,
          typename Other> constexpr auto detail::__equal__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
                                                           const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		if (lhs_0 == rhs_0
		    &&
		    lhs_N == rhs_N)
		{
			return true;
		}

		return __difcu__<Codec>(lhs_0, lhs_N)
		       ==
		       __difcu__<Other>(rhs_0, rhs_N)
		       &&
		       std::ranges::equal(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	if constexpr (!std::is_same_v<Codec, Other>)
	{
		const T* lhs_ptr {lhs_0};
		const U* rhs_ptr {rhs_0};

		for (; lhs_ptr < lhs_N && rhs_ptr < rhs_N; )
		{
			char32_t T_code;
			char32_t U_code;

			const auto T_step {Codec::next(lhs_ptr)};
			const auto U_step {Other::next(rhs_ptr)};

			Codec::decode(lhs_ptr, T_code, T_step);
			Other::decode(rhs_ptr, U_code, U_step);

			if (T_code != U_code)
			{
				return false;
			}

			lhs_ptr += T_step;
			rhs_ptr += U_step;
		}

		return lhs_ptr == lhs_N && rhs_ptr == rhs_N;
	}
}

template <typename Codec,
          typename Other> constexpr auto detail::__nqual__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
                                                           const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		if (lhs_0 == rhs_0
		    &&
		    lhs_N == rhs_N)
		{
			return false;
		}

		return __difcu__<Codec>(lhs_0, lhs_N)
		       !=
		       __difcu__<Other>(rhs_0, rhs_N)
		       ||
		       !std::ranges::equal(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	if constexpr (!std::is_same_v<Codec, Other>)
	{
		const T* lhs_ptr {lhs_0};
		const U* rhs_ptr {rhs_0};

		for (; lhs_ptr < lhs_N && rhs_ptr < rhs_N; )
		{
			char32_t T_code;
			char32_t U_code;

			const auto T_step {Codec::next(lhs_ptr)};
			const auto U_step {Other::next(rhs_ptr)};

			Codec::decode(lhs_ptr, T_code, T_step);
			Other::decode(rhs_ptr, U_code, U_step);

			if (T_code != U_code)
			{
				return true;
			}

			lhs_ptr += T_step;
			rhs_ptr += U_step;
		}

		return lhs_ptr != lhs_N || rhs_ptr != rhs_N;
	}
}

template <typename Codec,
          typename Other> constexpr auto detail::__swith__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
                                                           const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		if (lhs_0 == rhs_0
		    &&
		    lhs_N == rhs_N)
		{
			return true;
		}

		const auto lhs_len {__difcu__<Codec>(lhs_0, lhs_N)};
		const auto rhs_len {__difcu__<Other>(rhs_0, rhs_N)};

		return rhs_len == 0 // if rhs(delimeter) is an empty string (e.g. str.starts_with(""))
		       ||
		       (lhs_len >= rhs_len && std::ranges::equal(lhs_0, lhs_0 + rhs_len, rhs_0, rhs_N));
	}

	if constexpr (!std::is_same_v<Codec, Other>)
	{
		const T* lhs_ptr {lhs_0};
		const U* rhs_ptr {rhs_0};

		for (; lhs_ptr < lhs_N && rhs_ptr < rhs_N; )
		{
			char32_t T_code;
			char32_t U_code;

			const auto T_step {Codec::next(lhs_ptr)};
			const auto U_step {Other::next(rhs_ptr)};

			Codec::decode(lhs_ptr, T_code, T_step);
			Other::decode(rhs_ptr, U_code, U_step);

			if (T_code != U_code)
			{
				return false;
			}

			lhs_ptr += T_step;
			rhs_ptr += U_step;
		}

		return rhs_ptr == rhs_N;
	}
}

template <typename Codec,
          typename Other> constexpr auto detail::__ewith__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
                                                           const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		if (lhs_0 == rhs_0
		    &&
		    lhs_N == rhs_N)
		{
			return true;
		}

		const auto lhs_len {__difcu__<Codec>(lhs_0, lhs_N)};
		const auto rhs_len {__difcu__<Other>(rhs_0, rhs_N)};

		return rhs_len == 0 // if rhs(delimeter) is an empty string (e.g. str.ends_with(""))
		       ||
		       (lhs_len >= rhs_len && std::ranges::equal(lhs_N - rhs_len, lhs_N, rhs_0, rhs_N));
	}

	if constexpr (!std::is_same_v<Codec, Other>)
	{
		const T* lhs_ptr {lhs_N};
		const U* rhs_ptr {rhs_N};

		for (; lhs_0 < lhs_ptr && rhs_0 < rhs_ptr; )
		{
			char32_t T_code;
			char32_t U_code;

			const auto T_step {Codec::back(lhs_ptr)};
			const auto U_step {Other::back(rhs_ptr)};

			Codec::decode(lhs_ptr, T_code, T_step);
			Other::decode(rhs_ptr, U_code, U_step);

			if (T_code != U_code)
			{
				return false;
			}

			lhs_ptr += T_step;
			rhs_ptr += U_step;
		}

		return rhs_ptr == rhs_0;
	}
}

template <typename Codec,
          typename Other> constexpr auto detail::__scan__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
                                                          const typename Other::T* rhs_0, const typename Other::T* rhs_N,
                                                                                          const auto& fun /* lambda E */) noexcept -> void
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		const auto lhs_len {__difcu__<Codec>(lhs_0, lhs_N)};
		const auto rhs_len {__difcu__<Other>(rhs_0, rhs_N)};

		if (0 < lhs_len && 0 < rhs_len)
		{
			if (lhs_len == rhs_len)
			{
				if (lhs_0 == rhs_0
				    &&
				    lhs_N == rhs_N)
				{
					fun(lhs_0, lhs_N);
				}
				else if (__equal__<Codec, Other>(lhs_0, lhs_N,
				                                 rhs_0, rhs_N))
				{
					fun(lhs_0, lhs_N);
				}
			}
			else if (lhs_len < rhs_len)
			{
				// nothing to do...
			}
			else if (lhs_len > rhs_len)
			{
				const T* anchor;

				std::vector<size_t> tbl (rhs_len, 0);
				// std::vector<char32_t> rhs (rhs_len, 0);

				// uint32_t step;
				size_t i;
				size_t j;
				// char32_t code;

				i = 0;
				j = 0;

				for (const U* ptr {rhs_0}; ptr < rhs_N; ++ptr, ++i)
				{
					// step = Other::next(ptr);

					// Other::decode(ptr, rhs[i], step);

					while (0 < j && rhs_0[i] != rhs_0[j])
					{
						j = tbl[j - 1];
					}

					if /* match */ (rhs_0[i] == rhs_0[j])
					{
						tbl[i] = ++j;
					}
				}

				i = 0;
				j = 0;

				for (const T* ptr {lhs_0}; ptr < lhs_N; ++ptr, ++i)
				{
					// step = Codec::next(ptr);

					// Codec::decode(ptr, code, step);

					while (0 < j && *ptr != rhs_0[j])
					{
						j = tbl[j - 1];
					}

					if /* match */ (*ptr == rhs_0[j])
					{
						if (j == (  0  ))
						{
							// store ptr
							anchor = ptr; // <- 1st unit pos
						}

						++j;

						if (j == rhs_len)
						{
							// flush ptr
							fun(anchor, ptr + 1); j = 0;
						}
					}
				}
			}
		}
	}
	
	if constexpr (!std::is_same_v<Codec, Other>)
	{
		const auto lhs_len {__difcp__<Codec>(lhs_0, lhs_N)};
		const auto rhs_len {__difcp__<Other>(rhs_0, rhs_N)};

		if (0 < lhs_len && 0 < rhs_len)
		{
			if (lhs_len == rhs_len)
			{
				// if (lhs_0 == rhs_0
				//     &&
				//     lhs_N == rhs_N)
				// {
				// 	fun(lhs_0, lhs_N);
				// }
				/* else */ if (__equal__<Codec, Other>(lhs_0, lhs_N,
				                                       rhs_0, rhs_N))
				{
					fun(lhs_0, lhs_N);
				}
			}
			else if (lhs_len < rhs_len)
			{
				// nothing to do...
			}
			else if (lhs_len > rhs_len)
			{
				const T* anchor;

				std::vector<size_t> tbl (rhs_len, 0);
				std::vector<char32_t> rhs (rhs_len, 0);

				uint32_t step;
				size_t i;
				size_t j;
				char32_t code;

				i = 0;
				j = 0;

				for (const U* ptr {rhs_0}; ptr < rhs_N; ptr += step, ++i)
				{
					step = Other::next(ptr);

					Other::decode(ptr, rhs[i], step);

					while (0 < j && rhs[i] != rhs[j])
					{
						j = tbl[j - 1];
					}

					if /* match */ (rhs[i] == rhs[j])
					{
						tbl[i] = ++j;
					}
				}

				i = 0;
				j = 0;

				for (const T* ptr {lhs_0}; ptr < lhs_N; ptr += step, ++i)
				{
					step = Codec::next(ptr);

					Codec::decode(ptr, code, step);

					while (0 < j && code != rhs[j])
					{
						j = tbl[j - 1];
					}

					if /* match */ (code == rhs[j])
					{
						if (j == (  0  ))
						{
							// store ptr
							anchor = ptr; // <- 1st code pos
						}

						++j;

						if (j == rhs_len)
						{
							// flush ptr
							fun(anchor, ptr + step); j = 0;
						}
					}
				}
			}
		}
	}
}

template <typename Codec,
          typename Other> constexpr auto detail::__split__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                                       const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> std::vector<txt<Codec>>
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	std::vector<txt<Codec>> out;

	const T* last {lhs_0};

	__scan__<Codec, Other>(lhs_0, lhs_N,
	                       rhs_0, rhs_N,
		// on every distinct match found
		[&](const T* head, const T* tail)
		{
			if (head != last)
			{
				out.emplace_back(last, head);

				last = tail; // update anchor
			}
		}
	);

	if (last != lhs_0 && last < lhs_N)
	{
		out.emplace_back(last, lhs_N);
	}

	return out;
}

template <typename Codec,
          typename Other> constexpr auto detail::__match__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                                       const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> std::vector<txt<Codec>>
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	std::vector<txt<Codec>> out;

	__scan__<Codec, Other>(lhs_0, lhs_N,
	                       rhs_0, rhs_N,
		// on every distinct match found
		[&](const T* head, const T* tail)
		{
			out.emplace_back(head, tail);
		}
	);

	// if (last != lhs_0 && last < lhs_N)
	// {
	// 	out.emplace_back(last, lhs_N);
	// }

	return out;
}

template <typename Codec> constexpr auto detail::__substr__(const typename Codec::T* head, const typename Codec::T* tail, clamp  start, clamp  until) noexcept -> txt<Codec>
{
	typedef typename Codec::T T;

	// e.g. str.substr(N - 1, N - 0);

	assert(until < start);

	const T* foo {tail};
	
	for (size_t i {  0  }; i < until && head < foo; ++i, foo += Codec::back(foo)) {}

	const T* bar {foo};

	for (size_t i {until}; i < start && head < bar; ++i, bar += Codec::back(bar)) {}

	assert(head <= foo && foo <= tail);
	assert(head <= bar && bar <= tail);

	return {bar, foo};
}

template <typename Codec> constexpr auto detail::__substr__(const typename Codec::T* head, const typename Codec::T* tail, [[maybe_unused]] clamp  start, [[maybe_unused]] range  until) noexcept -> txt<Codec>
{
	typedef typename Codec::T T;

	// e.g. str.substr(N - 1, N);

	const T* foo {tail};

	for (size_t i {  0  }; i < start && head < foo; ++i, foo += Codec::back(foo)) {}

	const T* bar {tail};

	assert(head <= foo && foo <= tail);
	assert(head <= bar && bar <= tail);

	return {foo, bar};
}

template <typename Codec> constexpr auto detail::__substr__(const typename Codec::T* head, const typename Codec::T* tail, [[maybe_unused]] size_t start, [[maybe_unused]] clamp  until) noexcept -> txt<Codec>
{
	typedef typename Codec::T T;

	// e.g. str.substr(0, N - 1);

	const T* foo {head};

	for (size_t i {  0  }; i < start && foo < tail; ++i, foo += Codec::next(foo)) {}

	const T* bar {tail};

	for (size_t i {  0  }; i < until && head < bar; ++i, bar += Codec::back(bar)) {}

	assert(head <= foo && foo <= tail);
	assert(head <= bar && bar <= tail);

	return {foo, bar};
}

template <typename Codec> constexpr auto detail::__substr__(const typename Codec::T* head, const typename Codec::T* tail, [[maybe_unused]] size_t start, [[maybe_unused]] range  until) noexcept -> txt<Codec>
{
	typedef typename Codec::T T;

	// e.g. str.substr(0, N);

	const T* foo {head};

	for (size_t i {  0  }; i < start && foo < tail; ++i, foo += Codec::next(foo)) {}

	const T* bar {tail};

	assert(head <= foo && foo <= tail);
	assert(head <= bar && bar <= tail);

	return {foo, bar};
}

template <typename Codec> constexpr auto detail::__substr__(const typename Codec::T* head, const typename Codec::T* tail, [[maybe_unused]] size_t start, [[maybe_unused]] size_t until) noexcept -> txt<Codec>
{
	typedef typename Codec::T T;

	// e.g. str.substr(6, 9);

	assert(start < until);

	const T* foo {head};

	for (size_t i {  0  }; i < start && foo < tail; ++i, foo += Codec::next(foo)) {}

	const T* bar {foo};

	for (size_t i {start}; i < until && bar < tail; ++i, bar += Codec::next(bar)) {}

	assert(head <= foo && foo <= tail);
	assert(head <= bar && bar <= tail);

	return {foo, bar};
}

#pragma endregion CRTP::detail
#pragma region SSO23

template <typename Codec, typename Alloc> constexpr str<Codec, Alloc>::buffer::operator const typename str<Codec, Alloc>::T*() const noexcept
{
	return this->head;
}

template <typename Codec, typename Alloc> constexpr str<Codec, Alloc>::buffer::operator /*&*/ typename str<Codec, Alloc>::T*() /*&*/ noexcept
{
	return this->head;
}

template <typename Codec, typename Alloc> constexpr str<Codec, Alloc>::storage::storage() noexcept
{
	this->__union__.bytes[RMB] = MAX << SFT;
	// std::construct_at(&this->__union__.large);
}

template <typename Codec, typename Alloc> constexpr str<Codec, Alloc>::storage::~storage() noexcept
{
	if (this->mode() == LARGE)
	{
		allocator::deallocate
		(
			(*this),
			(*this).__union__.large.head,
			(*this).__union__.large.tail
			-
			(*this).__union__.large.head
		);
	}
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::storage::mode() const noexcept -> mode_t
{
	return static_cast<mode_t>(this->__union__.bytes[RMB] & MSK);
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::storage::mode() /*&*/ noexcept -> mode_t
{
	return static_cast<mode_t>(this->__union__.bytes[RMB] & MSK);
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::__head__() const noexcept -> const T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[0 /* ✨ roeses are red, violets are blue ✨ */]
	       :
	       &this->store.__union__.large[0 /* ✨ roeses are red, violets are blue ✨ */];
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::__head__() /*&*/ noexcept -> /*&*/ T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[0 /* ✨ roeses are red, violets are blue ✨ */]
	       :
	       &this->store.__union__.large[0 /* ✨ roeses are red, violets are blue ✨ */];
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::__tail__() const noexcept -> const T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[MAX - (this->store.__union__.bytes[RMB] >> SFT)]
	       :
	       &this->store.__union__.large[0x0 + (this->store.__union__.large.size >> 0x0)];
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::__tail__() /*&*/ noexcept -> /*&*/ T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[MAX - (this->store.__union__.bytes[RMB] >> SFT)]
	       :
	       &this->store.__union__.large[0x0 + (this->store.__union__.large.size >> 0x0)];
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::capacity(/* getter */) const noexcept -> size_t
{
	return this->store.mode() == SMALL
	       ?
	       MAX // or calculate the ptrdiff_t just as large mode as shown down below
	       :
	       this->store.__union__.large.tail - this->store.__union__.large.head - 1;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::capacity(size_t value) /*&*/ noexcept -> void
{
	if (this->capacity() < value)
	{
		T* head {allocator::allocate(this->store, value + 1)};
		T* tail {/* <one-past-the-end!> */(head + value + 1)};

		const auto size {this->size()};

		detail::__fcopy__<Codec, Codec>
		(
			this->__head__(),
			this->__tail__(),
			head // dest
		);

		if (this->store.mode() == LARGE)
		{
			allocator::deallocate
			(
				this->store,
				this->store.__union__.large.head,
				this->store.__union__.large.tail
				-
				this->store.__union__.large.head
			);
		}

		std::construct_at(&this->store.__union__.large);
		{
			this->store.__union__.large.head = head;
			this->store.__union__.large.tail = tail;
			this->store.__union__.large.size = size;
			this->store.__union__.large.meta = LARGE;
		}
	}
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::__size__(size_t value) noexcept -> void
{
	switch (this->store.mode())
	{
		case SMALL:
		{
			const auto slot {(MAX - value) << SFT};

			//┌────────────┐    ┌───────[LE]───────┐
			//│ 0b0XXXXXXX │ -> │ no need for skip │
			//└────────────┘    └──────────────────┘

			//┌────────────┐    ┌───────[BE]───────┐
			//│ 0bXXXXXXX0 │ -> │ skip right 1 bit │
			//└────────────┘    └──────────────────┘

			this->store.__union__.bytes[RMB] = slot;
			this->store.__union__.small[value] = '\0';
			break;
		}
		case LARGE:
		{
			this->store.__union__.large.size = value;
			this->store.__union__.large[value] = '\0';
			break;
		}
	}
}

#pragma endregion SSO23
#pragma region str

template <typename Codec, typename Alloc> constexpr str<Codec, Alloc>::operator const T*() const noexcept
{
	return this->__head__();
}

template <typename Codec, typename Alloc> constexpr str<Codec, Alloc>::operator /*&*/ T*() /*&*/ noexcept
{
	return this->__head__();
}

template <typename Codec, typename Alloc> constexpr str<Codec, Alloc>::str(const str& other) noexcept
{
	// copy constructor
	if (this != &other)
	{
		this->capacity(other.size());

		detail::__fcopy__<Codec, Codec>
		(
			other.__head__(),
			other.__tail__(),
			this->__head__()
		);

		this->__size__(other.size());
	}
}

template <typename Codec, typename Alloc> constexpr str<Codec, Alloc>::str(/*&*/ str&& other) noexcept
{
	// move constructor
	if (this != &other)
	{
		std::swap
		(
			this->store.__union__.bytes,
			other.store.__union__.bytes
		);
	}
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::operator=(const str& other) noexcept -> str&
{
	// copy assignment
	if (this != &other)
	{
		this->capacity(other.size());

		detail::__fcopy__<Codec, Codec>
		(
			other.__head__(),
			other.__tail__(),
			this->__head__()
		);

		this->__size__(other.size());
	}
	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::operator=(/*&*/ str&& other) noexcept -> str&
{
	// move assignment
	if (this != &other)
	{
		std::swap
		(
			this->store.__union__.bytes,
			other.store.__union__.bytes
		);
	}
	return *this;
}

template <typename Codec, typename Alloc>
template <typename Other, typename Arena> constexpr str<Codec, Alloc>::str(__OWNED__(str)) noexcept
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr str<Codec, Alloc>::str(__SLICE__(str)) noexcept
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr str<Codec, Alloc>::str(__EQSTR__(str)) noexcept requires (std::is_same_v<T, char>)
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr str<Codec, Alloc>::str(__08STR__(str)) noexcept /* encoding of char8_t is trivial */
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr str<Codec, Alloc>::str(__16STR__(str)) noexcept /* encoding of char16_t is trivial */
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr str<Codec, Alloc>::str(__32STR__(str)) noexcept /* encoding of char32_t is trivial */
{
	this->operator=(str);
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::begin() /*&*/ noexcept -> forward_iterator
{
	return {this, this->__head__(), static_cast<ptrdiff_t>(0)};
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::end() /*&*/ noexcept -> forward_iterator
{
	return {this, this->__tail__(), static_cast<ptrdiff_t>(this->length())};
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::rbegin() /*&*/ noexcept -> reverse_iterator
{
	return {this, this->__tail__(), static_cast<ptrdiff_t>(0)};
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::rend() /*&*/ noexcept -> reverse_iterator
{
	return {this, this->__head__(), static_cast<ptrdiff_t>(this->length())};
}

template <typename Codec, typename Alloc>
template <typename Other, typename Arena> constexpr auto str<Codec, Alloc>::operator=(__OWNED__(rhs))& noexcept -> str&
{
	this->__assign__<Other>(rhs.__head__(), rhs.__tail__()); return *this;
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto str<Codec, Alloc>::operator=(__SLICE__(rhs))& noexcept -> str&
{
	this->__assign__<Other>(rhs.__head__  , rhs.__tail__  ); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator=(__EQSTR__(rhs))& noexcept -> str& requires (std::is_same_v<T, char>)
{
	this->__assign__<Codec>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator=(__08STR__(rhs))& noexcept -> str& /* encoding of char8_t is trivial */
{
	this->__assign__<codec<"UTF-8">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator=(__16STR__(rhs))& noexcept -> str& /* encoding of char16_t is trivial */
{
	this->__assign__<codec<"UTF-16">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator=(__32STR__(rhs))& noexcept -> str& /* encoding of char32_t is trivial */
{
	this->__assign__<codec<"UTF-32">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <typename Other, typename Arena> constexpr auto str<Codec, Alloc>::operator=(__OWNED__(rhs))&& noexcept -> str&&
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto str<Codec, Alloc>::operator=(__SLICE__(rhs))&& noexcept -> str&&
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator=(__EQSTR__(rhs))&& noexcept -> str&& requires (std::is_same_v<T, char>)
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator=(__08STR__(rhs))&& noexcept -> str&& /* encoding of char8_t is trivial */
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator=(__16STR__(rhs))&& noexcept -> str&& /* encoding of char16_t is trivial */
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator=(__32STR__(rhs))&& noexcept -> str&& /* encoding of char32_t is trivial */
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <typename Other, typename Arena> constexpr auto str<Codec, Alloc>::operator+=(__OWNED__(rhs))& noexcept -> str&
{
	this->__concat__<Other>(rhs.__head__(), rhs.__tail__()); return *this;
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto str<Codec, Alloc>::operator+=(__SLICE__(rhs))& noexcept -> str&
{
	this->__concat__<Other>(rhs.__head__  , rhs.__tail__  ); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator+=(__EQSTR__(rhs))& noexcept -> str& requires (std::is_same_v<T, char>)
{
	this->__concat__<Codec>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator+=(__08STR__(rhs))& noexcept -> str& /* encoding of char8_t is trivial */
{
	this->__concat__<codec<"UTF-8">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator+=(__16STR__(rhs))& noexcept -> str& /* encoding of char16_t is trivial */
{
	this->__concat__<codec<"UTF-16">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator+=(__32STR__(rhs))& noexcept -> str& /* encoding of char32_t is trivial */
{
	this->__concat__<codec<"UTF-32">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <typename Other, typename Arena> constexpr auto str<Codec, Alloc>::operator+=(__OWNED__(rhs))&& noexcept -> str&&
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto str<Codec, Alloc>::operator+=(__SLICE__(rhs))&& noexcept -> str&&
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator+=(__EQSTR__(rhs))&& noexcept -> str&& requires (std::is_same_v<T, char>)
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator+=(__08STR__(rhs))&& noexcept -> str&& /* encoding of char8_t is trivial */
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator+=(__16STR__(rhs))&& noexcept -> str&& /* encoding of char16_t is trivial */
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto str<Codec, Alloc>::operator+=(__32STR__(rhs))&& noexcept -> str&& /* encoding of char32_t is trivial */
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto str<Codec, Alloc>::__assign__(const typename Other::T* rhs_0,
                                                                                       const typename Other::T* rhs_N) noexcept -> void
{
	const txt<Other> rhs {rhs_0, rhs_N};

	size_t size {0};

	if constexpr (std::is_same_v<Codec, Other>) { size += rhs.size(); }
	else { for (const auto code : rhs) { size += Codec::size(code); } }
	
	// size += this->size();

	this->capacity(size);

	detail::__fcopy__<Codec,
	                  Other>
	(
		rhs_0, // &rhs[0]
		rhs_N, // &rhs[N]
		this->__head__()
	);

	this->__size__(size);
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto str<Codec, Alloc>::__concat__(const typename Other::T* rhs_0,
                                                                                       const typename Other::T* rhs_N) noexcept -> void
{
	const txt<Other> rhs {rhs_0, rhs_N};

	size_t size {0};

	if constexpr (std::is_same_v<Codec, Other>) { size += rhs.size(); }
	else { for (const auto code : rhs) { size += Codec::size(code); } }
	
	size += this->size();

	this->capacity(size);

	detail::__fcopy__<Codec,
	                  Other>
	(
		rhs_0, // &rhs[0]
		rhs_N, // &rhs[N]
		this->__tail__()
	);

	this->__size__(size);
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::__insert__(T* dest, char32_t code, int8_t step) noexcept -> T*
{
	const T* tail {this->__tail__()};

	const auto a {0 < step // abs with T=int8_t
	              ?
	              static_cast<int8_t>(+step)
	              :
	              static_cast<int8_t>(-step)};

	const auto b {Codec::size(code)};

	/**/ if (a < b)
	{
		const auto old_l {this->size()};
		const auto new_l {old_l - a + b};

		if (this->capacity() <= new_l)
		{
			/**/ if (0 < step)
			{
				const auto delta {dest - this->__head__()};

				// 2x current capacity
				this->capacity(old_l * 2);

				dest = this->__head__() + delta;
				tail = this->__tail__()        ;
			}
			else if (step < 0)
			{
				const auto delta {this->__tail__() - dest};

				// 2x current capacity
				this->capacity(old_l * 2);

				dest = this->__tail__() - delta;
				tail = this->__tail__()        ;
			}
		}

		/**/ if (0 < step)
		{
			//                       dest
			//                       ↓
			//┌────────┬─────────────┬───┬──────────────┬────────┐
			//│ <head> │ left buffer │ A │ right buffer │ <tail> │
			//├────────┼─────────────┼───┴───┬──────────┴───┬────┴───┐
			//│ <head> │ left buffer │ [[B]] │ right buffer │ <tail> │
			//└────────┴─────────────┴───────┴──────────────┴────────┘
			//                       ↑
			//                       slot

			T* const slot {dest};

			detail::__rcopy__<Codec,
			                  Codec>
			(
				dest + a,
				tail + 0,
				dest + b
			);
			this->__size__(new_l);

			// write code point at slot
			Codec::encode(code, slot, +b);

			return slot;
		}
		else if (step < 0)
		{
			//                           dest
			//                           ↓
			//┌────────┬─────────────┬───┬──────────────┬────────┐
			//│ <head> │ left buffer │ A │ right buffer │ <tail> │
			//├────────┼─────────────┼───┴───┬──────────┴───┬────┴───┐
			//│ <head> │ left buffer │ [[B]] │ right buffer │ <tail> │
			//└────────┴─────────────┴───────┴──────────────┴────────┘
			//                               ↑
			//                               slot

			T* const slot {dest - a + b};

			detail::__rcopy__<Codec,
			                  Codec>
			(
				dest - 0 + 0,
				tail - 0 + 0,
				dest - a + b
			);
			this->__size__(new_l);

			// write code point at slot
			Codec::encode(code, slot, -b);

			return slot;
		}
	}
	else if (b < a)
	{
		const auto old_l {this->size()};
		const auto new_l {old_l - a + b};

		/**/ if (0 < step)
		{
			//                       dest
			//                       ↓
			//┌────────┬─────────────┬───────┬──────────────┬────────┐
			//│ <head> │ left buffer │ [[A]] │ right buffer │ <tail> │
			//├────────┼─────────────┼───┬───┴──────────┬───┴────┬───┘
			//│ <head> │ left buffer │ B │ right buffer │ <tail> │
			//└────────┴─────────────┴───┴──────────────┴────────┘
			//                       ↑
			//                       slot

			T* const slot {dest};

			detail::__fcopy__<Codec,
			                  Codec>
			(
				dest + a,
				tail + 0,
				dest + b
			);
			this->__size__(new_l);

			// write code point at slot
			Codec::encode(code, slot, +b);

			return slot;
		}
		else if (step < 0)
		{
			//                               dest
			//                               ↓
			//┌────────┬─────────────┬───────┬──────────────┬────────┐
			//│ <head> │ left buffer │ [[A]] │ right buffer │ <tail> │
			//├────────┼─────────────┼───┬───┴──────────┬───┴────┬───┘
			//│ <head> │ left buffer │ B │ right buffer │ <tail> │
			//└────────┴─────────────┴───┴──────────────┴────────┘
			//                           ↑
			//                           slot

			T* const slot {dest - a + b};

			detail::__fcopy__<Codec,
			                  Codec>
			(
				dest - 0 + 0,
				tail - 0 + 0,
				dest - a + b
			);
			this->__size__(new_l);

			// write code point at slot
			Codec::encode(code, slot, -b);

			return slot;
		}
	}

	// no need to shift buffer :D
	Codec::encode(code, dest, step);

	return dest;
}

#pragma endregion str
#pragma region str::forward_iterator

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator*() const noexcept -> value_type
{
	return {this, this->ptr}; // ← proxy
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator++(   ) noexcept -> forward_iterator&
{
	  this->ptr += Codec::next(this->ptr);

	++this->dif;

	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator++(int) noexcept -> forward_iterator
{
	const auto clone {*this};

	operator++();
	return clone;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator--(   ) noexcept -> forward_iterator&
{
	  this->ptr += Codec::back(this->ptr);

	--this->dif;

	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator--(int) noexcept -> forward_iterator
{
	const auto clone {*this};

	operator--();
	return clone;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator+(size_t value) noexcept -> forward_iterator
{
	const auto clone {*this};

	for (size_t i {0}; i < value; ++i)
	{
		++clone;
	}
	return clone;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator-(size_t value) noexcept -> forward_iterator
{
	const auto clone {*this};

	for (size_t i {0}; i < value; ++i)
	{
		--clone;
	}
	return clone;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator+=(size_t value) noexcept -> forward_iterator&
{
	for (size_t i {0}; i < value; ++i)
	{
		++(*this);
	}
	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator-=(size_t value) noexcept -> forward_iterator&
{
	for (size_t i {0}; i < value; ++i)
	{
		--(*this);
	}
	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator==(const forward_iterator& rhs) const noexcept -> bool
{
	return this->src == rhs.src && this->dif == rhs.dif;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::operator!=(const forward_iterator& rhs) const noexcept -> bool
{
	return this->src != rhs.src || this->dif != rhs.dif;
}

template <typename Codec, typename Alloc> [[nodiscard]] constexpr str<Codec, Alloc>::forward_iterator::proxy::operator char32_t() const noexcept
{
	char32_t code;

	Codec::decode(this->ptr, code, Codec::next(this->ptr));

	return code;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::proxy::operator=(char32_t code) noexcept -> proxy&
{
	const auto step {Codec::next(this->ptr)};

	this->src->ptr = this->ptr = this->src
	->
	src->__insert__(this->ptr, code, step);

	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::proxy::operator==(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() == code;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::forward_iterator::proxy::operator!=(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() != code;
}

#pragma endregion str::forward_iterator
#pragma region str::reverse_iterator

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator*() const noexcept -> value_type
{
	return {this, this->ptr}; // ← proxy
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator++(   ) noexcept -> reverse_iterator&
{
	  this->ptr += Codec::back(this->ptr);

	++this->dif;

	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator++(int) noexcept -> reverse_iterator
{
	const auto clone {*this};

	operator++();
	return clone;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator--(   ) noexcept -> reverse_iterator&
{
	  this->ptr += Codec::next(this->ptr);

	--this->dif;

	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator--(int) noexcept -> reverse_iterator
{
	const auto clone {*this};

	operator--();
	return clone;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator+(size_t value) noexcept -> reverse_iterator
{
	const auto clone {*this};

	for (size_t i {0}; i < value; ++i)
	{
		++clone;
	}
	return clone;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator-(size_t value) noexcept -> reverse_iterator
{
	const auto clone {*this};

	for (size_t i {0}; i < value; ++i)
	{
		--clone;
	}
	return clone;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator+=(size_t value) noexcept -> reverse_iterator&
{
	for (size_t i {0}; i < value; ++i)
	{
		++(*this);
	}
	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator-=(size_t value) noexcept -> reverse_iterator&
{
	for (size_t i {0}; i < value; ++i)
	{
		--(*this);
	}
	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator==(const reverse_iterator& rhs) const noexcept -> bool
{
	return this->src == rhs.src && this->dif == rhs.dif;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::operator!=(const reverse_iterator& rhs) const noexcept -> bool
{
	return this->src != rhs.src || this->dif != rhs.dif;
}

template <typename Codec, typename Alloc> [[nodiscard]] constexpr str<Codec, Alloc>::reverse_iterator::proxy::operator char32_t() const noexcept
{
	char32_t code;

	Codec::decode(this->ptr, code, Codec::back(this->ptr));

	return code;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::proxy::operator=(char32_t code) noexcept -> proxy&
{
	const auto step {Codec::back(this->ptr)};

	this->src->ptr = this->ptr = this->src
	->
	src->__insert__(this->ptr, code, step);

	return *this;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::proxy::operator==(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() == code;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reverse_iterator::proxy::operator!=(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() != code;
}

#pragma endregion str::reverse_iterator
#pragma region str::reader

template <typename Codec, typename Alloc> [[nodiscard]] constexpr str<Codec, Alloc>::reader::operator char32_t() const noexcept
{
	const T* head {this->src->__head__()};
	const T* tail {this->src->__tail__()};

	size_t i {0};

	if constexpr (!Codec::variable
	              &&
	              !Codec::stateful)
	{
		if (this->arg < this->src->size())
		{
			head += this->arg;
			goto __SHORTCUT__;
		}
		return '\0';
	}

	for (; head < tail; head += Codec::next(head))
	{
		if (this->arg == i++)
		{
			__SHORTCUT__:

			char32_t code;

			const auto step {Codec::next(head)};
			Codec::decode(head, code, step);

			return code;
		}
	}
	return U'\0';
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reader::operator==(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() == code;
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::reader::operator!=(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() != code;
}

#pragma endregion str::reader
#pragma region str::writer

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::writer::operator=(char32_t code) noexcept -> writer&
{
	const T* head {this->src->__head__()};
	const T* tail {this->src->__tail__()};

	size_t i {0};

	if constexpr (!Codec::variable
	              &&
	              !Codec::stateful)
	{
		if (this->arg < this->src->size())
		{
			head += this->arg;
			goto __SHORTCUT__;
		}
		return *this;
	}

	for (; head < tail; head += Codec::next(head))
	{
		if (this->arg == i++)
		{
			__SHORTCUT__:

			const auto step {Codec::next(head)};
			this->src->__insert__(head, code, step);

			return *this;
		}
	}
	return *this;
}

template <typename Codec, typename Alloc> [[nodiscard]] constexpr str<Codec, Alloc>::writer::operator char32_t() const noexcept
{
	return reader {this->src, this->arg}.operator char32_t();
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::writer::operator==(char32_t code) const noexcept -> bool
{
	return reader {this->src, this->arg}.operator==(code);
}

template <typename Codec, typename Alloc> constexpr auto str<Codec, Alloc>::writer::operator!=(char32_t code) const noexcept -> bool
{
	return reader {this->src, this->arg}.operator!=(code);
}

#pragma endregion str::writer
#pragma region txt::reader

template <typename Codec /* can't own */> [[nodiscard]] constexpr txt<Codec>::reader::operator char32_t() const noexcept
{
	const T* head {this->src->__head__};
	const T* tail {this->src->__tail__};

	size_t i {0};

	if constexpr (!Codec::variable
	              &&
	              !Codec::stateful)
	{
		if (this->arg < this->src->size())
		{
			head += this->arg;
			goto __SHORTCUT__;
		}
		return '\0';
	}

	for (; head < tail; head += Codec::next(head))
	{
		if (this->arg == i++)
		{
			__SHORTCUT__:

			char32_t code;

			const auto step {Codec::next(head)};
			Codec::decode(head, code, step);

			return code;
		}
	}
	return U'\0';
}

template <typename Codec /* can't own */> constexpr auto txt<Codec>::reader::operator==(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() == code;
}

template <typename Codec /* can't own */> constexpr auto txt<Codec>::reader::operator!=(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() != code;
}

#pragma endregion txt::reader
#pragma region txt::writer

template <typename Codec /* can't own */> [[nodiscard]] constexpr txt<Codec>::writer::operator char32_t() const noexcept
{
	return reader {this->src, this->arg}.operator char32_t();
}

template <typename Codec /* can't own */> constexpr auto txt<Codec>::writer::operator==(char32_t code) const noexcept -> bool
{
	return reader {this->src, this->arg}.operator==(code);
}

template <typename Codec /* can't own */> constexpr auto txt<Codec>::writer::operator!=(char32_t code) const noexcept -> bool
{
	return reader {this->src, this->arg}.operator!=(code);
}

#pragma endregion txt::writer
#pragma region filesystem

template <typename STRING>
// fs I/O at your service
auto fileof(const STRING& path) noexcept -> std::optional<std::variant
                                            <
                                            	str<codec<"UTF-8">>
                                            	,
                                            	str<codec<"UTF-16">>
                                            	,
                                            	str<codec<"UTF-32">>
                                            >>
{
	enum encoding : uint8_t
	{
		UTF8_STD = (0 << 4) | 0,
		UTF8_BOM = (1 << 4) | 3,
		UTF16_BE = (2 << 4) | 2,
		UTF16_LE = (3 << 4) | 2,
		UTF32_BE = (4 << 4) | 4,
		UTF32_LE = (5 << 4) | 4,
	};

	static const auto byte_order_mask
	{
		[](std::ifstream& ifs) noexcept -> encoding
		{
			char buffer[4];

			ifs.seekg(0, std::ios::beg); // move to the beginning of the file #A
			ifs.read(&buffer[0], 4); const auto bytes {ifs.gcount()}; ifs.clear();
			ifs.seekg(0, std::ios::beg); // move to the beginning of the file #B

			// 00 00 FE FF
			if (4 <= bytes
			    &&
			    buffer[0] == '\x00'
			    &&
			    buffer[1] == '\x00'
			    &&
			    buffer[2] == '\xFE'
			    &&
			    buffer[3] == '\xFF') [[unlikely]] return UTF32_BE;

			// FF FE 00 00
			if (4 <= bytes
			    &&
			    buffer[0] == '\xFF'
			    &&
			    buffer[1] == '\xFE'
			    &&
			    buffer[2] == '\x00'
			    &&
			    buffer[3] == '\x00') [[unlikely]] return UTF32_LE;

			// FE FF
			if (2 <= bytes
			    &&
			    buffer[0] == '\xFE'
			    &&
			    buffer[1] == '\xFF') [[unlikely]] return UTF16_BE;

			// FF FE
			if (2 <= bytes
			    &&
			    buffer[0] == '\xFF'
			    &&
			    buffer[1] == '\xFE') [[unlikely]] return UTF16_LE;

			// EF BB BF
			if (3 <= bytes
			    &&
			    buffer[0] == '\xEF'
			    &&
			    buffer[1] == '\xBB'
			    &&
			    buffer[2] == '\xBF') [[unlikely]] return UTF8_BOM;

			return UTF8_STD;
		}
	};

	static const auto write_as_native
	{
		[]<typename Codec, typename Alloc>(std::ifstream& ifs, str<Codec, Alloc>& str) noexcept -> void
		{
			typedef typename Codec::T T;

			T buffer;

			T* dest {str.__head__()};
			T* head {str.__head__()};

			while (ifs.read(reinterpret_cast<char*>(&buffer), sizeof(T)))
			{
				if (buffer == '\r'
				    &&
				    ifs.read(reinterpret_cast<char*>(&buffer), sizeof(T))
				    &&
				    buffer != '\n')
				{
					*(dest++) = '\n';
				}
				*(dest++) = buffer;
			}
			str.__size__(dest - head);
		}
	};

	static const auto write_as_foreign
	{
		[]<typename Codec, typename Alloc>(std::ifstream& ifs, str<Codec, Alloc>& str) noexcept -> void
		{
			typedef typename Codec::T T;

			T buffer;

			T* dest {str.__head__()};
			T* head {str.__head__()};

			while (ifs.read(reinterpret_cast<char*>(&buffer), sizeof(T)))
			{
				if ((buffer = std::byteswap(buffer)) == '\r'
				    &&
				    ifs.read(reinterpret_cast<char*>(&buffer), sizeof(T))
				    &&
				    (buffer = std::byteswap(buffer)) != '\n')
				{
					*(dest++) = '\n';
				}
				*(dest++) = buffer;
			}
			str.__size__(dest - head);
		}
	};

	const std::filesystem::path fs
	{
		[&]() -> std::filesystem::path
		{
			using file_t = decltype(fs);
			using string = std::string;

			// constructible on the fly
			if constexpr (std::is_constructible_v<file_t, STRING>)
			{
				return path;
			}
			// at least convertible to file_t!
			else if constexpr (std::is_convertible_v<STRING, file_t>)
			{
				return static_cast<file_t>(path);
			}
			// at least convertible to string!
			else if constexpr (std::is_convertible_v<STRING, string>)
			{
				return static_cast<string>(path);
			}
			// ...constexpr failuare! DEAD-END!
			else static_assert(!"ERROR! call to 'fileof' is ambigious");
		}()
	};

	if (std::ifstream ifs {fs, std::ios::binary})
	{
		const auto BOM {byte_order_mask(ifs)};
		const auto off {BOM & 0xF /* ..?? */};

		size_t max;

		// to the BOM
		ifs.seekg(off, std::ios::beg); max = ifs.tellg() - static_cast<std::streamoff>(0x0);
		// to the END
		ifs.seekg(0x0, std::ios::end); max = ifs.tellg() - static_cast<std::streamoff>(max);
		// to the BOM
		ifs.seekg(off, std::ios::beg); // BOM offset calculation is done above, now rest...

		#define IS_BIG          \
		(                       \
		    std::endian::native \
		             !=         \
		    std::endian::little \
		)                       \

		switch (BOM)
		{
			case UTF8_STD:
			{
				typedef codec<"UTF-8"> Codec; str<Codec> str;

				str.capacity(max / sizeof(typename Codec::T));

				if constexpr (!IS_BIG) write_as_native(ifs, str);
				                  else write_as_native(ifs, str);

				return str;
			}
			case UTF8_BOM:
			{
				typedef codec<"UTF-8"> Codec; str<Codec> str;

				str.capacity(max / sizeof(typename Codec::T));

				if constexpr (IS_BIG) write_as_native(ifs, str);
				                 else write_as_native(ifs, str);

				return str;
			}
			case UTF16_LE:
			{
				typedef codec<"UTF-16"> Codec; str<Codec> str;

				str.capacity(max / sizeof(typename Codec::T));

				if constexpr (!IS_BIG) write_as_native(ifs, str);
				                  else write_as_foreign(ifs, str);

				return str;
			}
			case UTF16_BE:
			{
				typedef codec<"UTF-16"> Codec; str<Codec> str;

				str.capacity(max / sizeof(typename Codec::T));

				if constexpr (IS_BIG) write_as_native(ifs, str);
				                 else write_as_foreign(ifs, str);

				return str;
			}
			case UTF32_LE:
			{
				typedef codec<"UTF-32"> Codec; str<Codec> str;

				str.capacity(max / sizeof(typename Codec::T));

				if constexpr (!IS_BIG) write_as_native(ifs, str);
				                  else write_as_foreign(ifs, str);

				return str;
			}
			case UTF32_BE:
			{
				typedef codec<"UTF-32"> Codec; str<Codec> str;

				str.capacity(max / sizeof(typename Codec::T));

				if constexpr (IS_BIG) write_as_native(ifs, str);
				                 else write_as_foreign(ifs, str);

				return str;
			}
		}
		#undef IS_BIG
	}
	return std::nullopt;
}

#pragma endregion filesystem

#undef __OWNED__
#undef __SLICE__
#undef __EQSTR__
#undef __08STR__
#undef __16STR__
#undef __32STR__

using utf8 = str<codec<"UTF-8">>;
using utf16 = str<codec<"UTF-16">>;
using utf32 = str<codec<"UTF-32">>;

using txt8 = txt<codec<"UTF-8">>;
using txt16 = txt<codec<"UTF-16">>;
using txt32 = txt<codec<"UTF-32">>;

#undef COPY_ASSIGNMENT
#undef MOVE_ASSIGNMENT

#undef COPY_CONSTRUCTOR
#undef MOVE_CONSTRUCTOR

template <size_t N> str(const char8_t (&_)[N]) -> str<codec<"UTF-8">>;
template <size_t N> str(const char16_t (&_)[N]) -> str<codec<"UTF-16">>;
template <size_t N> str(const char32_t (&_)[N]) -> str<codec<"UTF-32">>;

template <size_t N> txt(const char8_t (&_)[N]) -> txt<codec<"UTF-8">>;
template <size_t N> txt(const char16_t (&_)[N]) -> txt<codec<"UTF-16">>;
template <size_t N> txt(const char32_t (&_)[N]) -> txt<codec<"UTF-32">>;
}

template <typename Codec, typename Alloc> struct std::hash<utf::str<Codec, Alloc>>
{
	constexpr auto operator()(const utf::str<Codec, Alloc>& str) const noexcept -> size_t
	{
		uint32_t seed {0};

		for (const auto code : str)
		{
			seed = 31 * seed + code;
		}
		return static_cast<size_t>(seed);
	}
};

template <typename Codec /* can't own */> struct std::hash<utf::txt<Codec /*##*/>>
{
	constexpr auto operator()(const utf::txt<Codec /*##*/>& str) const noexcept -> size_t
	{
		uint32_t seed {0};

		for (const auto code : str)
		{
			seed = 31 * seed + code;
		}
		return static_cast<size_t>(seed);
	}
};

template <typename Codec, typename Alloc>
inline constexpr bool std::ranges::disable_sized_range<utf::str<Codec, Alloc>> = true;

template <typename Codec /* can't own */>
inline constexpr bool std::ranges::disable_sized_range<utf::txt<Codec /*##*/>> = true;
