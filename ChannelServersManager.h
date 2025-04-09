#pragma once
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <thread>

#include "Define.h"

class ChannelServersManager{
public:
	bool init();
	void EnterChannelServer(uint16_t channelNum_); // ���� �ش� ���� ����
	void LeaveChannelServer(uint16_t channelNum_); // ���� �ش� ���� ���� or �ش� ���� ���� ���� ����
	std::vector<std::atomic<uint16_t>> GetChannels(); // ä�� �ο� ���� ��ȯ�ϴ� �Լ�

private:
	std::vector<std::atomic<uint16_t>> channels; // �� ä���� �ο� ���� �����ϴ� vector
};
