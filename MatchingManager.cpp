#include "MatchingManager.h"

void MatchingManager::Init() {
    for (int i = 1; i <= 6; i++ ) { // Max i = MaxLevel/3 + 1 (Level Check Set)
        std::priority_queue<MatchingRoom*> k;
        matchingMap.emplace(i, std::priority_queue<MatchingRoom*>());
    }

    for (int i = 1; i <= 100; i++) { // (Max Room Set)
        roomNumQueue.push(i);
    }

    CreateMatchThread();
    TimeCheckThread();
}

bool MatchingManager::Insert(uint8_t userLevel_, UINT16 userSkt_, std::string userId_) {
    MatchingRoom* tempRoom;
    tempRoom->userLevel = userLevel_;
    tempRoom->userSkt = userSkt_;
    tempRoom->userId = userId_;

    tbb::concurrent_hash_map<uint8_t, std::priority_queue<MatchingRoom*>>::accessor accessor;

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
    std::cout << "MatchThread ����" << std::endl;
    return true;
}

bool MatchingManager::CreatTimeCheckThread() {
    timeChekcRun = true;
    timeCheckThread = std::thread([this]() {TimeCheckThread(); });
    std::cout << "TimeCheckThread ����" << std::endl;
    return true;
}

void MatchingManager::MatchingThread() {
    uint8_t tempRoomNum;
    Room* tempRoom;
    MatchingRoom* tempMatching1;
    MatchingRoom* tempMatching2;
    while (matchRun) {
        if (roomNumQueue.pop(tempRoomNum)) { // Exist Room Num
            for (int i = 1; i <= 6; i++) {
                tbb::concurrent_hash_map<uint8_t, std::priority_queue<MatchingRoom*>>::accessor accessor1;
                if (matchingMap.find(accessor1, i)) {
                    if (!accessor1->second.empty()) { 
                        tempMatching1 = accessor1->second.top();
                        accessor1->second.pop();
                        tbb::concurrent_hash_map<uint8_t, std::priority_queue<MatchingRoom*>>::accessor accessor2;
                        if (!accessor2->second.empty()) { // �ι�° ��� ������ ����
                            tempMatching2 = accessor2->second.top();
                            accessor2->second.pop();

                            { // �θ� ���� �� ���� �־��ֱ�
                                RAID_MATCHING_RESPONSE rMatchResPacket1;
                                RAID_MATCHING_RESPONSE rMatchResPacket2;
                                InGameUser* user1 = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(tempMatching1->userSkt)->GetObjNum());
                                InGameUser* user2 = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(tempMatching2->userSkt)->GetObjNum());
                                
                                // Send to User1 With User2 Info
                                rMatchResPacket1.PacketId = (UINT16)PACKET_ID::RAID_MATCHING_RESPONSE;
                                rMatchResPacket1.PacketLength = sizeof(RAID_MATCHING_RESPONSE);
                                rMatchResPacket1.uuId = user1->GetUuid();
                                rMatchResPacket1.timer = 2;
                                rMatchResPacket1.roomNum = tempRoomNum;
                                rMatchResPacket1.mobHp = 30; // ���߿� ���� hp Map ���� �����ϱ�

                                // Send to User2 with User1 Info
                                rMatchResPacket2.PacketId = (UINT16)PACKET_ID::RAID_MATCHING_RESPONSE;
                                rMatchResPacket2.PacketLength = sizeof(RAID_MATCHING_RESPONSE);
                                rMatchResPacket2.uuId = user2->GetUuid();
                                rMatchResPacket2.timer = 2;
                                rMatchResPacket2.roomNum = tempRoomNum;
                                rMatchResPacket2.mobHp = 30; // ���߿� ���� hp Map ���� �����ϱ�

                                // ������ ��û ó�� �ڿ� �� ���� ��û ������ (���� ��û�� �� ó���ϰ� �� ����)
                                redisManager->PushRedisPacket(tempMatching1->userSkt,sizeof(PacketInfo), (char*)&rMatchResPacket1); // Send User1 with Game Info && User2 Info
                                redisManager->PushRedisPacket(tempMatching1->userSkt, sizeof(PacketInfo), (char*)&rMatchResPacket2); // Send User2 with Game Info && User1 Info
                                
                                std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
                                std::chrono::time_point<std::chrono::steady_clock> endTime = now + std::chrono::minutes(2) + std::chrono::seconds(10); // Add 2m + 10 sec

                                endRoomCheckSet.insert(roomManager->MakeRoom(tempRoomNum, 30, tempMatching1->userSkt, tempMatching2->userSkt, user1, user2, endTime));
                            }

                            while (!roomNumQueue.pop(tempRoomNum)) { // �� �ѹ� ������ ���� ���� ��ġ���� ��ȯ�ɶ����� ���
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            }

                            continue; // ��ѹ� ������ �������� �Ѿ�� ����

                        }
                        else { // ���� ������ ��� ���� �Ѹ��̶� �ٽ� �ֱ�
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
        mDeleteRoom.lock();
        for (auto iter = endRoomCheckSet.begin(); iter != endRoomCheckSet.end(); iter++) {
            if (*iter == room_) {
                delete *iter;
                endRoomCheckSet.erase(iter);
                break;
            }
        }
        mDeleteRoom.unlock();
    }

    // �ٸ� Raid ���� ��û ���� Ÿ�� ����� ���� ó���Ǿ� ������ �ٷ� ���� Send
    RAID_END_REQUEST raidEndReqPacket1;
    RAID_END_REQUEST raidEndReqPacket2;
    InGameUser* user1 = room_->GetUser(0);
    InGameUser* user2 = room_->GetUser(1);

    // Send to User1 With User2 Info
    raidEndReqPacket1.PacketId = (UINT16)PACKET_ID::RAID_END_REQUEST;
    raidEndReqPacket1.PacketLength = sizeof(RAID_END_REQUEST);
    raidEndReqPacket1.uuId = user1->GetUuid();
    raidEndReqPacket1.userScore = room_->GetScore(0);
    raidEndReqPacket1.teamScore = room_->GetScore(1);

    connUsersManager->FindUser(room_->GetUserSkt(0))->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket1);

    // Send to User2 with User1 Info
    raidEndReqPacket2.PacketId = (UINT16)PACKET_ID::RAID_END_REQUEST;
    raidEndReqPacket2.PacketLength = sizeof(RAID_END_REQUEST);
    raidEndReqPacket2.uuId = user2->GetUuid();
    raidEndReqPacket2.teamScore = room_->GetScore(0);
    raidEndReqPacket2.userScore = room_->GetScore(1);

    connUsersManager->FindUser(room_->GetUserSkt(1))->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket2);

    // Send Message To Redis Cluster For Syncronize
    redisManager->SyncRaidScoreToRedis(raidEndReqPacket1, raidEndReqPacket2);

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

                // �ٸ� Raid ���� ��û ���� Ÿ�� ����� ���� ó���Ǿ� ������ �ٷ� ���� Send
                RAID_END_REQUEST raidEndReqPacket1;
                RAID_END_REQUEST raidEndReqPacket2;
                InGameUser* user1 = room_->GetUser(0);
                InGameUser* user2 = room_->GetUser(1);

                // Send to User1 With User2 Info
                raidEndReqPacket1.PacketId = (UINT16)PACKET_ID::RAID_END_REQUEST;
                raidEndReqPacket1.PacketLength = sizeof(RAID_END_REQUEST);
                raidEndReqPacket1.uuId = user1->GetUuid();
                raidEndReqPacket1.userScore = room_->GetScore(0);
                raidEndReqPacket1.teamScore = room_->GetScore(1);

                connUsersManager->FindUser(room_->GetUserSkt(0))->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket1);

                // Send to User2 with User1 Info
                raidEndReqPacket2.PacketId = (UINT16)PACKET_ID::RAID_END_REQUEST;
                raidEndReqPacket2.PacketLength = sizeof(RAID_END_REQUEST);
                raidEndReqPacket2.uuId = user2->GetUuid();
                raidEndReqPacket2.teamScore = room_->GetScore(0);
                raidEndReqPacket2.userScore = room_->GetScore(1);

                connUsersManager->FindUser(room_->GetUserSkt(1))->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket2);

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