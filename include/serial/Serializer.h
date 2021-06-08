#pragma once

#include <wglm/glm.hpp>
#include <vector>
#include <ostream>
#include <istream>
#include <type_traits>
#include <cstdint>
#include <string>
#include <array>
#include <bitset>
#include <optional>
#include <string_view>
#include <utility>

#include <tepp/tepp.h>

#define READ(X) if (!serializer.read(X)) return false
#define READBYTES(X, S) if (!serializer.readBytes(X, S)) return false
#define WRITE(X) if (!serializer.write(X)) return false
#define WRITEBYTES(X, S) if (!serializer.writeBytes(X, S)) return false

template<class T>
struct Serializable;

struct Write;
struct Read;

template<class T>
struct Wrapped
{
	using value_type = std::remove_cvref_t<T>;
	T& val;
	std::string_view name;
};

template<class T>
concept is_wrapped = requires (T t) { t.val; };

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
	bool readAll(Args&&... args);

	template<class... Args>
	bool writeAll(Args&&... args);

	template<class Selector, class... Args>
	bool runAll(Args&&... args);

	Serializer(std::ostream& writeStream_);
	Serializer(std::istream& readStream_);
	Serializer() = default;
	~Serializer() = default;
};

template<>
struct Serializable<std::string>
{
	static bool read(Serializer& serializer, std::string& val) {
		size_t size;
		READ(size);
		val.resize(size);
		return serializer.readBytes(&val[0], size);
	};

	static bool write(Serializer& serializer, std::string const& val) {
		WRITE(val.size());
		return serializer.writeBytes(val.c_str(), val.size());
	}
};

template<class T>
struct Serializable<std::optional<T>>
{
	static bool read(Serializer& serializer, std::optional<T>& val) {
		bool hasValue;
		READ(hasValue);
		if (hasValue) {
			val = T();
			return serializer.read(val.value());
		}
		else {
			val = std::nullopt;
			return true;
		}
	};

	static bool write(Serializer& serializer, std::optional<T> const& val) {
		if (val.has_value()) {
			WRITE(true);
			return serializer.write(val.value());
		}
		else {
			WRITE(false);
			return true;
		}
	}
};

using simple_types = te::list<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, std::byte>;

template<class T>
requires te::contains_v<simple_types, T>
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
struct Serializable<float>
{
	static bool read(Serializer& serializer, float& val) {
		std::string s;
		READ(s);
		char* end;
		val = std::strtof(s.c_str(), &end);
		assert(end != s.c_str());
		return (end != s.c_str());
	}

	static bool write(Serializer& serializer, float const& val) {
		return serializer.write(std::to_string(val));
	}
};

template<>
struct Serializable<glm::vec2>
{
	static bool read(Serializer& serializer, glm::vec2& val) {
		return serializer.readAll<float&, float&>(val.x, val.y);
	}

