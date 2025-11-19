// NOLINTBEGIN(*-magic-numbers, *-union-access, *-signed-bitwise, *-avoid-c-arrays, *-pointer-arithmetic, *-constant-array-index, *-explicit-conversions)

#pragma once

#include <bit>
#include <ios>
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <ostream>
#include <fstream>
#include <optional>
#include <algorithm>

#include <cstdio>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

//|----------------------------------------------------------------------------------------|
//| special thanks to facebook's folly::FBString.                                          |
//|                                                                                        |
//| SSO mode uses every bytes of heap string struct using union.                           |
//| this was achievable, thanks to the very clever memory layout.                          |
//|                                                                                        |
//| c-style string has null-terminator at the end to denote the end of the string.         |
//| here, spare capacity is written to the RMB, and once fully occupied, RMB becomes zero. |
//|----------------------------------------------------------------------------------------|

#define COPY_PTR_LOGIC(T) static constexpr auto copy(T& from, T& dest) noexcept -> void
#define SWAP_PTR_LOGIC(T) static constexpr auto swap(T& from, T& dest) noexcept -> void

#define COPY_ASSIGNMENT(T) constexpr auto operator=(const T& rhs) noexcept -> T&
#define MOVE_ASSIGNMENT(T) constexpr auto operator=(T&& rhs) noexcept -> T&

#define COPY_CONSTRUCTOR(T) constexpr T(const T& other) noexcept
#define MOVE_CONSTRUCTOR(T) constexpr T(T&& other) noexcept

inline auto operator<<(std::ostream& os, char32_t code) -> std::ostream&
{
	char out[4]; short unit {0};

	// 1-byte sequence
	if (code <= 0x7F)
	{
		out[unit++] = static_cast<char>(code);
	}
	// 2-byte sequence
	else if (code <= 0x7FF)
	{
		out[unit++] = 0xC0 | static_cast<char>((code >> 06) & 0x1F);
		out[unit++] = 0x80 | static_cast<char>((code >> 00) & 0x3F);
	}
	// 3-byte sequence
	else if (code <= 0xFFFF)
	{
		out[unit++] = 0xE0 | static_cast<char>((code >> 12) & 0x0F);
		out[unit++] = 0x80 | static_cast<char>((code >> 06) & 0x3F);
		out[unit++] = 0x80 | static_cast<char>((code >> 00) & 0x3F);
	}
	// 4-byte sequence
	else if (code <= 0x10FFFF)
	{
		out[unit++] = 0xF0 | static_cast<char>((code >> 18) & 0x07);
		out[unit++] = 0x80 | static_cast<char>((code >> 12) & 0x3F);
		out[unit++] = 0x80 | static_cast<char>((code >> 06) & 0x3F);
		out[unit++] = 0x80 | static_cast<char>((code >> 00) & 0x3F);
	}
	return os.write(out, unit);
}

template
<
	class T = char /* legacy */,
	class A = std::allocator<T>
>
requires
(
	std::is_same_v<T, char>
	||
	std::is_same_v<T, char8_t>
	||
	std::is_same_v<T, char16_t>
	||
	std::is_same_v<T, char32_t>
)
class c_str
{
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

	class cursor
	{
		friend c_str;

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

	class slice
	{
		friend c_str;

		const T* head;
		const T* tail;

		// for slice
		class reader
		{
			slice* src;
			size_t nth;

		public:

			reader
			(
				decltype(src) src,
				decltype(nth) nth
			)
			: src(src), nth(nth) {}

			[[nodiscard]] constexpr operator char32_t() const&& noexcept;
		};

		// for slice
		class writer
		      :
		      reader
		{
			friend reader;

		public:

			// constexpr auto operator=(char32_t code)&& noexcept -> writer&;
		};

	public:

		slice
		(
			decltype(head) head,
			decltype(tail) tail
		)
		: head(head), tail(tail) {}

		// returns the number of code units, excluding NULL-TERMINATOR.
		constexpr auto size() const noexcept -> size_t;
		// returns the number of code points, excluding NULL-TERMINATOR.
		constexpr auto length() const noexcept -> size_t;

		// returns a list of string slice, of which are product of what remains after a split.
		constexpr auto split(const c_str& value) const noexcept -> std::vector<slice>;
		// returns a list of string slice, of which are product of what remains after a split.
		constexpr auto split(const slice& value) const noexcept -> std::vector<slice>;
		// returns a list of string slice, of which are product of what remains after a split.
		template<size_t N>
		constexpr auto split(const T (&value)[N]) const noexcept -> std::vector<slice>;

		// returns a list of string slice, of which are indentical to the given parameter.
		constexpr auto match(const c_str& value) const noexcept -> std::vector<slice>;
		// returns a list of string slice, of which are indentical to the given parameter.
		constexpr auto match(const slice& value) const noexcept -> std::vector<slice>;
		// returns a list of string slice, of which are indentical to the given parameter.
		template<size_t N>
		constexpr auto match(const T (&value)[N]) const noexcept -> std::vector<slice>;

		// returns a `slice` after *start* code point, with max code point length of *until*.
		constexpr auto fsubstr(size_t start, size_t until = SIZE_MAX) const noexcept -> slice;
		// returns a `slice` after *start* code point, with max code point length of *until*.
		constexpr auto rsubstr(size_t start, size_t until = SIZE_MAX) const noexcept -> slice;

		// iterator

		constexpr auto begin() const noexcept -> cursor;
		constexpr auto end() const noexcept -> cursor;

		// subscript

		constexpr auto operator[](size_t value) const noexcept -> const T& requires (std::is_same_v<T, char>);
		constexpr auto operator[](size_t value)       noexcept -> const T& requires (std::is_same_v<T, char>);

		constexpr auto operator[](size_t value) const noexcept -> reader requires (!std::is_same_v<T, char>);
		constexpr auto operator[](size_t value)       noexcept -> writer requires (!std::is_same_v<T, char>);
	};

	static constexpr auto __split__(const T* head, const T* tail, const T* str_0, const T* str_N) noexcept -> std::vector<slice>;
	static constexpr auto __match__(const T* head, const T* tail, const T* str_0, const T* str_N) noexcept -> std::vector<slice>;

	static constexpr auto __fsubstr__(const T* head, const T* tail, size_t start, size_t until) noexcept -> slice;
	static constexpr auto __rsubstr__(const T* head, const T* tail, size_t start, size_t until) noexcept -> slice;

	// for c_str
	class reader
	{
		c_str* src;
		size_t nth;

	public:

		reader
		(
			decltype(src) src,
			decltype(nth) nth
		)
		: src(src), nth(nth) {}

		[[nodiscard]] constexpr operator char32_t() const&& noexcept;
	};

	// for c_str
	class writer
	      :
	      reader
	{
		friend reader;

	public:

		constexpr auto operator=(char32_t code)&& noexcept -> writer&;
	};

	struct buffer
	{
		T* head;
		T* tail;
		size_t size : (sizeof(size_t) * 8) - (sizeof(mode_t) * 8);
		size_t meta : (sizeof(mode_t) * 8) - (sizeof(mode_t) * 0);

		constexpr operator const T*() const noexcept;
		constexpr operator       T*()       noexcept;
	};

	static constexpr const uint8_t MAX {(sizeof(buffer) - 1) / (sizeof(T))};
	static constexpr const uint8_t RMB {(sizeof(buffer) - 1) * (    1    )};
	static constexpr const uint8_t SFT {IS_BIG ? (    1    ) : (    0    )};
	static constexpr const uint8_t MSK {IS_BIG ? 0b0000000'1 : 0b1'0000000};

