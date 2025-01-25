#pragma once
#include <mutex>

class CircularBuffer {
public:
	CircularBuffer(size_t bufferSize) : bufferSize(bufferSize), buffer(new char[bufferSize]) {}
	~CircularBuffer() {
		delete[] buffer;
	}

    // Write Data
    bool Write(const char* data, size_t size_) {}

    // Read Data
    bool Read(char* readData_, size_t size_) {}

    size_t DataSize() const {}

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