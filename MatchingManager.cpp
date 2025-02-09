#include "MatchingManager.h"

void MatchingManager::Init(const UINT16 maxClientCount_, const HANDLE sIOCPHandle_, RedisManager* redisManager_) {
    for (int i = 1; i <= 6; i++ ) { // Max i = MaxLevel/3 + 1 (Level Check Set)
        std::priority_queue<MatchingRoom*> k;
        matchingMap.emplace(i, std::priority_queue<MatchingRoom*>());
    }

    for (int i = 1; i <= maxClientCount_; i++) { // Room Number Set
        roomNumQueue.push(i);
    }

    for (int i = 0; i <= maxClientCount_ / 300; i++) { // 유저 300명당 하나의 UDP SOCKET 생성
        SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

        sockaddr_in udpAddr;
        udpAddr.sin_family = AF_INET;
        udpAddr.sin_port = htons(UDP_PORT+i);
        udpAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(udpSocket, (SOCKADDR*)&udpAddr, sizeof(udpAddr)) == SOCKET_ERROR) {
            std::cout << "UDP SOCKET BIND FAIL" << std::endl;
            closesocket(udpSocket);
            break;
        }

        HANDLE result = CreateIoCompletionPort((HANDLE)udpSocket, sIOCPHandle_, (ULONG_PTR)0, 0);

        if (result == NULL) {
            std::cerr << "UDP SOCKET IOCP BIND FAIL : " << GetLastError() << std::endl;
        }

        if (serverIP == '\0') { // 처음에만 serverIP에 IP값 할당해두기
            sockaddr_in addr;
            int addrSize = sizeof(addr);

            if (getsockname(udpSocket, (SOCKADDR*)&addr, &addrSize) == 0) {
                inet_ntop(AF_INET, &addr.sin_addr, serverIP, sizeof(serverIP));
            }
            else {
                std::cerr << "GET UDP SOCKET IP FAIL : " << WSAGetLastError() << std::endl;
            }
        }

        udpSockets.emplace_back(udpSocket);
    }

    redisManager = redisManager_;
    CreateMatchThread();
    TimeCheckThread();
}

SOCKET MatchingManager::GetUDPSocket(uint8_t roomNum_) {
    return udpSockets[roomNum_/300]; // 300명 단위로 나누어진 udp 소켓
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
    std::cout << "MatchThread 시작" << std::endl;
    return true;
}

bool MatchingManager::CreateTimeCheckThread() {
    timeChekcRun = true;
    timeCheckThread = std::thread([this]() {TimeCheckThread(); });
    std::cout << "TimeCheckThread 시작" << std::endl;
    return true;
}

bool MatchingManager::CreateUDPWorkThread(HANDLE sIOCPHandle_) {
    workRun = true;
    udpWorkThread = std::thread([this, sIOCPHandle_]() {UDPWorkThread(sIOCPHandle_); });
    std::cout << "UDPWorkThread 시작" << std::endl;
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
                        if (!accessor2->second.empty()) { // 두번째 대기 유저가 있음
                            tempMatching2 = accessor2->second.top();
                            accessor2->second.pop();

                            { // 두명 유저 방 만들어서 넣어주기
                                RAID_READY_REQUEST rReadyResPacket1;
                                RAID_READY_REQUEST rReadyResPacket2;
                                InGameUser* user1 = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(tempMatching1->userSkt)->GetObjNum());
                                InGameUser* user2 = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(tempMatching2->userSkt)->GetObjNum());
                                
                                // Send to User1 With User2 Info
                                rReadyResPacket1.PacketId = (UINT16)PACKET_ID::RAID_MATCHING_RESPONSE;
                                rReadyResPacket1.PacketLength = sizeof(RAID_MATCHING_RESPONSE);
                                rReadyResPacket1.uuId = user1->GetUuid();
                                rReadyResPacket1.timer = 2;
                                rReadyResPacket1.roomNum = tempRoomNum;
                                rReadyResPacket1.yourNum = 0;
                                rReadyResPacket1.mobHp = 30; // 나중에 몹당 hp Map 만들어서 설정하기
                                rReadyResPacket1.udpPort = UDP_PORT;
                                strcpy(rReadyResPacket1.serverIP, serverIP);

                                // Send to User2 with User1 Info
                                rReadyResPacket2.PacketId = (UINT16)PACKET_ID::RAID_MATCHING_RESPONSE;
                                rReadyResPacket2.PacketLength = sizeof(RAID_MATCHING_RESPONSE);
                                rReadyResPacket2.uuId = user2->GetUuid();
                                rReadyResPacket2.timer = 2;
                                rReadyResPacket2.roomNum = tempRoomNum;
                                rReadyResPacket2.yourNum = 1;
                                rReadyResPacket2.mobHp = 30; // 나중에 몹당 hp Map 만들어서 설정하기
                                rReadyResPacket2.udpPort = UDP_PORT;
                                strcpy(rReadyResPacket2.serverIP, serverIP);

                                // 마지막 요청 처리 뒤에 방 생성 요청 보내기 (전에 요청 건 다 처리하고 방 생성)
                                redisManager->PushRedisPacket(tempMatching1->userSkt, sizeof(PacketInfo), (char*)&rReadyResPacket1); // Send User1 with Game Info && User2 Info
                                redisManager->PushRedisPacket(tempMatching1->userSkt, sizeof(PacketInfo), (char*)&rReadyResPacket2); // Send User2 with Game Info && User1 Info

                                endRoomCheckSet.insert(roomManager->MakeRoom(tempRoomNum, 2, 30, tempMatching1->userSkt, tempMatching2->userSkt, user1, user2));
                            }

                            while (!roomNumQueue.pop(tempRoomNum)) { // 룸 넘버 없으면 현재 레벨 위치에서 반환될때까지 대기
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            }

                            continue; // 룸넘버 있으면 다음으로 넘어가서 실행

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

    // 다른 Raid 관련 요청 보다 타임 종료는 먼저 처리되야 함으로 바로 유저 Send
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

                // 다른 Raid 관련 요청 보다 타임 종료는 먼저 처리되야 함으로 바로 유저 Send
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


void MatchingManager::SyncMobHp(OverlappedUDP* overlappedUDP_, uint8_t roomNum_){
    DWORD dwSendBytes = 0;

   int result =  WSASendTo(udpSockets[roomNum_/300], &overlappedUDP_->wsaBuf, 1, &dwSendBytes, 0, (SOCKADDR*)&overlappedUDP_->userAddr, sizeof(overlappedUDP_->userAddr), (LPWSAOVERLAPPED)overlappedUDP_, NULL);

   if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
       std::cerr << "WSASendTo Fail : " << WSAGetLastError() << std::endl;
       delete[] overlappedUDP_->wsaBuf.buf;
       delete overlappedUDP_;
   }

}

void MatchingManager::UDPWorkThread(HANDLE sIOCPHandle_) {
    LPOVERLAPPED lpOverlapped = NULL;
    DWORD dwIoSize = 0;
    bool gqSucces = TRUE;

    while (workRun) {
        gqSucces = GetQueuedCompletionStatus(
            sIOCPHandle_,
            &dwIoSize,
            nullptr,
            &lpOverlapped,
            INFINITE
        );

        auto overlappedUDP = (OverlappedUDP*)lpOverlapped;

        if (overlappedUDP->taskType == TaskType::SEND) {
            delete[] overlappedUDP->wsaBuf.buf;
            delete overlappedUDP;
        }

        else if (overlappedUDP->taskType == TaskType::RECV) { // 나중에 필요할때 추가 생성

        }

    }

}