	static bool write(Serializer& serializer, glm::vec2 const& val) {
		return serializer.writeAll<float const&, float const&>(val.x, val.y);
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
inline bool Serializer::writeAll(Args&&... args) {
#ifdef SAFETY_CHECKS
	return (this->write(args) && ...);
#else
	(this->write(args), ...);
	return true;
#endif // SAFETY_CHECKS
}

template<class Selector, class ...Args>
inline bool Serializer::runAll(Args&& ...args) {
	if constexpr (std::is_same_v<Selector, Read>) {
		return this->readAll(std::forward<Args>(args)...);
	}
	else if constexpr (std::is_same_v<Selector, Write>) {
		return this->writeAll(std::forward<Args>(args)...);
	}
	else {
		static_assert(0);
	}
}

template<class... Args>
inline bool Serializer::readAll(Args&& ...args) {
#ifdef SAFETY_CHECKS
	return (this->read(args) && ...);
#else
	(this->read(args), ...);
	return true;
#endif // SAFETY_CHECKS
}

template<class T, size_t Size>
struct Serializable<std::array<T, Size>>
{
	static bool read(Serializer& serializer, std::array<T, Size>& vec) {
		size_t size;
		READ(size);

		assert(size == Size);
		if (size != Size) return false;

		for (size_t i = 0; i < size; i++) {
			READ(vec[i]);
		}
		return true;
	};

	static bool write(Serializer& serializer, std::array<T, Size> const& vec) {
		WRITE(Size);
		for (auto const& v : vec) {
			WRITE(v);
		}
		return true;
	}
};

template<size_t N>
struct Serializable<std::bitset<N>>
{
	static inline auto constexpr width = sizeof(unsigned long long);

	static bool read(Serializer& serializer, std::bitset<N>& b) {
		std::vector<uint64_t> parts;
		for (size_t n = 0; n < N; n += width) {
			uint64_t v;
			READ(v);
			parts.push_back(v);
		}
		std::bitset<N> bits;

		while (!parts.empty()) {
			auto v = parts.back();
			parts.pop_back();

			bits = static_cast<unsigned long long>(v);

			b <<= width * 8;
			b |= bits;
		}

		return true;
	};

	static bool write(Serializer& serializer, std::bitset<N> const& b_) {
		auto b = b_;
		std::bitset<N> ones = 0xFFFF'FFFF'FFFF'FFFFull;
		for (size_t n = 0; n < N; n += width) {
			unsigned long long p = (b & ones).to_ullong();
			b >>= width * 8;
			serializer.write(static_cast<uint64_t>(p));
		}
		return true;
	};
};

template<class T, size_t Size>
requires te::contains_v<simple_types, T>
struct Serializable<std::array<T, Size>>
{
	static bool read(Serializer& serializer, std::array<T, Size>& vec) {
		size_t size;
		READ(size);
		assert(Size == size);
		if (Size != size) return false;
		READBYTES(reinterpret_cast<char*>(vec.data()), vec.size() * sizeof(T));
		return true;
	};

	static bool write(Serializer& serializer, std::array<T, Size> const& vec) {
		WRITE(vec.size());
		WRITEBYTES(reinterpret_cast<char const*>(vec.data()), vec.size() * sizeof(T));
		return true;
	};
};

template<class T>
requires te::contains_v<simple_types, T>
struct Serializable<std::vector<T>>
{
	static bool read(Serializer& serializer, std::vector<T>& vec) {
		size_t size;
		READ(size);
		vec.resize(size);
		READBYTES(reinterpret_cast<char*>(vec.data()), vec.size() * sizeof(T));
		return true;
	};

	static bool write(Serializer& serializer, std::vector<T> const& vec) {
		WRITE(vec.size());
		WRITEBYTES(reinterpret_cast<char const*>(vec.data()), vec.size() * sizeof(T));
		return true;
	};
};

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
struct Serializable<std::unique_ptr<T>>
{
	static bool read(Serializer& serializer, std::unique_ptr<T>& ptr) {
		ptr = std::make_unique<T>();
		return serializer.read(*ptr);
	};

	static bool write(Serializer& serializer, std::unique_ptr<T> const& ptr) {
		assert(ptr.get() != nullptr);
		return serializer.write(*ptr);
	};
};

template<class T>
concept has_run = requires (T t, Serializer s) { Serializable<T>::template run<Read>(s, t); Serializable<T>::template run<Write>(s, t); };

template<class T>
inline bool Serializer::write(T const& val) {
	if constexpr (is_wrapped<T>) {
		//if constexpr (has_run<typename T::value_type>) {
		//	return Serializable<typename T::value_type>::template run<Write, typename T::value_type const&>(*this, val.val);
		//}
		//else {
		//	return Serializable<typename T::value_type>::write(*this, val.val);
		//}
		this->write(val.val);
		return true;
	}
	else {
		if constexpr (has_run<T>) {
			return Serializable<T>::template run<Write, T const&>(*this, val);
		}
		else {
			return Serializable<T>::write(*this, val);
		}
	}
}

template<class T>
inline bool Serializer::read(T& val) {
	if constexpr (is_wrapped<T>) {
		this->read(val.val);
		return true;
	}
	else {
		if constexpr (has_run<T>) {
			return Serializable<T>::template run<Read, T&>(*this, val);
		}
		else {
			return Serializable<T>::read(*this, val);
		}
	}
}