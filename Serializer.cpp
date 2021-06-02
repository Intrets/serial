#include "Serializer.h"

bool Serializer::writeBytes(char const* bytes, std::streamsize size) {
	assert(this->writeStream != nullptr);
	this->writeStream->write(bytes, size);
#ifdef SAFETY_CHECKS
	return this->writeStream->good();
#else
	return true;
#endif // SAFETY_CHECKS
}

bool Serializer::readBytes(char* target, std::streamsize size) {
	assert(this->readStream != nullptr);
	this->readStream->read(target, size);
#ifdef SAFETY_CHECKS
	return this->readStream->good();
#else
	return true;
#endif // SAFETY_CHECKS
}

Serializer::Serializer(std::ostream& writeStream_) : writeStream(&writeStream_) {
}

Serializer::Serializer(std::istream& readStream_) : readStream(&readStream_) {
}
