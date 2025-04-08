#include "ChannelManager.h"

bool ChannelManager::init() {
	channelVector.resize(7); // ä�μ� + 1
	for (auto& c : channelVector) c.store(0);
	return true;
}

void ChannelManager::EnterChannelServer(uint16_t channelNum_) {
	if (!channelVector[channelNum_].fetch_add(1) + 1 < 51) { // 1�� �����Ѱ� ���� �ο� �ʰ��� -1
		channelVector[channelNum_].fetch_sub(1);
	}
}

void ChannelManager::LeaveChannelServer(uint16_t channelNum_) {
	channelVector[channelNum_].fetch_sub(1);
}

std::vector<std::atomic<uint16_t>> ChannelManager::getChannelVector() {
	return channelVector;
}