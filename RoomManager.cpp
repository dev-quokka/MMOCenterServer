#include "RoomManager.h"

void RoomManager::Init() {
    for (int i = 1; i <= 6; i++ ) { // Max i = MaxLevel/3 + 1 (Level Check Set)
        std::priority_queue<MatchingRoom*> k;
        matchingMap.emplace(i, std::priority_queue<MatchingRoom*>());
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

    if (matchingMap.find(accessor, userLevel_/3 + 1)) { // Insert Success
        accessor->second.push(tempRoom);
        return true;
    }

    // Match Queue Full || Insert Fail
    return false;
}

bool RoomManager::CreateMatchThread() {
	matchRun = true;
    matchingThread = std::thread([this]() {MatchingThread(); });
    std::cout << "MatchThread 시작" << std::endl;
    return true;
}

bool RoomManager::CreatTimeCheckThread() {
    timeChekcRun = true;
    timeCheckThread = std::thread([this]() {TimeCheckThread(); });
    std::cout << "TimeCheckThread 시작" << std::endl;
    return true;
}

void RoomManager::MatchingThread() {
    uint8_t tempRoomNum;
    Room* tempRoom;
    MatchingRoom* tempMatching1;
    MatchingRoom* tempMatching2;
    while (matchRun) {
        for (int i = 1; i <= 6; i++) {
            tbb::concurrent_hash_map<uint8_t, std::priority_queue<MatchingRoom*>>::accessor accessor;
            if(matchingMap.find(accessor,i)) {
               if(!accessor->second.empty()) {
                   tempMatching1 = accessor->second.top();
                   accessor->second.pop();
                   if (!accessor->second.empty()) { // 두번째 대기 유저가 있음
                       tempMatching2 = accessor->second.top();
                       accessor->second.pop();

                       { // 두명 유저 방 만들어서 넣어주기
                           if (roomNumQueue.pop(tempRoomNum)) { // Exist Room Num

                           }
                           else { // Not Exist Room Num

                           }
                       }

                   }
                   else { // 현재 레벨에 대기 유저 한명
                       accessor->second.push(tempMatching1);
                   }
               }
            }
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