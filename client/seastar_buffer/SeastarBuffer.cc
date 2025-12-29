
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include "SeastarBuffer.hh"

using namespace distributed_logger;

SeastarBuffer::SeastarBuffer(size_t size){
        _readOffset = 0;
        _writeOffset = 0;
	std::cout<<"allocating buf "<<size<<std::endl;
        _buffer = seastar::temporary_buffer<char>(size);
}

SeastarBuffer::~SeastarBuffer(){
}

size_t SeastarBuffer::readData(char* data, size_t size){
        if (_buffer.size() <= size + _readOffset){
                size = _buffer.size() - _readOffset;
        }
        memcpy(data, _buffer.get() + _readOffset, size);
        _readOffset += size;
        return size;
}

char *SeastarBuffer::getData(){
        return _buffer.get_write();
}

size_t SeastarBuffer::getCapacity(){
        return _buffer.size();
}

size_t SeastarBuffer::writeData(const char* data, size_t size){
	if (_buffer.size() <= size + _writeOffset){
		size = _buffer.size() - _writeOffset;
	}
	std::cout<<"writing  "<<size<<" at "<<_writeOffset<<std::endl;
        memcpy(_buffer.get_write() + _writeOffset, data, size);
        _writeOffset += size;
        return size;
}

size_t SeastarBuffer::getReadOffset(){
        return _readOffset;
}

size_t SeastarBuffer::getWriteOffset(){
        return _writeOffset;
}

void SeastarBuffer::setReadOffset(size_t offset){
        _readOffset = offset;
}

void SeastarBuffer::setWriteOffset(size_t offset){
        _writeOffset = offset;
}

seastar::temporary_buffer<char> SeastarBuffer::getBuffer(){
	return std::move(_buffer);
}
