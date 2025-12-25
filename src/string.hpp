#pragma once

#include <cstdio>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include <bit>
#include <ios>
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <ostream>
#include <fstream>
#include <optional>
#include <concepts>
#include <algorithm>
#include <filesystem>

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

#define __C_STR__(name) const c_str<U, B>& name
#define __ARRAY__(name) const U (&name)[N]
#define __SLICE__(name) slice<U> name

//┌──────────────────────────────────────────────────────────────┐
//│ special thanks to facebook's folly::FBString.                │
//│                                                              │
//│ SSO mode uses every bytes of heap string struct using union  │
//│ this was achievable thanks to the very clever memory layout. │
//│                                                              │
//│ for more, watch https://www.youtube.com/watch?v=kPR8h4-qZdk. │
//└──────────────────────────────────────────────────────────────┘

inline auto operator<<(std::ostream& os, char32_t code) noexcept -> std::ostream&
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

template <typename T> concept unit_t = std::is_same_v<T, char8_t >
                                       ||
                                       std::is_same_v<T, char16_t>
                                       ||
                                       std::is_same_v<T, char32_t>;

template <typename T> concept allo_t = requires(T alloc, size_t N)
{
	typename std::allocator_traits<T>::size_type;
	typename std::allocator_traits<T>::value_type;

	{ alloc == alloc } -> std::same_as<bool>;
	{ alloc != alloc } -> std::same_as<bool>;

	{ std::allocator_traits<T>::allocate(alloc, N) };
};

template <unit_t T, allo_t A> class c_str ;
template <unit_t T /* ??? */> class slice ;
template <unit_t T /* ??? */> class codec ;
template <unit_t T /* ??? */> class cursor;
                              class concat;

enum class range : uint8_t {N};
struct clamp { const size_t _;
inline constexpr /**/ operator
size_t() const { return _; } };
inline constexpr auto operator-
(range, size_t offset) noexcept
-> clamp { return { offset }; }

namespace detail
{
	template <unit_t T>
	static constexpr auto _size(const T* head, const T* tail) noexcept -> size_t;

	template <unit_t T>
	static constexpr auto _length(const T* head, const T* tail) noexcept -> size_t;

	template <unit_t T,
	          unit_t U>
	static constexpr auto _fcopy(const U* head, const U* tail,
	                                            /*&*/ T* dest) noexcept -> size_t;

	template <unit_t T,
	          unit_t U>
	static constexpr auto _rcopy(const U* head, const U* tail,
	                                            /*&*/ T* dest) noexcept -> size_t;