	#undef IS_BIG

	//|---------------------------|
	//|           small           |
	//|------|------|------|------|
	//| head | tail | size | meta |
	//|------|------|------|------|
	//|           bytes           |
	//|---------------------------|

	struct storage : A
	{
		union
		{
			buffer large
			{
				.head = 0,
				.tail = 0,
				.size = 0,
				.meta = 0,
			};
			typedef T chunk_t;

			chunk_t small
			[sizeof(buffer) / sizeof(chunk_t)];

			uint8_t bytes
			[sizeof(buffer) / sizeof(uint8_t)];
		}
		__union__;

		//|----------------------------------------------------|
		//| in large mode, string data is written on the heap. |
		//| therefore it is crucial to free memory for safety. |
		//|----------------------------------------------------|

		constexpr storage() noexcept;
		constexpr ~storage() noexcept;

		//|-------------------------------------------------------|-------|
		//|                                                       |   ↓   |
		//|---------------|-------|-------|-------|-------|-------|-------|
		//| 1 bit | 1 bit | 1 bit | 1 bit | 1 bit | 1 bit | 1 bit | 1 bit |
		//|-------|-------|-------|-------|-------|-------|-------|-------|

		constexpr auto tag() const noexcept -> mode_t;
		constexpr auto tag()       noexcept -> mode_t;

		//|-------------------------------------------------------|-------|
		//|   ↓       ↓       ↓       ↓       ↓       ↓       ↓   |       |
		//|---------------|-------|-------|-------|-------|-------|-------|
		//| 1 bit | 1 bit | 1 bit | 1 bit | 1 bit | 1 bit | 1 bit | 1 bit |
		//|-------|-------|-------|-------|-------|-------|-------|-------|

		// returns ptr to the first element.
		constexpr auto head() const noexcept -> const T*;
		// returns ptr to the first element.
		constexpr auto head()       noexcept ->       T*;

		// returns ptr to the one-past-the-last element.
		constexpr auto tail() const noexcept -> const T*;
		// returns ptr to the one-past-the-last element.
		constexpr auto tail()       noexcept ->       T*;
	};

	// fixes invariant; use it after internal manipulation.
	constexpr auto __size__(size_t value) noexcept -> void;

	storage store;

	// sanity check
	static_assert(std::is_standard_layout_v<buffer>);
	static_assert(std::is_trivially_copyable_v<buffer>);
	// memory check
	static_assert(sizeof(buffer) == sizeof(size_t) * 3);
	static_assert(alignof(buffer) == alignof(size_t) * 1);
	// layout check
	static_assert(offsetof(buffer, head) == sizeof(size_t) * 0);
	static_assert(offsetof(buffer, tail) == sizeof(size_t) * 1);

public:

	class codec
	{
		// nothing to do...

	public:

		codec() = delete;

		static constexpr auto size(char32_t code) noexcept -> int8_t;

		static constexpr auto next(const T* ptr) noexcept -> int8_t;

		static constexpr auto back(const T* ptr) noexcept -> int8_t;

		static constexpr auto encode_ptr(const char32_t in, T* out, int8_t size) noexcept -> void;

		static constexpr auto decode_ptr(const T* in, char32_t& out, int8_t size) noexcept -> void;
	};

	// returns the content of a file with CR or CRLF to LF normalization.
	template<class S> friend auto fileof(const S& path) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>;
	// returns the content of a file with CR or CRLF to LF normalization.
	template<size_t N> friend auto fileof(const char (&path)[N]) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>;
	// returns the content of a file with CR or CRLF to LF normalization.
	template<size_t N> friend auto fileof(const char8_t (&path)[N]) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>;
	// returns the content of a file with CR or CRLF to LF normalization.
	template<size_t N> friend auto fileof(const char16_t (&path)[N]) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>;
	// returns the content of a file with CR or CRLF to LF normalization.
	template<size_t N> friend auto fileof(const char32_t (&path)[N]) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>;

	// convertion

	template<class U, class X> requires
	(
		!std::is_same_v<T, char>
		&&
		!std::is_same_v<U, char>
	)
	operator c_str<U, X>() const noexcept;
	operator std::string() const noexcept;

	// constructor

	constexpr c_str(/* empty string */) noexcept
	{
		// nothing to do...
	}

	template<size_t N>
	constexpr c_str(const T (&other)[N]) noexcept
	{
		this->capacity(N - 1);

		std::ranges::copy
		(
			&other[0 - 0],
			&other[N - 1],
			this->store.head()
		);
		// fix invariant
		this->__size__(N - 1);
	}

	// rule of 5

	COPY_CONSTRUCTOR(c_str)
	{
		if (this != &other)
		{
			copy(other, *this);
		}
	}

	MOVE_CONSTRUCTOR(c_str)
	{
		if (this != &other)
		{
			swap(other, *this);
		}
	}

	COPY_ASSIGNMENT(c_str)
	{
		if (this != &rhs)
		{
			copy(rhs, *this);
		}
		return *this;
	}

	MOVE_ASSIGNMENT(c_str)
	{
		if (this != &rhs)
		{
			swap(rhs, *this);
		}
		return *this;
	}

	COPY_PTR_LOGIC(c_str)
	{
		dest.capacity(from.size());

		std::ranges::copy
		(
			from.store.head(),
			from.store.tail(),
			dest.store.head()
		);
		// fix invariant
		dest.__size__(from.size());
	}

	SWAP_PTR_LOGIC(c_str)
	{
		if (dest.store.tag() == LARGE)
		{
			// ...release heap allocated memory
			std::allocator_traits<A>::deallocate
			(
				dest.store,
				dest.store.__union__.large.head,
				dest.store.__union__.large.tail
				-
				dest.store.__union__.large.head
			);
		}
		dest.store = from.store;

		// invalidate 'from' at its core
		from.store.__union__.large.head = 0;
		from.store.__union__.large.tail = 0;
		from.store.__union__.large.size = 0;
		from.store.__union__.large.meta = 0;
	}
	
	// returns the number of code units, excluding NULL-TERMINATOR.
	constexpr auto size() const noexcept -> size_t;
	// returns the number of code points, excluding NULL-TERMINATOR.
	constexpr auto length() const noexcept -> size_t;

	// returns the number of code units it can hold, excluding NULL-TERMINATOR.
	constexpr auto capacity(/* getter */) const noexcept -> size_t;
	// changes the number of code units it can hold, excluding NULL-TERMINATOR.
	constexpr auto capacity(size_t value)       noexcept -> void;

	// returns a list of string slice, of which are product of what remains after a split.
	constexpr auto split(const c_str& value) const noexcept -> std::vector<slice>;
	// returns a list of string slice, of which are product of what remains after a split.
	constexpr auto split(const slice& value) const noexcept -> std::vector<slice>;
	// returns a list of string slice, of which are product of what remains after a split.
	template<size_t N>
	constexpr auto split(const T (&value)[N]) const noexcept -> std::vector<slice>;

	// returns a list of string slice, of which are indentical to the given parameter.
	constexpr auto match(const c_str& value) const noexcept -> std::vector<slice>;
	// returns a list of string slice, of which are indentical to the given parameter.
	constexpr auto match(const slice& value) const noexcept -> std::vector<slice>;
	// returns a list of string slice, of which are indentical to the given parameter.
	template<size_t N>
	constexpr auto match(const T (&value)[N]) const noexcept -> std::vector<slice>;

