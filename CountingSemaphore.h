//#pragma once
//#include <mutex>
//#include <condition_variable>
//
//class CountingSemaphore {
//public:
//    CountingSemaphore(int count_) : count(count_) {}
//
//    void acquire() {
//        std::unique_lock<std::mutex> lock(mtx_);
//        cv_.wait(lock, [&]() { return count > 0; });
//        --count;
//    }
//
//    void release() {
//        std::unique_lock<std::mutex> lock(mtx_);
//        ++count;
//        cv_.notify_one();
//    }
//
//private:
//    std::mutex mtx_;
//    std::condition_variable cv_;
//    int count;
//};