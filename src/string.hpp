#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <bit>
#include <ios>
#include <memory>
#include <vector>
#include <variant>
#include <ostream>
#include <fstream>
#include <optional>
#include <algorithm>
#include <filesystem>

namespace utf {

//┌───────────────────────────────────────────────────────────┐
//│   ░██████                ░██████   ░██████████░█████████  │
//│  ░██   ░██              ░██   ░██      ░██    ░██     ░██ │
//│ ░██                    ░██             ░██    ░██     ░██ │
//│ ░██                     ░████████      ░██    ░█████████  │
//│ ░██                            ░██     ░██    ░██   ░██   │
//│  ░██   ░██              ░██   ░██      ░██    ░██    ░██  │
//│   ░██████  ░██████████   ░██████       ░██    ░██     ░██ │
//└───────────────────────────────────────────────────────────┘

#define COPY_ASSIGNMENT(T) constexpr auto operator=(const T& rhs) noexcept -> T&
#define MOVE_ASSIGNMENT(T) constexpr auto operator=(T&& rhs) noexcept -> T&

#define COPY_CONSTRUCTOR(T) constexpr T(const T& other) noexcept
#define MOVE_CONSTRUCTOR(T) constexpr T(T&& other) noexcept

//┌──────────────────────────────────────────────────────────────┐
//│ special thanks to facebook's folly::FBString.                │
//│                                                              │
//│ SSO mode uses every bytes of heap string struct using union  │
//│ this was achievable thanks to the very clever memory layout. │
//│                                                              │
//│ for more, watch https://www.youtube.com/watch?v=kPR8h4-qZdk. │
//└──────────────────────────────────────────────────────────────┘

template <typename Codec, typename Alloc = std::allocator<typename Codec::T>> class c_str;
template <typename Codec /* slice is a not-owning view of ptr<const unit> */> class slice;

#define __OWNED__(name) const c_str<Other, Arena>& name
#define __SLICE__(name) const slice<Other /*&*/ >  name
#define __EQSTR__(name) const char     (&name)[N]
#define __1BSTR__(name) const char8_t  (&name)[N]
#define __2BSTR__(name) const char16_t (&name)[N]
#define __4BSTR__(name) const char32_t (&name)[N]

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

// Coder & Decoder
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
	encode(const char32_t in, T* out, int8_t size) noexcept -> void;
	static constexpr auto // transform code units into a code point.
	decode(const T* in, char32_t& out, int8_t size) noexcept -> void;
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
	encode(const char32_t in, T* out, int8_t size) noexcept -> void;
	static constexpr auto // transform code units into a code point.
	decode(const T* in, char32_t& out, int8_t size) noexcept -> void;
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
	encode(const char32_t in, T* out, int8_t size) noexcept -> void;
	static constexpr auto // transform code units into a code point.
	decode(const T* in, char32_t& out, int8_t size) noexcept -> void;
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
	encode(const char32_t in, T* out, int8_t size) noexcept -> void;
	static constexpr auto // transform code units into a code point.
	decode(const T* in, char32_t& out, int8_t size) noexcept -> void;
};

