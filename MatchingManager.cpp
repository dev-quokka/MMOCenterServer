#include "MatchingManager.h"

void MatchingManager::Init(const uint16_t maxClientCount_, RedisManager* redisManager_, InGameUserManager* inGameUserManager_, RoomManager* roomManager_, ConnUsersManager* connUsersManager_) {
    for (int i = 1; i <= USER_MAX_LEVEL/3 + 1; i++ ) { // Max i = MaxLevel/3 + 1 (Level Check Set)
        matchingMap.emplace(i, std::set<MatchingRoom*, MatchingRoomComp>());
    }

    for (int i = 1; i <= maxClientCount_; i++) { // Room Number Set
        roomNumQueue.push(i);
    }

    inGameUserManager = inGameUserManager_;
    roomManager = roomManager_;
    connUsersManager = connUsersManager_;
    redisManager = redisManager_;
      
    CreateMatchThread();
    CreateTimeCheckThread();
}

bool MatchingManager::Insert(uint16_t userObjNum_, InGameUser* inGameUser_) {
    MatchingRoom* tempRoom = new MatchingRoom(userObjNum_, inGameUser_);

    tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor;
    uint16_t groupNum = inGameUser_->GetLevel() / 3 + 1;

    if (matchingMap.find(accessor, groupNum)) { // Insert Success
        accessor->second.insert(tempRoom);
        std::cout << "Insert Group " << groupNum << std::endl;
        return true;
    }

    // Match Queue Full || Insert Fail
    return false;
}

bool MatchingManager::CancelMatching(uint16_t userObjNum_, InGameUser* inGameUser_) {
    tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor;
    uint16_t groupNum = inGameUser_->GetLevel() / 3 + 1;
    matchingMap.find(accessor, groupNum);
    auto tempM = accessor->second;

    {
        std::lock_guard<std::mutex> guard(mDeleteMatch);
        for (auto iter = tempM.begin(); iter != tempM.end(); iter++) {
            if ((*iter)->userObjNum == userObjNum_) { // ��Ī ����� ���� ã��
                delete* iter;
                tempM.erase(iter);
                return true;
            }
        }
    }

    return false;
}


bool MatchingManager::CreateMatchThread() {
	matchRun = true;
    matchingThread = std::thread([this]() {MatchingThread(); });
    std::cout << "MatchThread Start" << std::endl;
    return true;
}

bool MatchingManager::CreateTimeCheckThread() {
    timeChekcRun = true;
    timeCheckThread = std::thread([this]() {TimeCheckThread(); });
    std::cout << "TimeCheckThread Start" << std::endl;
    return true;
}

