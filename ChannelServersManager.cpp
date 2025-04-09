#include "ChannelServersManager.h"

bool ChannelServersManager::init() {
	channels.resize(7); // ä�μ� + 1
	for (auto& c : channels) c.store(0);
	return true;
}

void ChannelServersManager::EnterChannelServer(uint16_t channelNum_) {
	if (!channels[channelNum_].fetch_add(1) + 1 < 51) { // 1�� �����Ѱ� ���� �ο� �ʰ��� -1
		channels[channelNum_].fetch_sub(1);
	}
}

void ChannelServersManager::LeaveChannelServer(uint16_t channelNum_) {
	channels[channelNum_].fetch_sub(1);
}

std::vector<std::atomic<uint16_t>> ChannelServersManager::GetChannels() {
	return channels;
}