template <typename Super, typename Codec> class string
{
	template <typename, typename> friend class string;

	typedef typename Codec::T T;

	constexpr auto head() const noexcept -> const T*;
	constexpr auto tail() const noexcept -> const T*;

	/* bidirectional iterator; tricky! */ class cursor
	{
		const T* ptr;

	public:

		constexpr cursor
		(
			decltype(ptr) ptr
		)
		noexcept : ptr(ptr) {}

		constexpr auto operator*() noexcept -> char32_t;
		constexpr auto operator&() noexcept -> const T*;

		constexpr auto operator++(   ) noexcept -> cursor&;
		constexpr auto operator++(int) noexcept -> cursor;

		constexpr auto operator--(   ) noexcept -> cursor&;
		constexpr auto operator--(int) noexcept -> cursor;

		constexpr auto operator==(const cursor& rhs) noexcept -> bool;
		constexpr auto operator!=(const cursor& rhs) noexcept -> bool;
	};

	template <typename LHS, typename RHS> class concat
	{
		typedef void* none_t;

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

		template <typename Other,
		          typename Arena>
		constexpr operator c_str<Other, Arena>() const noexcept;

		template <typename Other>
		constexpr auto operator=(slice<Other> rhs) noexcept -> concat<none_t, decltype(rhs)>;

		template <typename Other>
		constexpr auto operator+(slice<Other> rhs) noexcept -> concat<concat, decltype(rhs)>;
	};

public:

	[[deprecated]] operator const char*() const noexcept;
	
	// returns the number of code units, excluding NULL-TERMINATOR.
	constexpr auto size() const noexcept -> size_t;
	// returns the number of code points, excluding NULL-TERMINATOR.
	constexpr auto length() const noexcept -> size_t;

	// *self explanatory* returns whether or not it starts with *parameter*.
	template <typename Other, typename Arena>
	constexpr auto starts_with(__OWNED__(value)) const noexcept -> bool;
	template <typename Other /* can't own */>
	constexpr auto starts_with(__SLICE__(value)) const noexcept -> bool;
	template <size_t N>
	constexpr auto starts_with(__EQSTR__(value)) const noexcept -> bool requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto starts_with(__1BSTR__(value)) const noexcept -> bool /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto starts_with(__2BSTR__(value)) const noexcept -> bool /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto starts_with(__4BSTR__(value)) const noexcept -> bool /* encoding of char32_t is trivial */;

	// *self explanatory* returns whether or not it ends with *parameter*.
	template <typename Other, typename Arena>
	constexpr auto ends_with(__OWNED__(value)) const noexcept -> bool;
	template <typename Other /* can't own */>
	constexpr auto ends_with(__SLICE__(value)) const noexcept -> bool;
	template <size_t N>
	constexpr auto ends_with(__EQSTR__(value)) const noexcept -> bool requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto ends_with(__1BSTR__(value)) const noexcept -> bool /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto ends_with(__2BSTR__(value)) const noexcept -> bool /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto ends_with(__4BSTR__(value)) const noexcept -> bool /* encoding of char32_t is trivial */;

	// *self explanatory* returns whether or not it contains *parameter*.
	template <typename Other, typename Arena>
	constexpr auto contains(__OWNED__(value)) const noexcept -> bool;
	template <typename Other /* can't own */>
	constexpr auto contains(__SLICE__(value)) const noexcept -> bool;
	template <size_t N>
	constexpr auto contains(__EQSTR__(value)) const noexcept -> bool requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto contains(__1BSTR__(value)) const noexcept -> bool /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto contains(__2BSTR__(value)) const noexcept -> bool /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto contains(__4BSTR__(value)) const noexcept -> bool /* encoding of char32_t is trivial */;

	// returns a list of string slice, of which is a product of split aka division.
	template <typename Other, typename Arena>
	constexpr auto split(__OWNED__(value)) const noexcept -> std::vector<slice<Codec>>;
	template <typename Other /* can't own */>
	constexpr auto split(__SLICE__(value)) const noexcept -> std::vector<slice<Codec>>;
	template <size_t N>
	constexpr auto split(__EQSTR__(value)) const noexcept -> std::vector<slice<Codec>> requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto split(__1BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto split(__2BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto split(__4BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char32_t is trivial */;

	// returns a list of string slice, of which is a product of search occurrence.
	template <typename Other, typename Arena>
	constexpr auto match(__OWNED__(value)) const noexcept -> std::vector<slice<Codec>>;
	template <typename Other /* can't own */>
	constexpr auto match(__SLICE__(value)) const noexcept -> std::vector<slice<Codec>>;
	template <size_t N>
	constexpr auto match(__EQSTR__(value)) const noexcept -> std::vector<slice<Codec>> requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto match(__1BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto match(__2BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto match(__4BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char32_t is trivial */;

	// returns a slice, of which is a product of substring. N is a sentinel value.
	constexpr auto substr(clamp       start, clamp       until) const noexcept -> slice<Codec>;
	constexpr auto substr(clamp       start, range       until) const noexcept -> slice<Codec>;
	constexpr auto substr(size_t start, clamp       until) const noexcept -> slice<Codec>;
	constexpr auto substr(size_t start, range       until) const noexcept -> slice<Codec>;
	constexpr auto substr(size_t start, size_t until) const noexcept -> slice<Codec>;

	// iterator

	constexpr auto begin() const noexcept -> cursor;
	constexpr auto end() const noexcept -> cursor;

	// operators

	constexpr auto operator[](size_t value) const noexcept -> decltype(auto);
	constexpr auto operator[](size_t value) /*&*/ noexcept -> decltype(auto);

	template <typename Other, typename Arena>
	constexpr auto operator==(__OWNED__(rhs)) const noexcept -> bool;
	template <typename Other /* can't own */>
	constexpr auto operator==(__SLICE__(rhs)) const noexcept -> bool;
	template <size_t N>
	constexpr auto operator==(__EQSTR__(rhs)) const noexcept -> bool requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto operator==(__1BSTR__(rhs)) const noexcept -> bool /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto operator==(__2BSTR__(rhs)) const noexcept -> bool /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto operator==(__4BSTR__(rhs)) const noexcept -> bool /* encoding of char32_t is trivial */;

	template <typename Other, typename Arena>
	constexpr auto operator!=(__OWNED__(rhs)) const noexcept -> bool;
	template <typename Other /* can't own */>
	constexpr auto operator!=(__SLICE__(rhs)) const noexcept -> bool;
	template <size_t N>
	constexpr auto operator!=(__EQSTR__(rhs)) const noexcept -> bool requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto operator!=(__1BSTR__(rhs)) const noexcept -> bool /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto operator!=(__2BSTR__(rhs)) const noexcept -> bool /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto operator!=(__4BSTR__(rhs)) const noexcept -> bool /* encoding of char32_t is trivial */;

	template <typename Other, typename Arena>
	constexpr auto operator+(__OWNED__(rhs)) const noexcept -> concat<slice<Codec>, slice<Other>>;
	template <typename Other /* can't own */>
	constexpr auto operator+(__SLICE__(rhs)) const noexcept -> concat<slice<Codec>, slice<Other>>;
	template <size_t N>
	constexpr auto operator+(__EQSTR__(rhs)) const noexcept -> concat<slice<Codec>, slice<Codec>> requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto operator+(__1BSTR__(rhs)) const noexcept -> concat<slice<Codec>, slice<codec<"UTF-8">>> /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto operator+(__2BSTR__(rhs)) const noexcept -> concat<slice<Codec>, slice<codec<"UTF-16">>> /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto operator+(__4BSTR__(rhs)) const noexcept -> concat<slice<Codec>, slice<codec<"UTF-32">>> /* encoding of char32_t is trivial */;
};

template <typename Codec, typename Alloc> class c_str : public string<c_str<Codec, Alloc>, Codec>
{
	template <typename, typename> friend class c_str;
	template <typename  /*none*/> friend class slice;
	template <typename, typename> friend class string;

	typedef std::allocator_traits<Alloc> allocator;

	typedef typename Codec::T T;

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

	class reader
	{
		const c_str* src;
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
		/*&*/ c_str* src;
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

	friend reader; typedef reader reader;
	friend writer; typedef writer writer;

	// series of sanity checks
	static_assert(sizeof(storage) == sizeof(buffer));
	static_assert(std::is_standard_layout_v<buffer>);
	static_assert(std::is_trivially_copyable_v<buffer>);
	static_assert(sizeof(buffer) == sizeof(size_t) * 3);
	static_assert(alignof(buffer) == alignof(size_t) * 1);
	static_assert(offsetof(buffer, head) == sizeof(size_t) * 0);
	static_assert(offsetof(buffer, tail) == sizeof(size_t) * 1);

public:

	// optional; returns the content of a file with CRLF/CR to LF normalization.
	template <typename STRING> friend auto fileof(const STRING& path) noexcept
	->
	std::optional<std::variant
	<
		c_str<codec<"UTF-8">>
		,
		c_str<codec<"UTF-16">>
		,
		c_str<codec<"UTF-32">>
	>>;

	constexpr operator slice<Codec>() const noexcept;

	// rule of 5

	COPY_CONSTRUCTOR(c_str);
	MOVE_CONSTRUCTOR(c_str);

	COPY_ASSIGNMENT(c_str);
	MOVE_ASSIGNMENT(c_str);

	// constructors

	constexpr c_str(/* default! */) noexcept = default;
	template <typename Other, typename Arena>
	constexpr c_str(__OWNED__(str)) noexcept          ;
	template <typename Other /* can't own */>
	constexpr c_str(__SLICE__(str)) noexcept          ;
	template <size_t N>
	constexpr c_str(__EQSTR__(str)) noexcept requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr c_str(__1BSTR__(str)) noexcept /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr c_str(__2BSTR__(str)) noexcept /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr c_str(__4BSTR__(str)) noexcept /* encoding of char32_t is trivial */;

	// returns the number of code units it can hold, excluding NULL-TERMINATOR.
	constexpr auto capacity(/* getter */) const noexcept -> size_t;
	// changes the number of code units it can hold, excluding NULL-TERMINATOR.
	constexpr auto capacity(size_t value) /*&*/ noexcept -> void;

	// operators

	template <typename Other, typename Arena>
	constexpr auto operator=(__OWNED__(rhs))& noexcept -> c_str&;
	template <typename Other /* can't own */>
	constexpr auto operator=(__SLICE__(rhs))& noexcept -> c_str&;
	template <size_t N>
	constexpr auto operator=(__EQSTR__(rhs))& noexcept -> c_str& requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto operator=(__1BSTR__(rhs))& noexcept -> c_str& /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto operator=(__2BSTR__(rhs))& noexcept -> c_str& /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto operator=(__4BSTR__(rhs))& noexcept -> c_str& /* encoding of char32_t is trivial */;

	template <typename Other, typename Arena>
	constexpr auto operator=(__OWNED__(rhs))&& noexcept -> c_str&&;
	template <typename Other /* can't own */>
	constexpr auto operator=(__SLICE__(rhs))&& noexcept -> c_str&&;
	template <size_t N>
	constexpr auto operator=(__EQSTR__(rhs))&& noexcept -> c_str&& requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto operator=(__1BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto operator=(__2BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto operator=(__4BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char32_t is trivial */;

	template <typename Other, typename Arena>
	constexpr auto operator+=(__OWNED__(rhs))& noexcept -> c_str&;
	template <typename Other /* can't own */>
	constexpr auto operator+=(__SLICE__(rhs))& noexcept -> c_str&;
	template <size_t N>
	constexpr auto operator+=(__EQSTR__(rhs))& noexcept -> c_str& requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto operator+=(__1BSTR__(rhs))& noexcept -> c_str& /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto operator+=(__2BSTR__(rhs))& noexcept -> c_str& /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto operator+=(__4BSTR__(rhs))& noexcept -> c_str& /* encoding of char32_t is trivial */;

	template <typename Other, typename Arena>
	constexpr auto operator+=(__OWNED__(rhs))&& noexcept -> c_str&&;
	template <typename Other /* can't own */>
	constexpr auto operator+=(__SLICE__(rhs))&& noexcept -> c_str&&;
	template <size_t N>
	constexpr auto operator+=(__EQSTR__(rhs))&& noexcept -> c_str&& requires (std::is_same_v<T, char>);
	template <size_t N>
	constexpr auto operator+=(__1BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char8_t is trivial */;
	template <size_t N>
	constexpr auto operator+=(__2BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char16_t is trivial */;
	template <size_t N>
	constexpr auto operator+=(__4BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char32_t is trivial */;

private:

	template <typename Other>
	constexpr auto __assign__(const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> void;

	template <typename Other>
	constexpr auto __concat__(const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> void;
};

template <typename Codec /* can't own */> class slice : public string<slice<Codec /*&*/ >, Codec>
{
	template <typename, typename> friend class c_str;
	template <typename  /*none*/> friend class slice;
	template <typename, typename> friend class string;

	typedef typename Codec::T T;

	const T* __head__;
	const T* __tail__;

	class reader
	{
		const slice* src;
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
		/*&*/ slice* src;
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

	friend reader; typedef reader reader;
	friend writer; typedef writer writer;

public:

	constexpr slice
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
	constexpr slice
	(
		const T (&str)[N]
	)
	noexcept : __head__ {&str[N - N]},
	           __tail__ {&str[N - 1]}
	{
		// nothing to do...
	}

	COPY_CONSTRUCTOR(slice) = default;
	MOVE_CONSTRUCTOR(slice) = default;

	COPY_ASSIGNMENT(slice) = default;
	MOVE_ASSIGNMENT(slice) = default;
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
	static constexpr auto __s_with__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                 const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool;

	template <typename Codec,
	          typename Other>
	static constexpr auto __e_with__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                 const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool;

	template <typename Codec,
	          typename Other>
	static constexpr auto __scan__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                               const typename Other::T* rhs_0, const typename Other::T* rhs_N,
	                                                               const auto& fun /* lambda E */) noexcept -> void;

	template <typename Codec,
	          typename Other>
	static constexpr auto __split__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> std::vector<slice<Codec>>;

	template <typename Codec,
	          typename Other>
	static constexpr auto __match__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> std::vector<slice<Codec>>;

	template <typename Codec>
	static constexpr auto __substr__(const typename Codec::T* head, const typename Codec::T* tail, clamp  start, clamp  until) noexcept -> slice<Codec>;

	template <typename Codec>
	static constexpr auto __substr__(const typename Codec::T* head, const typename Codec::T* tail, clamp  start, range  until) noexcept -> slice<Codec>;

	template <typename Codec>
	static constexpr auto __substr__(const typename Codec::T* head, const typename Codec::T* tail, size_t start, clamp  until) noexcept -> slice<Codec>;

	template <typename Codec>
	static constexpr auto __substr__(const typename Codec::T* head, const typename Codec::T* tail, size_t start, range  until) noexcept -> slice<Codec>;

	template <typename Codec>
	static constexpr auto __substr__(const typename Codec::T* head, const typename Codec::T* tail, size_t start, size_t until) noexcept -> slice<Codec>;
};

#pragma region iostream

/* assumption; UTF-8 codepage terminal */ inline auto operator<<(std::ostream& os, char32_t code) noexcept -> decltype(os)
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

template <typename Other, typename Arena> inline auto operator<<(std::ostream& os, __OWNED__(str)) noexcept -> decltype(os)
{
	for (const auto code : str) { os << code; } return os;
}

template <typename Other /* can't own */> inline auto operator<<(std::ostream& os, __SLICE__(str)) noexcept -> decltype(os)
{
	for (const auto code : str) { os << code; } return os;
}

#pragma endregion iostream
#pragma region codec<"ASCII">

constexpr auto codec<"ASCII">::size(char32_t code) noexcept -> int8_t
{
	return 1;
}

constexpr auto codec<"ASCII">::next(const T* data) noexcept -> int8_t
{
	return +1;
}

constexpr auto codec<"ASCII">::back(const T* data) noexcept -> int8_t
{
	return -1;
}

constexpr auto codec<"ASCII">::encode(const char32_t in, T* out, int8_t size) noexcept -> void
{
	out[0] = static_cast<T>(in);
}

constexpr auto codec<"ASCII">::decode(const T* in, char32_t& out, int8_t size) noexcept -> void
{
	out = static_cast<char32_t>(in[0]);
}

#pragma endregion codec<"ASCII">
#pragma region codec<"UTF-8">

constexpr auto codec<"UTF-8">::size(char32_t code) noexcept -> int8_t
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

constexpr auto codec<"UTF-8">::next(const T* data) noexcept -> int8_t
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

constexpr auto codec<"UTF-8">::back(const T* data) noexcept -> int8_t
{
	int8_t i {-1};
	
	for (; (data[i] & 0xC0) == 0x80; --i) {}
	
	return i;
}

constexpr auto codec<"UTF-8">::encode(const char32_t in, T* out, int8_t size) noexcept -> void
{
	switch (size)
	{
		case +1:
		case -1:
		{
			out[0] = static_cast<T>(in);
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
			out[-1] = 0xC0 | ((in >> 06) & 0x1F);
			out[-0] = 0x80 | ((in >> 00) & 0x3F);
			break;
		}
		case +3:
		{
			out[0] = 0xE0 | ((in >> 12) & 0x0F);
			out[1] = 0x80 | ((in >> 06) & 0x3F);
			out[2] = 0x80 | ((in >> 00) & 0x3F);
			break;
		}
		case -3:
		{
			out[-2] = 0xE0 | ((in >> 12) & 0x0F);
			out[-1] = 0x80 | ((in >> 06) & 0x3F);
			out[-0] = 0x80 | ((in >> 00) & 0x3F);
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
			out[-3] = 0xF0 | ((in >> 18) & 0x07);
			out[-2] = 0x80 | ((in >> 12) & 0x3F);
			out[-1] = 0x80 | ((in >> 06) & 0x3F);
			out[-0] = 0x80 | ((in >> 00) & 0x3F);
			break;
		}
	}
}

constexpr auto codec<"UTF-8">::decode(const T* in, char32_t& out, int8_t size) noexcept -> void
{
	switch (size)
	{
		case +1:
		case -1:
		{
			out = static_cast<char32_t>(in[0]);
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
			out = ((in[-1] & 0x1F) << 06)
			      |
			      ((in[-0] & 0x3F) << 00);
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
			out = ((in[-2] & 0x0F) << 12)
			      |
			      ((in[-1] & 0x3F) << 06)
			      |
			      ((in[-0] & 0x3F) << 00);
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
			out = ((in[-3] & 0x07) << 18)
			      |
			      ((in[-2] & 0x3F) << 12)
			      |
			      ((in[-1] & 0x3F) << 06)
			      |
			      ((in[-0] & 0x3F) << 00);
			break;
		}
	}
}

#pragma endregion codec<"UTF-8">
#pragma region codec<"UTF-16">

constexpr auto codec<"UTF-16">::size(char32_t code) noexcept -> int8_t
{
	//┌───────────────────────┐
	//│ U+000000 ... U+00D7FF │ -> 1 code unit
	//│ U+00E000 ... U+00FFFF │ -> 1 code unit
	//│ U+000000 ... U+10FFFF │ -> 2 code unit
	//└───────────────────────┘

	return 1 + (0xFFFF /* pair? */ < code);
}

constexpr auto codec<"UTF-16">::next(const T* data) noexcept -> int8_t
{
	return +1 + ((data[0] >> 0xA) == 0x36);
}

constexpr auto codec<"UTF-16">::back(const T* data) noexcept -> int8_t
{
	int8_t i {-1};
	
	for (; (data[i] >> 0xA) == 0x37; --i) {}
	
	return i;
}

constexpr auto codec<"UTF-16">::encode(const char32_t in, T* out, int8_t size) noexcept -> void
{
	switch (size)
	{
		case +1:
		case -1:
		{
			out[0] = static_cast<T>(in);
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
			out[-1] = 0xD800 | (code / 0x400);
			out[-0] = 0xDC00 | (code & 0x3FF);
			break;
		}
	}
}

constexpr auto codec<"UTF-16">::decode(const T* in, char32_t& out, int8_t size) noexcept -> void
{
	switch (size)
	{
		case +1:
		case -1:
		{
			out = static_cast<char32_t>(in[0]);
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
			      ((in[-1] - 0xD800) << 10)
			      |
			      ((in[-0] - 0xDC00) << 00);
			break;
		}
	}
}

#pragma endregion codec<"UTF-16">
#pragma region codec<"UTF-32">

constexpr auto codec<"UTF-32">::size(char32_t code) noexcept -> int8_t
{
	return 1;
}

constexpr auto codec<"UTF-32">::next(const T* data) noexcept -> int8_t
{
	return +1;
}

constexpr auto codec<"UTF-32">::back(const T* data) noexcept -> int8_t
{
	return -1;
}

constexpr auto codec<"UTF-32">::encode(const char32_t in, T* out, int8_t size) noexcept -> void
{
	out[0] = static_cast<T>(in);
}

constexpr auto codec<"UTF-32">::decode(const T* in, char32_t& out, int8_t size) noexcept -> void
{
	out = static_cast<char32_t>(in[0]);
}

#pragma endregion codec<"UTF-32">
#pragma region CRTP

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::head() const noexcept -> const T*
{
	if constexpr (requires { static_cast<const Super*>(this)->__head__(); })
	     return static_cast<const Super*>(this)->__head__();
	else return static_cast<const Super*>(this)->__head__  ;
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::tail() const noexcept -> const T*
{
	if constexpr (requires { static_cast<const Super*>(this)->__tail__(); })
	     return static_cast<const Super*>(this)->__tail__();
	else return static_cast<const Super*>(this)->__tail__  ;
}

template <typename Super, typename Codec> string<Super, Codec>::operator const char*() const noexcept
{
	return reinterpret_cast<const char*>(this->head());
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::size() const noexcept -> size_t
{
	return detail::__difcu__<Codec /*&*/>(this->head(), this->tail());
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::length() const noexcept -> size_t
{
	return detail::__difcp__<Codec /*&*/>(this->head(), this->tail());
}

template <typename Super, typename Codec>
template <typename Other, typename Arena> constexpr auto string<Super, Codec>::starts_with(__OWNED__(value)) const noexcept -> bool
{
	return detail::__s_with__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Super, typename Codec>
template <typename Other /* can't own */> constexpr auto string<Super, Codec>::starts_with(__SLICE__(value)) const noexcept -> bool
{
	return detail::__s_with__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::starts_with(__EQSTR__(value)) const noexcept -> bool requires (std::is_same_v<T, char>)
{
	return detail::__s_with__<Codec, Codec>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::starts_with(__1BSTR__(value)) const noexcept -> bool /* encoding of char8_t is trivial */
{
	return detail::__s_with__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::starts_with(__2BSTR__(value)) const noexcept -> bool /* encoding of char16_t is trivial */
{
	return detail::__s_with__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::starts_with(__4BSTR__(value)) const noexcept -> bool /* encoding of char32_t is trivial */
{
	return detail::__s_with__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <typename Other, typename Arena> constexpr auto string<Super, Codec>::ends_with(__OWNED__(value)) const noexcept -> bool
{
	return detail::__e_with__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Super, typename Codec>
template <typename Other /* can't own */> constexpr auto string<Super, Codec>::ends_with(__SLICE__(value)) const noexcept -> bool
{
	return detail::__e_with__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::ends_with(__EQSTR__(value)) const noexcept -> bool requires (std::is_same_v<T, char>)
{
	return detail::__e_with__<Codec, Codec>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::ends_with(__1BSTR__(value)) const noexcept -> bool /* encoding of char8_t is trivial */
{
	return detail::__e_with__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::ends_with(__2BSTR__(value)) const noexcept -> bool /* encoding of char16_t is trivial */
{
	return detail::__e_with__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::ends_with(__4BSTR__(value)) const noexcept -> bool /* encoding of char32_t is trivial */
{
	return detail::__e_with__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <typename Other, typename Arena> constexpr auto string<Super, Codec>::split(__OWNED__(value)) const noexcept -> std::vector<slice<Codec>>
{
	return detail::__split__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Super, typename Codec>
template <typename Other /* can't own */> constexpr auto string<Super, Codec>::split(__SLICE__(value)) const noexcept -> std::vector<slice<Codec>>
{
	return detail::__split__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::split(__EQSTR__(value)) const noexcept -> std::vector<slice<Codec>> requires (std::is_same_v<T, char>)
{
	return detail::__split__<Codec, Codec>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::split(__1BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char8_t is trivial */
{
	return detail::__split__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::split(__2BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char16_t is trivial */
{
	return detail::__split__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::split(__4BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char32_t is trivial */
{
	return detail::__split__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <typename Other, typename Arena> constexpr auto string<Super, Codec>::match(__OWNED__(value)) const noexcept -> std::vector<slice<Codec>>
{
	return detail::__match__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Super, typename Codec>
template <typename Other /* can't own */> constexpr auto string<Super, Codec>::match(__SLICE__(value)) const noexcept -> std::vector<slice<Codec>>
{
	return detail::__match__<Codec, Other>(this->head(), this->tail(), value.head(), value.tail());
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::match(__EQSTR__(value)) const noexcept -> std::vector<slice<Codec>> requires (std::is_same_v<T, char>)
{
	return detail::__match__<Codec, Codec>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::match(__1BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char8_t is trivial */
{
	return detail::__match__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::match(__2BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char16_t is trivial */
{
	return detail::__match__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::match(__4BSTR__(value)) const noexcept -> std::vector<slice<Codec>> /* encoding of char32_t is trivial */
{
	return detail::__match__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::substr(clamp       start, clamp       until) const noexcept -> slice<Codec>
{
	return detail::__substr__<Codec /*&*/>(this->head(), this->tail(), start, until);
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::substr(clamp       start, range       until) const noexcept -> slice<Codec>
{
	return detail::__substr__<Codec /*&*/>(this->head(), this->tail(), start, until);
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::substr(size_t start, clamp       until) const noexcept -> slice<Codec>
{
	return detail::__substr__<Codec /*&*/>(this->head(), this->tail(), start, until);
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::substr(size_t start, range       until) const noexcept -> slice<Codec>
{
	return detail::__substr__<Codec /*&*/>(this->head(), this->tail(), start, until);
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::substr(size_t start, size_t until) const noexcept -> slice<Codec>
{
	return detail::__substr__<Codec /*&*/>(this->head(), this->tail(), start, until);
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::begin() const noexcept -> cursor
{
	return {this->head()};
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::end() const noexcept -> cursor
{
	return {this->tail()};
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::operator[](size_t value) const noexcept -> decltype(auto)
{
	return typename Super::reader {static_cast<Super*>(this), value};
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::operator[](size_t value) /*&*/ noexcept -> decltype(auto)
{
	return typename Super::writer {static_cast<Super*>(this), value};
}

template <typename Super, typename Codec>
template <typename Other, typename Arena> constexpr auto string<Super, Codec>::operator==(__OWNED__(rhs)) const noexcept -> bool
{
	return detail::__equal__<Codec, Other>(this->head(), this->tail(), rhs.head(), rhs.tail());
}

template <typename Super, typename Codec>
template <typename Other /* can't own */> constexpr auto string<Super, Codec>::operator==(__SLICE__(rhs)) const noexcept -> bool
{
	return detail::__equal__<Codec, Other>(this->head(), this->tail(), rhs.head(), rhs.tail());
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator==(__EQSTR__(rhs)) const noexcept -> bool requires (std::is_same_v<T, char>)
{
	return detail::__equal__<Codec, Codec>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator==(__1BSTR__(rhs)) const noexcept -> bool /* encoding of char8_t is trivial */
{
	return detail::__equal__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator==(__2BSTR__(rhs)) const noexcept -> bool /* encoding of char16_t is trivial */
{
	return detail::__equal__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator==(__4BSTR__(rhs)) const noexcept -> bool /* encoding of char32_t is trivial */
{
	return detail::__equal__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Super, typename Codec>
template <typename Other, typename Arena> constexpr auto string<Super, Codec>::operator!=(__OWNED__(rhs)) const noexcept -> bool
{
	return detail::__nqual__<Codec, Other>(this->head(), this->tail(), rhs.head(), rhs.tail());
}

template <typename Super, typename Codec>
template <typename Other /* can't own */> constexpr auto string<Super, Codec>::operator!=(__SLICE__(rhs)) const noexcept -> bool
{
	return detail::__nqual__<Codec, Other>(this->head(), this->tail(), rhs.head(), rhs.tail());
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator!=(__EQSTR__(rhs)) const noexcept -> bool requires (std::is_same_v<T, char>)
{
	return detail::__nqual__<Codec, Codec>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator!=(__1BSTR__(rhs)) const noexcept -> bool /* encoding of char8_t is trivial */
{
	return detail::__nqual__<Codec, codec<"UTF-8">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator!=(__2BSTR__(rhs)) const noexcept -> bool /* encoding of char16_t is trivial */
{
	return detail::__nqual__<Codec, codec<"UTF-16">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator!=(__4BSTR__(rhs)) const noexcept -> bool /* encoding of char32_t is trivial */
{
	return detail::__nqual__<Codec, codec<"UTF-32">>(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <typename Super, typename Codec>
template <typename Other, typename Arena> constexpr auto string<Super, Codec>::operator+(__OWNED__(rhs)) const noexcept -> concat<slice<Codec>, slice<Other>>
{
	return {slice<Codec> {this->head(), this->tail()}, slice<Other> {rhs.head(), rhs.tail()}};
}

template <typename Super, typename Codec>
template <typename Other /* can't own */> constexpr auto string<Super, Codec>::operator+(__SLICE__(rhs)) const noexcept -> concat<slice<Codec>, slice<Other>>
{
	return {slice<Codec> {this->head(), this->tail()}, slice<Other> {rhs.head(), rhs.tail()}};
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator+(__EQSTR__(rhs)) const noexcept -> concat<slice<Codec>, slice<Codec>> requires (std::is_same_v<T, char>)
{
	return {slice<Codec> {this->head(), this->tail()}, slice<Codec> {&rhs[N - N], &rhs[N - 1]}};
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator+(__1BSTR__(rhs)) const noexcept -> concat<slice<Codec>, slice<codec<"UTF-8">>> /* encoding of char8_t is trivial */
{
	return {slice<Codec> {this->head(), this->tail()}, slice<codec<"UTF-8">> {&rhs[N - N], &rhs[N - 1]}};
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator+(__2BSTR__(rhs)) const noexcept -> concat<slice<Codec>, slice<codec<"UTF-16">>> /* encoding of char16_t is trivial */
{
	return {slice<Codec> {this->head(), this->tail()}, slice<codec<"UTF-16">> {&rhs[N - N], &rhs[N - 1]}};
}

template <typename Super, typename Codec>
template <size_t                       N> constexpr auto string<Super, Codec>::operator+(__4BSTR__(rhs)) const noexcept -> concat<slice<Codec>, slice<codec<"UTF-32">>> /* encoding of char32_t is trivial */
{
	return {slice<Codec> {this->head(), this->tail()}, slice<codec<"UTF-32">> {&rhs[N - N], &rhs[N - 1]}};
}

#pragma endregion CRTP
#pragma region CRTP::cursor

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::cursor::operator*() noexcept -> char32_t
{
	char32_t T_out;

	const auto T_size {Codec::next(this->ptr)};
	Codec::decode(this->ptr, T_out, T_size);

	return T_out;
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::cursor::operator&() noexcept -> const T*
{
	return this->ptr;
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::cursor::operator++(   ) noexcept -> cursor&
{
	this->ptr += Codec::next(this->ptr); return *this;
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::cursor::operator++(int) noexcept -> cursor
{
	const auto clone {*this}; operator++(); return clone;
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::cursor::operator--(   ) noexcept -> cursor&
{
	this->ptr += Codec::back(this->ptr); return *this;
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::cursor::operator--(int) noexcept -> cursor
{
	const auto clone {*this}; operator--(); return clone;
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::cursor::operator==(const cursor& rhs) noexcept -> bool
{
	return this->ptr == rhs.ptr;
}

template <typename Super, typename Codec> constexpr auto string<Super, Codec>::cursor::operator!=(const cursor& rhs) noexcept -> bool
{
	return this->ptr != rhs.ptr;
}

#pragma endregion CRTP::cursor
#pragma region CRTP::concat

template <typename Super, typename Codec>
template <typename   LHS, typename   RHS>
template <typename Other, typename Arena> constexpr string<Super, Codec>::concat<LHS, RHS>::operator c_str<Other, Arena>() const noexcept
{
	typedef typename Codec::T T;

	size_t size {0};

	this->__for_each__([&](auto&& chunk)
	{
		typedef decltype(chunk) Ty;

		if constexpr (!std::is_same_v<Ty, none_t>)
		{
			[&]<typename 𝒞𝑜𝒹𝑒𝒸>(const slice<𝒞𝑜𝒹𝑒𝒸>& slice)
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
		}
	});

	c_str<Other, Arena> out;

	out.capacity(size);
	out.__size__(size);

	T* ptr {out.__head__()};

	this->__for_each__([&](auto&& chunk)
	{
		typedef decltype(chunk) Ty;

		if constexpr (!std::is_same_v<Ty, none_t>)
		{
			[&]<typename 𝒞𝑜𝒹𝑒𝒸>(const slice<𝒞𝑜𝒹𝑒𝒸>& slice)
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
		}
	});

	return out;
}

template <typename Super, typename Codec>
template <typename   LHS, typename   RHS> constexpr auto string<Super, Codec>::concat<LHS, RHS>::__for_each__(const auto&& fun) const noexcept -> void
{
	if constexpr (requires(LHS l) { l.__for_each__(fun); })
	{ this->lhs.__for_each__(fun); } else { fun(this->lhs); }

	if constexpr (requires(RHS r) { r.__for_each__(fun); })
	{ this->rhs.__for_each__(fun); } else { fun(this->rhs); }
}

template <typename Super, typename Codec>
template <typename   LHS, typename   RHS>
template <typename Other /* can't own */> constexpr auto string<Super, Codec>::concat<LHS, RHS>::operator=(slice<Other> rhs) noexcept -> concat<none_t, decltype(rhs)>
{
	return {&this, rhs};
}

template <typename Super, typename Codec>
template <typename   LHS, typename   RHS>
template <typename Other /* can't own */> constexpr auto string<Super, Codec>::concat<LHS, RHS>::operator+(slice<Other> rhs) noexcept -> concat<concat, decltype(rhs)>
{
	return {*this, rhs};
}

#pragma endregion CRTP::concat
#pragma region CRTP::detail

template <typename Codec /* exclusive */> constexpr auto detail::__difcu__(const typename Codec::T* head, const typename Codec::T* tail) noexcept -> size_t
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

template <typename Codec /* exclusive */> constexpr auto detail::__difcp__(const typename Codec::T* head, const typename Codec::T* tail) noexcept -> size_t
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

template <typename Codec, typename Other> constexpr auto detail::__fcopy__(const typename Other::T* head, const typename Other::T* tail,
                                                                                                          /*&*/ typename Codec::T* dest) noexcept -> size_t
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		std::ranges::copy/*forward*/
		(
			head,
			tail,
			dest
		);

		return __difcu__<Codec>(head, tail);
	}

	if constexpr (!std::is_same_v<Codec, Other>)
	{
		T* last {dest};

		// for (const U* sync {head}; sync < tail; )
		// {
		// 	char32_t code;

		// 	const auto U_size {Other::next(sync)};
		// 	Other::decode(sync, code, U_size);
		// 	const auto T_size {Codec::size(code)};

		// 	sync += U_size;
		// 	last += T_size;
		// }

		for (const U* sync {head}; sync < tail; )
		{
			char32_t code;

			const auto U_size {Other::next(sync)};
			Other::decode(sync, code, U_size);
			const auto T_size {Codec::size(code)};
			Codec::encode(code, last, T_size);

			sync += U_size;
			last += T_size;
		}

		return __difcu__<Codec>(dest, last);
	}
}

template <typename Codec, typename Other> constexpr auto detail::__rcopy__(const typename Other::T* head, const typename Other::T* tail,
                                                                                                          /*&*/ typename Codec::T* dest) noexcept -> size_t
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		std::ranges::copy_backward
		(
			head,
			tail,
			dest
		);

		return __difcu__<Codec>(head, tail);
	}

	if constexpr (!std::is_same_v<Codec, Other>)
	{
		T* last {dest};

		for (const U* sync {head}; sync < tail; )
		{
			char32_t code;

			const auto U_size {Other::next(sync)};
			Other::decode(sync, code, U_size);
			const auto T_size {Codec::size(code)};

			sync += U_size;
			last += T_size;
		}

		for (const U* sync {tail}; head < sync; )
		{
			char32_t code;

			const auto U_size {Other::back(sync)};
			Other::decode(sync, code, U_size);
			const auto T_size {Codec::size(code)};
			Codec::encode(code, last, T_size);

			sync += U_size;
			last += T_size;
		}

		return __difcu__<Codec>(dest, last);
	}
}

template <typename Codec, typename Other> constexpr auto detail::__equal__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
                                                                           const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
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

			const auto T_size {Codec::next(lhs_ptr)};
			const auto U_size {Other::next(rhs_ptr)};

			Codec::decode(lhs_ptr, T_code, T_size);
			Other::decode(rhs_ptr, U_code, U_size);

			if (T_code != U_code)
			{
				return false;
			}

			lhs_ptr += T_size;
			rhs_ptr += U_size;
		}

		return lhs_ptr == lhs_N && rhs_ptr == rhs_N;
	}
}

template <typename Codec, typename Other> constexpr auto detail::__nqual__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
                                                                           const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
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

			const auto T_size {Codec::next(lhs_ptr)};
			const auto U_size {Other::next(rhs_ptr)};

			Codec::decode(lhs_ptr, T_code, T_size);
			Other::decode(rhs_ptr, U_code, U_size);

			if (T_code != U_code)
			{
				return true;
			}

			lhs_ptr += T_size;
			rhs_ptr += U_size;
		}

		return lhs_ptr != lhs_N || rhs_ptr != rhs_N;
	}
}

template <typename Codec, typename Other> constexpr auto detail::__s_with__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
                                                                            const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		const auto lhs_len {__difcu__<Codec>(lhs_0, lhs_N)};
		const auto rhs_len {__difcu__<Other>(rhs_0, rhs_N)};

		return rhs_len == 0 // if rhs(delimeter) is an empty string (e.g. str.starts_with(""))
		       ||
		       (rhs_len <= lhs_len && std::ranges::equal(lhs_0, lhs_0 + rhs_len, rhs_0, rhs_N));
	}

	if constexpr (!std::is_same_v<Codec, Other>)
	{
		const T* lhs_ptr {lhs_0};
		const U* rhs_ptr {rhs_0};

		for (; lhs_ptr < lhs_N && rhs_ptr < rhs_N; )
		{
			char32_t T_code;
			char32_t U_code;

			const auto T_size {Codec::next(lhs_ptr)};
			const auto U_size {Other::next(rhs_ptr)};

			Codec::decode(lhs_ptr, T_code, T_size);
			Other::decode(rhs_ptr, U_code, U_size);

			if (T_code != U_code)
			{
				return false;
			}

			lhs_ptr += T_size;
			rhs_ptr += U_size;
		}

		return rhs_ptr == rhs_N;
	}
}

template <typename Codec, typename Other> constexpr auto detail::__e_with__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
                                                                            const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> bool
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	if constexpr (std::is_same_v<Codec, Other>)
	{
		const auto lhs_len {__difcu__<Codec>(lhs_0, lhs_N)};
		const auto rhs_len {__difcu__<Other>(rhs_0, rhs_N)};

		return rhs_len == 0 // if rhs(delimeter) is an empty string (e.g. str.ends_with(""))
		       ||
		       (rhs_len <= lhs_len && std::ranges::equal(lhs_N - rhs_len, lhs_N, rhs_0, rhs_N));
	}

	if constexpr (!std::is_same_v<Codec, Other>)
	{
		const T* lhs_ptr {lhs_N};
		const U* rhs_ptr {rhs_N};

		for (; lhs_0 < lhs_ptr && rhs_0 < rhs_ptr; )
		{
			char32_t T_code;
			char32_t U_code;

			const auto T_size {Codec::back(lhs_ptr)};
			const auto U_size {Other::back(rhs_ptr)};

			Codec::decode(lhs_ptr, T_code, T_size);
			Other::decode(rhs_ptr, U_code, U_size);

			if (T_code != U_code)
			{
				return false;
			}

			lhs_ptr += T_size;
			rhs_ptr += U_size;
		}

		return rhs_ptr == rhs_0;
	}
}

template <typename Codec, typename Other> constexpr auto detail::__scan__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
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
				if (__equal__<Codec, Other>(lhs_0, lhs_N,
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

				for (const U* ptr {rhs_0}; ptr < rhs_N; ptr += 1, ++i)
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

				for (const T* ptr {lhs_0}; ptr < lhs_N; ptr += 1, ++i)
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
				if (__equal__<Codec, Other>(lhs_0, lhs_N,
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

template <typename Codec, typename Other> constexpr auto detail::__split__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                                                       const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> std::vector<slice<Codec>>
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	std::vector<slice<Codec>> out;

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

	if (last < lhs_N)
	{
		out.emplace_back(last, lhs_N);
	}

	return out;
}

template <typename Codec, typename Other> constexpr auto detail::__match__(const typename Codec::T* lhs_0, const typename Codec::T* lhs_N,
	                                                                       const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> std::vector<slice<Codec>>
{
	typedef typename Codec::T T;
	typedef typename Other::T U;

	std::vector<slice<Codec>> out;

	__scan__<Codec, Other>(lhs_0, lhs_N,
	                       rhs_0, rhs_N,
		// on every distinct match found
		[&](const T* head, const T* tail)
		{
			out.emplace_back(head, tail);
		}
	);

	// if (last < lhs_N)
	// {
	// 	out.emplace_back(last, lhs_N);
	// }

	return out;
}

template <typename Codec /* exclusive */> constexpr auto detail::__substr__(const typename Codec::T* head, const typename Codec::T* tail, clamp       start, clamp       until) noexcept -> slice<Codec>
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

template <typename Codec /* exclusive */> constexpr auto detail::__substr__(const typename Codec::T* head, const typename Codec::T* tail, clamp       start, range       until) noexcept -> slice<Codec>
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

template <typename Codec /* exclusive */> constexpr auto detail::__substr__(const typename Codec::T* head, const typename Codec::T* tail, size_t start, clamp       until) noexcept -> slice<Codec>
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

template <typename Codec /* exclusive */> constexpr auto detail::__substr__(const typename Codec::T* head, const typename Codec::T* tail, size_t start, range       until) noexcept -> slice<Codec>
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

template <typename Codec /* exclusive */> constexpr auto detail::__substr__(const typename Codec::T* head, const typename Codec::T* tail, size_t start, size_t until) noexcept -> slice<Codec>
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

template <typename Codec, typename Alloc> constexpr c_str<Codec, Alloc>::buffer::operator const typename c_str<Codec, Alloc>::T*() const noexcept
{
	return this->head;
}

template <typename Codec, typename Alloc> constexpr c_str<Codec, Alloc>::buffer::operator /*&*/ typename c_str<Codec, Alloc>::T*() /*&*/ noexcept
{
	return this->head;
}

template <typename Codec, typename Alloc> constexpr c_str<Codec, Alloc>::storage::storage() noexcept
{
	this->__union__.bytes[RMB] = MAX << SFT;
	std::construct_at(this->__union__.small);
}

template <typename Codec, typename Alloc> constexpr c_str<Codec, Alloc>::storage::~storage() noexcept
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

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::storage::mode() const noexcept -> mode_t
{
	return static_cast<mode_t>(this->__union__.bytes[RMB] & MSK);
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::storage::mode() /*&*/ noexcept -> mode_t
{
	return static_cast<mode_t>(this->__union__.bytes[RMB] & MSK);
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::__head__() const noexcept -> const T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[0 /* ✨ roeses are red, violets are blue ✨ */]
	       :
	       &this->store.__union__.large[0 /* ✨ roeses are red, violets are blue ✨ */];
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::__head__() /*&*/ noexcept -> /*&*/ T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[0 /* ✨ roeses are red, violets are blue ✨ */]
	       :
	       &this->store.__union__.large[0 /* ✨ roeses are red, violets are blue ✨ */];
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::__tail__() const noexcept -> const T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[MAX - (this->store.__union__.bytes[RMB] >> SFT)]
	       :
	       &this->store.__union__.large[0x0 + (this->store.__union__.large.size >> 0x0)];
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::__tail__() /*&*/ noexcept -> /*&*/ T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[MAX - (this->store.__union__.bytes[RMB] >> SFT)]
	       :
	       &this->store.__union__.large[0x0 + (this->store.__union__.large.size >> 0x0)];
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::capacity(/* getter */) const noexcept -> size_t
{
	return this->store.mode() == SMALL
	       ?
	       MAX // or calculate the ptrdiff_t just as large mode as shown down below
	       :
	       this->store.__union__.large.tail - this->store.__union__.large.head - 1;
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::capacity(size_t value) /*&*/ noexcept -> void
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

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::__size__(size_t value) noexcept -> void
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
#pragma region c_str

template <typename Codec, typename Alloc> constexpr c_str<Codec, Alloc>::operator slice<Codec>() const noexcept
{
	return {this->__head__(), this->__tail__()};
}

// copy constructor
template <typename Codec, typename Alloc> constexpr c_str<Codec, Alloc>::c_str(const c_str& other) noexcept
{
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

// move constructor
template <typename Codec, typename Alloc> constexpr c_str<Codec, Alloc>::c_str(/*&*/ c_str&& other) noexcept
{
	if (this != &other)
	{
		std::swap
		(
			this->store.__union__.bytes,
			other.store.__union__.bytes
		);
	}
}

// copy assignment
template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::operator=(const c_str& other) noexcept -> c_str&
{
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

// move assignment
template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::operator=(/*&*/ c_str&& other) noexcept -> c_str&
{
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
template <typename Other, typename Arena> constexpr c_str<Codec, Alloc>::c_str(__OWNED__(str)) noexcept
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr c_str<Codec, Alloc>::c_str(__SLICE__(str)) noexcept
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr c_str<Codec, Alloc>::c_str(__EQSTR__(str)) noexcept requires (std::is_same_v<T, char>)
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr c_str<Codec, Alloc>::c_str(__1BSTR__(str)) noexcept /* encoding of char8_t is trivial */
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr c_str<Codec, Alloc>::c_str(__2BSTR__(str)) noexcept /* encoding of char16_t is trivial */
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr c_str<Codec, Alloc>::c_str(__4BSTR__(str)) noexcept /* encoding of char32_t is trivial */
{
	this->operator=(str);
}

template <typename Codec, typename Alloc>
template <typename Other, typename Arena> constexpr auto c_str<Codec, Alloc>::operator=(__OWNED__(rhs))& noexcept -> c_str&
{
	this->__assign__<Other>(rhs.__head__(), rhs.__tail__()); return *this;
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto c_str<Codec, Alloc>::operator=(__SLICE__(rhs))& noexcept -> c_str&
{
	this->__assign__<Other>(rhs.__head__, rhs.__tail__); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator=(__EQSTR__(rhs))& noexcept -> c_str& requires (std::is_same_v<T, char>)
{
	this->__assign__<Codec>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator=(__1BSTR__(rhs))& noexcept -> c_str& /* encoding of char8_t is trivial */
{
	this->__assign__<codec<"UTF-8">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator=(__2BSTR__(rhs))& noexcept -> c_str& /* encoding of char16_t is trivial */
{
	this->__assign__<codec<"UTF-16">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator=(__4BSTR__(rhs))& noexcept -> c_str& /* encoding of char32_t is trivial */
{
	this->__assign__<codec<"UTF-32">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <typename Other, typename Arena> constexpr auto c_str<Codec, Alloc>::operator=(__OWNED__(rhs))&& noexcept -> c_str&&
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto c_str<Codec, Alloc>::operator=(__SLICE__(rhs))&& noexcept -> c_str&&
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator=(__EQSTR__(rhs))&& noexcept -> c_str&& requires (std::is_same_v<T, char>)
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator=(__1BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char8_t is trivial */
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator=(__2BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char16_t is trivial */
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator=(__4BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char32_t is trivial */
{
	return std::move(this->operator=(rhs));
}

template <typename Codec, typename Alloc>
template <typename Other, typename Arena> constexpr auto c_str<Codec, Alloc>::operator+=(__OWNED__(rhs))& noexcept -> c_str&
{
	this->__concat__<Other>(rhs.__head__(), rhs.__tail__()); return *this;
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto c_str<Codec, Alloc>::operator+=(__SLICE__(rhs))& noexcept -> c_str&
{
	this->__concat__<Other>(rhs.__head__, rhs.__tail__); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator+=(__EQSTR__(rhs))& noexcept -> c_str& requires (std::is_same_v<T, char>)
{
	this->__concat__<Codec>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator+=(__1BSTR__(rhs))& noexcept -> c_str& /* encoding of char8_t is trivial */
{
	this->__concat__<codec<"UTF-8">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator+=(__2BSTR__(rhs))& noexcept -> c_str& /* encoding of char16_t is trivial */
{
	this->__concat__<codec<"UTF-16">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator+=(__4BSTR__(rhs))& noexcept -> c_str& /* encoding of char32_t is trivial */
{
	this->__concat__<codec<"UTF-32">>(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <typename Codec, typename Alloc>
template <typename Other, typename Arena> constexpr auto c_str<Codec, Alloc>::operator+=(__OWNED__(rhs))&& noexcept -> c_str&&
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto c_str<Codec, Alloc>::operator+=(__SLICE__(rhs))&& noexcept -> c_str&&
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator+=(__EQSTR__(rhs))&& noexcept -> c_str&& requires (std::is_same_v<T, char>)
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator+=(__1BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char8_t is trivial */
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator+=(__2BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char16_t is trivial */
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <size_t                       N> constexpr auto c_str<Codec, Alloc>::operator+=(__4BSTR__(rhs))&& noexcept -> c_str&& /* encoding of char32_t is trivial */
{
	return std::move(this->operator+=(rhs));
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto c_str<Codec, Alloc>::__assign__(const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> void
{
	const slice<Other> rhs {rhs_0, rhs_N};

	size_t size {0};

	if constexpr (std::is_same_v<Codec, Other>) { size += rhs.size(); }
	else { for (const auto code : rhs) { size += Codec::size(code); } }
	
	// size += this->size();

	this->capacity(size);

	detail::__fcopy__<Codec, Other>(rhs_0, rhs_N, this->__head__());

	this->__size__(size);
}

template <typename Codec, typename Alloc>
template <typename Other /* can't own */> constexpr auto c_str<Codec, Alloc>::__concat__(const typename Other::T* rhs_0, const typename Other::T* rhs_N) noexcept -> void
{
	const slice<Other> rhs {rhs_0, rhs_N};

	size_t size {0};

	if constexpr (std::is_same_v<Codec, Other>) { size += rhs.size(); }
	else { for (const auto code : rhs) { size += Codec::size(code); } }
	
	size += this->size();

	this->capacity(size);

	detail::__fcopy__<Codec, Other>(rhs_0, rhs_N, this->__head__());

	this->__size__(size);
}

#pragma endregion c_str
#pragma region slice

// nothing to do...

#pragma endregion slice
#pragma region c_str::reader

template <typename Codec, typename Alloc> [[nodiscard]] constexpr c_str<Codec, Alloc>::reader::operator char32_t() const noexcept
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

			char32_t T_code;

			const auto T_size {Codec::next(head)};
			Codec::decode(head, T_code, T_size);

			return T_code;
		}
	}
	return U'\0';
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::reader::operator==(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() == code;
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::reader::operator!=(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() != code;
}

#pragma endregion c_str::reader
#pragma region c_str::writer

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::writer::operator=(char32_t code) noexcept -> writer&
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

			const auto a {Codec::next(head)};
			const auto b {Codec::size(code)};

			if (a == b)
			{
				// no need to shift
			}
			else if (a < b)
			{
				//┌───┬───────────┐
				//│ a │ buffer T* │
				//├───┴───┬───────┴───┐
				//│   b   │ buffer T* │
				//└───────┴───────────┘

				const auto old_l {this->src->size()};
				const auto new_l {old_l + (b - a)};

				if (this->src->capacity() <= new_l)
				{
					this->src->capacity(new_l * 2);

					head = this->src->__head__();
					tail = this->src->__tail__();
				}

				detail::__rcopy__
				(
					&head[a],
					&tail[0],
					&head[b]
				);
				this->src->__size__(new_l);
			}
			else if (b < a)
			{
				//┌───────┬───────────┐
				//│   a   │ buffer T* │
				//├───┬───┴───────┬───┘
				//│ b │ buffer T* │
				//└───┴───────────┘

				const auto old_l {this->src->size()};
				const auto new_l {old_l - (a - b)};

				// if (this->src->capacity() <= new_l)
				// {
				// 	this->src->capacity(new_l * 2);

				// 	head = this->src->head();
				// 	tail = this->src->tail();
				// }

				detail::__fcopy__
				(
					&head[a],
					&tail[0],
					&head[b]
				);
				this->src->__size__(new_l);
			}
			// write code point at ptr
			Codec::encode(code, head, b);
			break;
		}
	}
	return *this;
}

template <typename Codec, typename Alloc> [[nodiscard]] constexpr c_str<Codec, Alloc>::writer::operator char32_t() const noexcept
{
	return reader {this->src, this->arg}.operator char32_t();
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::writer::operator==(char32_t code) const noexcept -> bool
{
	return reader {this->src, this->arg}.operator==(code);
}

template <typename Codec, typename Alloc> constexpr auto c_str<Codec, Alloc>::writer::operator!=(char32_t code) const noexcept -> bool
{
	return reader {this->src, this->arg}.operator!=(code);
}

#pragma endregion c_str::writer
#pragma region slice::reader

template <typename Codec /* can't own */> [[nodiscard]] constexpr slice<Codec>::reader::operator char32_t() const noexcept
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

			char32_t T_code;

			const auto T_size {Codec::next(head)};
			Codec::decode(head, T_code, T_size);

			return T_code;
		}
	}
	return U'\0';
}

template <typename Codec /* can't own */> constexpr auto slice<Codec>::reader::operator==(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() == code;
}

template <typename Codec /* can't own */> constexpr auto slice<Codec>::reader::operator!=(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() != code;
}

#pragma endregion slice::reader
#pragma region slice::writer

template <typename Codec /* can't own */> [[nodiscard]] constexpr slice<Codec>::writer::operator char32_t() const noexcept
{
	return reader {this->src, this->arg}.operator char32_t();
}

template <typename Codec /* can't own */> constexpr auto slice<Codec>::writer::operator==(char32_t code) const noexcept -> bool
{
	return reader {this->src, this->arg}.operator==(code);
}

template <typename Codec /* can't own */> constexpr auto slice<Codec>::writer::operator!=(char32_t code) const noexcept -> bool
{
	return reader {this->src, this->arg}.operator!=(code);
}

#pragma endregion slice::writer
#pragma region filesystem

template <typename STRING>
// fs I/O at your service
auto fileof(const STRING& path) noexcept -> std::optional<std::variant
                                            <
                                            	c_str<codec<"UTF-8">>
                                            	,
                                            	c_str<codec<"UTF-16">>
                                            	,
                                            	c_str<codec<"UTF-32">>
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

			ifs.seekg(0, std::ios::beg); // move to the beginning of the file :D
			ifs.read(&buffer[0], 4); const auto bytes {ifs.gcount()}; ifs.clear();
			ifs.seekg(0, std::ios::beg); // move to the beginning of the file :D

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
		[]<typename Codec, typename Alloc>(std::ifstream& ifs, c_str<Codec, Alloc>& str) noexcept -> void
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
		[]<typename Codec, typename Alloc>(std::ifstream& ifs, c_str<Codec, Alloc>& str) noexcept -> void
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
				c_str<codec<"UTF-8">> str;

				str.capacity(max / sizeof(char8_t));

				if constexpr (!IS_BIG) write_as_native(ifs, str);
				                  else write_as_native(ifs, str);

				return str;
			}
			case UTF8_BOM:
			{
				c_str<codec<"UTF-8">> str;

				str.capacity(max / sizeof(char8_t));

				if constexpr (IS_BIG) write_as_native(ifs, str);
				                 else write_as_native(ifs, str);

				return str;
			}
			case UTF16_LE:
			{
				c_str<codec<"UTF-16">> str;

				str.capacity(max / sizeof(char16_t));

				if constexpr (!IS_BIG) write_as_native(ifs, str);
				                  else write_as_foreign(ifs, str);

				return str;
			}
			case UTF16_BE:
			{
				c_str<codec<"UTF-16">> str;

				str.capacity(max / sizeof(char16_t));

				if constexpr (IS_BIG) write_as_native(ifs, str);
				                 else write_as_foreign(ifs, str);

				return str;
			}
			case UTF32_LE:
			{
				c_str<codec<"UTF-32">> str;

				str.capacity(max / sizeof(char32_t));

				if constexpr (!IS_BIG) write_as_native(ifs, str);
				                  else write_as_foreign(ifs, str);

				return str;
			}
			case UTF32_BE:
			{
				c_str<codec<"UTF-32">> str;

				str.capacity(max / sizeof(char32_t));

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
#undef __1BSTR__
#undef __2BSTR__
#undef __4BSTR__

typedef c_str<codec<"UTF-8">> utf8;
typedef c_str<codec<"UTF-16">> utf16;
typedef c_str<codec<"UTF-32">> utf32;

typedef slice<codec<"UTF-8">> txt8;
typedef slice<codec<"UTF-16">> txt16;
typedef slice<codec<"UTF-32">> txt32;

#undef COPY_ASSIGNMENT
#undef MOVE_ASSIGNMENT

#undef COPY_CONSTRUCTOR
#undef MOVE_CONSTRUCTOR

template <size_t N> c_str(const char8_t (&str)[N]) -> c_str<codec<"UTF-8">>;
template <size_t N> c_str(const char16_t (&str)[N]) -> c_str<codec<"UTF-16">>;
template <size_t N> c_str(const char32_t (&str)[N]) -> c_str<codec<"UTF-32">>;

template <size_t N> slice(const char8_t (&str)[N]) -> slice<codec<"UTF-8">>;
template <size_t N> slice(const char16_t (&str)[N]) -> slice<codec<"UTF-16">>;
template <size_t N> slice(const char32_t (&str)[N]) -> slice<codec<"UTF-32">>;

} // namespace utf
