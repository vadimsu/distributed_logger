#include "EventBuffer.hh"
#include <cstring>
#include <stdlib.h>

EventBuffer::EventBuffer(size_t size){
        _readOffset = 0;
        _writeOffset = 10;//to comply with the protocol where first 6 bytes are 2 for type and 4 for length
        _size = size + 10;
        _buffer = (char*)calloc(1,_size);
}

EventBuffer::~EventBuffer(){
        free(_buffer);
}

size_t EventBuffer::readData(char* data, size_t size){
        if (_size <= size + _readOffset){
                size = _size - _readOffset;
        }
        memcpy(data, _buffer + _readOffset, size);
        _readOffset += size;
        return size;
}

char *EventBuffer::getData(){
        return _buffer;
}

size_t EventBuffer::getCapacity(){
        return _size;
}

size_t EventBuffer::writeData(const char* data, size_t size){
        if (_size <= size + _writeOffset){
                size = _size - _writeOffset;
        }
        memcpy(_buffer + _writeOffset, data, size);
        _writeOffset += size;
        return size;
}

size_t EventBuffer::getReadOffset(){
        return _readOffset;
}

size_t EventBuffer::getWriteOffset(){
        return _writeOffset;
}

void EventBuffer::setReadOffset(size_t offset){
        _readOffset = offset;
}

void EventBuffer::setWriteOffset(size_t offset){
        _writeOffset = offset;
}
