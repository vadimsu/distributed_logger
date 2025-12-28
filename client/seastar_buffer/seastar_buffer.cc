
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include "seastar_buffer.hh"

using namespace DistributedLogger;

EventBufferSeastar::EventBufferSeastar(size_t size){
        _readOffset = 0;
        _writeOffset = 0;
	std::cout<<"allocating buf "<<size<<std::endl;
        _buffer = seastar::temporary_buffer<char>(size);
}

EventBufferSeastar::~EventBufferSeastar(){
}

size_t EventBufferSeastar::readData(char* data, size_t size){
        if (_buffer.size() <= size + _readOffset){
                size = _buffer.size() - _readOffset;
        }
        memcpy(data, _buffer.get() + _readOffset, size);
        _readOffset += size;
        return size;
}

char *EventBufferSeastar::getData(){
        return _buffer.get_write();
}

size_t EventBufferSeastar::getCapacity(){
        return _buffer.size();
}

size_t EventBufferSeastar::writeData(const char* data, size_t size){
	if (_buffer.size() <= size + _writeOffset){
		size = _buffer.size() - _writeOffset;
	}
	std::cout<<"writing  "<<size<<" at "<<_writeOffset<<std::endl;
        memcpy(_buffer.get_write() + _writeOffset, data, size);
        _writeOffset += size;
        return size;
}

size_t EventBufferSeastar::getReadOffset(){
        return _readOffset;
}

size_t EventBufferSeastar::getWriteOffset(){
        return _writeOffset;
}

void EventBufferSeastar::setReadOffset(size_t offset){
        _readOffset = offset;
}

void EventBufferSeastar::setWriteOffset(size_t offset){
        _writeOffset = offset;
}

seastar::temporary_buffer<char> EventBufferSeastar::getBuffer(){
	return std::move(_buffer);
}
