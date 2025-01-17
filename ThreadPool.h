#pragma once
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <functional>

class ThreadPool {
public:
	ThreadPool() {}
	~ThreadPool() {}
public:
	void ProcTask() {
		while (true) {
			std::function<void()> task;

			{ // queue에서 함수를 가져오는 코드 블록
				std::unique_lock<std::mutex> lock(queue_mu);

				if (stop && work_queue.empty()) return; // stop 신호가 ture고 큐가 비어있으면 계산이 끝난것임으로 쓰레드 종료

				while (1) { // stop 신호나 큐가 들어올때까지 conditional_wait 실행
					if (!work_queue.empty()) break; // 큐가 들어오면 task 실행을 위해 break
					if (stop && work_queue.empty()) return; // 쓰레드 종료
				}

				task = work_queue.front();
				work_queue.pop(); // queue에서 작업을 가져온 후 pop
			}

			task(); // queue에서 가져온 함수 실행

		}
	}

	void Total(int N_, int M_) {
		Pushqueue(); // N을 N/threadcnt만큼 나눈 범위의 값 계산하는 function 함수 queue에 삽입

		for (int i = 0; i < threadcnt; i++) {
			threadpool.emplace_back(&ThreadPool::ProcTask, this);
		}
	}

private:
	bool stop = 0;
	int threadcnt = 0;

	std::mutex queue_mu;
	std::vector<std::thread> threadpool;
	std::queue<std::function<void()>> work_queue;

	void Pushqueue() {
		for (int i = 0; i < ; i++) {
		}
	}
};