#pragma once

// std
#include <type_traits>
#include <span>
#include <array>
// internal
#include "defines.h"
#include "types.h"

namespace util
{
	template <class... T>
	struct always_false : std::false_type { };
	template<class... T>
	constexpr bool always_false_v = always_false<T...>::value;


	template<typename E>
	concept Enum = std::is_enum_v<E>;
	template<typename I>
	concept Integral = std::is_integral_v<I>;
	template<typename C>
	concept Container = requires(C c) // Lightweight Container concept
	{
		typename C::value_type;
		typename C::reference;
		typename C::const_reference;
		typename C::iterator;
		typename C::const_iterator;
		typename C::difference_type;
		typename C::size_type;
		{ c.size() } -> std::same_as<typename C::size_type>;
	};

	
	template<Enum EnumType>
	constexpr auto to_underlying(EnumType _enum)
		-> std::underlying_type_t<EnumType>
	{
		return static_cast<std::underlying_type_t<EnumType>>(_enum);
	}

	template<typename T, size_t size>
	constexpr size_t total_size(const T(&)[size])
	{
		return size * sizeof(T);
	}

	template<template<typename, typename...> class C, typename T, typename... Args>
	constexpr size_t total_size(C<T, Args...> const& container)
	{
		return std::size(container) * sizeof(T);
	}

	template<Integral T>
	constexpr size_t total_size(T const&)
	{
		return sizeof(T);
	}

	template<class T>
	constexpr size_t total_size(T const&)
	{
		return sizeof(T);
	}

	template<class T>
	constexpr size_t total_size()
	{
		return sizeof(T);
	}

	template<typename T, size_t Size>
	constexpr std::span<const T> to_span(const T(&arr)[Size])
	{
		return std::span(&arr[0], Size);
	}
	template<typename T, size_t Size>
	constexpr std::span<T> to_span(T(&arr)[Size])
	{
		return std::span(&arr[0], Size);
	}

	template<typename T, size_t Size>
	constexpr std::span<T, Size> to_span(std::array<T, Size>& arr)
	{
		return std::span(arr.data(), Size);
	}

	template<typename T, size_t Size>
	constexpr std::span<const T, Size> to_span(const std::array<T, Size>& arr)
	{
		return std::span(arr.data(), Size);
	}

	template<typename T>
	constexpr T byte_swap(T const value)
	{
		static_assert(std::is_integral_v<T>, "Can't swap bytes of non integral value.");
		if constexpr(sizeof(T) == 1)
		{
			return value;
		} else if constexpr(sizeof(T) == 2)
		{
			return (value & 0x00FF) << BYTES_TO_BITS(1) | 
				    value			>> BYTES_TO_BITS(1);
		} else if constexpr(sizeof(T) == 4)
		{
			return (value & 0xFF)	 << BYTES_TO_BITS(3) | 
				(value & 0xFF'00)	 << BYTES_TO_BITS(1) | 
				(value & 0xFF'00'00) >> BYTES_TO_BITS(1) | 
				value				 >> BYTES_TO_BITS(3);
		} else if constexpr(sizeof(T) == 8)
		{
			return (value & 0xFF)				 << BYTES_TO_BITS(7) |
				(value & 0xFF'00)				 << BYTES_TO_BITS(5) |
				(value & 0xFF'00'00)			 << BYTES_TO_BITS(3) |
				(value & 0xFF'00'00'00)			 << BYTES_TO_BITS(1) |
				(value & 0xFF'00'00'00'00)		 >> BYTES_TO_BITS(1) |
				(value & 0xFF'00'00'00'00'00)	 >> BYTES_TO_BITS(3) |
				(value & 0xFF'00'00'00'00'00'00) >> BYTES_TO_BITS(5) |
				value							 >> BYTES_TO_BITS(7);
		} else
		{
			static_assert(always_false_v<T>, "Can't swap bytes. Size of type is unequal to 1, 2, 4 or 8 Bytes.");
		}
		return {};
	}

}

#define create_bitwise_operators(enum_t) \
	constexpr enum_t operator | (enum_t a, enum_t b) noexcept { return static_cast<enum_t>(to_underlying(a) | to_underlying(b)); } \
	enum_t& operator |= (enum_t& a, enum_t b) noexcept { return a = static_cast<enum_t>(to_underlying(a) | to_underlying(b)); } \
	constexpr enum_t operator & (enum_t a, enum_t b) noexcept { return static_cast<enum_t>(to_underlying(a) & to_underlying(b)); } \
	enum_t& operator &= (enum_t& a, enum_t b) noexcept { return a = static_cast<enum_t>(to_underlying(a) & to_underlying(b)); } \
	constexpr enum_t operator ~ (enum_t a) noexcept { return static_cast<enum_t>(~to_underlying(a)); } \
	constexpr enum_t operator ^ (enum_t a, enum_t b) noexcept { return static_cast<enum_t>(to_underlying(a) ^ to_underlying(b)); } \
	enum_t& operator ^= (enum_t& a, enum_t b) noexcept { return a = static_cast<enum_t>(to_underlying(a) ^ to_underlying(b)); }
