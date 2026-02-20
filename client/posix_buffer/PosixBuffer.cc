#include "PosixBuffer.hh"
#include <cstring>
#include <stdlib.h>
#include <iostream>

using namespace distributed_logger;

PosixBuffer::PosixBuffer(size_t size){
        _readOffset = 0;
        _writeOffset = 0;
	_size = size;
        _buffer = (char*)calloc(1,_size);
}

PosixBuffer::~PosixBuffer(){
        free(_buffer);
}

size_t PosixBuffer::readData(char* data, size_t size){
        if (_size <= size + _readOffset){
                size = _size - _readOffset;
        }
        memcpy(data, _buffer + _readOffset, size);
        _readOffset += size;
        return size;
}

char *PosixBuffer::getData(){
        return _buffer;
}

size_t PosixBuffer::getCapacity(){
        return _size;
}

size_t PosixBuffer::writeData(const char* data, size_t size){
	if (_size <= size + _writeOffset){
		size = _size - _writeOffset;
	}
        memcpy(_buffer + _writeOffset, data, size);
        _writeOffset += size;
        return size;
}

size_t PosixBuffer::getReadOffset(){
        return _readOffset;
}

size_t PosixBuffer::getWriteOffset(){
        return _writeOffset;
}

void PosixBuffer::setReadOffset(size_t offset){
        _readOffset = offset;
}

void PosixBuffer::setWriteOffset(size_t offset){
        _writeOffset = offset;
}