	template <unit_t T,
	          unit_t U>
	static constexpr auto _equal(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> bool;

	template <unit_t T,
	          unit_t U>
	static constexpr auto _nqual(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> bool;

	template <unit_t T,
	          unit_t U>
	static constexpr auto _scan(const T* lhs_0, const T* lhs_N,
	                            const U* rhs_0, const U* rhs_N,
	                            const auto& fun /* lambda */) noexcept -> void;

	template <unit_t T,
	          unit_t U>
	static constexpr auto _split(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> std::vector<slice<T>>;

	template <unit_t T,
	          unit_t U>
	static constexpr auto _match(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> std::vector<slice<T>>;

	template <unit_t T>
	static constexpr auto _range(const T* head, const T* tail, clamp  start, clamp  until) noexcept -> slice<T>;
	template <unit_t T>
	static constexpr auto _range(const T* head, const T* tail, clamp  start, range  until) noexcept -> slice<T>;
	template <unit_t T>
	static constexpr auto _range(const T* head, const T* tail, size_t start, clamp  until) noexcept -> slice<T>;
	template <unit_t T>
	static constexpr auto _range(const T* head, const T* tail, size_t start, range  until) noexcept -> slice<T>;
	template <unit_t T>
	static constexpr auto _range(const T* head, const T* tail, size_t start, size_t until) noexcept -> slice<T>;
}

template <unit_t T, allo_t A = std::allocator<T>> class c_str
{
	typedef std::allocator_traits<A> allocator;

	friend slice<T>;
	friend concat;

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

	struct storage : A
	{
		union
		{
			buffer large;

			typedef T chunk_t;

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
	constexpr auto head() const noexcept -> const T*;
	// returns ptr to the first element.
	constexpr auto head() /*&*/ noexcept -> /*&*/ T*;

	// returns ptr to the one-past-the-last element.
	constexpr auto tail() const noexcept -> const T*;
	// returns ptr to the one-past-the-last element.
	constexpr auto tail() /*&*/ noexcept -> /*&*/ T*;

	// fixes invariant; use it after internal manipulation.
	constexpr auto __size__(size_t value) noexcept -> void;

	storage store;

	class reader
	{
		protected:
		c_str* src;
		size_t arg;

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

	class writer : public reader
	{
		// nothing to do...

	public: using reader::reader;

		constexpr auto operator=(char32_t code) noexcept -> writer&;
	};

	// series of sanity-check!

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
	std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>;

	template <unit_t U,
	          allo_t B>
	constexpr operator c_str<U, B>() const noexcept;
	constexpr operator std::string() const noexcept;

	constexpr c_str() noexcept = default;

	template <unit_t U>
	constexpr c_str(__SLICE__(str)) noexcept;
	template <unit_t U,
	          allo_t B>
	constexpr c_str(__C_STR__(str)) noexcept;
	template <unit_t U,
	          size_t N>
	constexpr c_str(__ARRAY__(str)) noexcept;

	COPY_CONSTRUCTOR(c_str);
	MOVE_CONSTRUCTOR(c_str);

	COPY_ASSIGNMENT(c_str);
	MOVE_ASSIGNMENT(c_str);

	// returns the number of code units, excluding NULL-TERMINATOR.
	constexpr auto size() const noexcept -> size_t;
	// returns the number of code points, excluding NULL-TERMINATOR.
	constexpr auto length() const noexcept -> size_t;

	// returns the number of code units it can hold, excluding NULL-TERMINATOR.
	constexpr auto capacity(/* getter */) const noexcept -> size_t;
	// changes the number of code units it can hold, excluding NULL-TERMINATOR.
	constexpr auto capacity(size_t value) /*&*/ noexcept -> void;

	// returns a list of string slice, of which are product of split aka division.
	template <unit_t U>
	constexpr auto split(__SLICE__(value)) const noexcept -> std::vector<slice<T>>;
	template <unit_t U,
	          allo_t B>
	constexpr auto split(__C_STR__(value)) const noexcept -> std::vector<slice<T>>;
	template <unit_t U,
	          size_t N>
	constexpr auto split(__ARRAY__(value)) const noexcept -> std::vector<slice<T>>;

	// returns a list of string slice, of which are product of search occurrence.
	template <unit_t U>
	constexpr auto match(__SLICE__(value)) const noexcept -> std::vector<slice<T>>;
	template <unit_t U,
	          allo_t B>
	constexpr auto match(__C_STR__(value)) const noexcept -> std::vector<slice<T>>;
	template <unit_t U,
	          size_t N>
	constexpr auto match(__ARRAY__(value)) const noexcept -> std::vector<slice<T>>;

	// *self explanatory* returns whether or not it starts with *parameter*.
	template <unit_t U>
	constexpr auto starts_with(__SLICE__(value)) const noexcept -> bool;
	template <unit_t U,
	          allo_t B>
	constexpr auto starts_with(__C_STR__(value)) const noexcept -> bool;
	template <unit_t U,
	          size_t N>
	constexpr auto starts_with(__ARRAY__(value)) const noexcept -> bool;

	// *self explanatory* returns whether or not it ends with *parameter*.
	template <unit_t U>
	constexpr auto ends_with(__SLICE__(value)) const noexcept -> bool;
	template <unit_t U,
	          allo_t B>
	constexpr auto ends_with(__C_STR__(value)) const noexcept -> bool;
	template <unit_t U,
	          size_t N>
	constexpr auto ends_with(__ARRAY__(value)) const noexcept -> bool;

	// *self explanatory* returns whether or not it contains *parameter*.
	template <unit_t U>
	constexpr auto contains(__SLICE__(value)) const noexcept -> bool;
	template <unit_t U,
	          allo_t B>
	constexpr auto contains(__C_STR__(value)) const noexcept -> bool;
	template <unit_t U,
	          size_t N>
	constexpr auto contains(__ARRAY__(value)) const noexcept -> bool;

	// iterator

	constexpr auto begin() const noexcept -> cursor<T>;
	constexpr auto end() const noexcept -> cursor<T>;

	// operators

	template <unit_t U>
	constexpr auto operator=(__SLICE__(rhs)) noexcept -> c_str&;
	template <unit_t U,
	          allo_t B>
	constexpr auto operator=(__C_STR__(rhs)) noexcept -> c_str&;
	template <unit_t U,
	          size_t N>
	constexpr auto operator=(__ARRAY__(rhs)) noexcept -> c_str&;

	template <unit_t U>
	constexpr auto operator+=(__SLICE__(rhs)) noexcept -> c_str&;
	template <unit_t U,
	          allo_t B>
	constexpr auto operator+=(__C_STR__(rhs)) noexcept -> c_str&;
	template <unit_t U,
	          size_t N>
	constexpr auto operator+=(__ARRAY__(rhs)) noexcept -> c_str&;

	template <unit_t U>
	constexpr auto operator+(__SLICE__(rhs)) const noexcept -> concat;
	template <unit_t U,
	          allo_t B>
	constexpr auto operator+(__C_STR__(rhs)) const noexcept -> concat;
	template <unit_t U,
	          size_t N>
	constexpr auto operator+(__ARRAY__(rhs)) const noexcept -> concat;

	template <unit_t U>
	constexpr auto operator==(__SLICE__(rhs)) const noexcept -> bool;
	template <unit_t U,
	          allo_t B>
	constexpr auto operator==(__C_STR__(rhs)) const noexcept -> bool;
	template <unit_t U,
	          size_t N>
	constexpr auto operator==(__ARRAY__(rhs)) const noexcept -> bool;

	template <unit_t U>
	constexpr auto operator!=(__SLICE__(rhs)) const noexcept -> bool;
	template <unit_t U,
	          allo_t B>
	constexpr auto operator!=(__C_STR__(rhs)) const noexcept -> bool;
	template <unit_t U,
	          size_t N>
	constexpr auto operator!=(__ARRAY__(rhs)) const noexcept -> bool;

	// read & write

	constexpr auto operator[](size_t value) const noexcept -> reader;
	constexpr auto operator[](size_t value) /*&*/ noexcept -> writer;

	// range syntax

	constexpr auto operator[](clamp  start, clamp  until) const noexcept -> slice<T>;
	constexpr auto operator[](clamp  start, range  until) const noexcept -> slice<T>;
	constexpr auto operator[](size_t start, clamp  until) const noexcept -> slice<T>;
	constexpr auto operator[](size_t start, range  until) const noexcept -> slice<T>;
	constexpr auto operator[](size_t start, size_t until) const noexcept -> slice<T>;

	// friends

	template <unit_t U,
	          size_t N>
	friend constexpr auto operator+(__ARRAY__(lhs), const c_str& rhs) noexcept -> concat;
	template <unit_t U,
	          size_t N>
	friend constexpr auto operator==(__ARRAY__(lhs), const c_str& rhs) noexcept -> bool;
	template <unit_t U,
	          size_t N>
	friend constexpr auto operator!=(__ARRAY__(lhs), const c_str& rhs) noexcept -> bool;

	// iostream

	friend auto operator<<(std::ostream& os, const c_str& str) noexcept -> std::ostream&
	{
		for (const auto code : str) { os << code; } return os;
	}

private:

	template <unit_t U>
	constexpr auto _equal(const U* rhs_0, const U* rhs_N) noexcept -> void;

	template <unit_t U>
	constexpr auto _pqual(const U* rhs_0, const U* rhs_N) noexcept -> void;
};

template <unit_t T /* c_str's non-owning view */> class slice
{
	friend c_str<T>;
	friend concat;

	const T* head;
	const T* tail;

	class reader
	{
		protected:
		slice* src;
		size_t arg;

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

	class writer : public reader
	{
		// nothing to do...

	public: using reader::reader;

		// constexpr auto operator=(char32_t code) noexcept -> writer&;
	};

public:

	constexpr slice
	(
		decltype(head) head,
		decltype(tail) tail
	)
	noexcept : head {head},
	           tail {tail}
	{
		// nothing to do...
	}

	template <size_t N>
	constexpr slice
	(
		const T (&str)[N]
	)
	noexcept : head {&str[N - N]},
	           tail {&str[N - 1]}
	{
		// nothing to do...
	}

	template <allo_t A>
	constexpr slice
	(
		const c_str<T, A>& str
	)
	noexcept : head {str.head()},
	           tail {str.tail()}
	{
		// nothing to do...
	}

	COPY_CONSTRUCTOR(slice) = default;
	MOVE_CONSTRUCTOR(slice) = default;

	COPY_ASSIGNMENT(slice) = default;
	MOVE_ASSIGNMENT(slice) = default;

	// returns the number of code units, excluding NULL-TERMINATOR.
	constexpr auto size() const noexcept -> size_t;
	// returns the number of code points, excluding NULL-TERMINATOR.
	constexpr auto length() const noexcept -> size_t;

	// returns a list of string slice, of which are product of split aka division.
	template <unit_t U>
	constexpr auto split(__SLICE__(value)) const noexcept -> std::vector<slice<T>>;
	template <unit_t U,
	          allo_t B>
	constexpr auto split(__C_STR__(value)) const noexcept -> std::vector<slice<T>>;
	template <unit_t U,
	          size_t N>
	constexpr auto split(__ARRAY__(value)) const noexcept -> std::vector<slice<T>>;

	// returns a list of string slice, of which are product of search occurrence.
	template <unit_t U>
	constexpr auto match(__SLICE__(value)) const noexcept -> std::vector<slice<T>>;
	template <unit_t U,
	          allo_t B>
	constexpr auto match(__C_STR__(value)) const noexcept -> std::vector<slice<T>>;
	template <unit_t U,
	          size_t N>
	constexpr auto match(__ARRAY__(value)) const noexcept -> std::vector<slice<T>>;

	// *self explanatory* returns whether or not it starts with *parameter*.
	template <unit_t U>
	constexpr auto starts_with(__SLICE__(value)) const noexcept -> bool;
	template <unit_t U,
	          allo_t B>
	constexpr auto starts_with(__C_STR__(value)) const noexcept -> bool;
	template <unit_t U,
	          size_t N>
	constexpr auto starts_with(__ARRAY__(value)) const noexcept -> bool;

	// *self explanatory* returns whether or not it ends with *parameter*.
	template <unit_t U>
	constexpr auto ends_with(__SLICE__(value)) const noexcept -> bool;
	template <unit_t U,
	          allo_t B>
	constexpr auto ends_with(__C_STR__(value)) const noexcept -> bool;
	template <unit_t U,
	          size_t N>
	constexpr auto ends_with(__ARRAY__(value)) const noexcept -> bool;

	// *self explanatory* returns whether or not it contains *parameter*.
	template <unit_t U>
	constexpr auto contains(__SLICE__(value)) const noexcept -> bool;
	template <unit_t U,
	          allo_t B>
	constexpr auto contains(__C_STR__(value)) const noexcept -> bool;
	template <unit_t U,
	          size_t N>
	constexpr auto contains(__ARRAY__(value)) const noexcept -> bool;

	// iterator

	constexpr auto begin() const noexcept -> cursor<T>;
	constexpr auto end() const noexcept -> cursor<T>;

	// operators

	template <unit_t U>
	constexpr auto operator+(__SLICE__(rhs)) const noexcept -> concat;
	template <unit_t U,
	          allo_t B>
	constexpr auto operator+(__C_STR__(rhs)) const noexcept -> concat;
	template <unit_t U,
	          size_t N>
	constexpr auto operator+(__ARRAY__(rhs)) const noexcept -> concat;

	template <unit_t U>
	constexpr auto operator==(__SLICE__(rhs)) const noexcept -> bool;
	template <unit_t U,
	          allo_t B>
	constexpr auto operator==(__C_STR__(rhs)) const noexcept -> bool;
	template <unit_t U,
	          size_t N>
	constexpr auto operator==(__ARRAY__(rhs)) const noexcept -> bool;

	template <unit_t U>
	constexpr auto operator!=(__SLICE__(rhs)) const noexcept -> bool;
	template <unit_t U,
	          allo_t B>
	constexpr auto operator!=(__C_STR__(rhs)) const noexcept -> bool;
	template <unit_t U,
	          size_t N>
	constexpr auto operator!=(__ARRAY__(rhs)) const noexcept -> bool;

	// read & write

	constexpr auto operator[](size_t value) const noexcept -> reader;
	constexpr auto operator[](size_t value) /*&*/ noexcept -> writer;

	// range syntax

	constexpr auto operator[](clamp  start, clamp  until) const noexcept -> slice<T>;
	constexpr auto operator[](clamp  start, range  until) const noexcept -> slice<T>;
	constexpr auto operator[](size_t start, clamp  until) const noexcept -> slice<T>;
	constexpr auto operator[](size_t start, range  until) const noexcept -> slice<T>;
	constexpr auto operator[](size_t start, size_t until) const noexcept -> slice<T>;

	// friends

	template <unit_t U,
	          size_t N>
	friend constexpr auto operator+(__ARRAY__(lhs), const slice& rhs) noexcept -> concat;
	template <unit_t U,
	          size_t N>
	friend constexpr auto operator==(__ARRAY__(lhs), const slice& rhs) noexcept -> bool;
	template <unit_t U,
	          size_t N>
	friend constexpr auto operator!=(__ARRAY__(lhs), const slice& rhs) noexcept -> bool;

	// iostream

	friend auto operator<<(std::ostream& os, const slice& str) noexcept -> std::ostream&
	{
		for (const auto code : str) { os << code; } return os;
	}
};

template <unit_t T> class codec
{
	// nothing to do...

public:

	static constexpr auto size(char32_t code) noexcept -> int8_t;
	static constexpr auto next(const T* data) noexcept -> int8_t;
	static constexpr auto back(const T* data) noexcept -> int8_t;

	static constexpr auto
	encode_ptr(const char32_t in, T* out, int8_t size) noexcept -> void;
	static constexpr auto
	decode_ptr(const T* in, char32_t& out, int8_t size) noexcept -> void;
};

template <unit_t T> class cursor
{
	const T* ptr;

public:

	cursor(decltype(ptr) ptr) : ptr(ptr) {}

	constexpr auto operator*() noexcept -> char32_t;
	constexpr auto operator&() noexcept -> const T*;

	constexpr auto operator++(   ) noexcept -> cursor&;
	constexpr auto operator++(int) noexcept -> cursor;

	constexpr auto operator--(   ) noexcept -> cursor&;
	constexpr auto operator--(int) noexcept -> cursor;

	constexpr auto operator==(const cursor& rhs) noexcept -> bool;
	constexpr auto operator!=(const cursor& rhs) noexcept -> bool;
};

/* proxy for A+B */ class concat
{
	typedef std::variant
	<
		slice<char8_t>
		,
		slice<char16_t>
		,
		slice<char32_t>
	>
	str;

	std::vector<str> data;

public:

	constexpr concat
	(
		str lhs,
		str rhs
	)
	noexcept : data {lhs, rhs}
	{
		// nothing to do...
	}

	template <unit_t T>
	[[nodiscard]] constexpr operator c_str<T>() const noexcept;

	// reset

	template <unit_t U>
	constexpr auto operator=(__SLICE__(rhs)) noexcept -> concat&;
	template <unit_t U,
	          allo_t B>
	constexpr auto operator=(__C_STR__(rhs)) noexcept -> concat&;
	template <unit_t U,
	          size_t N>
	constexpr auto operator=(__ARRAY__(rhs)) noexcept -> concat&;

	// append

	template <unit_t U>
	constexpr auto operator+(__SLICE__(rhs)) noexcept -> concat&;
	template <unit_t U,
	          allo_t B>
	constexpr auto operator+(__C_STR__(rhs)) noexcept -> concat&;
	template <unit_t U,
	          size_t N>
	constexpr auto operator+(__ARRAY__(rhs)) noexcept -> concat&;
};

#pragma region codec

template <unit_t T /* ??? */> constexpr auto codec<T>::size(char32_t code) noexcept -> int8_t
{
	if constexpr (std::is_same_v<T, char8_t>)
	{
		const auto N {std::bit_width
		(static_cast<uint32_t>(code))};

		//┌───────────────────────┐
		//│ U+000000 ... U+00007F │ -> 1 code unit
		//│ U+000080 ... U+0007FF │ -> 2 code unit
		//│ U+000800 ... U+00FFFF │ -> 3 code unit
		//│ U+010000 ... U+10FFFF │ -> 4 code unit
		//└───────────────────────┘

		return 1 + (8 <= N) + (12 <= N) + (17 <= N);
	}

	if constexpr (std::is_same_v<T, char16_t>)
	{
		//┌───────────────────────┐
		//│ U+000000 ... U+00D7FF │ -> 1 code unit
		//│ U+00E000 ... U+00FFFF │ -> 1 code unit
		//│ U+000000 ... U+10FFFF │ -> 2 code unit
		//└───────────────────────┘

		return 1 + (0xFFFF /* surrogate */ < code);
	}

	if constexpr (std::is_same_v<T, char32_t>)
	{
		return 1;
	}
}

template <unit_t T /* ??? */> constexpr auto codec<T>::next(const T* data) noexcept -> int8_t
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

	if constexpr (std::is_same_v<T, char8_t>)
	{
		return TABLE[(data[0] >> 0x4) & 0x0F];
	}

	if constexpr (std::is_same_v<T, char16_t>)
	{
		return +1 + ((data[0] >> 0xA) == 0x36);
	}

	if constexpr (std::is_same_v<T, char32_t>)
	{
		return +1;
	}
}

template <unit_t T /* ??? */> constexpr auto codec<T>::back(const T* data) noexcept -> int8_t
{
	if constexpr (std::is_same_v<T, char8_t>)
	{
		int8_t i {-1};
		
		for (; (data[i] & 0xC0) == 0x80; --i) {}
		
		return i;
	}

	if constexpr (std::is_same_v<T, char16_t>)
	{
		int8_t i {-1};
		
		for (; (data[i] >> 0xA) == 0x37; --i) {}
		
		return i;
	}

	if constexpr (std::is_same_v<T, char32_t>)
	{
		return -1;
	}
}

template <unit_t T /* ??? */> constexpr auto codec<T>::encode_ptr(const char32_t in, T *out, int8_t size) noexcept -> void
{
	if constexpr (std::is_same_v<T, char8_t>)
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

	if constexpr (std::is_same_v<T, char16_t>)
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

	if constexpr (std::is_same_v<T, char32_t>)
	{
		out[0] = static_cast<T>(in);
	}
}

template <unit_t T /* ??? */> constexpr auto codec<T>::decode_ptr(const T *in, char32_t &out, int8_t size) noexcept -> void
{
	if constexpr (std::is_same_v<T, char8_t>)
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

	if constexpr (std::is_same_v<T, char16_t>)
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

	if constexpr (std::is_same_v<T, char32_t>)
	{
		out = static_cast<char32_t>(in[0]);
	}
}

#pragma endregion codec
#pragma region cursor

template <unit_t T /* ??? */> constexpr auto cursor<T>::operator*() noexcept -> char32_t
{
	char32_t T_out;

	const auto T_size {codec<T>::next(this->ptr)};
	codec<T>::decode_ptr(this->ptr, T_out, T_size);

	return T_out;
}

template <unit_t T /* ??? */> constexpr auto cursor<T>::operator&() noexcept -> const T*
{
	return this->ptr;
}

template <unit_t T /* ??? */> constexpr auto cursor<T>::operator++(   ) noexcept -> cursor&
{
	this->ptr += codec<T>::next(this->ptr); return *this;
}

template <unit_t T /* ??? */> constexpr auto cursor<T>::operator++(int) noexcept -> cursor
{
	const auto clone {*this}; operator++(); return clone;
}

template <unit_t T /* ??? */> constexpr auto cursor<T>::operator--(   ) noexcept -> cursor&
{
	this->ptr += codec<T>::back(this->ptr); return *this;
}

template <unit_t T /* ??? */> constexpr auto cursor<T>::operator--(int) noexcept -> cursor
{
	const auto clone {*this}; operator--(); return clone;
}

template <unit_t T /* ??? */> constexpr auto cursor<T>::operator==(const cursor& rhs) noexcept -> bool
{
	return this->ptr == rhs.ptr;
}

template <unit_t T /* ??? */> constexpr auto cursor<T>::operator!=(const cursor& rhs) noexcept -> bool
{
	return this->ptr != rhs.ptr;
}

#pragma endregion cursor
#pragma region concat

template <unit_t T /* ??? */> constexpr concat::operator c_str<T>() const noexcept
{
	c_str<T> out; size_t size {0};

	for (const auto& str : this->data)
	{
		if (std::holds_alternative<slice<T>>(str))
		{
			size += std::get<slice<T>>(str).size();
		}
		else if (std::holds_alternative<slice<char8_t>>(str))
		{
			typedef char8_t U;

			for (const auto code : std::get<slice<U>>(str))
			{
				size += codec<T>::size(code);
			}
		}
		else if (std::holds_alternative<slice<char16_t>>(str))
		{
			typedef char16_t U;

			for (const auto code : std::get<slice<U>>(str))
			{
				size += codec<T>::size(code);
			}
		}
		else if (std::holds_alternative<slice<char32_t>>(str))
		{
			typedef char32_t U;

			for (const auto code : std::get<slice<U>>(str))
			{
				size += codec<T>::size(code);
			}
		}
	}

	out.capacity(size);
	T* ptr {out.head()};
	out.__size__(size);

	for (const auto& str : this->data)
	{
		if (std::holds_alternative<slice<T>>(str))
		{
			ptr += detail::_fcopy(std::get<slice<T>>(str).head, std::get<slice<T>>(str).tail, ptr);
		}
		else if (std::holds_alternative<slice<char8_t>>(str))
		{
			typedef char8_t U;

			ptr += detail::_fcopy(std::get<slice<U>>(str).head, std::get<slice<U>>(str).tail, ptr);
		}
		else if (std::holds_alternative<slice<char16_t>>(str))
		{
			typedef char16_t U;

			ptr += detail::_fcopy(std::get<slice<U>>(str).head, std::get<slice<U>>(str).tail, ptr);
		}
		else if (std::holds_alternative<slice<char32_t>>(str))
		{
			typedef char32_t U;

			ptr += detail::_fcopy(std::get<slice<U>>(str).head, std::get<slice<U>>(str).tail, ptr);
		}
	}

	return out;
}

template <unit_t U /* ??? */> constexpr auto concat::operator=(__SLICE__(rhs)) noexcept -> concat&
{
	this->data.clear(); this->data.push_back(rhs); return *this;
}

template <unit_t U, allo_t B> constexpr auto concat::operator=(__C_STR__(rhs)) noexcept -> concat&
{
	this->data.clear(); this->data.push_back(rhs); return *this;
}

template <unit_t U, size_t N> constexpr auto concat::operator=(__ARRAY__(rhs)) noexcept -> concat&
{
	this->data.clear(); this->data.push_back(rhs); return *this;
}

template <unit_t U /* ??? */> constexpr auto concat::operator+(__SLICE__(rhs)) noexcept -> concat&
{
	/* keep it as-is */ this->data.push_back(rhs); return *this;
}

template <unit_t U, allo_t B> constexpr auto concat::operator+(__C_STR__(rhs)) noexcept -> concat&
{
	/* keep it as-is */ this->data.push_back(rhs); return *this;
}

template <unit_t U, size_t N> constexpr auto concat::operator+(__ARRAY__(rhs)) noexcept -> concat&
{
	/* keep it as-is */ this->data.push_back(rhs); return *this;
}

#pragma endregion concat
#pragma region detail

namespace detail
{
	template <unit_t T>
	static constexpr auto _size(const T* head, const T* tail) noexcept -> size_t
	{
		if constexpr (std::is_same_v<T, char8_t>)
		{
			return tail - head;
		}

		if constexpr (std::is_same_v<T, char16_t>)
		{
			return tail - head;
		}

		if constexpr (std::is_same_v<T, char32_t>)
		{
			return tail - head;
		}
	}

	template <unit_t T>
	static constexpr auto _length(const T* head, const T* tail) noexcept -> size_t
	{
		if constexpr (std::is_same_v<T, char8_t>)
		{
			size_t out {0};

			for (const T* ptr {head}; ptr < tail; ++out, ptr += codec<T>::next(ptr)) {}

			return out;
		}

		if constexpr (std::is_same_v<T, char16_t>)
		{
			size_t out {0};

			for (const T* ptr {head}; ptr < tail; ++out, ptr += codec<T>::next(ptr)) {}

			return out;
		}

		if constexpr (std::is_same_v<T, char32_t>)
		{
			// size_t out {0};

			// for (const T* ptr {head}; ptr < tail; ++out, ptr += codec<T>::next(ptr)) {}

			// return out;

			return tail - head;
		}
	}

	template <unit_t T,
	          unit_t U>
	static constexpr auto _fcopy(const U* head, const U* tail,
	                                            /*&*/ T* dest) noexcept -> size_t
	{
		if constexpr (std::is_same_v<T, U>)
		{
			std::ranges::copy/*forward*/(head, tail, dest); return tail - head;
		}

		if constexpr (!std::is_same_v<T, U>)
		{
			T* out {dest};

			// for (const U* ptr {head}; ptr < tail; )
			// {
			// 	char32_t U_out;
			//
			// 	const auto U_size {codec<U>::next(ptr)};
			// 	codec<U>::decode_ptr(ptr, U_out, U_size);
			// 	const auto T_size {codec<T>::size(U_out)};
			//
			// 	ptr += U_size;
			// 	out += T_size;
			// }

			for (const U* ptr {head}; ptr < tail; )
			{
				char32_t U_out;

				const auto U_size {codec<U>::next(ptr)};
				codec<U>::decode_ptr(ptr, U_out, U_size);
				const auto T_size {codec<T>::size(U_out)};
				codec<T>::encode_ptr(U_out, out, T_size);

				ptr += U_size;
				out += T_size;
			}

			return out - dest;
		}
	}

	template <unit_t T,
	          unit_t U>
	static constexpr auto _rcopy(const U* head, const U* tail,
	                                            /*&*/ T* dest) noexcept -> size_t
	{
		if constexpr (std::is_same_v<T, U>)
		{
			std::ranges::copy_backward(head, tail, dest); return tail - head;
		}

		if constexpr (!std::is_same_v<T, U>)
		{
			T* out {dest};

			for (const U* ptr {head}; ptr < tail; )
			{
				char32_t U_out;

				const auto U_size {codec<U>::next(ptr)};
				codec<U>::decode_ptr(ptr, U_out, U_size);
				const auto T_size {codec<T>::size(U_out)};

				ptr += U_size;
				out += T_size;
			}

			for (const U* ptr {tail}; head < ptr; )
			{
				char32_t U_out;

				const auto U_size {codec<U>::back(ptr)};
				codec<U>::decode_ptr(ptr, U_out, U_size);
				const auto T_size {codec<T>::size(U_out)};
				codec<T>::encode_ptr(U_out, out, T_size);

				ptr += U_size;
				out += T_size;
			}

			return out - dest;
		}
	}

	template <unit_t T,
	          unit_t U>
	static constexpr auto _equal(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> bool
	{
		if constexpr (std::is_same_v<T, U>)
		{
			return detail::_size(lhs_0, lhs_N)
			       ==
			       detail::_size(rhs_0, rhs_N)
			       &&
			       std::ranges::equal(lhs_0, lhs_N, rhs_0, rhs_N);
		}

		if constexpr (!std::is_same_v<T, U>)
		{
			const T* lhs_ptr {lhs_0};
			const U* rhs_ptr {rhs_0};

			for (; lhs_ptr < lhs_N && rhs_ptr < rhs_N;)
			{
				char32_t T_out;
				char32_t U_out;

				const auto T_size {codec<T>::next(lhs_ptr)};
				const auto U_size {codec<U>::next(rhs_ptr)};

				codec<T>::decode_ptr(lhs_ptr, T_out, T_size);
				codec<U>::decode_ptr(rhs_ptr, U_out, U_size);

				if (T_out != U_out)
				{
					return false;
				}

				lhs_ptr += T_size;
				rhs_ptr += U_size;
			}
			return lhs_ptr == lhs_N && rhs_ptr == rhs_N;
		}
	}

	template <unit_t T,
	          unit_t U>
	static constexpr auto _nqual(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> bool
	{
		if constexpr (std::is_same_v<T, U>)
		{
			return detail::_size(lhs_0, lhs_N)
			       !=
			       detail::_size(rhs_0, rhs_N)
			       ||
			       !std::ranges::equal(lhs_0, lhs_N, rhs_0, rhs_N);
		}

		if constexpr (!std::is_same_v<T, U>)
		{
			const T* lhs_ptr {lhs_0};
			const U* rhs_ptr {rhs_0};

			for (; lhs_ptr < lhs_N && rhs_ptr < rhs_N;)
			{
				char32_t T_out;
				char32_t U_out;

				const auto T_size {codec<T>::next(lhs_ptr)};
				const auto U_size {codec<U>::next(rhs_ptr)};

				codec<T>::decode_ptr(lhs_ptr, T_out, T_size);
				codec<U>::decode_ptr(rhs_ptr, U_out, U_size);

				if (T_out != U_out)
				{
					return true;
				}

				lhs_ptr += T_size;
				rhs_ptr += U_size;
			}
			return lhs_ptr != lhs_N || rhs_ptr != rhs_N;
		}
	}

	template <unit_t T,
	          unit_t U>
	static constexpr auto _scan(const T* lhs_0, const T* lhs_N,
	                            const U* rhs_0, const U* rhs_N,
	                            const auto& fun /* lambda */) noexcept -> void
	{
		typedef char32_t code_t;

		if (lhs_N - lhs_0 == 0) return;
		if (rhs_N - rhs_0 == 0) return;

		const auto lhs_len {detail::_length(lhs_0, lhs_N)};
		const auto rhs_len {detail::_length(rhs_0, rhs_N)};

		if (lhs_len < rhs_len) return;

		std::vector<size_t> lps (rhs_len); lps[0] = 0;
		std::vector<code_t> ptn (rhs_len); ptn[0] = 0;

		// LPS build
		{
			size_t i {0};
			size_t j {0};

			for (cursor<U> it {rhs_0}; &it != rhs_N; ++it)
			{
				ptn[i] = *it;

				while (0 < j && ptn[i] != ptn[j])
				{
					j = lps[j - 1];
				}
				j += (0 < i && ptn[i] == ptn[j]);

				lps[i++] = j;
			}
		}

		// KMP search
		{
			cursor<T> head {nullptr};
			cursor<T> tail {nullptr};

			size_t j {0};

			for (cursor<T> it {lhs_0}; &it != lhs_N; ++it)
			{
				const auto code {*(tail = it)};

				while (0 < j && code != ptn[j])
				{
					j = lps[j - 1];
				}
				// attempt match extension..!

				if (code == ptn[j])
				{
					if (j++ == 0)
					{
						head = it;
					}
				}
				if (j == rhs_len)
				{
					assert(&head != nullptr);
					assert(&tail != nullptr);

					fun(&head, &++tail);

					j = lps[j - 1];
				}
			}
		}
	}

	template <unit_t T,
	          unit_t U>
	static constexpr auto _split(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> std::vector<slice<T>>
	{
		std::vector<slice<T>> out;

		const T* last {lhs_0};

		detail::_scan
		(
			lhs_0, lhs_N,
			rhs_0, rhs_N,
			// calback on every iteration..!
			[&](const T* head, const T* tail)
			{
				out.emplace_back(last, head);

				last = tail;
			}
		);

		if (last < lhs_N)
		{
			out.emplace_back(last, lhs_N);
		}

		return out;
	}

	template <unit_t T,
	          unit_t U>
	static constexpr auto _match(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> std::vector<slice<T>>
	{
		std::vector<slice<T>> out;

		detail::_scan
		(
			lhs_0, lhs_N,
			rhs_0, rhs_N,
			// calback on every iteration..!
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
	
	template <unit_t T>
	static constexpr auto _range(const T* head, const T* tail, clamp  start, clamp  until) noexcept -> slice<T>
	{
		assert(until < start);

		const T* foo {tail};
		
		for (size_t i {  0  }; i < until && head < foo; ++i, foo += codec<T>::back(foo)) {}

		const T* bar {foo};

		for (size_t i {until}; i < start && head < bar; ++i, bar += codec<T>::back(bar)) {}

		assert(head <= foo && foo <= tail);
		assert(head <= bar && bar <= tail);

		return {bar, foo};
	}

	template <unit_t T>
	static constexpr auto _range(const T* head, const T* tail, clamp  start, range  until) noexcept -> slice<T>
	{
		const T* foo {tail};

		for (size_t i {  0  }; i < start && head < foo; ++i, foo += codec<T>::back(foo)) {}

		const T* bar {tail};

		assert(head <= foo && foo <= tail);
		assert(head <= bar && bar <= tail);

		return {foo, bar};
	}

	template <unit_t T>
	static constexpr auto _range(const T* head, const T* tail, size_t start, clamp  until) noexcept -> slice<T>
	{
		const T* foo {head};

		for (size_t i {  0  }; i < start && foo < tail; ++i, foo += codec<T>::next(foo)) {}

		const T* bar {tail};

		for (size_t i {  0  }; i < until && head < bar; ++i, bar += codec<T>::back(bar)) {}

		assert(head <= foo && foo <= tail);
		assert(head <= bar && bar <= tail);

		return {foo, bar};
	}

	template <unit_t T>
	static constexpr auto _range(const T* head, const T* tail, size_t start, range  until) noexcept -> slice<T>
	{
		const T* foo {head};

		for (size_t i {  0  }; i < start && foo < tail; ++i, foo += codec<T>::next(foo)) {}

		const T* bar {tail};

		assert(head <= foo && foo <= tail);
		assert(head <= bar && bar <= tail);

		return {foo, bar};
	}

	template <unit_t T>
	static constexpr auto _range(const T* head, const T* tail, size_t start, size_t until) noexcept -> slice<T>
	{
		assert(start < until);

		const T* foo {head};

		for (size_t i {  0  }; i < start && foo < tail; ++i, foo += codec<T>::next(foo)) {}

		const T* bar {foo};

		for (size_t i {start}; i < until && bar < tail; ++i, bar += codec<T>::next(bar)) {}

		assert(head <= foo && foo <= tail);
		assert(head <= bar && bar <= tail);

		return {foo, bar};
	}
}

#pragma endregion detail
#pragma region SSO23

template <unit_t T, allo_t A> constexpr c_str<T, A>::buffer::operator const T*() const noexcept
{
	return this->head;
}

template <unit_t T, allo_t A> constexpr c_str<T, A>::buffer::operator /*&*/ T*() /*&*/ noexcept
{
	return this->head;
}

template <unit_t T, allo_t A> constexpr c_str<T, A>::storage::storage() noexcept
{
	this->__union__.bytes[RMB] = MAX << SFT;
	std::construct_at(this->__union__.small);
}

template <unit_t T, allo_t A> constexpr c_str<T, A>::storage::~storage() noexcept
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

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::storage::mode() const noexcept -> mode_t
{
	return static_cast<mode_t>(this->__union__.bytes[RMB] & MSK);
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::storage::mode() /*&*/ noexcept -> mode_t
{
	return static_cast<mode_t>(this->__union__.bytes[RMB] & MSK);
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::head() const noexcept -> const T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[0 /* ✨ roeses are red, violets are blue ✨ */]
	       :
	       &this->store.__union__.large[0 /* ✨ roeses are red, violets are blue ✨ */];
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::head() /*&*/ noexcept -> /*&*/ T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[0 /* ✨ roeses are red, violets are blue ✨ */]
	       :
	       &this->store.__union__.large[0 /* ✨ roeses are red, violets are blue ✨ */];
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::tail() const noexcept -> const T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[MAX - (this->store.__union__.bytes[RMB] >> SFT)]
	       :
	       &this->store.__union__.large[0x0 + (this->store.__union__.large.size >> 0x0)];
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::tail() /*&*/ noexcept -> /*&*/ T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[MAX - (this->store.__union__.bytes[RMB] >> SFT)]
	       :
	       &this->store.__union__.large[0x0 + (this->store.__union__.large.size >> 0x0)];
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::capacity(/* getter */) const noexcept -> size_t
{
	return this->store.mode() == SMALL
	       ?
	       MAX // or calculate the ptrdiff_t just as large mode as shown down below
	       :
	       this->store.__union__.large.tail - this->store.__union__.large.head - 1;
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::capacity(size_t value) /*&*/ noexcept -> void
{
	if (this->capacity() < value)
	{
		T* head {allocator::allocate(this->store, value + 1)};
		T* tail {/* <one-past-the-end!> */(head + value + 1)};

		const auto size {this->size()};

		detail::_fcopy
		(
			this->head(),
			this->tail(),
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

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::__size__(size_t value) noexcept -> void
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

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr c_str<T, A>::operator c_str<U, B>() const noexcept
{
	c_str<U, B> out;

	if (0 < this->size())
	{
		size_t size {0};

		const T* head {this->head()};
		const T* tail {this->tail()};

		for (/* walk through self, from head to tail */; head < tail; )
		{
			char32_t T_out;

			const auto T_size {codec<T>::next(head)};
			codec<T>::decode_ptr(head, T_out, T_size);
			const auto U_size {codec<U>::size(T_out)};

			head += T_size;
			size += U_size;
		}

		out.capacity(size);
		out.__size__(size);
		// back to the start
		head = this->head();

		for (U* data {reinterpret_cast<U*>(out.head())}; head < tail; )
		{
			char32_t T_out;

			const auto T_size {codec<T>::next(head)};
			codec<T>::decode_ptr(head, T_out, T_size);
			const auto U_size {codec<U>::size(T_out)};
			codec<U>::encode_ptr(T_out, data, U_size);

			head += T_size;
			data += U_size;
		}
	}
	return out;
}

template <unit_t T, allo_t A> constexpr c_str<T, A>::operator std::string() const noexcept
{
	typedef char8_t U;

	std::string out;

	if (0 < this->size())
	{
		size_t size {0};

		const T* head {this->head()};
		const T* tail {this->tail()};

		for (/* walk through self, from head to tail */; head < tail; )
		{
			char32_t T_out;

			const auto T_size {codec<T>::next(head)};
			codec<T>::decode_ptr(head, T_out, T_size);
			const auto U_size {codec<U>::size(T_out)};

			head += T_size;
			size += U_size;
		}

		out.resize(size);
		// back to the start
		head = this->head();

		for (U* data {reinterpret_cast<U*>(out.data())}; head < tail; )
		{
			char32_t T_out;

			const auto T_size {codec<T>::next(head)};
			codec<T>::decode_ptr(head, T_out, T_size);
			const auto U_size {codec<U>::size(T_out)};
			codec<U>::encode_ptr(T_out, data, U_size);

			head += T_size;
			data += U_size;
		}
	}
	return out;
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr c_str<T, A>::c_str(__SLICE__(str)) noexcept
{
	this->operator=(str);
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr c_str<T, A>::c_str(__C_STR__(str)) noexcept
{
	this->operator=(str);
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr c_str<T, A>::c_str(__ARRAY__(str)) noexcept
{
	this->operator=(str);
}

template <unit_t T, allo_t A> constexpr c_str<T, A>::c_str(const c_str& other) noexcept
{
	if (this != &other)
	{
		this->capacity(other.size());

		detail::_fcopy
		(
			other.head(),
			other.tail(),
			this->head()
		);

		this->__size__(other.size());
	}
}

template <unit_t T, allo_t A> constexpr c_str<T, A>::c_str(/*&*/ c_str&& other) noexcept
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

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::operator=(const c_str& other) noexcept -> c_str&
{
	if (this != &other)
	{
		this->capacity(other.size());

		detail::_fcopy
		(
			other.head(),
			other.tail(),
			this->head()
		);

		this->__size__(other.size());
	}
	return *this;
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::operator=(/*&*/ c_str&& other) noexcept -> c_str&
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

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::size() const noexcept -> size_t
{
	return detail::_size(this->head(), this->tail());
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::length() const noexcept -> size_t
{
	return detail::_length(this->head(), this->tail());
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::split(__SLICE__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_split(this->head(), this->tail(), value.head, value.tail);
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr auto c_str<T, A>::split(__C_STR__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_split(this->head(), this->tail(), value.head(), value.tail());
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr auto c_str<T, A>::split(__ARRAY__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_split(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::match(__SLICE__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_match(this->head(), this->tail(), value.head, value.tail);
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr auto c_str<T, A>::match(__C_STR__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_match(this->head(), this->tail(), value.head(), value.tail());
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr auto c_str<T, A>::match(__ARRAY__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_match(this->head(), this->tail(), &value[N - N], &value[N - 1]);
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::starts_with(__SLICE__(value)) const noexcept -> bool
{
	return detail::_range(this->head(), this->tail(), 0, detail::_length(value.head, value.tail)) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr auto c_str<T, A>::starts_with(__C_STR__(value)) const noexcept -> bool
{
	return detail::_range(this->head(), this->tail(), 0, detail::_length(value.head(), value.tail())) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr auto c_str<T, A>::starts_with(__ARRAY__(value)) const noexcept -> bool
{
	return detail::_range(this->head(), this->tail(), 0, detail::_length(&value[N - N], &value[N - 1])) == value;
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::ends_with(__SLICE__(value)) const noexcept -> bool
{
	return detail::_range(this->head(), this->tail(), range::N - detail::_length(value.head, value.tail), range::N) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr auto c_str<T, A>::ends_with(__C_STR__(value)) const noexcept -> bool
{
	return detail::_range(this->head(), this->tail(), range::N - detail::_length(value.head(), value.tail()), range::N) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr auto c_str<T, A>::ends_with(__ARRAY__(value)) const noexcept -> bool
{
	return detail::_range(this->head(), this->tail(), range::N - detail::_length(&value[N - N], &value[N - 1]), range::N) == value;
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::contains(__SLICE__(value)) const noexcept -> bool
{
	return !detail::_match(this->head(), this->tail(), value.head, value.tail).empty();
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr auto c_str<T, A>::contains(__C_STR__(value)) const noexcept -> bool
{
	return !detail::_match(this->head(), this->tail(), value.head(), value.tail()).empty();
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr auto c_str<T, A>::contains(__ARRAY__(value)) const noexcept -> bool
{
	return !detail::_match(this->head(), this->tail(), &value[N - N], &value[N - 1]).empty();
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::begin() const noexcept -> cursor<T>
{
	return {this->head()};
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::end() const noexcept -> cursor<T>
{
	return {this->tail()};
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::operator=(__SLICE__(rhs)) noexcept -> c_str&
{
	_equal(rhs.head, rhs.tail); return *this;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr auto c_str<T, A>::operator=(__C_STR__(rhs)) noexcept -> c_str&
{
	_equal(rhs.head(), rhs.tail()); return *this;
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr auto c_str<T, A>::operator=(__ARRAY__(rhs)) noexcept -> c_str&
{
	_equal(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::operator+=(__SLICE__(rhs)) noexcept -> c_str&
{
	_pqual(rhs.head, rhs.tail); return *this;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr auto c_str<T, A>::operator+=(__C_STR__(rhs)) noexcept -> c_str&
{
	_pqual(rhs.head(), rhs.tail()); return *this;
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr auto c_str<T, A>::operator+=(__ARRAY__(rhs)) noexcept -> c_str&
{
	_pqual(&rhs[N - N], &rhs[N - 1]); return *this;
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::operator+(__SLICE__(rhs)) const noexcept -> concat
{
	return {*this, slice<U> {rhs}};
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr auto c_str<T, A>::operator+(__C_STR__(rhs)) const noexcept -> concat
{
	return {*this, slice<U> {rhs}};
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr auto c_str<T, A>::operator+(__ARRAY__(rhs)) const noexcept -> concat
{
	return {*this, slice<U> {rhs}};
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::operator==(__SLICE__(rhs)) const noexcept -> bool
{
	return detail::_equal(this->head(), this->tail(), rhs.head, rhs.tail);
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr auto c_str<T, A>::operator==(__C_STR__(rhs)) const noexcept -> bool
{
	return detail::_equal(this->head(), this->tail(), rhs.head(), rhs.tail());
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr auto c_str<T, A>::operator==(__ARRAY__(rhs)) const noexcept -> bool
{
	return detail::_equal(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::operator!=(__SLICE__(rhs)) const noexcept -> bool
{
	return detail::_nqual(this->head(), this->tail(), rhs.head, rhs.tail);
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B> constexpr auto c_str<T, A>::operator!=(__C_STR__(rhs)) const noexcept -> bool
{
	return detail::_nqual(this->head(), this->tail(), rhs.head(), rhs.tail());
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N> constexpr auto c_str<T, A>::operator!=(__ARRAY__(rhs)) const noexcept -> bool
{
	return detail::_nqual(this->head(), this->tail(), &rhs[N - N], &rhs[N - 1]);
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::operator[](size_t value) const noexcept -> reader
{
	return {this, value};
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::operator[](size_t value) /*&*/ noexcept -> writer
{
	return {this, value};
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::operator[](clamp  start, clamp  until) const noexcept -> slice<T>
{
	return detail::_range(this->head(), this->tail(), start, until);
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::operator[](clamp  start, range  until) const noexcept -> slice<T>
{
	return detail::_range(this->head(), this->tail(), start, until);
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::operator[](size_t start, clamp  until) const noexcept -> slice<T>
{
	return detail::_range(this->head(), this->tail(), start, until);
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::operator[](size_t start, range  until) const noexcept -> slice<T>
{
	return detail::_range(this->head(), this->tail(), start, until);
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::operator[](size_t start, size_t until) const noexcept -> slice<T>
{
	return detail::_range(this->head(), this->tail(), start, until);
}

template <unit_t T, allo_t A,
          unit_t U, size_t N> constexpr auto operator+(__ARRAY__(lhs), const c_str<T, A>& rhs) noexcept -> concat
{
	return {slice<U> {&lhs[N - N], &lhs[N - 1]}, rhs};
}

template <unit_t T, allo_t A,
          unit_t U, size_t N> constexpr auto operator==(__ARRAY__(lhs), const c_str<T, A>& rhs) noexcept -> bool
{
	return detail::_equal(&lhs[N - N], &lhs[N - 1], rhs.head(), rhs.tail());
}

template <unit_t T, allo_t A,
          unit_t U, size_t N> constexpr auto operator!=(__ARRAY__(lhs), const c_str<T, A>& rhs) noexcept -> bool
{
	return detail::_nqual(&lhs[N - N], &lhs[N - 1], rhs.head(), rhs.tail());
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::_equal(const U* rhs_0, const U* rhs_N) noexcept -> void
{
	size_t size {0};

	if constexpr (std::is_same_v<T, U>) /* ptrdiff */ { size += detail::_size(rhs_0, rhs_N); }
	else { for (const auto code : slice<U> {rhs_0, rhs_N}) { size += codec<T>::size(code); } }
	
	// size += this->size();

	this->capacity(size);

	detail::_fcopy
	(
		rhs_0, // U*
		rhs_N, // U*
		this->head()
	);

	this->__size__(size);
}

template <unit_t T, allo_t A>
template <unit_t U /* ??? */> constexpr auto c_str<T, A>::_pqual(const U* rhs_0, const U* rhs_N) noexcept -> void
{
	size_t size {0};

	if constexpr (std::is_same_v<T, U>) /* ptrdiff */ { size += detail::_size(rhs_0, rhs_N); }
	else { for (const auto code : slice<U> {rhs_0, rhs_N}) { size += codec<T>::size(code); } }
	
	size += this->size();

	this->capacity(size);

	detail::_fcopy
	(
		rhs_0, // U*
		rhs_N, // U*
		this->tail()
	);

	this->__size__(size);
}

#pragma endregion c_str
#pragma region slice

template <unit_t T /* ??? */> constexpr auto slice<T>::size() const noexcept -> size_t
{
	return detail::_size(this->head, this->tail);
}

template <unit_t T /* ??? */> constexpr auto slice<T>::length() const noexcept -> size_t
{
	return detail::_length(this->head, this->tail);
}

template <unit_t T /* ??? */>
template <unit_t U /* ??? */> constexpr auto slice<T>::split(__SLICE__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_split(this->head, this->tail, value.head, value.tail);
}

template <unit_t T /* ??? */>
template <unit_t U, allo_t B> constexpr auto slice<T>::split(__C_STR__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_split(this->head, this->tail, value.head(), value.tail());
}

template <unit_t T /* ??? */>
template <unit_t U, size_t N> constexpr auto slice<T>::split(__ARRAY__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_split(this->head, this->tail, &value[N - N], &value[N - 1]);
}

template <unit_t T /* ??? */>
template <unit_t U /* ??? */> constexpr auto slice<T>::match(__SLICE__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_match(this->head, this->tail, value.head, value.tail);
}

template <unit_t T /* ??? */>
template <unit_t U, allo_t B> constexpr auto slice<T>::match(__C_STR__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_match(this->head, this->tail, value.head(), value.tail());
}

template <unit_t T /* ??? */>
template <unit_t U, size_t N> constexpr auto slice<T>::match(__ARRAY__(value)) const noexcept -> std::vector<slice<T>>
{
	return detail::_match(this->head, this->tail, &value[N - N], &value[N - 1]);
}

template <unit_t T /* ??? */>
template <unit_t U /* ??? */> constexpr auto slice<T>::starts_with(__SLICE__(value)) const noexcept -> bool
{
	return detail::_range(this->head, this->tail, 0, detail::_length(value.head, value.tail)) == value;
}

template <unit_t T /* ??? */>
template <unit_t U, allo_t B> constexpr auto slice<T>::starts_with(__C_STR__(value)) const noexcept -> bool
{
	return detail::_range(this->head, this->tail, 0, detail::_length(value.head(), value.tail())) == value;
}

template <unit_t T /* ??? */>
template <unit_t U, size_t N> constexpr auto slice<T>::starts_with(__ARRAY__(value)) const noexcept -> bool
{
	return detail::_range(this->head, this->tail, 0, detail::_length(&value[N - N], &value[N - 1])) == value;
}

template <unit_t T /* ??? */>
template <unit_t U /* ??? */> constexpr auto slice<T>::ends_with(__SLICE__(value)) const noexcept -> bool
{
	return detail::_range(this->head, this->tail, range::N - detail::_length(value.head, value.tail), range::N) == value;
}

template <unit_t T /* ??? */>
template <unit_t U, allo_t B> constexpr auto slice<T>::ends_with(__C_STR__(value)) const noexcept -> bool
{
	return detail::_range(this->head, this->tail, range::N - detail::_length(value.head(), value.tail()), range::N) == value;
}

template <unit_t T /* ??? */>
template <unit_t U, size_t N> constexpr auto slice<T>::ends_with(__ARRAY__(value)) const noexcept -> bool
{
	return detail::_range(this->head, this->tail, range::N - detail::_length(&value[N - N], &value[N - 1]), range::N) == value;
}

template <unit_t T /* ??? */>
template <unit_t U /* ??? */> constexpr auto slice<T>::contains(__SLICE__(value)) const noexcept -> bool
{
	return !detail::_match(this->head, this->tail, value.head, value.tail).empty();
}

template <unit_t T /* ??? */>
template <unit_t U, allo_t B> constexpr auto slice<T>::contains(__C_STR__(value)) const noexcept -> bool
{
	return !detail::_match(this->head, this->tail, value.head(), value.tail()).empty();
}

template <unit_t T /* ??? */>
template <unit_t U, size_t N> constexpr auto slice<T>::contains(__ARRAY__(value)) const noexcept -> bool
{
	return !detail::_match(this->head, this->tail, &value[N - N], &value[N - 1]).empty();
}

template <unit_t T /* ??? */> constexpr auto slice<T>::begin() const noexcept -> cursor<T>
{
	return {this->head};
}

template <unit_t T /* ??? */> constexpr auto slice<T>::end() const noexcept -> cursor<T>
{
	return {this->tail};
}

template <unit_t T /* ??? */>
template <unit_t U /* ??? */> constexpr auto slice<T>::operator+(__SLICE__(rhs)) const noexcept -> concat
{
	return {*this, slice<U> {rhs}};
}

template <unit_t T /* ??? */>
template <unit_t U, allo_t B> constexpr auto slice<T>::operator+(__C_STR__(rhs)) const noexcept -> concat
{
	return {*this, slice<U> {rhs}};
}

template <unit_t T /* ??? */>
template <unit_t U, size_t N> constexpr auto slice<T>::operator+(__ARRAY__(rhs)) const noexcept -> concat
{
	return {*this, slice<U> {rhs}};
}

template <unit_t T /* ??? */>
template <unit_t U /* ??? */> constexpr auto slice<T>::operator==(__SLICE__(rhs)) const noexcept -> bool
{
	return detail::_equal(this->head, this->tail, rhs.head, rhs.tail);
}

template <unit_t T /* ??? */>
template <unit_t U, allo_t B> constexpr auto slice<T>::operator==(__C_STR__(rhs)) const noexcept -> bool
{
	return detail::_equal(this->head, this->tail, rhs.head(), rhs.tail());
}

template <unit_t T /* ??? */>
template <unit_t U, size_t N> constexpr auto slice<T>::operator==(__ARRAY__(rhs)) const noexcept -> bool
{
	return detail::_equal(this->head, this->tail, &rhs[N - N], &rhs[N - 1]);
}

template <unit_t T /* ??? */>
template <unit_t U /* ??? */> constexpr auto slice<T>::operator!=(__SLICE__(rhs)) const noexcept -> bool
{
	return detail::_nqual(this->head, this->tail, rhs.head, rhs.tail);
}

template <unit_t T /* ??? */>
template <unit_t U, allo_t B> constexpr auto slice<T>::operator!=(__C_STR__(rhs)) const noexcept -> bool
{
	return detail::_nqual(this->head, this->tail, rhs.head(), rhs.tail());
}

template <unit_t T /* ??? */>
template <unit_t U, size_t N> constexpr auto slice<T>::operator!=(__ARRAY__(rhs)) const noexcept -> bool
{
	return detail::_nqual(this->head, this->tail, &rhs[N - N], &rhs[N - 1]);
}

template <unit_t T /* ??? */> constexpr auto slice<T>::operator[](size_t value) const noexcept -> reader
{
	return {this, value};
}

template <unit_t T /* ??? */> constexpr auto slice<T>::operator[](size_t value) /*&*/ noexcept -> writer
{
	return {this, value};
}

template <unit_t T /* ??? */> constexpr auto slice<T>::operator[](clamp  start, clamp  until) const noexcept -> slice<T>
{
	return detail::_range(this->head, this->tail, start, until);
}

template <unit_t T /* ??? */> constexpr auto slice<T>::operator[](clamp  start, range  until) const noexcept -> slice<T>
{
	return detail::_range(this->head, this->tail, start, until);
}

template <unit_t T /* ??? */> constexpr auto slice<T>::operator[](size_t start, clamp  until) const noexcept -> slice<T>
{
	return detail::_range(this->head, this->tail, start, until);
}

template <unit_t T /* ??? */> constexpr auto slice<T>::operator[](size_t start, range  until) const noexcept -> slice<T>
{
	return detail::_range(this->head, this->tail, start, until);
}

template <unit_t T /* ??? */> constexpr auto slice<T>::operator[](size_t start, size_t until) const noexcept -> slice<T>
{
	return detail::_range(this->head, this->tail, start, until);
}

template <unit_t T /* ??? */,
          unit_t U, size_t N> constexpr auto operator+(__ARRAY__(lhs), const slice<T>& rhs) noexcept -> concat
{
	return {slice<U> {&lhs[N - N], &lhs[N - 1]}, rhs};
}

template <unit_t T /* ??? */,
          unit_t U, size_t N> constexpr auto operator==(__ARRAY__(lhs), const slice<T>& rhs) noexcept -> bool
{
	return detail::_equal(&lhs[N - N], &lhs[N - 1], rhs.head, rhs.tail);
}

template <unit_t T /* ??? */,
          unit_t U, size_t N> constexpr auto operator!=(__ARRAY__(lhs), const slice<T>& rhs) noexcept -> bool
{
	return detail::_nqual(&lhs[N - N], &lhs[N - 1], rhs.head, rhs.tail);
}

#pragma endregion slice
#pragma region c_str::reader

template <unit_t T, allo_t A> [[nodiscard]] constexpr c_str<T, A>::reader::operator char32_t() const noexcept
{
	const T* head {this->src->head()};
	const T* tail {this->src->tail()};

	size_t i {0};

	if constexpr (std::is_same_v<T, char32_t>)
	{
		if (this->arg < this->src->size())
		{
			head += this->arg;
			goto __SHORTCUT__;
		}
		return '\0';
	}

	for (; head < tail; head += codec<T>::next(head))
	{
		if (this->arg == i++)
		{
			__SHORTCUT__:

			char32_t T_out;

			const auto T_size {codec<T>::next(head)};
			codec<T>::decode_ptr(head, T_out, T_size);

			return T_out;
		}
	}
	return U'\0';
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::reader::operator==(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() == code;
}

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::reader::operator!=(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() != code;
}

#pragma endregion c_str::reader
#pragma region c_str::writer

template <unit_t T, allo_t A> constexpr auto c_str<T, A>::writer::operator=(char32_t code) noexcept -> writer&
{
	const T* head {this->src->head()};
	const T* tail {this->src->tail()};

	size_t i {0};

	if constexpr (std::is_same_v<T, char32_t>)
	{
		if (this->arg < this->src->size())
		{
			head += this->arg;
			goto __SHORTCUT__;
		}
		return *this;
	}

	for (; head < tail; head += codec<T>::next(head))
	{
		if (this->arg == i++)
		{
			__SHORTCUT__:

			const auto a {codec<T>::next(head)};
			const auto b {codec<T>::size(code)};

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

					head = this->src->head();
					tail = this->src->tail();
				}

				detail::_rcopy
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

				detail::_fcopy
				(
					&head[a],
					&tail[0],
					&head[b]
				);
				this->src->__size__(new_l);
			}
			// finally, write code point at ptr
			codec<T>::encode_ptr(code, head, b);
			break;
		}
	}
	return *this;
}

#pragma endregion c_str::writer
#pragma region slice::reader

template <unit_t T /* ??? */> [[nodiscard]] constexpr slice<T>::reader::operator char32_t() const noexcept
{
	const T* head {this->src->head};
	const T* tail {this->src->tail};

	size_t i {0};

	if constexpr (std::is_same_v<T, char32_t>)
	{
		if (this->arg < this->src->size())
		{
			head += this->arg;
			goto __SHORTCUT__;
		}
		return '\0';
	}

	for (; head < tail; head += codec<T>::next(head))
	{
		if (this->arg == i++)
		{
			__SHORTCUT__:

			char32_t T_out;

			const auto T_size {codec<T>::next(head)};
			codec<T>::decode_ptr(head, T_out, T_size);

			return T_out;
		}
	}
	return U'\0';
}

template <unit_t T /* ??? */> constexpr auto slice<T>::reader::operator==(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() == code;
}

template <unit_t T /* ??? */> constexpr auto slice<T>::reader::operator!=(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() != code;
}

#pragma endregion slice::reader
#pragma region slice::writer

// only exist for symmetry

#pragma endregion slice::writer

template <typename STRING>
// fs I/O at your service
auto fileof(const STRING& path) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>
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
		[]<unit_t T, allo_t A>(std::ifstream& ifs, c_str<T, A>& str) noexcept -> void
		{
			T buffer;

			T* foo {str.head()};
			T* bar {str.head()};

			while (ifs.read(reinterpret_cast<char*>(&buffer), sizeof(T)))
			{
				if (buffer == '\r'
				    &&
				    ifs.read(reinterpret_cast<char*>(&buffer), sizeof(T))
				    &&
				    buffer != '\n')
				{
					*(foo++) = '\n';
				}
				*(foo++) = buffer;
			}
			str.__size__(foo - bar);
		}
	};

	static const auto write_as_foreign
	{
		[]<unit_t T, allo_t A>(std::ifstream& ifs, c_str<T, A>& str) noexcept -> void
		{
			T buffer;

			T* foo {str.head()};
			T* bar {str.head()};

			while (ifs.read(reinterpret_cast<char*>(&buffer), sizeof(T)))
			{
				if ((buffer = std::byteswap(buffer)) == '\r'
				    &&
				    ifs.read(reinterpret_cast<char*>(&buffer), sizeof(T))
				    &&
				    (buffer = std::byteswap(buffer)) != '\n')
				{
					*(foo++) = '\n';
				}
				*(foo++) = buffer;
			}
			str.__size__(foo - bar);
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
				c_str<char8_t> str;

				str.capacity(max / sizeof(char8_t));

				if constexpr (!IS_BIG) write_as_native(ifs, str);
				                  else write_as_native(ifs, str);

				return str;
			}
			case UTF8_BOM:
			{
				c_str<char8_t> str;

				str.capacity(max / sizeof(char8_t));

				if constexpr (IS_BIG) write_as_native(ifs, str);
				                 else write_as_native(ifs, str);

				return str;
			}
			case UTF16_LE:
			{
				c_str<char16_t> str;

				str.capacity(max / sizeof(char16_t));

				if constexpr (!IS_BIG) write_as_native(ifs, str);
				                  else write_as_foreign(ifs, str);

				return str;
			}
			case UTF16_BE:
			{
				c_str<char16_t> str;

				str.capacity(max / sizeof(char16_t));

				if constexpr (IS_BIG) write_as_native(ifs, str);
				                 else write_as_foreign(ifs, str);

				return str;
			}
			case UTF32_LE:
			{
				c_str<char32_t> str;

				str.capacity(max / sizeof(char32_t));

				if constexpr (!IS_BIG) write_as_native(ifs, str);
				                  else write_as_foreign(ifs, str);

				return str;
			}
			case UTF32_BE:
			{
				c_str<char32_t> str;

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

typedef c_str<char8_t> utf8;
typedef c_str<char16_t> utf16;
typedef c_str<char32_t> utf32;

typedef slice<char8_t> txt8;
typedef slice<char16_t> txt16;
typedef slice<char32_t> txt32;

#undef __SLICE__
#undef __C_STR__
#undef __ARRAY__

#undef COPY_ASSIGNMENT
#undef MOVE_ASSIGNMENT

#undef COPY_CONSTRUCTOR
#undef MOVE_CONSTRUCTOR

template <size_t N> c_str(const char8_t (&str)[N]) -> c_str<char8_t>;
template <size_t N> c_str(const char16_t (&str)[N]) -> c_str<char16_t>;
template <size_t N> c_str(const char32_t (&str)[N]) -> c_str<char32_t>;

template <size_t N> slice(const char8_t (&str)[N]) -> slice<char8_t>;
template <size_t N> slice(const char16_t (&str)[N]) -> slice<char16_t>;
template <size_t N> slice(const char32_t (&str)[N]) -> slice<char32_t>;
