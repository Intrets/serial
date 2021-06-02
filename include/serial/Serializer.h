#pragma once

#include <wglm/glm.hpp>
#include <vector>
#include <ostream>
#include <istream>
#include <type_traits>
#include <cstdint>

#include <tepp/tepp.h>

#define SAFETY_CHECKS

#ifdef SAFETY_CHECKS
#define READ(X) if (!serializer.read(X)) return false
#define WRITE(X) if (!serializer.write(X)) return false
#else
#define READ(X) serializer.read(X)
#define WRITE(X) serialiser.write(X)
#endif

template<class T>
struct Serializable;

struct Write;
struct Read;

struct Serializer
{
	std::istream* readStream{ nullptr };
	std::ostream* writeStream{ nullptr };

	bool writeBytes(char const* bytes, std::streamsize size);
	bool readBytes(char* target, std::streamsize size);


	template<class T>
	bool write(T const& val);

	template<class T>
	bool read(T& val);


	template<class... Args>
	bool readAll(Args&... args);

	template<class... Args>
	bool writeAll(Args&... args);

	template<class Selector, class... Args>
	bool runAll(Args&... args);


	Serializer(std::ostream& writeStream_);
	Serializer(std::istream& readStream_);
};

template<class T>
requires te::contains_v<te::list<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>, T>
struct Serializable<T>
{
	static bool read(Serializer& serializer, T& val) {
		return serializer.readBytes(reinterpret_cast<char*>(&val), sizeof(val));
	};

	static bool write(Serializer& serializer, T const& val) {
		return serializer.writeBytes(reinterpret_cast<char const*>(&val), sizeof(val));
	}
};

template<class T>
requires (std::is_same_v<size_t, T> && !std::is_same_v<size_t, uint64_t>)
struct Serializable<T>
{
	static bool read(Serializer& serializer, T& val) {
		uint64_t v;
		READ(v);
		val = static_cast<size_t>(v);
		return true;
	};

	static bool write(Serializer& serializer, T const& val) {
		uint64_t v = static_cast<uint64_t>(val);
		return serializer.write(v);
	}
};

template<>
struct Serializable<bool>
{
	static bool read(Serializer& serializer, bool& val) {
		int8_t v;
		READ(v);
		val = static_cast<bool>(v);
		return true;
	};

	static bool write(Serializer& serializer, bool const& val) {
		return serializer.write(static_cast<int8_t>(val));
	}
};

template<>
struct Serializable<glm::ivec2>
{
	static bool read(Serializer& serializer, glm::ivec2& val) {
		int32_t x;
		READ(x);
		val.x = x;
		int32_t y;
		READ(y);
		val.y = y;
		return true;
	}

	static bool write(Serializer& serializer, glm::ivec2 const& val) {
		return
			serializer.write(static_cast<int32_t>(val.x)) &&
			serializer.write(static_cast<int32_t>(val.y));
	}
};

template<class... Args>
inline bool Serializer::writeAll(Args&... args) {
#ifdef SAFETY_CHECKS
	return (this->write(args) && ...);
#else
	(this->write(args), ...);
	return true;
#endif // SAFETY_CHECKS
}

template<class Selector, class ...Args>
inline bool Serializer::runAll(Args& ...args) {
	if constexpr (std::is_same_v<Selector, Read>) {
		return this->readAll(args...);
	}
	else if constexpr (std::is_same_v<Selector, Write>) {
		return this->writeAll(args...);
	}
	else {
		static_assert(0);
	}
}

template<class... Args>
inline bool Serializer::readAll(Args& ...args) {
#ifdef SAFETY_CHECKS
	return (this->read(args) && ...);
#else
	(this->read(args), ...);
	return true;
#endif // SAFETY_CHECKS
}

template<class T>
struct Serializable<std::vector<T>>
{
	static bool read(Serializer& serializer, std::vector<T>& vec) {
		size_t size;
		READ(size);
		vec.resize(size);
		for (size_t i = 0; i < size; i++) {
			READ(vec[i]);
		}
		return true;
	};

	static bool write(Serializer& serializer, std::vector<T> const& vec) {
		WRITE(vec.size());
		for (auto const& v : vec) {
			WRITE(v);
		}
		return true;
	}
};

template<class T>
concept has_run = requires (T t, Serializer s) { Serializable<T>::template run<Read>(s, t); Serializable<T>::template run<Write>(s, t); };

template<class T>
inline bool Serializer::write(T const& val) {
	if constexpr (has_run<T>) {
		return Serializable<T>::template run<Write, T const&>(*this, val);
	}
	else {
		return Serializable<T>::write(*this, val);
	}
}

template<class T>
inline bool Serializer::read(T& val) {
	if constexpr (has_run<T>) {
		return Serializable<T>::template run<Read, T&>(*this, val);
	}
	else {
		return Serializable<T>::read(*this, val);
	}
}