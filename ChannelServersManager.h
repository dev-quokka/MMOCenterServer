#pragma once
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <thread>

#include "ChannelServer.h"

class ChannelServersManager{
public:
	bool init();
	bool EnterChannelServer(uint16_t channelNum_); // ���� �ش� ���� ����
	void LeaveChannelServer(uint16_t channelNum_); // ���� �ش� ���� ���� or �ش� ���� ���� ���� ����
	std::vector<uint16_t> GetServerCounts(); // ä�� �ο� ���� ��ȯ�ϴ� �Լ�

private:
	std::vector<ChannelServer*> servers;
};
