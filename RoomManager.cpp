#include "RoomManager.h"

void RoomManager::Init() {
    for (int i = 1; i <= 6; i++ ) { // Max i = MaxLevel/3 + 1 (Level Check Set)
        std::priority_queue<MatchingRoom*> k;
        matchMap.emplace(i, std::priority_queue<MatchingRoom*>());
    }

    for (int i = 0; i < 100; i++) { // (Max Room Set)
        roomNumQueue.push(i);
    }

    CreateMatchThread();
    TimeCheckThread();
}

bool RoomManager::Insert(uint8_t userLevel_, UINT16 userSkt_) {
    tbb::concurrent_hash_map<uint8_t, std::priority_queue<MatchingRoom*>>::accessor accessor;
    MatchingRoom* tempRoom;
    tempRoom->userSkt = userSkt_;

    if (matchMap.find(accessor, userLevel_/3 + 1)) { // Insert Success
        accessor->second.push(tempRoom);
        return true;
    }

    // Match Queue Full || Insert Fail
    return false;
}

bool RoomManager::CreateMatchThread() {
	matchRun = true;
    matchThread = std::thread([this]() {MatchThread(); });
    std::cout << "MatchThread 시작" << std::endl;
    return true;
}

bool RoomManager::CreatTimeCheckThread() {
    timeChekcRun = true;
    timeCheckThread = std::thread([this]() {TimeCheckThread(); });
    std::cout << "TimeCheckThread 시작" << std::endl;
    return true;
}

void RoomManager::MatchThread() {
    while (matchRun) {
        for (int i = 1; i <= 6; i++) {

        }
    }
}

void RoomManager::TimeCheckThread() {
    while (timeChekcRun) {
        if (!rtCheckQueue.empty()) {
            if (rtCheckQueue.top().endTime <= std::chrono::steady_clock::now()) {

            }
            else {

            }
        } 
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}