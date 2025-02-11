#pragma once
#include <mutex>
#include <cstdint>
#include <windows.h>

class CircularBuffer {
public:
	CircularBuffer(UINT32 bufferSize) : bufferSize(bufferSize), buffer(new char[bufferSize]) {}
	~CircularBuffer() {
		delete[] buffer;
	}

    // Write Data
    bool Write(const char* data, UINT32 size_) {}

    // Read Data
    bool Read(char* readData_, UINT32 size_) {}

	UINT32 DataSize() const {}

private:
	// 8 bytes
	char* buffer;
	const UINT32 bufferSize;
	UINT32 writePos = 0; // Current Write Position
	UINT32 readPos = 0;  // Current Read Position
	UINT32 currentSize = 0; // Current Buffer Size

    // 80 bytes
    std::mutex bufferMutex;
};