void MatchingManager::MatchingThread() {
    uint16_t cnt = 1;
    uint16_t tempRoomNum = 0;
    MatchingRoom* tempMatching1;
    MatchingRoom* tempMatching2;

    while (matchRun) {
        if (tempRoomNum == 0) { // ��� �ѹ� �̾Ƽ� ���� �̾ƾ���
            if (roomNumQueue.pop(tempRoomNum)) { // Exist Room Num
                for (int i = cnt; i <= 6; i++) {
                    tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor1;
                    if (matchingMap.find(accessor1, i)) { // i��° ���� �׷� �ѹ� üũ

                        if (!accessor1->second.empty()) { // ���� �Ѹ��̶� ����
                            tempMatching1 = *accessor1->second.begin();

                            if (tempMatching1->inGameUser->GetId() != inGameUserManager->GetInGameUserByObjNum((connUsersManager->FindUser(tempMatching1->userObjNum)
                                ->GetObjNum()))->GetId()) { // ���� �������� ������ ���̵�� ���ؼ� �ٸ��� �̹� ���� ���������� �������� �Ѿ��
                                accessor1->second.erase(accessor1->second.begin());
                                delete tempMatching1;
                                continue;
                            }

                            accessor1->second.erase(accessor1->second.begin());

                            if (!accessor1->second.empty()) { // �ι�° ��� ������ ����
                                tempMatching2 = *accessor1->second.begin();

                                if (tempMatching2->inGameUser->GetId() != inGameUserManager->GetInGameUserByObjNum((connUsersManager->FindUser(tempMatching2->userObjNum)->GetObjNum()))
                                    ->GetId()) { // �̹� ���� ������ �������� �Ѿ��
                                    accessor1->second.erase(accessor1->second.begin());
                                    delete tempMatching2;
                                    continue;
                                }

                                accessor1->second.erase(accessor1->second.begin());

                                { // �θ� ���� �� ���� �־��ֱ�
                                    RAID_READY_REQUEST rReadyResPacket1;
                                    RAID_READY_REQUEST rReadyResPacket2;

                                    // Send to User1 With User2 Info
                                    rReadyResPacket1.PacketId = (uint16_t)PACKET_ID::RAID_READY_REQUEST;
                                    rReadyResPacket1.PacketLength = sizeof(RAID_READY_REQUEST);
                                    rReadyResPacket1.timer = 2;
                                    rReadyResPacket1.roomNum = tempRoomNum;
                                    rReadyResPacket1.yourNum = 0;
                                    rReadyResPacket1.mobHp = 30; // ���߿� ���� hp Map ���� �����ϱ�

                                    // Send to User2 with User1 Info
                                    rReadyResPacket2.PacketId = (uint16_t)PACKET_ID::RAID_READY_REQUEST;
                                    rReadyResPacket2.PacketLength = sizeof(RAID_READY_REQUEST);
                                    rReadyResPacket2.timer = 2;
                                    rReadyResPacket2.roomNum = tempRoomNum;
                                    rReadyResPacket2.yourNum = 1;
                                    rReadyResPacket2.mobHp = 30; // ���߿� ���� hp Map ���� �����ϱ�

                                    // ������ ��û ó�� �ڿ� �� ���� ��û ������ (���� ��û �� �� ó���ϰ� �� ����)
                                    connUsersManager->FindUser(tempMatching1->userObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&rReadyResPacket1);
                                    connUsersManager->FindUser(tempMatching2->userObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&rReadyResPacket2);

                                    endRoomCheckSet.insert(roomManager->MakeRoom(tempRoomNum, 2, 30, tempMatching1->userObjNum, 
                                        tempMatching2->userObjNum, tempMatching1->inGameUser, tempMatching2->inGameUser));
                                }

                                delete tempMatching1;
                                delete tempMatching2;

                                if (cnt == 6) cnt = 1; // ��� ��Ī�� ���� ��ȣ �׷���� ��Ī�� ���� cnt üũ
                                else cnt++;
                                tempRoomNum = 0;
                                break;
                            }
                            else { // ���� ������ ��� ���� �Ѹ��̶� �ٽ� �ֱ�
                                accessor1->second.insert(tempMatching1);
                            }
                        }
                    }
                }

            }
            else { // Not Exist Room Num
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        else {
            for (int i = 1; i <= 6; i++) {
                tbb::concurrent_hash_map<uint16_t, std::set<MatchingRoom*, MatchingRoomComp>>::accessor accessor1;
                if (matchingMap.find(accessor1, i)) { // i��° ���� �׷� �ѹ� üũ

                    if (!accessor1->second.empty()) { // ���� �Ѹ��̶� ����
                        tempMatching1 = *accessor1->second.begin();

                        if (tempMatching1->inGameUser->GetId() != inGameUserManager->GetInGameUserByObjNum((connUsersManager->FindUser(tempMatching1->userObjNum)->GetObjNum()))->GetId()) { // ���� �������� ������ ���̵�� ���ؼ� �ٸ��� �̹� ���� ���������� �������� �Ѿ��
                            accessor1->second.erase(accessor1->second.begin());
                            delete tempMatching1;
                            continue;
                        }
                        accessor1->second.erase(accessor1->second.begin());

                        if (!accessor1->second.empty()) { // �ι�° ��� ������ ����
                            tempMatching2 = *accessor1->second.begin();

                            if (tempMatching2->inGameUser->GetId() != inGameUserManager->GetInGameUserByObjNum((connUsersManager->FindUser(tempMatching2->userObjNum)->GetObjNum()))->GetId()) { // �̹� ���� ������ �������� �Ѿ��
                                accessor1->second.erase(accessor1->second.begin());
                                delete tempMatching2;
                                continue;
                            }

                            accessor1->second.erase(accessor1->second.begin());

                            { // �θ� ���� �� ���� �־��ֱ�
                                RAID_READY_REQUEST rReadyResPacket1;
                                RAID_READY_REQUEST rReadyResPacket2;

                                // Send to User1 With User2 Info
                                rReadyResPacket1.PacketId = (uint16_t)PACKET_ID::RAID_READY_REQUEST;
                                rReadyResPacket1.PacketLength = sizeof(RAID_READY_REQUEST);
                                rReadyResPacket1.timer = 2;
                                rReadyResPacket1.roomNum = tempRoomNum;
                                rReadyResPacket1.yourNum = 0;
                                rReadyResPacket1.mobHp = 30; // ���߿� ���� hp Map ���� �����ϱ�

                                // Send to User2 with User1 Info
                                rReadyResPacket2.PacketId = (uint16_t)PACKET_ID::RAID_READY_REQUEST;
                                rReadyResPacket2.PacketLength = sizeof(RAID_READY_REQUEST);
                                rReadyResPacket2.timer = 2;
                                rReadyResPacket2.roomNum = tempRoomNum;
                                rReadyResPacket2.yourNum = 1;
                                rReadyResPacket2.mobHp = 30; // ���߿� ���� hp Map ���� �����ϱ�

                                // ������ ��û ó�� �ڿ� �� ���� ��û ������ (���� ��û �� �� ó���ϰ� �� ����)
                                connUsersManager->FindUser(tempMatching1->userObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&rReadyResPacket1);
                                connUsersManager->FindUser(tempMatching2->userObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&rReadyResPacket2);

                                endRoomCheckSet.insert(roomManager->MakeRoom(tempRoomNum, 2, 30, tempMatching1->userObjNum, tempMatching2->userObjNum, tempMatching1->inGameUser, tempMatching2->inGameUser));
                            }

                            delete tempMatching1;
                            delete tempMatching2;

                            if (cnt == 6) cnt = 1;
                            else cnt++;
                            tempRoomNum = 0;
                            break;
                        }
                        else { // ���� ������ ��� ���� �Ѹ��̶� �ٽ� �ֱ�
                            accessor1->second.insert(tempMatching1);
                        }
                    }
                }
            }
        }
    }
}

void MatchingManager::DeleteMob(Room* room_) { // �� ���� �׿�����

    if (room_->TimeOverCheck()) { // Ÿ�Ӿƿ� üũ�Ǽ� �̹� endRoomCheckSet���� ���� ������ (�� ������ ó��)
        roomManager->DeleteRoom(room_->GetRoomNum());
        roomNumQueue.push(room_->GetRoomNum());
        return;
    }

    {    // Ÿ�Ӿƿ����� ���� ��Ƽ� endRoomCheckSet ���� ����
        std::lock_guard<std::mutex> guard(mDeleteRoom);
        for (auto iter = endRoomCheckSet.begin(); iter != endRoomCheckSet.end(); iter++) {
            if (*iter == room_) {
                delete *iter;
                endRoomCheckSet.erase(iter);
                break;
            }
        }
    }

    roomManager->DeleteRoom(room_->GetRoomNum());
    roomNumQueue.push(room_->GetRoomNum());
}

void MatchingManager::TimeCheckThread() {
    std::chrono::steady_clock::time_point now;
    Room* room_;
    while (timeChekcRun) {
        if (!endRoomCheckSet.empty()) { // Room Exist
            room_ = (*endRoomCheckSet.begin());
            if (room_->GetEndTime() <= std::chrono::steady_clock::now()) {
                std::cout << "Ÿ�� �ƿ�. ���̵� ����" << std::endl;
                room_->TimeOver();
                endRoomCheckSet.erase(endRoomCheckSet.begin());
            }
            else {
                std::cout << "������" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        } 
        else { // Room Not Exist
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}
