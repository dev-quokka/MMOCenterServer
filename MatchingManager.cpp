#include "MatchingManager.h"

void MatchingManager::Init(const uint16_t maxClientCount_, RedisManager* redisManager_, InGameUserManager* inGameUserManager_, RoomManager* roomManager_, ConnUsersManager* connUsersManager_) {
    for (int i = 1; i <= USER_MAX_LEVEL/3 + 1; i++ ) { // Max i = MaxLevel/3 + 1 (Level Check Set)
        std::priority_queue<MatchingRoom*> k;
        matchingMap.emplace(i, std::priority_queue<MatchingRoom*>());
    }

    for (int i = 1; i <= maxClientCount_; i++) { // Room Number Set
        roomNumQueue.push(i);
    }

    inGameUserManager = inGameUserManager_;
    roomManager = roomManager_;
    connUsersManager = connUsersManager_;
    redisManager = redisManager_;
      
    CreateMatchThread();
    TimeCheckThread();
}

bool MatchingManager::Insert(uint16_t userLevel_, uint16_t userObjNum_, std::string userId_) {
    MatchingRoom* tempRoom = new MatchingRoom(userLevel_, userObjNum_, userId_);

    tbb::concurrent_hash_map<uint16_t, std::priority_queue<MatchingRoom*>>::accessor accessor;

    if (matchingMap.find(accessor, userLevel_/3 + 1)) { // Insert Success
        accessor->second.push(tempRoom);
        return true;
    }

    // Match Queue Full || Insert Fail
    return false;
}

bool MatchingManager::CreateMatchThread() {
	matchRun = true;
    matchingThread = std::thread([this]() {MatchingThread(); });
    std::cout << "MatchThread 시작" << std::endl;
    return true;
}

bool MatchingManager::CreateTimeCheckThread() {
    timeChekcRun = true;
    timeCheckThread = std::thread([this]() {TimeCheckThread(); });
    std::cout << "TimeCheckThread 시작" << std::endl;
    return true;
}

