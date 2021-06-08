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

#define PRINT_DEF(X) static bool run(Print, Serializer& serializer, X&& obj)
#define ALL(X) Wrapped{ obj.X, #X }

template<class T>
struct Serializable;

struct Write {};
struct Read {};
struct Print {};

template<class T>
struct Wrapped
{
	using value_type = std::remove_cvref_t<T>;
	T& val;
	std::string_view name;
};


template<class T>
struct is_wrapped_ : std::false_type {};

template<class T>
struct is_wrapped_<Wrapped<T>> : std::true_type {};

template<class T>
concept is_wrapped = is_wrapped_<T>::value;

struct Serializer
{
	std::istream* readStream{ nullptr };
	std::ostream* writeStream{ nullptr };

	bool writeBytes(char const* bytes, std::streamsize size);
	bool readBytes(char* target, std::streamsize size);


	template<class T>
	bool write(T&& val);

	template<class T>
	bool read(T&& val);


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
	static bool run(Read, Serializer& serializer, std::string&& val) {
		size_t size;
		READ(size);
		val.resize(size);
		return serializer.readBytes(&val[0], size);
	};

	static bool run(Write, Serializer& serializer, std::string&& val) {
		WRITE(val.size());
		return serializer.writeBytes(val.c_str(), val.size());
	}

	static bool run(Print, Serializer& serializer, std::string&& val) {
		return true;
	}
};


