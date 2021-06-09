#include "Serializer.h"

std::string Serializer::getIndendation() const {
	if (this->spaces.has_value()) {
		return std::string(this->indentationLevel * this->spaces.value(), ' ');
	}
	else {
		return std::string(this->indentationLevel, '\t');
	}
}

bool Serializer::writeBytes(char const* bytes, std::streamsize size) {
	assert(this->writeStream != nullptr);
	this->writeStream->write(bytes, size);
	return this->writeStream->good();
}

bool Serializer::readBytes(char* target, std::streamsize size) {
	assert(this->readStream != nullptr);
	this->readStream->read(target, size);
	return this->readStream->good();
}

bool Serializer::printIndentedString(std::string const& str) {
	*this->writeStream << '\n' << this->getIndendation() << str;
	return true;
}

bool Serializer::printString(std::string_view sv) {
	*(this->writeStream) << sv;
	return true;
}

Serializer::Serializer(std::ostream& writeStream_) : writeStream(&writeStream_) {
}

Serializer::Serializer(std::istream& readStream_) : readStream(&readStream_) {
}