void MatchingManager::MatchingThread() {
    uint16_t tempRoomNum;
    Room* tempRoom;
    MatchingRoom* tempMatching1;
    MatchingRoom* tempMatching2;
    while (matchRun) {
        if (roomNumQueue.pop(tempRoomNum)) { // Exist Room Num
            for (int i = 1; i <= 6; i++) {
                tbb::concurrent_hash_map<uint16_t, std::priority_queue<MatchingRoom*>>::accessor accessor1;
                if (matchingMap.find(accessor1, i)) {
                    if (!accessor1->second.empty()) { 
                        tempMatching1 = accessor1->second.top();
                        if (tempMatching1->userId != inGameUserManager->GetInGameUserByObjNum((connUsersManager->FindUser(tempMatching1->userObjNum)->GetObjNum()))->GetId() ) { // 이미 나간 유저면 다음으로 넘어가기
                            accessor1->second.pop();
                            delete tempMatching1;
                            continue;
                        }
                        accessor1->second.pop();

                        if (!accessor1->second.empty()) { // 두번째 대기 유저가 있음
                            tempMatching2 = accessor1->second.top();
                            if (tempMatching2->userId != inGameUserManager->GetInGameUserByObjNum((connUsersManager->FindUser(tempMatching2->userObjNum)->GetObjNum()))->GetId()) { // 이미 나간 유저면 다음으로 넘어가기
                                accessor1->second.pop();
                                delete tempMatching2;
                                continue;
                            }
                            accessor1->second.pop();

                            { // 두명 유저 방 만들어서 넣어주기
                                RAID_READY_REQUEST rReadyResPacket1;
                                RAID_READY_REQUEST rReadyResPacket2;
                                InGameUser* user1 = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(tempMatching1->userObjNum)->GetObjNum());
                                InGameUser* user2 = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(tempMatching2->userObjNum)->GetObjNum());
                                
                                // Send to User1 With User2 Info
                                rReadyResPacket1.PacketId = (uint16_t)PACKET_ID::RAID_MATCHING_RESPONSE;
                                rReadyResPacket1.PacketLength = sizeof(RAID_MATCHING_RESPONSE);
                                rReadyResPacket1.timer = 2;
                                rReadyResPacket1.roomNum = tempRoomNum;
                                rReadyResPacket1.yourNum = 0;
                                rReadyResPacket1.mobHp = 30; // 나중에 몹당 hp Map 만들어서 설정하기
                                rReadyResPacket1.udpPort = UDP_PORT;
                                strcpy_s(rReadyResPacket1.serverIP, serverIP);

                                // Send to User2 with User1 Info
                                rReadyResPacket2.PacketId = (uint16_t)PACKET_ID::RAID_MATCHING_RESPONSE;
                                rReadyResPacket2.PacketLength = sizeof(RAID_MATCHING_RESPONSE);
                                rReadyResPacket2.timer = 2;
                                rReadyResPacket2.roomNum = tempRoomNum;
                                rReadyResPacket2.yourNum = 1;
                                rReadyResPacket2.mobHp = 30; // 나중에 몹당 hp Map 만들어서 설정하기
                                rReadyResPacket2.udpPort = UDP_PORT;
                                strcpy_s(rReadyResPacket2.serverIP, serverIP);

                                // 마지막 요청 처리 뒤에 방 생성 요청 보내기 (전에 요청 건 다 처리하고 방 생성)
                                redisManager->PushRedisPacket(tempMatching1->userObjNum, sizeof(PacketInfo), (char*)&rReadyResPacket1); // Send User1 with Game Info && User2 Info
                                redisManager->PushRedisPacket(tempMatching2->userObjNum, sizeof(PacketInfo), (char*)&rReadyResPacket2); // Send User2 with Game Info && User1 Info

                                endRoomCheckSet.insert(roomManager->MakeRoom(tempRoomNum, 2, 30, tempMatching1->userObjNum, tempMatching2->userObjNum, user1, user2));
                            }
                            delete tempMatching1;
                            delete tempMatching2;
                        }
                        else { // 현재 레벨에 대기 유저 한명이라 다시 넣기
                            accessor1->second.push(tempMatching1);
                        }
                    }
                }
            }
        }
        else { // Not Exist Room Num
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void MatchingManager::DeleteMob(Room* room_) {
    {
        std::lock_guard<std::mutex> guard(mDeleteRoom);
        for (auto iter = endRoomCheckSet.begin(); iter != endRoomCheckSet.end(); iter++) {
            if (*iter == room_) {
                delete *iter;
                endRoomCheckSet.erase(iter);
                break;
            }
        }
    }

    // 다른 Raid 관련 요청 보다 타임 종료는 먼저 처리되야 함으로 바로 유저 Send
    RAID_END_REQUEST raidEndReqPacket1;
    RAID_END_REQUEST raidEndReqPacket2;
    InGameUser* user1 = room_->GetUser(0);
    InGameUser* user2 = room_->GetUser(1);

    // Send to User1 With User2 Info
    raidEndReqPacket1.PacketId = (uint16_t)PACKET_ID::RAID_END_REQUEST;
    raidEndReqPacket1.PacketLength = sizeof(RAID_END_REQUEST);
    raidEndReqPacket1.userScore = room_->GetScore(0);
    raidEndReqPacket1.teamScore = room_->GetScore(1);

    connUsersManager->FindUser(room_->GetUserObjNum(0))->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket1);

    // Send to User2 with User1 Info
    raidEndReqPacket2.PacketId = (uint16_t)PACKET_ID::RAID_END_REQUEST;
    raidEndReqPacket2.PacketLength = sizeof(RAID_END_REQUEST);
    raidEndReqPacket2.teamScore = room_->GetScore(0);
    raidEndReqPacket2.userScore = room_->GetScore(1);

    connUsersManager->FindUser(room_->GetUserObjNum(1))->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket2);

    roomManager->DeleteRoom(room_->GetRoomNum());
    roomNumQueue.push(room_->GetRoomNum());
}

void MatchingManager::TimeCheckThread() {
    std::chrono::steady_clock::time_point now;
    Room* room_;
    while (timeChekcRun) {
        if (!endRoomCheckSet.empty()) { // Room Exist
            room_ = (*endRoomCheckSet.begin());
            now = std::chrono::steady_clock::now();
            if (room_->GetEndTime() <= now) {

                // 다른 Raid 관련 요청 보다 타임 종료는 먼저 처리되야 함으로 바로 유저 Send
                RAID_END_REQUEST raidEndReqPacket1;
                RAID_END_REQUEST raidEndReqPacket2;
                InGameUser* user1 = room_->GetUser(0);
                InGameUser* user2 = room_->GetUser(1);

                // Send to User1 With User2 Info
                raidEndReqPacket1.PacketId = (uint16_t)PACKET_ID::RAID_END_REQUEST;
                raidEndReqPacket1.PacketLength = sizeof(RAID_END_REQUEST);
                raidEndReqPacket1.userScore = room_->GetScore(0);
                raidEndReqPacket1.teamScore = room_->GetScore(1);

                connUsersManager->FindUser(room_->GetUserObjNum(0))->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket1);

                // Send to User2 with User1 Info
                raidEndReqPacket2.PacketId = (uint16_t)PACKET_ID::RAID_END_REQUEST;
                raidEndReqPacket2.PacketLength = sizeof(RAID_END_REQUEST);
                raidEndReqPacket2.teamScore = room_->GetScore(0);
                raidEndReqPacket2.userScore = room_->GetScore(1);

                connUsersManager->FindUser(room_->GetUserObjNum(1))->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket2);

                // Send Message To Redis Cluster For Syncronize
                redisManager->SyncRaidScoreToRedis(raidEndReqPacket1, raidEndReqPacket2);

                roomManager->DeleteRoom(room_->GetRoomNum());
                roomNumQueue.push(room_->GetRoomNum());
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } 
        else { // Room Not Exist
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}
