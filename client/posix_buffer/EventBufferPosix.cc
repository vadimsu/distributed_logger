#include "EventBufferPosix.hh"
#include <cstring>
#include <stdlib.h>
#include <iostream>

using namespace DistributedLogger;

EventBufferPosix::EventBufferPosix(size_t size){
        _readOffset = 0;
        _writeOffset = 0;
	_size = size;
	std::cout<<"allocating buf "<<size<<std::endl;
        _buffer = (char*)calloc(1,_size);
}

EventBufferPosix::~EventBufferPosix(){
        free(_buffer);
}

size_t EventBufferPosix::readData(char* data, size_t size){
        if (_size <= size + _readOffset){
                size = _size - _readOffset;
        }
        memcpy(data, _buffer + _readOffset, size);
        _readOffset += size;
        return size;
}

char *EventBufferPosix::getData(){
        return _buffer;
}

size_t EventBufferPosix::getCapacity(){
        return _size;
}

size_t EventBufferPosix::writeData(const char* data, size_t size){
	if (_size <= size + _writeOffset){
		size = _size - _writeOffset;
	}
	std::cout<<"writing  "<<size<<" at "<<_writeOffset<<std::endl;
        memcpy(_buffer + _writeOffset, data, size);
        _writeOffset += size;
        return size;
}

size_t EventBufferPosix::getReadOffset(){
        return _readOffset;
}

size_t EventBufferPosix::getWriteOffset(){
        return _writeOffset;
}

void EventBufferPosix::setReadOffset(size_t offset){
        _readOffset = offset;
}

void EventBufferPosix::setWriteOffset(size_t offset){
        _writeOffset = offset;
}
