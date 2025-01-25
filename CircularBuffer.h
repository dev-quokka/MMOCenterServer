#pragma once
#include <mutex>

class CircularBuffer {
public:
	CircularBuffer(size_t bufferSize) : bufferSize(bufferSize), buffer(new char[bufferSize]) {}
	~CircularBuffer() {
		delete[] buffer;
	}

    // Write Data
    bool Write(const char* data, size_t size) {
        std::lock_guard<std::mutex> guard(bufferMutex);
        
        if ((bufferSize - currentSize) < size) { // Empty Space
            return false;
        }

        size_t endSpace = bufferSize - writePos;
        if (endSpace >= size) {
            std::memcpy(buffer + writePos, data, size);
        }
        else {
            std::memcpy(buffer + writePos, data, endSpace);
            std::memcpy(buffer, data + endSpace, size - endSpace);
        }

        writePos = (writePos + size) % bufferSize;
        currentSize += size;

        return true;
    }

    // Read Data
    bool Read(char* dest, size_t size) {
        std::lock_guard<std::mutex> guard(bufferMutex);

        if (currentSize < size) {
            return false;
        }

        size_t endSpace = bufferSize - readPos;
        if (endSpace >= size) {
            std::memcpy(dest, buffer + readPos, size);
        }
        else {
            std::memcpy(dest, buffer + readPos, endSpace);
            std::memcpy(dest + endSpace, buffer, size - endSpace);
        }

        readPos = (readPos + size) % bufferSize;
        currentSize -= size;

        return true;
    }

    size_t DataSize() const {
        return currentSize;
    }

private:
	// 8 bytes
	char* buffer;
	const size_t bufferSize;
	size_t writePos = 0; // Current Write Position
	size_t readPos = 0;  // Current Read Position
	size_t currentSize = 0; // Current Buffer Size

    // 80 bytes
    std::mutex bufferMutex;
};