template<class T>
struct Serializable<std::optional<T>>
{
	static bool run(Read, Serializer& serializer, std::optional<T>&& val) {
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

	static bool run(Write, Serializer& serializer, std::optional<T>&& val) {
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
	static bool run(Read, Serializer& serializer, T&& val) {
		return serializer.readBytes(reinterpret_cast<char*>(&val), sizeof(val));
	};

	static bool run(Write, Serializer& serializer, T&& val) {
		return serializer.writeBytes(reinterpret_cast<char const*>(&val), sizeof(val));
	}
};

template<class T>
requires (std::is_same_v<size_t, T> && !std::is_same_v<size_t, uint64_t>)
struct Serializable<T>
{
	static bool run(Read, Serializer& serializer, T&& val) {
		uint64_t v;
		READ(v);
		val = static_cast<size_t>(v);
		return true;
	};

	static bool run(Write, Serializer& serializer, T&& val) {
		uint64_t v = static_cast<uint64_t>(val);
		return serializer.write(v);
	}
};

template<>
struct Serializable<bool>
{
	static bool run(Read, Serializer& serializer, bool&& val) {
		int8_t v;
		READ(v);
		val = static_cast<bool>(v);
		return true;
	};

	static bool run(Write, Serializer& serializer, bool&& val) {
		return serializer.write(static_cast<int8_t>(val));
	}
};

template<>
struct Serializable<float>
{
	static bool run(Read, Serializer& serializer, float&& val) {
		std::string s;
		READ(s);
		char* end;
		val = std::strtof(s.c_str(), &end);
		assert(end != s.c_str());
		return (end != s.c_str());
	}

	static bool run(Write, Serializer& serializer, float&& val) {
		return serializer.write(std::to_string(val));
	}
};

template<>
struct Serializable<glm::vec2>
{
	template<class Selector>
	static bool run(Selector, Serializer& serializer, glm::vec2&& val) {
		return serializer.runAll<Selector>(val.x, val.y);
	};
};

template<>
struct Serializable<glm::ivec2>
{
	static bool run(Read, Serializer& serializer, glm::ivec2&& val) {
		int32_t x;
		READ(x);
		val.x = x;
		int32_t y;
		READ(y);
		val.y = y;
		return true;
	}

	static bool run(Write, Serializer& serializer, glm::ivec2&& val) {
		return
			serializer.write(static_cast<int32_t>(val.x)) &&
			serializer.write(static_cast<int32_t>(val.y));
	}
};

template<class... Args>
inline bool Serializer::writeAll(Args&&... args) {
	return (this->write(args) && ...);
}

template<class Selector, class... Args>
inline bool Serializer::runAll(Args&& ...args) {
	if constexpr (std::is_same_v<Selector, Read>) {
		return this->readAll(std::forward<Args>(args)...);
	}
	else if constexpr (std::is_same_v<Selector, Write>) {
		return this->writeAll(std::forward<Args>(args)...);
	}
	else if constexpr (std::is_same_v<Selector, Print>) {
		return true;
	}
	else {
		static_assert(0);
	}
}

template<class... Args>
inline bool Serializer::readAll(Args&& ...args) {
	return (this->read(args) && ...);
}

template<class T, size_t Size>
struct Serializable<std::array<T, Size>>
{
	static bool run(Read, Serializer& serializer, std::array<T, Size>&& vec) {
		size_t size;
		READ(size);

		assert(size == Size);
		if (size != Size) return false;

		for (size_t i = 0; i < size; i++) {
			READ(vec[i]);
		}
		return true;
	};

	static bool run(Write, Serializer& serializer, std::array<T, Size>&& vec) {
		WRITE(Size);
		for (auto& v : vec) {
			WRITE(v);
		}
		return true;
	}
};

template<size_t N>
struct Serializable<std::bitset<N>>
{
	static inline auto constexpr width = sizeof(unsigned long long);

	static bool run(Read, Serializer& serializer, std::bitset<N>&& b) {
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

	static bool run(Write, Serializer& serializer, std::bitset<N>&& b_) {
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
	static bool run(Read, Serializer& serializer, std::array<T, Size>&& vec) {
		size_t size;
		READ(size);
		assert(Size == size);
		if (Size != size) return false;
		READBYTES(reinterpret_cast<char*>(vec.data()), vec.size() * sizeof(T));
		return true;
	};

	static bool run(Write, Serializer& serializer, std::array<T, Size>&& vec) {
		WRITE(vec.size());
		WRITEBYTES(reinterpret_cast<char *>(vec.data()), vec.size() * sizeof(T));
		return true;
	};
};

template<class T>
requires te::contains_v<simple_types, T>
struct Serializable<std::vector<T>>
{
	static bool run(Read, Serializer& serializer, std::vector<T>&& vec) {
		size_t size;
		READ(size);
		vec.resize(size);
		READBYTES(reinterpret_cast<char*>(vec.data()), vec.size() * sizeof(T));
		return true;
	};

	static bool run(Write, Serializer& serializer, std::vector<T>&& vec) {
		WRITE(vec.size());
		WRITEBYTES(reinterpret_cast<char *>(vec.data()), vec.size() * sizeof(T));
		return true;
	};
};

template<class T>
struct Serializable<std::vector<T>>
{
	static bool run(Read, Serializer& serializer, std::vector<T>&& vec) {
		size_t size;
		READ(size);
		vec.resize(size);
		for (size_t i = 0; i < size; i++) {
			READ(vec[i]);
		}
		return true;
	};

	static bool run(Write, Serializer& serializer, std::vector<T>&& vec) {
		WRITE(vec.size());
		for (auto& v : vec) {
			WRITE(v);
		}
		return true;
	}
};

template<class T>
struct Serializable<std::unique_ptr<T>>
{
	static bool run(Read, Serializer& serializer, std::unique_ptr<T>&& ptr) {
		ptr = std::make_unique<T>();
		return serializer.read(*ptr);
	};

	static bool run(Write, Serializer& serializer, std::unique_ptr<T>&& ptr) {
		assert(ptr.get() != nullptr);
		return serializer.write(*ptr);
	};
};

template<class T>
inline bool Serializer::write(T&& val) {
	using W = std::remove_cvref_t<T>;
	if constexpr (is_wrapped<W>) {
		return this->write(val.val);
	}
	else {
		return Serializable<W>::run(Write{}, *this, std::forward<W>(val));
	}
}

template<class T>
inline bool Serializer::read(T&& val) {
	//Serializable<std::string>::run(Read{}, *this, "123");
	using W = std::remove_cvref_t<T>;
	if constexpr (is_wrapped<W>) {
		return this->read(val.val);
	}
	else {
		return Serializable<W>::run(Read{}, *this, std::forward<W>(val));
	}
}