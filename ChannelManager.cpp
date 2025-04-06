#include "ChannelManager.h"

bool ChannelManager::CreateChannelUserCountSyncThread() {
	channelUserCountSyncThreadRun = true;
	channelUserCountSyncThread = std::thread([this]() { ChannelUserCountSyncThread(); });
	return true;
}

void ChannelManager::ChannelUserCountSyncThread() {
	while (channelUserCountSyncThreadRun) {

		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}