#pragma once
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <thread>
#include <sw/redis++/redis++.h>

#include "Room.h"

class InGameUser;

class ChannelManager {
public:
	ChannelManager(std::shared_ptr<sw::redis::RedisCluster> redis_) {
		redis = redis_;
	}
	~ChannelManager() {
		channelUserCountSyncThreadRun = false;
		if (channelUserCountSyncThread.joinable()) {
			channelUserCountSyncThread.join();
		}
	}

	bool CreateChannelUserCountSyncThread();
	void ChannelUserCountSyncThread(); // 2�ʿ� �ѹ��� ä�� ���� �� �ҷ����� ������

private:
	// 80 bytes
	std::unordered_map<uint16_t, Room*> channelMap;

	// 16 byte
	std::shared_ptr<sw::redis::RedisCluster> redis;
	std::thread channelUserCountSyncThread;

	// 1 byte
	std::atomic<bool> channelUserCountSyncThreadRun = false;
};
