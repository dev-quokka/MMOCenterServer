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

			{ // queue���� �Լ��� �������� �ڵ� ���
				std::unique_lock<std::mutex> lock(queue_mu);

				if (stop && work_queue.empty()) return; // stop ��ȣ�� ture�� ť�� ��������� ����� ������������ ������ ����

				while (1) { // stop ��ȣ�� ť�� ���ö����� conditional_wait ����
					if (!work_queue.empty()) break; // ť�� ������ task ������ ���� break
					if (stop && work_queue.empty()) return; // ������ ����
				}

				task = work_queue.front();
				work_queue.pop(); // queue���� �۾��� ������ �� pop
			}

			task(); // queue���� ������ �Լ� ����

		}
	}

	void Total(int N_, int M_) {
		Pushqueue(); // N�� N/threadcnt��ŭ ���� ������ �� ����ϴ� function �Լ� queue�� ����

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