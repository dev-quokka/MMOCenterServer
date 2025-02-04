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
                                ConnUser* user1;
                                ConnUser* user2;
                                // Send to User1 with User2 Info
                                rMatchResPacket1.PacketId = (UINT16)PACKET_ID::RAID_MATCHING_RESPONSE;
                                rMatchResPacket1.PacketLength = sizeof(RAID_MATCHING_RESPONSE);
                                rMatchResPacket1.uuId = user1->GetUuid();
                                rMatchResPacket1.timer = 2;
                                rMatchResPacket1.roomNum = tempRoomNum;
                                rMatchResPacket1.mobHp = 30; // ���߿� ���� hp Map ���� �����ϱ�
                                rMatchResPacket1.teamLevel = tempMatching2->userLevel;
                                rMatchResPacket1.teamUserSkt = tempMatching2->userSkt;
                                rMatchResPacket1.teamId = tempMatching2->userId;

                                // Send to User2 with User1 Info
                                rMatchResPacket2.PacketId = (UINT16)PACKET_ID::RAID_MATCHING_RESPONSE;
                                rMatchResPacket2.PacketLength = sizeof(RAID_MATCHING_RESPONSE);
                                rMatchResPacket2.uuId = user2->GetUuid();
                                rMatchResPacket2.timer = 2;
                                rMatchResPacket2.roomNum = tempRoomNum;
                                rMatchResPacket2.mobHp = 30; // ���߿� ���� hp Map ���� �����ϱ�
                                rMatchResPacket2.teamLevel = tempMatching1->userLevel;
                                rMatchResPacket2.teamUserSkt = tempMatching1->userSkt;
                                rMatchResPacket2.teamId = tempMatching1->userId;

                                user1->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&rMatchResPacket1); // Send User1 with Game Info && User2 Info
                                user2->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&rMatchResPacket1); // Send User2 with Game Info && User1 Info
                                
                                Room* room();
                            }

                            tempRoom = 0;

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

void MatchingManager::TimeCheckThread() {
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