	// returns a `slice` after *start* code point, with max code point length of *until*.
	constexpr auto fsubstr(size_t start, size_t until = SIZE_MAX) const noexcept -> slice;
	// returns a `slice` after *start* code point, with max code point length of *until*.
	constexpr auto rsubstr(size_t start, size_t until = SIZE_MAX) const noexcept -> slice;

	// iterator

	constexpr auto begin() const noexcept -> cursor;
	constexpr auto end() const noexcept -> cursor;

	// subscript

	constexpr auto operator[](size_t value) const noexcept -> const T& requires (std::is_same_v<T, char>);
	constexpr auto operator[](size_t value)       noexcept ->       T& requires (std::is_same_v<T, char>);

	constexpr auto operator[](size_t value) const noexcept -> reader requires (!std::is_same_v<T, char>);
	constexpr auto operator[](size_t value)       noexcept -> writer requires (!std::is_same_v<T, char>);

	// iostream
	
	friend auto operator<<(std::ostream& os, const c_str& str) noexcept -> std::ostream&
	{
		for (const auto code : str) { os << code; } return os;
	}
	
	friend auto operator<<(std::ostream& os, const slice& str) noexcept -> std::ostream&
	{
		for (const auto code : str) { os << code; } return os;
	}

	// A += B

	friend constexpr auto operator+=(c_str& lhs, const c_str& rhs) noexcept -> c_str&
	{
		const auto size {lhs.size() + rhs.size()};

		if (0 < rhs.size())
		{
			lhs.capacity(size);

			// overrides '\0'
			std::ranges::copy
			(
				rhs.store.head(),
				rhs.store.tail(),
				lhs.store.tail()
			);
			// fix invariant
			lhs.__size__(size);
		}
		return lhs;
	}
	
	friend constexpr auto operator+=(c_str& lhs, const slice& rhs) noexcept -> c_str&
	{
		const auto size {lhs.size() + rhs.size()};

		if (0 < rhs.size())
		{
			lhs.capacity(size);

			// overrides '\0'
			std::ranges::copy
			(
				rhs.head,
				rhs.tail,
				lhs.store.tail()
			);
			// fix invariant
			lhs.__size__(size);
		}
		return lhs;
	}

	template<size_t N>
	friend constexpr auto operator+=(c_str& lhs, const T (&rhs)[N]) noexcept -> c_str&
	{
		const auto size {lhs.size() + N - 1};

		if (0 < N - 1)
		{
			lhs.capacity(size);

			// overrides '\0'
			std::ranges::copy
			(
				&rhs[0 - 0],
				&rhs[N - 1],
				lhs.store.tail()
			);
			// fix invariant
			lhs.__size__(size);
		}
		return lhs;
	}

	// A + B

	friend constexpr auto operator+(const c_str& lhs, const c_str& rhs) noexcept -> c_str
	{
		c_str str;

		str += lhs;
		str += rhs;

		return str;
	}

	friend constexpr auto operator+(const slice& lhs, const slice& rhs) noexcept -> c_str
	{
		c_str str;

		str += lhs;
		str += rhs;

		return str;
	}
	
	friend constexpr auto operator+(const c_str& lhs, const slice& rhs) noexcept -> c_str
	{
		c_str str;

		str += lhs;
		str += rhs;

		return str;
	}

	friend constexpr auto operator+(const slice& lhs, const c_str& rhs) noexcept -> c_str
	{
		c_str str;

		str += lhs;
		str += rhs;

		return str;
	}

	template<size_t N>
	friend constexpr auto operator+(const c_str& lhs, const T (&rhs)[N]) noexcept -> c_str
	{
		c_str str;

		str += lhs;
		str += rhs;

		return str;
	}

	template<size_t N>
	friend constexpr auto operator+(const T (&lhs)[N], const c_str& rhs) noexcept -> c_str
	{
		c_str str;

		str += lhs;
		str += rhs;

		return str;
	}

	template<size_t N>
	friend constexpr auto operator+(const slice& lhs, const T (&rhs)[N]) noexcept -> c_str
	{
		c_str str;

		str += lhs;
		str += rhs;

		return str;
	}

	template<size_t N>
	friend constexpr auto operator+(const T (&lhs)[N], const slice& rhs) noexcept -> c_str
	{
		c_str str;

		str += lhs;
		str += rhs;

		return str;
	}

	// A == B

	friend constexpr auto operator==(const c_str& lhs, const c_str& rhs) noexcept -> bool
	{
		const auto* lhs_0 {lhs.store.head()}; const auto* rhs_0 {rhs.store.head()};
		const auto* lhs_N {lhs.store.tail()}; const auto* rhs_N {rhs.store.tail()};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return &lhs == &rhs || (lhs.size() == rhs.size() && std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1));
	}

