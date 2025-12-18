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
#include <concepts>
#include <algorithm>
#include <filesystem>

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

inline auto operator<<(std::ostream& os, char32_t code) noexcept -> std::ostream&
{
	char out[4]; short unit {0};

	// 1-byte sequence
	/**/ if (code <= 0x00007F)
	{
		out[unit++] = static_cast<char>(code);
	}
	// 2-byte sequence
	else if (code <= 0x0007FF)
	{
		out[unit++] = 0xC0 | static_cast<char>((code >> 06) & 0x1F);
		out[unit++] = 0x80 | static_cast<char>((code >> 00) & 0x3F);
	}
	// 3-byte sequence
	else if (code <= 0x00FFFF)
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

template <typename T> concept unit_t = std::is_same_v<T, char/**/>
                                       ||
                                       std::is_same_v<T, char8_t >
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

enum class range : uint8_t {N};
struct clamp { const size_t _;
inline constexpr /**/ operator
size_t() const { return _; } };
inline constexpr auto operator-
(range, size_t offset) noexcept
-> clamp { return { offset }; }

template
<
	unit_t T = char /* legacy */,
	allo_t A = std::allocator<T>
>
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

		class reader
		{

		protected:

			slice* src;
			size_t arg;

		public:

			reader
			(
				decltype(src) src,
				decltype(arg) arg
			)
			: src(src), arg(arg) {}

			[[nodiscard]] constexpr operator char32_t() const noexcept;

			auto constexpr operator==(char32_t code) const noexcept -> bool;
			auto constexpr operator!=(char32_t code) const noexcept -> bool;
		};

		class writer
		      :
		      reader
		{
			// nothing to do...

		public:

			using reader::operator char32_t;
			
			using reader::operator==;
			using reader::operator!=;

			using reader::reader/**/;

			// constexpr auto operator=(char32_t code) noexcept -> writer&;
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

		// returns a list of string slice, of which are product of split aka division.
		template <unit_t U, allo_t B>
		constexpr auto split(const c_str<U, B> /*&*/ & value) const noexcept -> std::vector<slice>;
		// returns a list of string slice, of which are product of split aka division.
		template <unit_t U, allo_t B>
		constexpr auto split(const c_str<U, B>::slice& value) const noexcept -> std::vector<slice>;
		// returns a list of string slice, of which are product of split aka division.
		template <unit_t U, size_t N>
		constexpr auto split(const U /*literal*/ (&value)[N]) const noexcept -> std::vector<slice>;

		// returns a list of string slice, of which are product of search occurrence.
		template <unit_t U, allo_t B>
		constexpr auto match(const c_str<U, B> /*&*/ & value) const noexcept -> std::vector<slice>;
		// returns a list of string slice, of which are product of search occurrence.
		template <unit_t U, allo_t B>
		constexpr auto match(const c_str<U, B>::slice& value) const noexcept -> std::vector<slice>;
		// returns a list of string slice, of which are product of search occurrence.
		template <unit_t U, size_t N>
		constexpr auto match(const U /*literal*/ (&value)[N]) const noexcept -> std::vector<slice>;

		// *self explanatory* returns whether or not it starts with *parameter*.
		template <unit_t U, allo_t B>
		constexpr auto starts_with(const c_str<U, B> /*&*/ & value) const noexcept -> bool;
		// *self explanatory* returns whether or not it starts with *parameter*.
		template <unit_t U, allo_t B>
		constexpr auto starts_with(const c_str<U, B>::slice& value) const noexcept -> bool;
		// *self explanatory* returns whether or not it starts with *parameter*.
		template <unit_t U, size_t N>
		constexpr auto starts_with(const U /*literal*/ (&value)[N]) const noexcept -> bool;

		// *self explanatory* returns whether or not it ends with *parameter*.
		template <unit_t U, allo_t B>
		constexpr auto ends_with(const c_str<U, B> /*&*/ & value) const noexcept -> bool;
		// *self explanatory* returns whether or not it ends with *parameter*.
		template <unit_t U, allo_t B>
		constexpr auto ends_with(const c_str<U, B>::slice& value) const noexcept -> bool;
		// *self explanatory* returns whether or not it ends with *parameter*.
		template <unit_t U, size_t N>
		constexpr auto ends_with(const U /*literal*/ (&value)[N]) const noexcept -> bool;

		// *self explanatory* returns whether or not it contains *parameter*.
		template <unit_t U, allo_t B>
		constexpr auto contains(const c_str<U, B> /*&*/ & value) const noexcept -> bool;
		// *self explanatory* returns whether or not it contains *parameter*.
		template <unit_t U, allo_t B>
		constexpr auto contains(const c_str<U, B>::slice& value) const noexcept -> bool;
		// *self explanatory* returns whether or not it contains *parameter*.
		template <unit_t U, size_t N>
		constexpr auto contains(const U /*literal*/ (&value)[N]) const noexcept -> bool;

		// iterator

		constexpr auto begin() const noexcept -> cursor;
		constexpr auto end() const noexcept -> cursor;

		// read & write

		constexpr auto operator[](size_t value) const noexcept -> reader;
		constexpr auto operator[](size_t value) /*&*/ noexcept -> writer;

		// range syntax

		constexpr auto operator[](clamp  start, clamp  until) const noexcept -> slice;
		constexpr auto operator[](clamp  start, range  until) const noexcept -> slice;
		constexpr auto operator[](size_t start, clamp  until) const noexcept -> slice;
		constexpr auto operator[](size_t start, range  until) const noexcept -> slice;
		constexpr auto operator[](size_t start, size_t until) const noexcept -> slice;
	};

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _size(const T* head, const T* tail) noexcept -> size_t;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _size(const U* head, const U* tail) noexcept -> size_t;

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _length(const T* head, const T* tail) noexcept -> size_t;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _length(const U* head, const U* tail) noexcept -> size_t;

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _fcopy(const T* head, const T* tail,
	                                            /*&*/ T* dest) noexcept -> void;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _fcopy(const U* head, const U* tail,
	                                            /*&*/ T* dest) noexcept -> void;

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _rcopy(const T* head, const T* tail,
	                                            /*&*/ T* dest) noexcept -> void;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _rcopy(const U* head, const U* tail,
	                                            /*&*/ T* dest) noexcept -> void;

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _equal(const T* lhs_0, const T* lhs_N,
	                             const T* rhs_0, const T* rhs_N) noexcept -> bool;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _equal(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> bool;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _equal(const U* lhs_0, const U* lhs_N,
	                             const T* rhs_0, const T* rhs_N) noexcept -> bool;

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _nqual(const T* lhs_0, const T* lhs_N,
	                             const T* rhs_0, const T* rhs_N) noexcept -> bool;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _nqual(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> bool;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _nqual(const U* lhs_0, const U* lhs_N,
	                             const T* rhs_0, const T* rhs_N) noexcept -> bool;

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _equal(c_str &  lhs_0,
	                             const T* rhs_0, const T* rhs_N) noexcept -> void;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _equal(c_str &  lhs_0,
	                             const U* rhs_0, const U* rhs_N) noexcept -> void;

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _pqual(c_str &  lhs_0,
	                             const T* rhs_0, const T* rhs_N) noexcept -> void;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _pqual(c_str &  lhs_0,
	                             const U* rhs_0, const U* rhs_N) noexcept -> void;

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _cplus(const T* lhs_0, const T* lhs_N,
	                             const T* rhs_0, const T* rhs_N) noexcept -> c_str;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _cplus(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> c_str;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _cplus(const U* lhs_0, const U* lhs_N,
	                             const T* rhs_0, const T* rhs_N) noexcept -> c_str;

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _split(const T* lhs_0, const T* lhs_N,
	                             const T* rhs_0, const T* rhs_N) noexcept -> std::vector<slice>;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _split(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> std::vector<slice>;

	// constraint: where typeof(lhs) == typeof(rhs)
	static constexpr auto _match(const T* lhs_0, const T* lhs_N,
	                             const T* rhs_0, const T* rhs_N) noexcept -> std::vector<slice>;
	template <unit_t U> requires (!std::is_same_v<T, U>)
	static constexpr auto _match(const T* lhs_0, const T* lhs_N,
	                             const U* rhs_0, const U* rhs_N) noexcept -> std::vector<slice>;
	
	static constexpr auto _range(const T* head, const T* tail, clamp  start, clamp  until) noexcept -> slice;
	static constexpr auto _range(const T* head, const T* tail, clamp  start, range  until) noexcept -> slice;
	static constexpr auto _range(const T* head, const T* tail, size_t start, clamp  until) noexcept -> slice;
	static constexpr auto _range(const T* head, const T* tail, size_t start, range  until) noexcept -> slice;
	static constexpr auto _range(const T* head, const T* tail, size_t start, size_t until) noexcept -> slice;

	class reader
	{

	protected:

		c_str* src;
		size_t arg;

	public:

		reader
		(
			decltype(src) src,
			decltype(arg) arg
		)
		: src(src), arg(arg) {}

		[[nodiscard]] constexpr operator char32_t() const noexcept;

		auto constexpr operator==(char32_t code) const noexcept -> bool;
		auto constexpr operator!=(char32_t code) const noexcept -> bool;
	};

	class writer
	      :
	      reader
	{
		// nothing to do...

	public:

		using reader::operator char32_t;
		
		using reader::operator==;
		using reader::operator!=;

		using reader::reader/**/;

		constexpr auto operator=(char32_t code) noexcept -> writer&;
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

		constexpr storage() noexcept;
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

	// optional; returns the content of a file with CRLF/CR to LF normalization.
	template <typename STRING> friend auto fileof(const STRING& path) noexcept
	->
	std::optional<std::variant<c_str<char8_t>, c_str<char16_t>, c_str<char32_t>>>;

	template <unit_t U, allo_t B>
	// only allow conversion for utf-N
	requires (!std::is_same_v<T, char>
	          &&
	          !std::is_same_v<U, char>)
	// re-encode content into diff format.
	operator c_str<U, B>() const noexcept;
	// re-encode content into std::string.
	operator std::string() const noexcept;

	struct codec
	{
		codec() = delete;

		static constexpr auto size(char32_t code) noexcept -> int8_t;
		static constexpr auto next(const T* ptr ) noexcept -> int8_t;
		static constexpr auto back(const T* ptr ) noexcept -> int8_t;
		static constexpr auto encode_ptr(const char32_t in, T* out, int8_t size) noexcept -> void;
		static constexpr auto decode_ptr(const T* in, char32_t& out, int8_t size) noexcept -> void;
	};

	constexpr c_str(/* empty string; SSO is on */) noexcept
	{
		// nothing to do...
	}

	template <unit_t U, allo_t B>
	constexpr c_str(const c_str<U, B> /*&*/ & rhs) noexcept
	{
		this->operator=(rhs);
	}
	
	template <unit_t U, allo_t B>
	constexpr c_str(const c_str<U, B>::slice& rhs) noexcept
	{
		this->operator=(rhs);
	}

	template <unit_t U, size_t N>
	constexpr c_str(const U /*literal*/ (&rhs)[N]) noexcept
	{
		this->operator=(rhs);
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

		_fcopy
		(
			from.head(),
			from.tail(),
			dest.head()
		);

		dest.__size__(from.size());
	}

	SWAP_PTR_LOGIC(c_str)
	{
		// storage is trivially copyable
		std::swap(dest.store, from.store);
	}
	
	// returns the number of code units, excluding NULL-TERMINATOR.
	constexpr auto size() const noexcept -> size_t;
	// returns the number of code points, excluding NULL-TERMINATOR.
	constexpr auto length() const noexcept -> size_t;

	// returns the number of code units it can hold, excluding NULL-TERMINATOR.
	constexpr auto capacity(/* getter */) const noexcept -> size_t;
	// changes the number of code units it can hold, excluding NULL-TERMINATOR.
	constexpr auto capacity(size_t value) /*&*/ noexcept -> void;

	// returns a list of string slice, of which are product of split aka division.
	template <unit_t U, allo_t B>
	constexpr auto split(const c_str<U, B> /*&*/ & value) const noexcept -> std::vector<slice>;
	// returns a list of string slice, of which are product of split aka division.
	template <unit_t U, allo_t B>
	constexpr auto split(const c_str<U, B>::slice& value) const noexcept -> std::vector<slice>;
	// returns a list of string slice, of which are product of split aka division.
	template <unit_t U, size_t N>
	constexpr auto split(const U /*literal*/ (&value)[N]) const noexcept -> std::vector<slice>;

	// returns a list of string slice, of which are product of search occurrence.
	template <unit_t U, allo_t B>
	constexpr auto match(const c_str<U, B> /*&*/ & value) const noexcept -> std::vector<slice>;
	// returns a list of string slice, of which are product of search occurrence.
	template <unit_t U, allo_t B>
	constexpr auto match(const c_str<U, B>::slice& value) const noexcept -> std::vector<slice>;
	// returns a list of string slice, of which are product of search occurrence.
	template <unit_t U, size_t N>
	constexpr auto match(const U /*literal*/ (&value)[N]) const noexcept -> std::vector<slice>;

	// *self explanatory* returns whether or not it starts with *parameter*.
	template <unit_t U, allo_t B>
	constexpr auto starts_with(const c_str<U, B> /*&*/ & value) const noexcept -> bool;
	// *self explanatory* returns whether or not it starts with *parameter*.
	template <unit_t U, allo_t B>
	constexpr auto starts_with(const c_str<U, B>::slice& value) const noexcept -> bool;
	// *self explanatory* returns whether or not it starts with *parameter*.
	template <unit_t U, size_t N>
	constexpr auto starts_with(const U /*literal*/ (&value)[N]) const noexcept -> bool;

	// *self explanatory* returns whether or not it ends with *parameter*.
	template <unit_t U, allo_t B>
	constexpr auto ends_with(const c_str<U, B> /*&*/ & value) const noexcept -> bool;
	// *self explanatory* returns whether or not it ends with *parameter*.
	template <unit_t U, allo_t B>
	constexpr auto ends_with(const c_str<U, B>::slice& value) const noexcept -> bool;
	// *self explanatory* returns whether or not it ends with *parameter*.
	template <unit_t U, size_t N>
	constexpr auto ends_with(const U /*literal*/ (&value)[N]) const noexcept -> bool;

	// *self explanatory* returns whether or not it contains *parameter*.
	template <unit_t U, allo_t B>
	constexpr auto contains(const c_str<U, B> /*&*/ & value) const noexcept -> bool;
	// *self explanatory* returns whether or not it contains *parameter*.
	template <unit_t U, allo_t B>
	constexpr auto contains(const c_str<U, B>::slice& value) const noexcept -> bool;
	// *self explanatory* returns whether or not it contains *parameter*.
	template <unit_t U, size_t N>
	constexpr auto contains(const U /*literal*/ (&value)[N]) const noexcept -> bool;

	// iterator

	constexpr auto begin() const noexcept -> cursor;
	constexpr auto end() const noexcept -> cursor;

	// read & write

	constexpr auto operator[](size_t value) const noexcept -> reader;
	constexpr auto operator[](size_t value) /*&*/ noexcept -> writer;

	// range syntax

	constexpr auto operator[](clamp  start, clamp  until) const noexcept -> slice;
	constexpr auto operator[](clamp  start, range  until) const noexcept -> slice;
	constexpr auto operator[](size_t start, clamp  until) const noexcept -> slice;
	constexpr auto operator[](size_t start, range  until) const noexcept -> slice;
	constexpr auto operator[](size_t start, size_t until) const noexcept -> slice;

	// iostream
	
	friend auto operator<<(std::ostream& os, const c_str& str) noexcept -> std::ostream&
	{
		for (const auto code : str) { os << code; } return os;
	}
	
	friend auto operator<<(std::ostream& os, const slice& str) noexcept -> std::ostream&
	{
		for (const auto code : str) { os << code; } return os;
	}

	// A = B

	template <unit_t U, allo_t B>
	constexpr auto operator=(const c_str<U, B> /*&*/ & rhs) noexcept -> c_str&
	{
		_equal(*this, rhs.head(), rhs.tail());
		
		return *this;
	}
	
	template <unit_t U, allo_t B>
	constexpr auto operator=(const c_str<U, B>::slice& rhs) noexcept -> c_str&
	{
		_equal(*this, rhs.head, rhs.tail);
		
		return *this;
	}

	template <unit_t U, size_t N>
	constexpr auto operator=(const U /*literal*/ (&rhs)[N]) noexcept -> c_str&
	{
		_equal(*this, &rhs[0x0], &rhs[N-1]);

		return *this;
	}

	// A += B

	template <unit_t U, allo_t B>
	constexpr auto operator+=(const c_str<U, B> /*&*/ & rhs) noexcept -> c_str&
	{
		_pqual(*this, rhs.head(), rhs.tail());

		return *this;
	}
	
	template <unit_t U, allo_t B>
	constexpr auto operator+=(const c_str<U, B>::slice& rhs) noexcept -> c_str&
	{
		_pqual(*this, rhs.head, rhs.tail);

		return *this;
	}

	template <unit_t U, size_t N>
	constexpr auto operator+=(const U /*literal*/ (&rhs)[N]) noexcept -> c_str&
	{
		_pqual(*this, &rhs[0x0], &rhs[N-1]);

		return *this;
	}

	// A + B

	template <unit_t U, allo_t B>
	friend constexpr auto operator+(const c_str& lhs, const c_str<U, B> /*&*/ & rhs) noexcept -> c_str
	{
		return _cplus(lhs.head(), lhs.tail(), rhs.head(), rhs.tail());
	}

	template <unit_t U, allo_t B>
	friend constexpr auto operator+(const slice& lhs, const c_str<U, B>::slice& rhs) noexcept -> c_str
	{
		return _cplus(lhs.head, lhs.tail, rhs.head, rhs.tail);
	}

	template <unit_t U, allo_t B>
	friend constexpr auto operator+(const c_str& lhs, const c_str<U, B>::slice& rhs) noexcept -> c_str
	{
		return _cplus(lhs.head(), lhs.tail(), rhs.head, rhs.tail);
	}

	template <unit_t U, allo_t B>
	friend constexpr auto operator+(const slice& lhs, const c_str<U, B> /*&*/ & rhs) noexcept -> c_str
	{
		return _cplus(lhs.head, lhs.tail, rhs.head(), rhs.tail());
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator+(const c_str& lhs, const U /*literal*/ (&rhs)[N]) noexcept -> c_str
	{
		return _cplus(lhs.head(), lhs.tail(), &rhs[0x0], &rhs[N-1]);
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator+(const U /*literal*/ (&lhs)[N], const c_str& rhs) noexcept -> c_str
	{
		return _cplus(&lhs[0x0], &lhs[N-1], rhs.head(), rhs.tail());
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator+(const slice& lhs, const U /*literal*/ (&rhs)[N]) noexcept -> c_str
	{
		return _cplus(lhs.head, lhs.tail, &rhs[0x0], &rhs[N-1]);
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator+(const U /*literal*/ (&lhs)[N], const slice& rhs) noexcept -> c_str
	{
		return _cplus(&lhs[0x0], &lhs[N-1], rhs.head, rhs.tail);
	}

	// A == B

	template <unit_t U, allo_t B>
	friend constexpr auto operator==(const c_str& lhs, const c_str<U, B> /*&*/ & rhs) noexcept -> bool
	{
		const T* lhs_0 {lhs.head()}; const U* rhs_0 {rhs.head()};
		const T* lhs_N {lhs.tail()}; const U* rhs_N {rhs.tail()};

		return &lhs == &rhs || _equal(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, allo_t B>
	friend constexpr auto operator==(const slice& lhs, const c_str<U, B>::slice& rhs) noexcept -> bool
	{
		const T* lhs_0 {lhs.head}; const U* rhs_0 {rhs.head};
		const T* lhs_N {lhs.tail}; const U* rhs_N {rhs.tail};

		return &lhs == &rhs || _equal(lhs_0, lhs_N, rhs_0, rhs_N);
	}
	
	template <unit_t U, allo_t B>
	friend constexpr auto operator==(const c_str& lhs, const c_str<U, B>::slice& rhs) noexcept -> bool
	{
		const T* lhs_0 {lhs.head()}; const U* rhs_0 {rhs.head};
		const T* lhs_N {lhs.tail()}; const U* rhs_N {rhs.tail};

		return _equal(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, allo_t B>
	friend constexpr auto operator==(const slice& lhs, const c_str<U, B> /*&*/ & rhs) noexcept -> bool
	{
		const T* lhs_0 {lhs.head}; const U* rhs_0 {rhs.head()};
		const T* lhs_N {lhs.tail}; const U* rhs_N {rhs.tail()};

		return _equal(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator==(const c_str& lhs, const U /*literal*/ (&rhs)[N]) noexcept -> bool
	{
		const T* lhs_0 {lhs.head()}; const U* rhs_0 {&rhs[0x0]};
		const T* lhs_N {lhs.tail()}; const U* rhs_N {&rhs[N-1]};

		return _equal(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator==(const U /*literal*/ (&lhs)[N], const c_str& rhs) noexcept -> bool
	{
		const T* lhs_0 {&lhs[0x0]}; const U* rhs_0 {rhs.head()};
		const T* lhs_N {&lhs[N-1]}; const U* rhs_N {rhs.tail()};

		return _equal(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator==(const slice& lhs, const U /*literal*/ (&rhs)[N]) noexcept -> bool
	{
		const T* lhs_0 {lhs.head}; const U* rhs_0 {&rhs[0x0]};
		const T* lhs_N {lhs.tail}; const U* rhs_N {&rhs[N-1]};

		return _equal(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator==(const U /*literal*/ (&lhs)[N], const slice& rhs) noexcept -> bool
	{
		const T* lhs_0 {&lhs[0x0]}; const U* rhs_0 {rhs.head};
		const T* lhs_N {&lhs[N-1]}; const U* rhs_N {rhs.tail};

		return _equal(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	// A != B

	template <unit_t U, allo_t B>
	friend constexpr auto operator!=(const c_str& lhs, const c_str<U, B> /*&*/ & rhs) noexcept -> bool
	{
		const T* lhs_0 {lhs.head()}; const U* rhs_0 {rhs.head()};
		const T* lhs_N {lhs.tail()}; const U* rhs_N {rhs.tail()};

		return &lhs != &rhs && _nqual(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, allo_t B>
	friend constexpr auto operator!=(const slice& lhs, const c_str<U, B>::slice& rhs) noexcept -> bool
	{
		const T* lhs_0 {lhs.head}; const U* rhs_0 {rhs.head};
		const T* lhs_N {lhs.tail}; const U* rhs_N {rhs.tail};

		return &lhs != &rhs && _nqual(lhs_0, lhs_N, rhs_0, rhs_N);
	}
	
	template <unit_t U, allo_t B>
	friend constexpr auto operator!=(const c_str& lhs, const c_str<U, B>::slice& rhs) noexcept -> bool
	{
		const T* lhs_0 {lhs.head()}; const U* rhs_0 {rhs.head};
		const T* lhs_N {lhs.tail()}; const U* rhs_N {rhs.tail};

		return _nqual(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, allo_t B>
	friend constexpr auto operator!=(const slice& lhs, const c_str<U, B> /*&*/ & rhs) noexcept -> bool
	{
		const T* lhs_0 {lhs.head}; const U* rhs_0 {rhs.head()};
		const T* lhs_N {lhs.tail}; const U* rhs_N {rhs.tail()};

		return _nqual(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator!=(const c_str& lhs, const U /*literal*/ (&rhs)[N]) noexcept -> bool
	{
		const T* lhs_0 {lhs.head()}; const U* rhs_0 {&rhs[0x0]};
		const T* lhs_N {lhs.tail()}; const U* rhs_N {&rhs[N-1]};

		return _nqual(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator!=(const U /*literal*/ (&lhs)[N], const c_str& rhs) noexcept -> bool
	{
		const T* lhs_0 {&lhs[0x0]}; const U* rhs_0 {rhs.head()};
		const T* lhs_N {&lhs[N-1]}; const U* rhs_N {rhs.tail()};

		return _nqual(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator!=(const slice& lhs, const U /*literal*/ (&rhs)[N]) noexcept -> bool
	{
		const T* lhs_0 {lhs.head}; const U* rhs_0 {&rhs[0x0]};
		const T* lhs_N {lhs.tail}; const U* rhs_N {&rhs[N-1]};

		return _nqual(lhs_0, lhs_N, rhs_0, rhs_N);
	}

	template <unit_t U, size_t N>
	friend constexpr auto operator!=(const U /*literal*/ (&lhs)[N], const slice& rhs) noexcept -> bool
	{
		const T* lhs_0 {&lhs[0x0]}; const U* rhs_0 {rhs.head};
		const T* lhs_N {&lhs[N-1]}; const U* rhs_N {rhs.tail};

		return _nqual(lhs_0, lhs_N, rhs_0, rhs_N);
	}
};

// https://en.wikipedia.org/wiki/UTF-8
using utf8 = c_str<char8_t>;
// https://en.wikipedia.org/wiki/UTF-16
using utf16 = c_str<char16_t>;
// https://en.wikipedia.org/wiki/UTF-32
using utf32 = c_str<char32_t>;

#pragma region logic

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
requires (!std::is_same_v<T, char>
          &&
          !std::is_same_v<U, char>)

c_str<T, A>::operator c_str<U, B>() const noexcept
{
	c_str<U, B> str;
	
	if (0 < this->size())
	{
		size_t sum {0};
		
		const T* foo {this->head()};
		const T* bar {this->tail()};
		
		for (/* walk through ptr, from head to tail */; foo < bar;)
		{
			char32_t out;

			const auto T_size {c_str<T>::codec::next(foo)};
			c_str<T>::codec::decode_ptr(foo, out, T_size);
			const auto U_size {c_str<U>::codec::size(out)};

			foo += T_size;
			sum += U_size;
		}

		str.capacity(sum);
		str.__size__(sum);
		// reset to init
		foo = this->head();

		for (U* ptr {reinterpret_cast<U*>(str.head())}; foo < bar;)
		{
			char32_t out;

			const auto T_size {c_str<T>::codec::next(foo)};
			c_str<T>::codec::decode_ptr(foo, out, T_size);
			const auto U_size {c_str<U>::codec::size(out)};
			c_str<U>::codec::encode_ptr(out, ptr, U_size);

			foo += T_size;
			ptr += U_size;
		}
	}
	return str;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
c_str<T, A>::operator std::string() const noexcept
{
	typedef char8_t U;

	std::string str;

	if (0 < this->size())
	{
		size_t sum {0};

		const T* foo {this->head()};
		const T* bar {this->tail()};

		for (/* walk through ptr, from head to tail */; foo < bar;)
		{
			char32_t out;

			const auto T_size {c_str<T>::codec::next(foo)};
			c_str<T>::codec::decode_ptr(foo, out, T_size);
			const auto U_size {c_str<U>::codec::size(out)};

			foo += T_size;
			sum += U_size;
		}

		str.resize(sum);
		// reset to init
		foo = this->head();

		for (U* ptr {reinterpret_cast<U*>(str.data())}; foo < bar;)
		{
			char32_t out;

			const auto T_size {c_str<T>::codec::next(foo)};
			c_str<T>::codec::decode_ptr(foo, out, T_size);
			const auto U_size {c_str<U>::codec::size(out)};
			c_str<U>::codec::encode_ptr(out, ptr, U_size);

			foo += T_size;
			ptr += U_size;
		}
	}
	return str;
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_size(const T* head, const T* tail) noexcept -> size_t
{
	//|--------------------|
	//| Roses are red      |
	//|   Violets are blue |
	//| Sugar is sweet     |
	//|   & So are you :D  |
	//|--------------------|
	
	return tail - head;
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_size(const U* head, const U* tail) noexcept -> size_t
{
	size_t sum {0};

	for (const U* ptr {head}; ptr < tail;)
	{
		char32_t out;

		const auto U_size {c_str<U>::codec::next(ptr)};
		c_str<U>::codec::decode_ptr(ptr, out, U_size);
		const auto T_size {c_str<T>::codec::size(out)};

		ptr += U_size;
		sum += T_size;
	}
	return sum;
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_length(const T* head, const T* tail) noexcept -> size_t
{
	if constexpr (std::is_same_v<T, char/**/>
	              ||
	              std::is_same_v<T, char32_t>)
	{
		return tail - head;
	}
	size_t sum {0};
	
	for (const T* ptr {head}; ptr < tail; ++sum)
	{
		ptr += c_str<T>::codec::next(ptr);
	}
	return sum;
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_length(const U* head, const U* tail) noexcept -> size_t
{
	if constexpr (std::is_same_v<U, char/**/>
	              ||
	              std::is_same_v<U, char32_t>)
	{
		return tail - head;
	}
	size_t sum {0};

	for (const U* ptr {head}; ptr < tail; ++sum)
	{
		ptr += c_str<U>::codec::next(ptr);
	}
	return sum;
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_fcopy(const T* head, const T* tail,
                                                  /*&*/ T* dest) noexcept -> void
{
	std::ranges::copy/*forward*/(head, tail, dest);
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_fcopy(const U* head, const U* tail,
                                                  /*&*/ T* dest) noexcept -> void
{
	const T* foo {dest};
	/*
	for (const U* ptr {head}; ptr < tail;)
	{
		char32_t out;

		const auto U_size {c_str<U>::codec::next(ptr)};
		c_str<U>::codec::decode_ptr(ptr, out, U_size);
		const auto T_size {c_str<T>::codec::size(out)};

		ptr += U_size;
		foo += T_size;
	}
	*/
	for (const U* ptr {head}; ptr < tail;)
	{
		char32_t out;

		const auto U_size {c_str<U>::codec::next(ptr)};
		c_str<U>::codec::decode_ptr(ptr, out, U_size);
		const auto T_size {c_str<T>::codec::size(out)};
		c_str<T>::codec::encode_ptr(out, ptr, T_size);

		ptr += U_size;
		foo += T_size;
	}
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_rcopy(const T* head, const T* tail,
                                                  /*&*/ T* dest) noexcept -> void
{
	std::ranges::copy_backward(head, tail, dest);
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_rcopy(const U* head, const U* tail,
                                                  /*&*/ T* dest) noexcept -> void
{
	const T* foo {dest};

	for (const U* ptr {head}; ptr < tail;)
	{
		char32_t out;

		const auto U_size {c_str<U>::codec::next(ptr)};
		c_str<U>::codec::decode_ptr(ptr, out, U_size);
		const auto T_size {c_str<T>::codec::size(out)};

		ptr += U_size;
		foo += T_size;
	}

	for (const U* ptr {tail}; head < ptr;)
	{
		char32_t out;

		const auto U_size {c_str<U>::codec::back(ptr)};
		c_str<U>::codec::decode_ptr(ptr, out, U_size);
		const auto T_size {c_str<T>::codec::size(out)};
		c_str<T>::codec::encode_ptr(out, ptr, T_size);

		ptr += U_size;
		foo += T_size;
	}
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_equal(const T* lhs_0, const T* lhs_N,
                                   const T* rhs_0, const T* rhs_N) noexcept -> bool
{
		return _size(lhs_0, lhs_N)
		       ==
		       _size(rhs_0, rhs_N)
		       &&
		       std::ranges::equal(lhs_0, lhs_N, rhs_0, rhs_N);
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_equal(const T* lhs_0, const T* lhs_N,
                                   const U* rhs_0, const U* rhs_N) noexcept -> bool
{
	const T* foo {lhs_0};
	const U* bar {rhs_0};

	for (; foo < lhs_N && bar < rhs_N;)
	{
		char32_t T_out;
		char32_t U_out;

		const auto T_size {c_str<T>::codec::next(foo)};
		const auto U_size {c_str<U>::codec::next(bar)};

		c_str<T>::codec::decode_ptr(foo, T_out, T_size);
		c_str<U>::codec::decode_ptr(bar, U_out, U_size);

		if (T_out != U_out)
		{
			return false;
		}
		foo += T_size;
		bar += U_size;
	}
	return true;
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_equal(const U* lhs_0, const U* lhs_N,
                                   const T* rhs_0, const T* rhs_N) noexcept -> bool
{
	const T* foo {lhs_0};
	const U* bar {rhs_0};

	for (; foo < lhs_N && bar < rhs_N;)
	{
		char32_t T_out;
		char32_t U_out;

		const auto T_size {c_str<T>::codec::next(foo)};
		const auto U_size {c_str<U>::codec::next(bar)};

		c_str<T>::codec::decode_ptr(foo, T_out, T_size);
		c_str<U>::codec::decode_ptr(bar, U_out, U_size);

		if (T_out != U_out)
		{
			return false;
		}
		foo += T_size;
		bar += U_size;
	}
	return true;
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_nqual(const T* lhs_0, const T* lhs_N,
                                   const T* rhs_0, const T* rhs_N) noexcept -> bool
{
	return _size(lhs_0, lhs_N)
	       !=
	       _size(rhs_0, rhs_N)
	       ||
	       !std::ranges::equal(lhs_0, lhs_N, rhs_0, rhs_N);
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_nqual(const T* lhs_0, const T* lhs_N,
                                   const U* rhs_0, const U* rhs_N) noexcept -> bool
{
	const T* foo {lhs_0};
	const U* bar {rhs_0};

	for (; foo < lhs_N && bar < rhs_N;)
	{
		char32_t T_out;
		char32_t U_out;

		const auto T_size {c_str<T>::codec::next(foo)};
		const auto U_size {c_str<U>::codec::next(bar)};

		c_str<T>::codec::decode_ptr(foo, T_out, T_size);
		c_str<U>::codec::decode_ptr(bar, U_out, U_size);

		if (T_out != U_out)
		{
			return true;
		}
		foo += T_size;
		bar += U_size;
	}
	return false;
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_equal(c_str &  lhs_0,
                                   const T* rhs_0, const T* rhs_N) noexcept -> void
{
	const auto sum {_size(rhs_0, rhs_N)};

	// fix overflow
	lhs_0.capacity(sum);

	_fcopy
	(
		rhs_0, // U*
		rhs_N, // U*
		lhs_0.head()
	);

	// fix invariant
	lhs_0.__size__(sum);
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_equal(c_str &  lhs_0,
                                   const U* rhs_0, const U* rhs_N) noexcept -> void
{
	const auto sum {_size(rhs_0, rhs_N)};

	// fix overflow
	lhs_0.capacity(sum);

	_fcopy
	(
		rhs_0, // U*
		rhs_N, // U*
		lhs_0.head()
	);

	// fix invariant
	lhs_0.__size__(sum);
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_pqual(c_str &  lhs_0,
                                   const T* rhs_0, const T* rhs_N) noexcept -> void
{
	const auto sum {lhs_0.size() + _size(rhs_0, rhs_N)};

	// fix overflow
	lhs_0.capacity(sum);

	_fcopy
	(
		rhs_0, // U*
		rhs_N, // U*
		lhs_0.tail()
	);

	// fix invariant
	lhs_0.__size__(sum);
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_pqual(c_str &  lhs_0,
                                   const U* rhs_0, const U* rhs_N) noexcept -> void
{
	const auto sum {lhs_0.size() + _size(rhs_0, rhs_N)};

	// fix overflow
	lhs_0.capacity(sum);

	_fcopy
	(
		rhs_0, // U*
		rhs_N, // U*
		lhs_0.tail()
	);

	// fix invariant
	lhs_0.__size__(sum);
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_nqual(const U* lhs_0, const U* lhs_N,
                                   const T* rhs_0, const T* rhs_N) noexcept -> bool
{
	const T* foo {lhs_0};
	const U* bar {rhs_0};

	for (; foo < lhs_N && bar < rhs_N;)
	{
		char32_t T_out;
		char32_t U_out;

		const auto T_size {c_str<T>::codec::next(foo)};
		const auto U_size {c_str<U>::codec::next(bar)};

		c_str<T>::codec::decode_ptr(foo, T_out, T_size);
		c_str<U>::codec::decode_ptr(bar, U_out, U_size);

		if (T_out == U_out)
		{
			return false;
		}
		foo += T_size;
		bar += U_size;
	}
	return true;
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_cplus(const T* lhs_0, const T* lhs_N,
                                   const T* rhs_0, const T* rhs_N) noexcept -> c_str
{
	const auto lhs_L {_size(lhs_0, lhs_N)};
	const auto rhs_L {_size(rhs_0, rhs_N)};

	c_str str;

	// fix overflow
	str.capacity(lhs_L + rhs_L);

	T* const dest {str.head()};

	_fcopy
	(
		lhs_0, // const T*
		lhs_N, // const T*
		&dest[lhs_L * 0]
	);
	_fcopy
	(
		rhs_0, // const T*
		rhs_N, // const T*
		&dest[lhs_L * 1]
	);
	
	// fix invariant
	str.__size__(lhs_L + rhs_L);

	return str;
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_cplus(const T* lhs_0, const T* lhs_N,
                                   const U* rhs_0, const U* rhs_N) noexcept -> c_str
{
	const auto lhs_L {_size(lhs_0, lhs_N)};
	const auto rhs_L {_size(rhs_0, rhs_N)};

	c_str str;

	// fix overflow
	str.capacity(lhs_L + rhs_L);

	T* const dest {str.head()};

	_fcopy
	(
		lhs_0, // const T*
		lhs_N, // const T*
		&dest[lhs_L * 0]
	);
	_fcopy
	(
		rhs_0, // const U*
		rhs_N, // const U*
		&dest[lhs_L * 1]
	);

	// fix invariant
	str.__size__(lhs_L + rhs_L);

	return str;
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_cplus(const U* lhs_0, const U* lhs_N,
                                   const T* rhs_0, const T* rhs_N) noexcept -> c_str
{
	const auto lhs_L {_size(lhs_0, lhs_N)};
	const auto rhs_L {_size(rhs_0, rhs_N)};

	c_str str;

	// fix overflow
	str.capacity(lhs_L + rhs_L);

	T* const dest {str.head()};

	_fcopy
	(
		lhs_0, // const U*
		lhs_N, // const U*
		&dest[lhs_L * 0]
	);
	_fcopy
	(
		rhs_0, // const T*
		rhs_N, // const T*
		&dest[lhs_L * 1]
	);

	// fix invariant
	str.__size__(lhs_L + rhs_L);

	return str;
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_split(const T* lhs_0, const T* lhs_N,
                                   const T* rhs_0, const T* rhs_N) noexcept -> std::vector<slice>
{
	std::vector<slice> out;

	const auto lhs_L {_size(lhs_0, lhs_N)};
	const auto rhs_L {_size(rhs_0, rhs_N)};

	if (rhs_L == 0 || lhs_L < rhs_L) return out;

	// longest proper suffix for KMP search
	size_t* const lps {new size_t[rhs_L]};

	lps[0] = 0;

	for (size_t i {1}, j {0}; i < rhs_L; ++i)
	{
		while (0 < j && rhs_0[i] != rhs_0[j])
		{
			j = lps[j - 1];
		}
		lps[i] = rhs_0[i] == rhs_0[j] ? ++j : 0;
	}

	size_t x {0};

	for (size_t i {0}, j {0}; i < lhs_L; ++i)
	{
		while (0 < j && lhs_0[i] != rhs_0[j])
		{
			j = lps[j - 1];
		}
		/* equal */ if (lhs_0[i] == rhs_0[j])
		{
			++j;

			if (j == rhs_L)
			{
				out.emplace_back(&lhs_0[x], &lhs_0[i - rhs_L + 1]);

				j = lps[j - 1];

				x = i + 1;
			}
		}
	}

	if (x < lhs_L)
	{
		out.emplace_back(&lhs_0[x], &lhs_0[lhs_L]);
	}

	delete[] lps;
	return out;
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_split(const T* lhs_0, const T* lhs_N,
                                   const U* rhs_0, const U* rhs_N) noexcept -> std::vector<slice>
{
	std::vector<slice> out;

	const T* foo {lhs_0};
	const T* bar {lhs_0};

	size_t ahead {0};
	size_t equal {0};

	const T* lorem;
	const U* ipsum;

	for (; bar + ahead < lhs_N; bar += c_str<T>::codec::next(bar))
	{
		for (equal = 0, lorem = bar, ipsum = rhs_0; lorem < lhs_N && ipsum < rhs_N; ++equal)
		{
			char32_t T_out;
			char32_t U_out;

			const auto T_size {c_str<T>::codec::next(lorem)};
			const auto U_size {c_str<U>::codec::next(ipsum)};

			c_str<T>::codec::decode_ptr(lorem, T_out, T_size);
			c_str<U>::codec::decode_ptr(ipsum, U_out, U_size);

			if (T_out != U_out)
			{
				break;
			}
			lorem += T_size;
			ipsum += U_size;
		}

		if (ipsum == rhs_N)
		{
			out.emplace_back(&foo[0], &bar[0]);

			foo = bar += (ahead = lorem - bar);
		}
	}
	if (foo < lhs_N)
	{
		out.emplace_back(foo, lhs_N);
	}
	return out;
}

template <unit_t T, allo_t A>
                            // [typeof(lhs) == typeof(rhs)]
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_match(const T* lhs_0, const T* lhs_N,
                                   const T* rhs_0, const T* rhs_N) noexcept -> std::vector<slice>
{
	std::vector<slice> out;

	const auto lhs_L {_size(lhs_0, lhs_N)};
	const auto rhs_L {_size(rhs_0, rhs_N)};

	if (rhs_L == 0 || lhs_L < rhs_L) return out;

	// longest proper suffix for KMP search
	size_t* const lps {new size_t[rhs_L]};

	lps[0] = 0;

	for (size_t i {1}, j {0}; i < rhs_L; ++i)
	{
		while (0 < j && rhs_0[i] != rhs_0[j])
		{
			j = lps[j - 1];
		}
		lps[i] = rhs_0[i] == rhs_0[j] ? ++j : 0;
	}

	for (size_t i {0}, j {0}; i < lhs_L; ++i)
	{
		while (0 < j && lhs_0[i] != rhs_0[j])
		{
			j = lps[j - 1];
		}
		/* equal */ if (lhs_0[i] == rhs_0[j])
		{
			++j;

			if (j == rhs_L)
			{
				out.emplace_back(&lhs_0[i - rhs_L + 1], &lhs_0[i]);

				j = lps[j - 1];
			}
		}
	}

	delete[] lps;
	return out;
}

template <unit_t T, allo_t A>
template <unit_t U          > requires (!std::is_same_v<T, U>)
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_match(const T* lhs_0, const T* lhs_N,
                                   const U* rhs_0, const U* rhs_N) noexcept -> std::vector<slice>
{
	std::vector<slice> out;

	const T* foo {lhs_0};

	size_t ahead {0};
	size_t equal {0};

	const T* lorem;
	const U* ipsum;

	for (; foo + ahead < lhs_N; foo += c_str<T>::codec::next(foo))
	{
		for (equal = 0, lorem = foo, ipsum = rhs_0; lorem < lhs_N && ipsum < rhs_N; ++equal)
		{
			char32_t T_out;
			char32_t U_out;

			const auto T_size {c_str<T>::codec::next(lorem)};
			const auto U_size {c_str<U>::codec::next(ipsum)};

			c_str<T>::codec::decode_ptr(lorem, T_out, T_size);
			c_str<U>::codec::decode_ptr(ipsum, U_out, U_size);

			if (T_out != U_out)
			{
				break;
			}
			lorem += T_size;
			ipsum += U_size;
		}

		if (ipsum == rhs_N)
		{
			out.emplace_back(foo, lorem);

			foo += (ahead = lorem - foo);
		}
	}
	return out;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_range(const T* head, const T* tail, clamp  start, clamp  until) noexcept -> slice
{
	assert(until < start);

	const T* foo {tail};
	
	for (size_t i {0/**/}; i < until && head < foo; ++i, foo += codec::back(foo)) {}

	const T* bar {foo};

	for (size_t i {until}; i < start && head < bar; ++i, bar += codec::back(bar)) {}

	assert(head <= foo && foo <= tail);
	assert(head <= bar && bar <= tail);

	return {bar, foo};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_range(const T* head, const T* tail, clamp  start, range  until) noexcept -> slice
{
	const T* foo {tail};

	for (size_t i {0/**/}; i < start && head < foo; ++i, foo += codec::back(foo)) {}

	const T* bar {tail};

	assert(head <= foo && foo <= tail);
	assert(head <= bar && bar <= tail);

	return {foo, bar};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_range(const T* head, const T* tail, size_t start, clamp  until) noexcept -> slice
{
	const T* foo {head};

	for (size_t i {0/**/}; i < start && foo < tail; ++i, foo += codec::next(foo)) {}

	const T* bar {tail};

	for (size_t i {0/**/}; i < until && head < bar; ++i, bar += codec::back(bar)) {}

	assert(head <= foo && foo <= tail);
	assert(head <= bar && bar <= tail);

	return {foo, bar};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_range(const T* head, const T* tail, size_t start, range  until) noexcept -> slice
{
	const T* foo {head};

	for (size_t i {0/**/}; i < start && foo < tail; ++i, foo += codec::next(foo)) {}

	const T* bar {tail};

	assert(head <= foo && foo <= tail);
	assert(head <= bar && bar <= tail);

	return {foo, bar};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::_range(const T* head, const T* tail, size_t start, size_t until) noexcept -> slice
{
	assert(start < until);

	const T* foo {head};

	for (size_t i {0/**/}; i < start && foo < tail; ++i, foo += codec::next(foo)) {}

	const T* bar {foo};

	for (size_t i {start}; i < until && bar < tail; ++i, bar += codec::next(bar)) {}

	assert(head <= foo && foo <= tail);
	assert(head <= bar && bar <= tail);

	return {foo, bar};
}

#pragma endregion logic

#pragma region codec

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::codec::size(char32_t code) noexcept -> int8_t
{
	if constexpr (std::is_same_v<T, char/**/>)
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

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::codec::next(const T* ptr ) noexcept -> int8_t
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

	if constexpr (std::is_same_v<T, char/**/>)
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

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::codec::back(const T* ptr ) noexcept -> int8_t
{
	if constexpr (std::is_same_v<T, char/**/>)
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
		// walk backward until leading surrogate code unit is found
		int8_t i {-1}; for (; (ptr[i] >> 0xA) == 0x37; --i) {} return i;
	}
	if constexpr (std::is_same_v<T, char32_t>)
	{
		return -1;
	}
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::codec::encode_ptr(const char32_t in, T* out, int8_t size) noexcept -> void
{
	if constexpr (std::is_same_v<T, char/**/>)
	{
		out[0] = static_cast<T>(in);
	}
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

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::codec::decode_ptr(const T* in, char32_t& out, int8_t size) noexcept -> void
{
	if constexpr (std::is_same_v<T, char/**/>)
	{
		out = static_cast<char32_t>(in[0]);
	}
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

#pragma region SSO

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr c_str<T, A>::buffer::operator const T*() const noexcept
{
	return this->head;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr c_str<T, A>::buffer::operator /*&*/ T*() /*&*/ noexcept
{
	return this->head;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr c_str<T, A>::storage::storage() noexcept
{
	this->__union__.bytes[RMB] = MAX << SFT;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr c_str<T, A>::storage::~storage() noexcept
{
	if (this->mode() == LARGE)
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

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::storage::mode() const noexcept -> mode_t
{
	return static_cast<mode_t>(this->__union__.bytes[RMB] & MSK);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::storage::mode() /*&*/ noexcept -> mode_t
{
	return static_cast<mode_t>(this->__union__.bytes[RMB] & MSK);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::head() const noexcept -> const T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[0 /* ✨ roeses are red, violets are blue ✨ */]
	       :
	       &this->store.__union__.large[0 /* ✨ roeses are red, violets are blue ✨ */];
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::head() /*&*/ noexcept -> /*&*/ T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[0 /* ✨ roeses are red, violets are blue ✨ */]
	       :
	       &this->store.__union__.large[0 /* ✨ roeses are red, violets are blue ✨ */];
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::tail() const noexcept -> const T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[MAX - (this->store.__union__.bytes[RMB] >> SFT)]
	       :
	       &this->store.__union__.large[0x0 + (this->store.__union__.large.size >> 0x0)];
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::tail() /*&*/ noexcept -> /*&*/ T*
{
	return this->store.mode() == SMALL
	       ?
	       &this->store.__union__.small[MAX - (this->store.__union__.bytes[RMB] >> SFT)]
	       :
	       &this->store.__union__.large[0x0 + (this->store.__union__.large.size >> 0x0)];
}

#pragma endregion SSO

#pragma region cursor

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::cursor::operator*() noexcept -> char32_t
{
	char32_t out;

	const auto size {codec::next(this->ptr)};
	codec::decode_ptr(this->ptr, out, size);

	return out;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::cursor::operator&() noexcept -> const T*
{
	return this->ptr;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::cursor::operator++(   ) noexcept -> cursor&
{
	this->ptr += codec::next(this->ptr); return *this;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::cursor::operator++(int) noexcept -> cursor
{
	const auto clone {*this}; operator++(); return clone;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::cursor::operator--(   ) noexcept -> cursor&
{
	this->ptr += codec::back(this->ptr); return *this;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::cursor::operator--(int) noexcept -> cursor
{
	const auto clone {*this}; operator--(); return clone;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::cursor::operator==(const cursor& rhs) noexcept -> bool
{
	return this->ptr == rhs.ptr;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::cursor::operator!=(const cursor& rhs) noexcept -> bool
{
	return this->ptr != rhs.ptr;
}

#pragma endregion cursor

#pragma region c_str

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::__size__(size_t value) /*&*/ noexcept -> void
{
	switch (this->store.mode())
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

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::capacity(/* getter */) const noexcept -> size_t
{
	return this->store.mode() == SMALL
	       ?
	       MAX // or calculate the std::ptrdiff_t just as large mode as shown below
	       :
	       this->store.__union__.large.tail - this->store.__union__.large.head - 1;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::capacity(size_t value) /*&*/ noexcept -> void
{
	if (this->capacity() < value)
	{
		T* head {std::allocator_traits<A>::allocate(this->store, value + 1)};

		_fcopy
		(
			this->head(),
			this->tail(),
			head // dest
		);

		if (this->store.mode() == LARGE)
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

		T* tail {head + value + 1};

		const auto size {this->size()};

		this->store.__union__.large.head = head;
		this->store.__union__.large.tail = tail;
		this->store.__union__.large.size = size;
		this->store.__union__.large.meta = LARGE;
	}
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::size() const noexcept -> size_t
{
	return _size(this->head(), this->tail());
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::length() const noexcept -> size_t
{
	return _length(this->head(), this->tail());
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::split(const c_str<U, B> /*&*/ & value) const noexcept -> std::vector<slice>
{
	return _split(this->head(), this->tail(), value.head(), value.tail());
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::split(const c_str<U, B>::slice& value) const noexcept -> std::vector<slice>
{
	return _split(this->head(), this->tail(), value.head, value.tail);
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::split(const U /*literal*/ (&value)[N]) const noexcept -> std::vector<slice>
{
	return _split(this->head(), this->tail(), &value[0x0], &value[N-1]);
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::match(const c_str<U, B> /*&*/ & value) const noexcept -> std::vector<slice>
{
	return _match(this->head(), this->tail(), value.head(), value.tail());
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::match(const c_str<U, B>::slice& value) const noexcept -> std::vector<slice>
{
	return _match(this->head(), this->tail(), value.head, value.tail);
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::match(const U /*literal*/ (&value)[N]) const noexcept -> std::vector<slice>
{
	return _match(this->head(), this->tail(), &value[0x0], &value[N-1]);
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::starts_with(const c_str<U, B> /*&*/ & value) const noexcept -> bool
{
	return this->operator[](0, value.length()) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::starts_with(const c_str<U, B>::slice& value) const noexcept -> bool
{
	return this->operator[](0, value.length()) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::starts_with(const U /*literal*/ (&value)[N]) const noexcept -> bool
{
	return this->operator[](0, (N - 1)) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::ends_with(const c_str<U, B> /*&*/ & value) const noexcept -> bool
{
	return this->operator[](range::N - value.length(), range::N) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::ends_with(const c_str<U, B>::slice& value) const noexcept -> bool
{
	return this->operator[](range::N - value.length(), range::N) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::ends_with(const U /*literal*/ (&value)[N]) const noexcept -> bool
{
	return this->operator[](range::N - (N - 1), range::N) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::contains(const c_str<U, B> /*&*/ & value) const noexcept -> bool
{
	return !this->match(value).empty();
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::contains(const c_str<U, B>::slice& value) const noexcept -> bool
{
	return !this->match(value).empty();
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::contains(const U /*literal*/ (&value)[N]) const noexcept -> bool
{
	return !this->match(value).empty();
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::begin() const noexcept -> cursor
{
	return {this->head()};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::end() const noexcept -> cursor
{
	return {this->tail()};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::operator[](size_t value) const noexcept -> reader
{
	return {this, value};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::operator[](size_t value) /*&*/ noexcept -> writer
{
	return {this, value};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::operator[](clamp  start, clamp  until) const noexcept -> slice
{
	return _range(this->head(), this->tail(), start, until);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::operator[](clamp  start, range  until) const noexcept -> slice
{
	return _range(this->head(), this->tail(), start, until);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::operator[](size_t start, clamp  until) const noexcept -> slice
{
	return _range(this->head(), this->tail(), start, until);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::operator[](size_t start, range  until) const noexcept -> slice
{
	return _range(this->head(), this->tail(), start, until);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::operator[](size_t start, size_t until) const noexcept -> slice
{
	return _range(this->head(), this->tail(), start, until);
}

#pragma endregion c_str

#pragma region slice

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::size() const noexcept -> size_t
{
	return _size(this->head, this->tail);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::length() const noexcept -> size_t
{
	return _length(this->head, this->tail);
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::split(const c_str<U, B> /*&*/ & value) const noexcept -> std::vector<slice>
{
	return _split(this->head, this->tail, value.head(), value.tail());
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::split(const c_str<U, B>::slice& value) const noexcept -> std::vector<slice>
{
	return _split(this->head, this->tail, value.head, value.tail);
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::split(const U /*literal*/ (&value)[N]) const noexcept -> std::vector<slice>
{
	return _split(this->head, this->tail, &value[0x0], &value[N-1]);
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::match(const c_str<U, B> /*&*/ & value) const noexcept -> std::vector<slice>
{
	return _match(this->head, this->tail, value.head(), value.tail());
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::match(const c_str<U, B>::slice& value) const noexcept -> std::vector<slice>
{
	return _match(this->head, this->tail, value.head, value.tail);
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::match(const U /*literal*/ (&value)[N]) const noexcept -> std::vector<slice>
{
	return _match(this->head, this->tail, &value[0x0], &value[N-1]);
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::starts_with(const c_str<U, B> /*&*/ & value) const noexcept -> bool
{
	return this->operator[](0, value.length()) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::starts_with(const c_str<U, B>::slice& value) const noexcept -> bool
{
	return this->operator[](0, value.length()) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::starts_with(const U /*literal*/ (&value)[N]) const noexcept -> bool
{
	return this->operator[](0, (N - 1)) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::ends_with(const c_str<U, B> /*&*/ & value) const noexcept -> bool
{
	return this->operator[](range::N - value.length(), range::N) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::ends_with(const c_str<U, B>::slice& value) const noexcept -> bool
{
	return this->operator[](range::N - value.length(), range::N) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::ends_with(const U /*literal*/ (&value)[N]) const noexcept -> bool
{
	return this->operator[](range::N - (N - 1), range::N) == value;
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::contains(const c_str<U, B> /*&*/ & value) const noexcept -> bool
{
	return !this->match(value).empty();
}

template <unit_t T, allo_t A>
template <unit_t U, allo_t B>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::contains(const c_str<U, B>::slice& value) const noexcept -> bool
{
	return !this->match(value).empty();
}

template <unit_t T, allo_t A>
template <unit_t U, size_t N>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::contains(const U /*literal*/ (&value)[N]) const noexcept -> bool
{
	return !this->match(value).empty();
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::begin() const noexcept -> cursor
{
	return {this->head};
}

template <unit_t T, allo_t A>
constexpr auto c_str<T, A>::slice::end() const noexcept -> cursor
{
	return {this->tail};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::operator[](size_t value) const noexcept -> reader
{
	return {this, value};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::operator[](size_t value) /*&*/ noexcept -> writer
{
	return {this, value};
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::operator[](clamp  start, clamp  until) const noexcept -> slice
{
	return _range(this->head, this->tail, start, until);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::operator[](clamp  start, range  until) const noexcept -> slice
{
	return _range(this->head, this->tail, start, until);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::operator[](size_t start, clamp  until) const noexcept -> slice
{
	return _range(this->head, this->tail, start, until);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::operator[](size_t start, range  until) const noexcept -> slice
{
	return _range(this->head, this->tail, start, until);
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::slice::operator[](size_t start, size_t until) const noexcept -> slice
{
	return _range(this->head, this->tail, start, until);
}

#pragma endregion slice

#pragma region c_str::reader

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
[[nodiscard]] constexpr c_str<T, A>::reader::operator char32_t() const noexcept
{
	const T* head {this->src->head()};
	const T* tail {this->src->tail()};

	size_t i {0};

	if constexpr (std::is_same_v<T, char/**/>
	              ||
	              std::is_same_v<T, char32_t>)
	{
		if (this->arg < this->src->size())
		{
			head += this->arg;
			goto __SHORTCUT__;
		}
		return U'\uFFFD';
	}

	for (; head < tail; ++i, head += codec::next(head))
	{
		if (i == this->arg)
		{
			__SHORTCUT__:

			char32_t out;

			const auto size {codec::next(head)};
			codec::decode_ptr(head, out, size);

			return out;
		}
	}
	return U'\uFFFD';
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
auto constexpr c_str<T, A>::reader::operator==(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() == code;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
auto constexpr c_str<T, A>::reader::operator!=(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() != code;
}

#pragma endregion c_str::reader

#pragma region c_str::writer

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
constexpr auto c_str<T, A>::writer::operator=(char32_t code) noexcept -> writer&
{
	T* head {this->src->head()};
	T* tail {this->src->tail()};

	size_t i {0};

	if constexpr (std::is_same_v<T, char/**/>
	              ||
	              std::is_same_v<T, char32_t>)
	{
		if (this->arg < this->src->size())
		{
			head += this->arg;
			goto __SHORTCUT__;
		}
		return *this;
	}

	for (; head < tail; ++i, head += codec::next(head))
	{
		if (i == this->arg)
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

				if (this->src->capacity() <= new_l)
				{
					this->src->capacity(new_l * 2);

					head = this->src->head();
					tail = this->src->tail();
				}
				_rcopy
				(
					&head[a],
					&tail[0],
					&head[b]
				);
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

				_fcopy
				(
					&head[a],
					&tail[0],
					&head[b]
				);
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

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
[[nodiscard]] constexpr c_str<T, A>::slice::reader::operator char32_t() const noexcept
{
	const T* head {this->src->head};
	const T* tail {this->src->tail};

	size_t i {0};

	if constexpr (std::is_same_v<T, char/**/>
	              ||
	              std::is_same_v<T, char32_t>)
	{
		if (this->arg < this->src->size())
		{
			head += this->arg;
			goto __SHORTCUT__;
		}
		return U'\uFFFD';
	}

	for (; head < tail; ++i, head += codec::next(head))
	{
		if (i == this->arg)
		{
			__SHORTCUT__:

			char32_t out;

			const auto size {codec::next(head)};
			codec::decode_ptr(head, out, size);

			return out;
		}
	}
	return U'\uFFFD';
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
auto constexpr c_str<T, A>::slice::reader::operator==(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() == code;
}

template <unit_t T, allo_t A>
//────────────୨ৎ────────────//
auto constexpr c_str<T, A>::slice::reader::operator!=(char32_t code) const noexcept -> bool
{
	return this->operator char32_t() != code;
}

#pragma endregion slice::reader

#undef COPY_PTR_LOGIC
#undef SWAP_PTR_LOGIC

#undef COPY_ASSIGNMENT
#undef MOVE_ASSIGNMENT

#undef COPY_CONSTRUCTOR
#undef MOVE_CONSTRUCTOR

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

// NOLINTEND(*-magic-numbers, *-union-access, *-signed-bitwise, *-avoid-c-arrays, *-pointer-arithmetic, *-constant-array-index, *-explicit-conversions)
