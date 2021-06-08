#include "Serializer.h"

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

Serializer::Serializer(std::ostream& writeStream_) : writeStream(&writeStream_) {
}

Serializer::Serializer(std::istream& readStream_) : readStream(&readStream_) {
}
