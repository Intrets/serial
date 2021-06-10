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
#include <iostream>
#include <format>
#include <bit>

#include <tepp/tepp.h>

#include "ByteSwap.h"

#define READ(X) if (!serializer.read(X)) return false
#define WRITE(X) if (!serializer.write(X)) return false
#define PRINT(X) if (!serializer.print(X)) return false

#define PRINT_DEF(X) static bool run(Print, Serializer& serializer, X&& obj)
#define PRINT_DEF2(X, Y) static bool run(Print, Serializer& serializer, X,Y&& obj)
#define READ_DEF(X) static bool run(Read, Serializer& serializer, X&& obj)
#define READ_DEF2(X, Y) static bool run(Read, Serializer& serializer, X,Y&& obj)
#define WRITE_DEF(X) static bool run(Write, Serializer& serializer, X&& obj)
#define WRITE_DEF2(X, Y) static bool run(Write, Serializer& serializer, X,Y&& obj)
#define ALL_DEF(X) template<class Selector> static bool run(Selector, Serializer& serializer, X&& obj)
#define ALL(X) Wrapped{ obj.X, #X }

namespace serial
{
	static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);
	constexpr auto targetEndianness = std::endian::big;

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

		int32_t indentationLevel = 0;
		int32_t repeatLevel = 3;
		bool lastValueSimple = true;
		std::optional<int32_t> spaces = 4;

		std::string getIndendation() const;

		bool writeBytes(char const* bytes, std::streamsize size);
		bool readBytes(char* target, std::streamsize size);


		template<class T>
		bool write(T&& val);

		template<class T>
		bool read(T&& val);

		template<class T>
		bool print(T&& val);

		bool printIndentedString(std::string const& str);

		bool printString(std::string_view sv);

		template<class T>
		bool printSimpleValue(T&& val);


		template<class... Args>
		bool readAll(Args&&... args);

		template<class... Args>
		bool writeAll(Args&&... args);

		template<class... Args>
		bool printAll(Args&&... args);

		template<class Selector, class... Args>
		bool runAll(Args&&... args);

		Serializer(std::ostream& writeStream_);
		Serializer(std::istream& readStream_);
		Serializer() = default;
		~Serializer() = default;
	};

	template<class T>
	concept has_print = requires (T t, Serializer s) {
		Serializable<T>::run(Print{}, s, std::forward<T>(t));
	};

	template<class T>
	concept has_type_name = requires() { Serializable<T>::typeName; };

	template<class T>
	static std::string_view getName() {
		if constexpr (has_type_name<T>) {
			return Serializable<T>::typeName;
		}
		else {
			return "TYPE";
		}
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
			return serializer.printSimpleValue(std::forward<std::string>(val));
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
	constexpr char* simpleTypeName;

	template<>
	constexpr auto simpleTypeName<int8_t> = "int8";
	template<>
	constexpr auto simpleTypeName<uint8_t> = "uint8";
	template<>
	constexpr auto simpleTypeName<int16_t> = "int16";
	template<>
	constexpr auto simpleTypeName<uint16_t> = "uint16";
	template<>
	constexpr auto simpleTypeName<int32_t> = "int32";
	template<>
	constexpr auto simpleTypeName<uint32_t> = "uint32";
	template<>
	constexpr auto simpleTypeName<int64_t> = "int64";
	template<>
	constexpr auto simpleTypeName<uint64_t> = "uint64";
	template<>
	constexpr auto simpleTypeName<std::byte> = "byte";

	template<class T>
	requires te::contains_v<simple_types, T>
		struct Serializable<T>
	{
		inline static const auto typeName = simpleTypeName<T>;

		READ_DEF(T) {
			if constexpr (targetEndianness == std::endian::native) {
				return serializer.readBytes(reinterpret_cast<char*>(&obj), sizeof(obj));
			}
			else {
				if (!serializer.readBytes(reinterpret_cast<char*>(&obj), sizeof(obj))) {
					return false;
				}

				obj = byteSwap(obj);
				return true;
			}
		};

		WRITE_DEF(T) {
			if constexpr (targetEndianness == std::endian::native) {
				return serializer.writeBytes(reinterpret_cast<char const*>(&obj), sizeof(obj));
			}
			else {
				T obj2 = byteSwap(obj);

				return serializer.writeBytes(reinterpret_cast<char const*>(&obj2), sizeof(obj2));
			}
		}

		PRINT_DEF(T) {
			return serializer.printSimpleValue(std::forward<T>(obj));
		}
	};

	template<class T>
	requires (std::is_same_v<size_t, T> && !std::is_same_v<size_t, uint64_t>)
		struct Serializable<T>
	{
		READ_DEF(T) {
			uint64_t v;
			READ(v);
			obj = static_cast<size_t>(v);
			return true;
		};

		WRITE_DEF(T) {
			uint64_t v = static_cast<uint64_t>(obj);
			return serializer.write(v);
		}

		PRINT_DEF(T) {
			return serializer.printSimpleValue(std::forward<T>(obj));
		}
	};

	template<>
	struct Serializable<bool>
	{
		READ_DEF(bool) {
			int8_t v;
			READ(v);
			obj = static_cast<bool>(v);
			return true;
		};

		WRITE_DEF(bool) {
			return serializer.write(static_cast<int8_t>(obj));
		}
	};

	template<>
	struct Serializable<float>
	{
		READ_DEF(float) {
			std::string s;
			READ(s);
			char* end;
			obj = std::strtof(s.c_str(), &end);
			assert(end != s.c_str());
			return (end != s.c_str());
		}

		WRITE_DEF(float) {
			return serializer.write(std::to_string(obj));
		}
	};

	template<>
	struct Serializable<glm::vec2>
	{
		ALL_DEF(glm::vec2) {
			return serializer.runAll<Selector>(
				ALL(x),
				ALL(y)
				);
		}
	};

	template<>
	struct Serializable<glm::ivec2>
	{
		READ_DEF(glm::ivec2) {
			int32_t x;
			READ(x);
			obj.x = x;
			int32_t y;
			READ(y);
			obj.y = y;
			return true;
		}

		WRITE_DEF(glm::ivec2) {
			return
				serializer.write(static_cast<int32_t>(obj.x)) &&
				serializer.write(static_cast<int32_t>(obj.y));
		}
	};

	template<class... Args>
	inline bool Serializer::writeAll(Args&&... args) {
		return (this->write(args) && ...);
	}

	template<class... Args>
	inline bool Serializer::printAll(Args&&... args) {
		this->printString("{");
		this->indentationLevel++;
		auto b = (this->print(args) && ...);
		this->indentationLevel--;
		this->printIndentedString("}");
		return b;
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
			return this->printAll(std::forward<Args>(args)...);
		}
		else {
			static_assert(0);
		}
	}

	template<class T>
	inline bool Serializer::printSimpleValue(T&& val) {
		this->lastValueSimple = true;
		if constexpr (std::is_same_v<T, uint8_t>) {
			*this->writeStream << static_cast<int>(val);
		}
		else if constexpr (std::integral<T>) {
			*this->writeStream << val;
		}
		else if constexpr (std::is_same_v<T, std::byte>) {
			*this->writeStream << "0x" << std::hex << static_cast<unsigned>(val) << std::dec;
		}
		else if constexpr (std::is_same_v<T, std::string>) {
			*this->writeStream << '\"' << val << '\"';
		}
		else {
			*this->writeStream << " unknown " << val;
		}
		return true;
	}

	template<class... Args>
	inline bool Serializer::readAll(Args&& ...args) {
		return (this->read(args) && ...);
	}

	template<class T>
	static bool arrayPrint(Serializer& serializer, T&& obj) {
		int32_t i = 0;
		int32_t end = std::min(static_cast<int32_t>(obj.size()), serializer.repeatLevel);
		if (end > 0) {
			serializer.indentationLevel++;
			serializer.printIndentedString("");

			PRINT(obj[i]);
			i++;
			for (; i < end; i++) {
				auto& v = obj[i];
				if (serializer.lastValueSimple) {
					serializer.printString(", ");
				}
				else {
					serializer.printIndentedString(", ");
				}
				PRINT(v);
			}

			serializer.indentationLevel--;
		}

		if (serializer.repeatLevel < obj.size()) {
			serializer.printString(std::format(" and {} more...", obj.size() - serializer.repeatLevel));
		}
		serializer.printIndentedString("}");
		return true;
	}

	template<class T, size_t Size>
	struct Serializable<std::array<T, Size>>
	{
		inline static const auto typeName = "array";

		READ_DEF2(std::array<T, Size>) {
			size_t size;
			READ(size);

			assert(size == Size);
			if (size != Size) return false;

			for (size_t i = 0; i < size; i++) {
				READ(obj[i]);
			}
			return true;
		};

		WRITE_DEF2(std::array<T, Size>) {
			WRITE(Size);
			for (auto& v : obj) {
				WRITE(v);
			}
			return true;
		}

		PRINT_DEF2(std::array<T, Size>) {
			serializer.printString(std::format("array<{}, {}> {{ ", getName<T>(), Size));
			return arrayPrint(serializer, obj);
		}
	};

	template<size_t N>
	struct Serializable<std::bitset<N>>
	{
		static inline auto constexpr width = sizeof(unsigned long long);
		inline static const auto typeName = std::format("bitset<{}>", N);

		READ_DEF(std::bitset<N>) {
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

				obj <<= width * 8;
				obj |= bits;
			}

			return true;
		};

		WRITE_DEF(std::bitset<N>) {
			auto b = obj;
			std::bitset<N> ones = 0xFFFF'FFFF'FFFF'FFFFull;
			for (size_t n = 0; n < N; n += width) {
				unsigned long long p = (b & ones).to_ullong();
				b >>= width * 8;
				serializer.write(static_cast<uint64_t>(p));
			}
			return true;
		};

		PRINT_DEF(std::bitset<N>) {
			serializer.printString(std::format("bitset<{}> 0b{}", N, obj.to_string()));
			return true;
		}
	};

	template<class T>
	struct Serializable<std::vector<T>>
	{
		inline static const auto typeName = "vector";

		READ_DEF(std::vector<T>) {
			size_t size;
			READ(size);
			obj.resize(size);
			for (size_t i = 0; i < size; i++) {
				READ(obj[i]);
			}
			return true;
		};

		WRITE_DEF(std::vector<T>) {
			WRITE(obj.size());
			for (auto& v : obj) {
				WRITE(v);
			}
			return true;
		}

		PRINT_DEF(std::vector<T>) {
			serializer.printString(std::format("vector<{}> {{ ", getName<T>()));
			return arrayPrint(serializer, obj);
		}
	};

	template<class T>
	struct Serializable<std::unique_ptr<T>>
	{
		READ_DEF(std::unique_ptr<T>) {
			obj = std::make_unique<T>();
			return serializer.read(*obj);
		};

		WRITE_DEF(std::unique_ptr<T>) {
			assert(obj.get() != nullptr);
			return serializer.write(*obj);
		};

		PRINT_DEF(std::unique_ptr<T>) {
			serializer.printString(std::format("unique_ptr<{}>", getName<T>()));
			return serializer.print(*obj);
		}
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
		using W = std::remove_cvref_t<T>;
		if constexpr (is_wrapped<W>) {
			return this->read(val.val);
		}
		else {
			return Serializable<W>::run(Read{}, *this, std::forward<W>(val));
		}
	}

	template<class T>
	inline bool Serializer::print(T&& val) {
		using W = std::remove_cvref_t<T>;
		if constexpr (is_wrapped<W>) {
			this->printString(std::format("\n{}{} {} ", this->getIndendation(), val.name, getName<W::value_type>()));
			auto b = this->print(val.val);
			this->lastValueSimple = false;
			return b;
		}
		else if constexpr (has_print<W>) {
			return Serializable<W>::run(Print{}, *this, std::forward<W>(val));
		}
		else {
			assert(0);
			return false;
		}
	}
}
