#pragma once

#include <cstdlib>
#include <algorithm>
#include <cassert>

template<class T>
requires (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8)
static T byteSwap(T val) {
	if constexpr (sizeof(T) == 1) {
		return val;
	}
	else if constexpr (sizeof(T) == 2) {
#ifdef WIN32
		static_assert(sizeof(unsigned short) == 2);
		auto w = _byteswap_ushort(*reinterpret_cast<unsigned short*>(&val));
		return *reinterpret_cast<T*>(&w);
#else
		char* r = reinterpret_cast<char*>(&val);
		std::swap(r[1], r[0]);
		return val;
#endif
	}
	else if constexpr (sizeof(T) == 4) {
#ifdef WIN32
		static_assert(sizeof(unsigned long) == 4);
		auto w = _byteswap_ulong(*reinterpret_cast<unsigned long*>(&val));
		return *reinterpret_cast<T*>(&w);
#else
		char* r = reinterpret_cast<char*>(&val);
		std::swap(r[3], r[0]);
		std::swap(r[2], r[1]);
		return val;
#endif
	}
	else if constexpr (sizeof(T) == 8) {
#ifdef WIN32
		static_assert(sizeof(unsigned long long) == 8);
		auto w = _byteswap_uint64(*reinterpret_cast<unsigned long long*>(&val));
		return *reinterpret_cast<T*>(&w);
#else
		char* r = reinterpret_cast<char*>(&val);
		std::swap(r[7], r[0]);
		std::swap(r[6], r[1]);
		std::swap(r[5], r[2]);
		std::swap(r[4], r[3]);
		return val;
#endif
	}
	else {
		return val;
	}
}