	friend constexpr auto operator==(const slice& lhs, const slice& rhs) noexcept -> bool
	{
		const auto* lhs_0 {lhs.head}; const auto* rhs_0 {rhs.head};
		const auto* lhs_N {lhs.tail}; const auto* rhs_N {rhs.tail};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return &lhs == &rhs || (lhs.size() == rhs.size() && std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1));
	}
	
	friend constexpr auto operator==(const c_str& lhs, const slice& rhs) noexcept -> bool
	{
		const auto* lhs_0 {lhs.store.head()}; const auto* rhs_0 {rhs.head};
		const auto* lhs_N {lhs.store.tail()}; const auto* rhs_N {rhs.tail};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return lhs.size() == rhs.size() && std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	friend constexpr auto operator==(const slice& lhs, const c_str& rhs) noexcept -> bool
	{
		const auto* lhs_0 {lhs.head}; const auto* rhs_0 {rhs.store.head()};
		const auto* lhs_N {lhs.tail}; const auto* rhs_N {rhs.store.tail()};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return lhs.size() == rhs.size() && std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	template<size_t N>
	friend constexpr auto operator==(const c_str& lhs, const T (&rhs)[N]) noexcept -> bool
	{
		const auto* lhs_0 {lhs.store.head()}; const auto* rhs_0 {&rhs[0 - 0]};
		const auto* lhs_N {lhs.store.tail()}; const auto* rhs_N {&rhs[N - 1]};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return lhs.size() == N - 1 && std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	template<size_t N>
	friend constexpr auto operator==(const T (&lhs)[N], const c_str& rhs) noexcept -> bool
	{
		const auto* lhs_0 {&lhs[0 - 0]}; const auto* rhs_0 {rhs.store.head()};
		const auto* lhs_N {&lhs[N - 1]}; const auto* rhs_N {rhs.store.tail()};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return N - 1 == rhs.size() && std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	template<size_t N>
	friend constexpr auto operator==(const slice& lhs, const T (&rhs)[N]) noexcept -> bool
	{
		const auto* lhs_0 {lhs.head}; const auto* rhs_0 {&rhs[0 - 0]};
		const auto* lhs_N {lhs.tail}; const auto* rhs_N {&rhs[N - 1]};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return lhs.size() == N - 1 && std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	template<size_t N>
	friend constexpr auto operator==(const T (&lhs)[N], const slice& rhs) noexcept -> bool
	{
		const auto* lhs_0 {&lhs[0 - 0]}; const auto* rhs_0 {rhs.head};
		const auto* lhs_N {&lhs[N - 1]}; const auto* rhs_N {rhs.tail};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return N - 1 == rhs.size() && std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	// A != B

	friend constexpr auto operator!=(const c_str& lhs, const c_str& rhs) noexcept -> bool
	{
		const auto* lhs_0 {lhs.store.head()}; const auto* rhs_0 {rhs.store.head()};
		const auto* lhs_N {lhs.store.tail()}; const auto* rhs_N {rhs.store.tail()};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return &lhs != &rhs && (lhs.size() != rhs.size() || !std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1));
	}

	friend constexpr auto operator!=(const slice& lhs, const slice& rhs) noexcept -> bool
	{
		if (&lhs != &rhs) { return true; }

		const auto* lhs_0 {lhs.head}; const auto* rhs_0 {rhs.head};
		const auto* lhs_N {lhs.tail}; const auto* rhs_N {rhs.tail};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return &lhs != &rhs && (lhs.size() != rhs.size() || !std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1));
	}
	
	friend constexpr auto operator!=(const c_str& lhs, const slice& rhs) noexcept -> bool
	{
		const auto* lhs_0 {lhs.store.head()}; const auto* rhs_0 {rhs.head};
		const auto* lhs_N {lhs.store.tail()}; const auto* rhs_N {rhs.tail};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return lhs.size() != rhs.size() || !std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	friend constexpr auto operator!=(const slice& lhs, const c_str& rhs) noexcept -> bool
	{
		const auto* lhs_0 {lhs.head}; const auto* rhs_0 {rhs.store.head()};
		const auto* lhs_N {lhs.tail}; const auto* rhs_N {rhs.store.tail()};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return lhs.size() != rhs.size() || !std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	template<size_t N>
	friend constexpr auto operator!=(const c_str& lhs, const T (&rhs)[N]) noexcept -> bool
	{
		const auto* lhs_0 {lhs.store.head()}; const auto* rhs_0 {&rhs[0 - 0]};
		const auto* lhs_N {lhs.store.tail()}; const auto* rhs_N {&rhs[N - 1]};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return lhs.size() != N - 1 || !std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	template<size_t N>
	friend constexpr auto operator!=(const T (&lhs)[N], const c_str& rhs) noexcept -> bool
	{
		const auto* lhs_0 {&lhs[0 - 0]}; const auto* rhs_0 {rhs.store.head()};
		const auto* lhs_N {&lhs[N - 1]}; const auto* rhs_N {rhs.store.tail()};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return N - 1 != rhs.size() || !std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	template<size_t N>
	friend constexpr auto operator!=(const slice& lhs, const T (&rhs)[N]) noexcept -> bool
	{
		const auto* lhs_0 {lhs.head}; const auto* rhs_0 {&rhs[0 - 0]};
		const auto* lhs_N {lhs.tail}; const auto* rhs_N {&rhs[N - 1]};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return lhs.size() != N - 1 || !std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}

	template<size_t N>
	friend constexpr auto operator!=(const T (&lhs)[N], const slice& rhs) noexcept -> bool
	{
		const auto* lhs_0 {&lhs[0 - 0]}; const auto* rhs_0 {rhs.head};
		const auto* lhs_N {&lhs[N - 1]}; const auto* rhs_N {rhs.tail};

		// assert(lhs_N - lhs_0 == rhs_N - rhs_0);

		return N - 1 != rhs.size() || !std::ranges::equal(lhs_0, lhs_N - 1, rhs_0, rhs_N - 1);
	}
};

#define IMPL                    \
                                \
template                        \
<                               \
    class T,                    \
    class A                     \
>                               \
requires                        \
(                               \
    std::is_same_v<T, char>     \
    ||                          \
    std::is_same_v<T, char8_t>  \
    ||                          \
    std::is_same_v<T, char16_t> \
    ||                          \
    std::is_same_v<T, char32_t> \
)                               \

IMPL template<class U, class X> requires (!std::is_same_v<T, char> && !std::is_same_v<U, char>) c_str<T, A>::operator c_str<U, X>() const noexcept
{
	c_str<U, X> str;
	
	if (0 < this->size())
	{
		const auto size {[&](){ size_t impl {0}; for (const auto code : *this) { impl += c_str<U, X>::codec::size(code); } return impl; }()};

		str.capacity(size);
		str.__size__(size);

		auto* ptr {reinterpret_cast<U*>(str.store.head())};

		for (const auto code : *this)
		{
			const auto size {c_str<U, X>::codec::size(code)};
			c_str<U, X>::codec::encode_ptr(code, ptr, size);
			ptr += size;
		}
	}
	return str;
}

IMPL /* backward compatibility. unfortunately, this requires re-encoding, from head to tail. */ c_str<T, A>::operator std::string() const noexcept
{
	std::string str;

	if (0 < this->size())
	{
		const auto size {[&](){ size_t impl {0}; for (const auto code : *this) { impl += c_str<char8_t>::codec::size(code); } return impl; }()};
	
		str.reserve(size);
		str.resize(size);

		auto* ptr {reinterpret_cast<char8_t*>(str.data())};

		for (const auto code : *this)
		{
			const auto size {c_str<char8_t>::codec::size(code)};
			c_str<char8_t>::codec::encode_ptr(code, ptr, size);
			ptr += size;
		}
	}
	return str;
}

IMPL constexpr auto c_str<T, A>::__split__(const T* head, const T* tail, const T* str_0, const T* str_N) noexcept -> std::vector<slice>
{	
	const auto delta {str_N - str_0};
	
	std::vector<slice> out;

	auto* foo {head};
	auto* bar {head};

	for (; bar + delta < tail; ++bar)
	{
		size_t i {0};

		for (; i < delta; ++i)
		{
			if (bar[i] != str_0[i]) { break; }
		}
		if (i == delta)
		{
			// ...identical portion
			out.emplace_back(&foo[0], &bar[0]);

			foo = bar + delta;
			bar = bar + delta;
		}
	}
	if (foo < tail)
	{
		// ...remaining portion
		out.emplace_back(foo, tail);
	}

	return out;
}

IMPL constexpr auto c_str<T, A>::__match__(const T* head, const T* tail, const T* str_0, const T* str_N) noexcept -> std::vector<slice>
{
	const auto delta {str_N - str_0};
	
	std::vector<slice> out;

	auto* foo {head};

	for (; foo + delta < tail; ++foo)
	{
		size_t i {0};

		for (; i < delta; ++i)
		{
			if (foo[i] != str_0[i]) { break; }
		}
		if (i == delta)
		{
			// ...identical portion
			out.emplace_back(&foo[0], &foo[i]);

			foo += delta;
		}
	}

	return out;
}

IMPL constexpr auto c_str<T, A>::__fsubstr__(const T* head, const T* tail, size_t start, size_t until) noexcept -> slice
{
	// O(1) accessing: utf-32 specific trick
	if constexpr (std::is_same_v<T, char32_t>)
	{
		assert(&head[start] < tail);
		assert(&head[until] < tail);

		return {&head[start], &head[until]};
	}

	size_t foo {0};
	
	for (size_t i {0}; i < start; ++i)
	{
		foo += codec::next(&head[foo]);
	}

	size_t bar {foo};

	for (size_t i {0}; i < until; ++i)
	{
		bar += codec::next(&head[bar]);
	}

	assert(&head[foo] < tail);
	assert(&head[bar] < tail);

	return {&head[foo], &head[bar]};
}

IMPL constexpr auto c_str<T, A>::__rsubstr__(const T* head, const T* tail, size_t start, size_t until) noexcept -> slice
{
	// O(1) accessing: utf-32 specific trick
	if constexpr (std::is_same_v<T, char32_t>)
	{
		assert(head < &tail[start]);
		assert(head < &tail[until]);

		return {&tail[start], &tail[until]};
	}

	size_t foo {0};
	
	for (size_t i {0}; i < start; ++i)
	{
		foo += codec::back(&tail[foo]);
	}

	size_t bar {foo};

	for (size_t i {0}; i < until; ++i)
	{
		bar += codec::back(&tail[bar]);
	}

	assert(head < &tail[foo]);
	assert(head < &tail[bar]);

	return {&tail[foo], &tail[bar]};
}

#pragma region SSO

IMPL constexpr c_str<T, A>::buffer::operator const T*() const noexcept
{
	return this->head;
}

IMPL constexpr c_str<T, A>::buffer::operator       T*()       noexcept
{
	return this->head;
}

IMPL constexpr c_str<T, A>::storage::storage() noexcept
{
	this->__union__.bytes[RMB] = MAX << SFT;
}

IMPL constexpr c_str<T, A>::storage::~storage() noexcept
{
	if (this->tag() == LARGE)
	{
		// ...release heap allocated memory
		std::allocator_traits<A>::deallocate
		(
			(*this),
			(*this).__union__.large.head,
			(*this).__union__.large.tail
			-
			(*this).__union__.large.head
		);
	}
}

IMPL constexpr auto c_str<T, A>::storage::tag() const noexcept -> mode_t
{
	// or static_cast<mode_t>(this->__union__.large.meta & MSK);
	return static_cast<mode_t>(this->__union__.bytes[RMB] & MSK);
}

IMPL constexpr auto c_str<T, A>::storage::tag()       noexcept -> mode_t
{
	// or static_cast<mode_t>(this->__union__.large.meta & MSK);
	return static_cast<mode_t>(this->__union__.bytes[RMB] & MSK);
}

IMPL constexpr auto c_str<T, A>::storage::head() const noexcept -> const T*
{
	return this->tag() == SMALL
	       ?
	       this->__union__.small
	       :
	       this->__union__.large;
}

IMPL constexpr auto c_str<T, A>::storage::head()       noexcept ->       T*
{
	return this->tag() == SMALL
	       ?
	       this->__union__.small
	       :
	       this->__union__.large;
}

IMPL constexpr auto c_str<T, A>::storage::tail() const noexcept -> const T*
{
	return this->tag() == SMALL
	       ?
	       &this->__union__.small[MAX - (this->__union__.bytes[RMB] >> SFT)]
	       :
	       &this->__union__.large[0x0 + (this->__union__.large.size >> 0x0)];
}

IMPL constexpr auto c_str<T, A>::storage::tail()       noexcept ->       T*
{
	return this->tag() == SMALL
	       ?
	       &this->__union__.small[MAX - (this->__union__.bytes[RMB] >> SFT)]
	       :
	       &this->__union__.large[0x0 + (this->__union__.large.size >> 0x0)];
}

#pragma endregion SSO

#pragma region c_str

IMPL constexpr auto c_str<T, A>::size() const noexcept -> size_t
{
	return this->store.tag() == SMALL
	       ?
	       MAX - (this->store.__union__.bytes[RMB] >> SFT)
	       :
	       0x0 + (this->store.__union__.large.size >> 0x0);
}

IMPL constexpr auto c_str<T, A>::length() const noexcept -> size_t
{
	if constexpr (std::is_same_v<T, char32_t>)
	{
		return this->size();
	}
	auto* head {this->store.head()};
	auto* tail {this->store.tail()};

	size_t out {0};

	for (; head < tail - 1; ++out)
	{
		head += codec::next(head);
	}
	return out;
}

IMPL constexpr auto c_str<T, A>::__size__(size_t value)        noexcept -> void
{
	switch (this->store.tag())
	{
		case SMALL:
		{
			const auto slot {(MAX - value) << SFT};

			//|------------|    |-------[LE]-------|
			//| 0b0XXXXXXX | -> | no need for skip |
			//|------------|    |------------------|

			//|------------|    |-------[BE]-------|
			//| 0bXXXXXXX0 | -> | skip right 1 bit |
			//|------------|    |------------------|

			this->store.__union__.bytes[RMB] = slot;
			// ⚠️ null-terminator ⚠️
			this->store.__union__.small[value] = '\0';
			break;
		}
		case LARGE:
		{
			this->store.__union__.large.size = value;
			// ⚠️ null-terminator ⚠️
			this->store.__union__.large[value] = '\0';
			break;
		}
	}
}

IMPL constexpr auto c_str<T, A>::capacity(/* getter */) const noexcept -> size_t
{
	return this->store.tag() == SMALL
	       ?
	       MAX // or calculate the std::ptrdiff_t just as large mode as shown below
		   :
		   this->store.__union__.large.tail - this->store.__union__.large.head - 1;
}

IMPL constexpr auto c_str<T, A>::capacity(size_t value)       noexcept -> void
{
	if (this->capacity() < value)
	{
		auto* head {std::allocator_traits<A>::allocate(this->store, value + 1)};

		// data migration
		std::ranges::copy
		(
			this->store.head(),
			this->store.tail(),
			head // new T[value]
		);

		if (this->store.tag() == LARGE)
		{
			// ...release heap allocated memory
			std::allocator_traits<A>::deallocate
			(
				this->store,
				this->store.__union__.large.head,
				this->store.__union__.large.tail
				-
				this->store.__union__.large.head
			);
		}

		auto* tail {head + value + 1};

		const auto size {this->size()};

		this->store.__union__.large.head = head;
		this->store.__union__.large.tail = tail;
		this->store.__union__.large.size = size;
		this->store.__union__.large.meta = LARGE;
	}
}

IMPL /* ver. T=c_str */ constexpr auto c_str<T, A>::split(const c_str& value) const noexcept -> std::vector<slice>
{
	return __split__(this->store.head(), this->store.tail(), value.store.head(), value.store.tail());
}

IMPL /* ver. T=slice */ constexpr auto c_str<T, A>::split(const slice& value) const noexcept -> std::vector<slice>
{
	return __split__(this->store.head(), this->store.tail(), value.head, value.tail);
}

IMPL template<size_t N> constexpr auto c_str<T, A>::split(const T (&value)[N]) const noexcept -> std::vector<slice>
{
	return __split__(this->store.head(), this->store.tail(), &value[0], &value[N - 1]);
}

IMPL /* ver. T=c_str */ constexpr auto c_str<T, A>::match(const c_str& value) const noexcept -> std::vector<slice>
{
	return __match__(this->store.head(), this->store.tail(), value.store.head(), value.store.tail());
}

IMPL /* ver. T=slice */ constexpr auto c_str<T, A>::match(const slice& value) const noexcept -> std::vector<slice>
{
	return __split__(this->store.head(), this->store.tail(), value.head, value.tail);
}

IMPL template<size_t N> constexpr auto c_str<T, A>::match(const T (&value)[N]) const noexcept -> std::vector<slice>
{
	return __split__(this->store.head(), this->store.tail(), &value[0], &value[N - 1]);
}

IMPL constexpr auto c_str<T, A>::fsubstr(size_t start, size_t until) const noexcept -> slice
{
	return __fsubstr__(this->store.head(), this->store.tail(), start, until);
}

IMPL constexpr auto c_str<T, A>::rsubstr(size_t start, size_t until) const noexcept -> slice
{
	return __rsubstr__(this->store.head(), this->store.tail(), start, until);
}

IMPL constexpr auto c_str<T, A>::begin() const noexcept -> cursor
{
	return {this->store.head()};
}

IMPL constexpr auto c_str<T, A>::end() const noexcept -> cursor
{
	return {this->store.tail()};
}

IMPL constexpr auto c_str<T, A>::operator[](size_t value) const noexcept -> const T& requires (std::is_same_v<T, char>)
{
	return this->store.head()[value];
}

IMPL constexpr auto c_str<T, A>::operator[](size_t value)       noexcept ->       T& requires (std::is_same_v<T, char>)
{
	return this->store.head()[value];
}

IMPL constexpr auto c_str<T, A>::operator[](size_t value) const noexcept -> reader requires (!std::is_same_v<T, char>)
{
	return {*this, value};
}

IMPL constexpr auto c_str<T, A>::operator[](size_t value)       noexcept -> writer requires (!std::is_same_v<T, char>)
{
	return {*this, value};
}

#pragma endregion c_str

#pragma region slice

IMPL constexpr auto c_str<T, A>::slice::size() const noexcept -> size_t
{
	return static_cast<const T*>(this->tail)
	       -
	       static_cast<const T*>(this->head);
}

IMPL constexpr auto c_str<T, A>::slice::length() const noexcept -> size_t
{
	if constexpr (std::is_same_v<T, char32_t>)
	{
		return this->size();
	}
	auto* head {this->head};
	auto* tail {this->tail};
	
	size_t out {0};
	
	for (; head < tail - 0; ++out)
	{
		head += codec::next(head);
	}
	return out;
}

IMPL /* ver. T=c_str */ constexpr auto c_str<T, A>::slice::split(const c_str& value) const noexcept -> std::vector<slice>
{
	return __split__(this->head, this->tail, value.store.head(), value.store.tail());
}

IMPL /* ver. T=slice */ constexpr auto c_str<T, A>::slice::split(const slice& value) const noexcept -> std::vector<slice>
{
	return __split__(this->head, this->tail, value.head, value.tail);
}

IMPL template<size_t N> constexpr auto c_str<T, A>::slice::split(const T (&value)[N]) const noexcept -> std::vector<slice>
{
	return __split__(this->head, this->tail, &value[0], &value[N - 1]);
}

IMPL /* ver. T=c_str */ constexpr auto c_str<T, A>::slice::match(const c_str& value) const noexcept -> std::vector<slice>
{
	return __match__(this->head, this->tail, value.store.head(), value.store.tail());
}

IMPL /* ver. T=slice */ constexpr auto c_str<T, A>::slice::match(const slice& value) const noexcept -> std::vector<slice>
{
	return __match__(this->head, this->tail, value.head, value.tail);
}

IMPL template<size_t N> constexpr auto c_str<T, A>::slice::match(const T (&value)[N]) const noexcept -> std::vector<slice>
{
	return __match__(this->head, this->tail, &value[0], &value[N - 1]);
}

IMPL constexpr auto c_str<T, A>::slice::fsubstr(size_t start, size_t until) const noexcept -> slice
{
	return __fsubstr__(this->head, this->tail, start, until);
}

IMPL constexpr auto c_str<T, A>::slice::rsubstr(size_t start, size_t until) const noexcept -> slice
{
	return __rsubstr__(this->head, this->tail, start, until);
}

IMPL constexpr auto c_str<T, A>::slice::begin() const noexcept -> cursor
{
	return {this->head};
}

IMPL constexpr auto c_str<T, A>::slice::end() const noexcept -> cursor
{
	return {this->tail};
}

IMPL constexpr auto c_str<T, A>::slice::operator[](size_t value) const noexcept -> const T& requires (std::is_same_v<T, char>)
{
	return this->head[value];
}

IMPL constexpr auto c_str<T, A>::slice::operator[](size_t value)       noexcept -> const T& requires (std::is_same_v<T, char>)
{
	return this->head[value];
}

IMPL constexpr auto c_str<T, A>::slice::operator[](size_t value) const noexcept -> reader requires (!std::is_same_v<T, char>)
{
	return {*this, value};
}

IMPL constexpr auto c_str<T, A>::slice::operator[](size_t value)       noexcept -> writer requires (!std::is_same_v<T, char>)
{
	return {*this, value};
}

#pragma endregion slice

#pragma region c_str::codec

IMPL constexpr auto c_str<T, A>::codec::size(char32_t code) noexcept -> int8_t
{
	if constexpr (std::is_same_v<T, char>)
	{
		return 1;
	}
	if constexpr (std::is_same_v<T, char8_t>)
	{
		const auto N {std::bit_width
		(static_cast<uint32_t>(code))};

		//|-----------------------|
		//| U+000000 ... U+00007F | -> 1 code unit
		//| U+000080 ... U+0007FF | -> 2 code unit
		//| U+000800 ... U+00FFFF | -> 3 code unit
		//| U+010000 ... U+10FFFF | -> 4 code unit
		//|-----------------------|

		return 1 + (8 <= N) + (12 <= N) + (17 <= N);
	}
	if constexpr (std::is_same_v<T, char16_t>)
	{
		//|-----------------------|
		//| U+000000 ... U+00D7FF | -> 1 code unit
		//| U+00E000 ... U+00FFFF | -> 1 code unit
		//| U+000000 ... U+10FFFF | -> 2 code unit
		//|-----------------------|

		return 1 + (0xFFFF /* surrogate */ < code);
	}
	if constexpr (std::is_same_v<T, char32_t>)
	{
		return 1;
	}
}

IMPL constexpr auto c_str<T, A>::codec::next(const T* ptr) noexcept -> int8_t
{
	constexpr static const int8_t TABLE[]
	{
		/*| 0x0 |*/ 1, /*| 0x1 |*/ 1,
		/*| 0x2 |*/ 1, /*| 0x3 |*/ 1,
		/*| 0x4 |*/ 1, /*| 0x5 |*/ 1,
		/*| 0x6 |*/ 1, /*| 0x7 |*/ 1,
		/*| 0x8 |*/ 1, /*| 0x9 |*/ 1,
		/*| 0xA |*/ 1, /*| 0xB |*/ 1,
		/*| 0xC |*/ 2, /*| 0xD |*/ 2,
		/*| 0xE |*/ 3, /*| 0xF |*/ 4,
	};

	if constexpr (std::is_same_v<T, char>)
	{
		return 1;
	}
	if constexpr (std::is_same_v<T, char8_t>)
	{
		// branchless programming for utf-8
		return TABLE[(ptr[0] >> 0x4) & 0x0F];
	}
	if constexpr (std::is_same_v<T, char16_t>)
	{
		// branchless programming for utf-16
		return 1 + ((ptr[0] >> 0xA) == 0x36);
	}
	if constexpr (std::is_same_v<T, char32_t>)
	{
		return 1;
	}
}

IMPL constexpr auto c_str<T, A>::codec::back(const T* ptr) noexcept -> int8_t
{
	if constexpr (std::is_same_v<T, char>)
	{
		return -1;
	}
	if constexpr (std::is_same_v<T, char8_t>)
	{
		// walk backward until non-continuation code unit is found
		int8_t i {-1}; for (; (ptr[i] & 0xC0) == 0x80; --i) {} return i;
	}
	if constexpr (std::is_same_v<T, char16_t>)
	{
		// walk backward until encountering leading surrogate is found
		int8_t i {-1}; for (; (ptr[i] >> 0xA) == 0x37; --i) {} return i;
	}
	if constexpr (std::is_same_v<T, char32_t>)
	{
		return -1;
	}
}

IMPL constexpr auto c_str<T, A>::codec::encode_ptr(const char32_t in, T* out, int8_t size) noexcept -> void
{
	if constexpr (std::is_same_v<T, char>)
	{
		out[0] = static_cast<T>(in);
	}
	if constexpr (std::is_same_v<T, char8_t>)
	{
		switch (size)
		{
			case 1:
			{
				out[0] = static_cast<T>(in);

				break;
			}
			case 2:
			{
				out[0] = 0xC0 | ((in >> 06) & 0x1F);
				out[1] = 0x80 | ((in >> 00) & 0x3F);

				break;
			}
			case 3:
			{
				out[0] = 0xE0 | ((in >> 12) & 0x0F);
				out[1] = 0x80 | ((in >> 06) & 0x3F);
				out[2] = 0x80 | ((in >> 00) & 0x3F);

				break;
			}
			case 4:
			{
				out[0] = 0xF0 | ((in >> 18) & 0x07);
				out[1] = 0x80 | ((in >> 12) & 0x3F);
				out[2] = 0x80 | ((in >> 06) & 0x3F);
				out[3] = 0x80 | ((in >> 00) & 0x3F);

				break;
			}
		}
	}
	if constexpr (std::is_same_v<T, char16_t>)
	{
		switch (size)
		{
			case 1:
			{
				out[0] = static_cast<T>(in);

				break;
			}
			case 2:
			{
				const auto code {in - 0x10000};

				out[0] = 0xD800 | (code / 0x400);
				out[1] = 0xDC00 | (code & 0x3FF);

				break;
			}
		}
	}
	if constexpr (std::is_same_v<T, char32_t>)
	{
		out[0] = static_cast<T>(in);
	}
}

IMPL constexpr auto c_str<T, A>::codec::decode_ptr(const T* in, char32_t& out, int8_t size) noexcept -> void
{
	if constexpr (std::is_same_v<T, char>)
	{
		out = static_cast<char32_t>(in[0]);
	}
	if constexpr (std::is_same_v<T, char8_t>)
	{
		switch (size)
		{
			case 1:
			{
				out = static_cast<char32_t>(in[0]);

				break;
			}
			case 2:
			{
				out = ((in[0] & 0x1F) << 06)
				      |
				      ((in[1] & 0x3F) << 00);

				break;
			}
			case 3:
			{
				out = ((in[0] & 0x0F) << 12)
				      |
				      ((in[1] & 0x3F) << 06)
				      |
				      ((in[2] & 0x3F) << 00);

				break;
			}
			case 4:
			{
				out = ((in[0] & 0x07) << 18)
				      |
				      ((in[1] & 0x3F) << 12)
				      |
				      ((in[2] & 0x3F) << 06)
				      |
				      ((in[3] & 0x3F) << 00);

				break;
			}
		}
	}
	if constexpr (std::is_same_v<T, char16_t>)
	{
		switch (size)
		{
			case 1:
			{
				out = static_cast<char32_t>(in[0]);

				break;
			}
			case 2:
			{
				out = 0x10000 // supplymentary
				      |
				      ((in[0] - 0xD800) << 10)
				      |
				      ((in[1] - 0xDC00) << 00);

				break;
			}
		}
	}
	if constexpr (std::is_same_v<T, char32_t>)
	{
		out = static_cast<char32_t>(in[0]);
	}
}

#pragma endregion c_str::codec

#pragma region c_str::cursor

IMPL constexpr auto c_str<T, A>::cursor::operator*() noexcept -> char32_t
{
	char32_t code;

	const auto size {codec::next(this->ptr)};
	codec::decode_ptr(this->ptr, code, size);

	return code;
}

IMPL constexpr auto c_str<T, A>::cursor::operator&() noexcept -> const T*
{
	return this->ptr;
}

IMPL constexpr auto c_str<T, A>::cursor::operator++(   ) noexcept -> cursor&
{
	this->ptr += codec::next(this->ptr);

	return *this;
}

IMPL constexpr auto c_str<T, A>::cursor::operator++(int) noexcept -> cursor
{
	const auto clone {*this};

	operator++();

	return clone;
}

IMPL constexpr auto c_str<T, A>::cursor::operator--(   ) noexcept -> cursor&
{
	this->ptr += codec::back(this->ptr);

	return *this;
}

IMPL constexpr auto c_str<T, A>::cursor::operator--(int) noexcept -> cursor
{
	const auto clone {*this};

	operator--();

	return clone;
}

IMPL constexpr auto c_str<T, A>::cursor::operator==(const cursor& rhs) noexcept -> bool
{
	return this->ptr == rhs.ptr;
}

IMPL constexpr auto c_str<T, A>::cursor::operator!=(const cursor& rhs) noexcept -> bool
{
	return this->ptr != rhs.ptr;
}

#pragma endregion c_str::cursor

#pragma region c_str::reader

IMPL [[nodiscard]] constexpr c_str<T, A>::reader::operator char32_t() const&& noexcept
{
	auto* head {this->src->store.head()};
	auto* tail {this->src->store.tail()};

	size_t i {0};

	// O(1) accessing: utf-32 specific trick
	if constexpr (std::is_same_v<T, char32_t>)
	{
		if (this->nth < this->src->size())
		{
			head += this->nth;
			goto __SHORTCUT__;
		}
		return U'\uFFFD';
	}

	for (; head < tail; ++i, head += codec::next(head))
	{
		if (i == this->nth)
		{
			__SHORTCUT__:

			char32_t code;

			const auto size {codec::next(head)};
			codec::decode_ptr(head, code, size);

			return code;
		}
	}
	return U'\uFFFD';
}

#pragma endregion c_str::reader

#pragma region c_str::writer

IMPL constexpr auto c_str<T, A>::writer::operator=(char32_t code)&& noexcept -> writer&
{
	auto* head {this->src->store.head()};
	auto* tail {this->src->store.tail()};

	size_t i {0};

	// O(1) accessing: utf-32 specific trick
	if constexpr (std::is_same_v<T, char32_t>)
	{
		if (this->nth < this->src->size())
		{
			head += this->nth;
			goto __SHORTCUT__;
		}
		return *this;
	}

	for (; head < tail; ++i, head += codec::next(head))
	{
		if (i == this->nth)
		{
			__SHORTCUT__:

			const auto a {codec::next(head)};
			const auto b {codec::size(code)};

			if (a == b)
			{
				// nothing to do...
			}
			else if (a < b)
			{
				//|---|--------------|
				//| a | source range |
				//|---|---|----------|---|
				//|   b   | source range |
				//|-------|--------------|
			
				const auto old_l {this->src->size()};
				const auto new_l {old_l + (b - a)};

				if (this->src.capacity() <= new_l)
				{
					this->src.capacity(new_l * 2);
				}

				std::ranges::copy_backward
				(
					&head[a],
					&tail[0],
					&head[b]
				);
				// fix invariant
				this->src->__size__(new_l);
			}
			else if (a > b)
			{
				//|-------|--------------|
				//|   a   | source range |
				//|---|---|----------|---|
				//| b | source range |
				//|---|--------------|

				const auto old_l {this->src->size()};
				const auto new_l {old_l - (a - b)};

				std::ranges::copy//forward
				(
					&head[a],
					&tail[0],
					&head[b]
				);
				// fix invariant
				this->src->__size__(new_l);
			}
			// ♻️ set code point in place ♻️
			codec::encode_ptr(code, head, b);
			break;
		}
	}
	return *this;
}

#pragma endregion c_str::writer

#pragma region slice::reader

IMPL [[nodiscard]] constexpr c_str<T, A>::slice::reader::operator char32_t() const&& noexcept
{
	auto* head {this->src->head};
	auto* tail {this->src->tail};

	size_t i {0};

	// O(1) accessing: utf-32 specific trick
	if constexpr (std::is_same_v<T, char32_t>)
	{
		if (this->nth < this->src->size())
		{
			head += this->nth;
			goto __SHORTCUT__;
		}
		return U'\uFFFD';
	}

	for (; head < tail; ++i, head += codec::next(head))
	{
		if (i == this->nth)
		{
			__SHORTCUT__:

			char32_t code;

			const auto size {codec::next(head)};
			codec::decode_ptr(head, code, size);

			return code;
		}
	}
	return U'\uFFFD';
}

#pragma endregion slice::reader

#undef IMPL

#undef COPY_PTR_LOGIC
#undef SWAP_PTR_LOGIC

#undef COPY_ASSIGNMENT
#undef MOVE_ASSIGNMENT

#undef COPY_CONSTRUCTOR
#undef MOVE_CONSTRUCTOR

// https://en.wikipedia.org/wiki/UTF-8
typedef c_str<char8_t> utf8;
// https://en.wikipedia.org/wiki/UTF-16
typedef c_str<char16_t> utf16;
// https://en.wikipedia.org/wiki/UTF-32
typedef c_str<char32_t> utf32;

template<class S> auto fileof(const S& path) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>
{
	enum format : uint8_t
	{
		UTF8_STD = (0 << 4) | 0,
		UTF8_BOM = (1 << 4) | 3,
		UTF16_BE = (2 << 4) | 2,
		UTF16_LE = (3 << 4) | 2,
		UTF32_BE = (4 << 4) | 4,
		UTF32_LE = (5 << 4) | 4,
	};

	struct lambda
	{
		static auto byte_order_mask(std::ifstream& ifs) noexcept -> format
		{
			char buffer[4];

			ifs.seekg(0, std::ios::beg);
			ifs.read(&buffer[0], 4);
			ifs.seekg(0, std::ios::beg);

			// 00 00 FE FF
			if (buffer[0] == '\x00'
				&&
				buffer[1] == '\x00'
				&&
				buffer[2] == '\xFE'
				&&
				buffer[3] == '\xFF') [[unlikely]] return UTF32_BE;

			// FF FE 00 00
			if (buffer[0] == '\xFF'
				&&
				buffer[1] == '\xFE'
				&&
				buffer[2] == '\x00'
				&&
				buffer[3] == '\x00') [[unlikely]] return UTF32_LE;

			// FE FF
			if (buffer[0] == '\xFE'
				&&
				buffer[1] == '\xFF') [[unlikely]] return UTF16_BE;

			// FF FE
			if (buffer[0] == '\xFF'
				&&
				buffer[1] == '\xFE') [[unlikely]] return UTF16_LE;

			// EF BB BF
			if (buffer[0] == '\xEF'
				&&
				buffer[1] == '\xBB'
				&&
				buffer[2] == '\xBF') [[unlikely]] return UTF8_BOM;

			return UTF8_STD;
		}

		// for utf-8
		static auto write_as_native(std::ifstream& ifs, c_str<char8_t>& str) noexcept
		{
			typedef char8_t T; T buffer;

			T* foo;
			T* bar;

			foo = bar = str.store.head();

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
			// fix invariant
			str.__size__(foo - bar);
		}

		// for utf-16
		static auto write_as_native(std::ifstream& ifs, c_str<char16_t>& str) noexcept
		{
			typedef char16_t T; T buffer;

			T* foo;
			T* bar;

			foo = bar = str.store.head();

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
			// fix invariant
			str.__size__(foo - bar);
		}

		// for utf-32
		static auto write_as_native(std::ifstream& ifs, c_str<char32_t>& str) noexcept
		{
			typedef char32_t T; T buffer;

			T* foo;
			T* bar;

			foo = bar = str.store.head();

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
			// fix invariant
			str.__size__(foo - bar);
		}

		// for utf-8
		static auto write_as_foreign(std::ifstream& ifs, c_str<char8_t>& str) noexcept
		{
			typedef char8_t T; T buffer;

			T* foo;
			T* bar;

			foo = bar = str.store.head();

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
			// fix invariant
			str.__size__(foo - bar);
		}

		// for utf-16
		static auto write_as_foreign(std::ifstream& ifs, c_str<char16_t>& str) noexcept
		{
			typedef char16_t T; T buffer;

			T* foo;
			T* bar;

			foo = bar = str.store.head();

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
		}

		// for utf-32
		static auto write_as_foreign(std::ifstream& ifs, c_str<char32_t>& str) noexcept
		{
			typedef char32_t T; T buffer;

			T* foo;
			T* bar;

			foo = bar = str.store.head();

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
		}
	};


	if (std::ifstream ifs {path, std::ios::binary})
	{
		const auto BOM {lambda::byte_order_mask(ifs)};
		const auto off {BOM & 0xF /* bitwise hack */};

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

				str.capacity((max / sizeof(char8_t)) + 1);

				if constexpr (!IS_BIG) lambda::write_as_native(ifs, str);
				                  else lambda::write_as_native(ifs, str);

				return str;
			}
			case UTF8_BOM:
			{
				c_str<char8_t> str;

				str.capacity((max / sizeof(char8_t)) + 1);

				if constexpr (IS_BIG) lambda::write_as_native(ifs, str);
				                 else lambda::write_as_native(ifs, str);

				return str;
			}
			case UTF16_LE:
			{
				c_str<char16_t> str;

				str.capacity((max / sizeof(char16_t)) + 1);

				if constexpr (!IS_BIG) lambda::write_as_native(ifs, str);
				                  else lambda::write_as_foreign(ifs, str);

				return str;
			}
			case UTF16_BE:
			{
				c_str<char16_t> str;

				str.capacity((max / sizeof(char16_t)) + 1);

				if constexpr (IS_BIG) lambda::write_as_native(ifs, str);
				                 else lambda::write_as_foreign(ifs, str);

				return str;
			}
			case UTF32_LE:
			{
				c_str<char32_t> str;

				str.capacity((max / sizeof(char32_t)) + 1);

				if constexpr (!IS_BIG) lambda::write_as_native(ifs, str);
				                  else lambda::write_as_foreign(ifs, str);

				return str;
			}
			case UTF32_BE:
			{
				c_str<char32_t> str;

				str.capacity((max / sizeof(char32_t)) + 1);

				if constexpr (IS_BIG) lambda::write_as_native(ifs, str);
				                 else lambda::write_as_foreign(ifs, str);

				return str;
			}
		}
		#undef IS_BIG
	}
	return std::nullopt;
}

template<size_t N> auto fileof(const char (&path)[N]) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>
{
	return fileof(c_str {path});
}

template<size_t N> auto fileof(const char8_t (&path)[N]) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>
{
	return fileof(utf8 {path});
}

template<size_t N> auto fileof(const char16_t (&path)[N]) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>
{
	return fileof(utf16 {path});
}

template<size_t N> auto fileof(const char32_t (&path)[N]) noexcept -> std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>
{
	return fileof(utf32 {path});
}

// NOLINTEND(*-magic-numbers, *-union-access, *-signed-bitwise, *-avoid-c-arrays, *-pointer-arithmetic, *-constant-array-index, *-explicit-conversions)
