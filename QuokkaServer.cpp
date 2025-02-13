#include "QuokkaServer.h"

bool QuokkaServer::init(const uint16_t MaxThreadCnt_, int port_) {
    WSADATA wsadata;
    int check = 0;
    MaxThreadCnt = MaxThreadCnt_; // 워크 스레드 개수 설정

    check = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (check) {
        std::cout << "WSAStartup 실패" << std::endl;
        return false;
    }

    ServerSKT = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
    if (ServerSKT == INVALID_SOCKET) {
        std::cout << "Server Socket 생성 실패" << std::endl;
        return false;
    }

    SOCKADDR_IN addr;
    addr.sin_port = htons(port_);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    check = bind(ServerSKT, (SOCKADDR*)&addr, sizeof(addr));
    if (check) {
        std::cout << "bind 함수 실패" << std::endl;
        return false;
    }

    check = listen(ServerSKT, SOMAXCONN);
    if (check) {
        std::cout << "listen 함수 실패" << std::endl;
        return false;
    }

    sIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MaxThreadCnt);
    if (sIOCPHandle == NULL) {
        std::cout << "iocp 핸들 생성 실패" << std::endl;
        return false;
    }

    auto bIOCPHandle = CreateIoCompletionPort((HANDLE)ServerSKT, sIOCPHandle, (uint32_t)0, 0);
    if (bIOCPHandle == nullptr) {
        std::cout << "iocp 핸들 바인드 실패" << std::endl;
        return false;
    }

    std::cout << "소켓 생성 성공" << std::endl;
    return true;
}

bool QuokkaServer::StartWork() {
    bool check = CreateWorkThread();
    if (!check) {
        std::cout << "WorkThread 생성 실패" << std::endl;
        return false;
    }

    check = CreateAccepterThread();
    if (!check) {
        std::cout << "CreateAccepterThread 생성 실패" << std::endl;
        return false;
    }

    p_RedisManager = new RedisManager;
    p_ConnUsersManagerManager = new ConnUsersManager;

    for (int i = 0; i < maxClientCount; i++) { // Make ConnUsers Queue
        SOCKET TempSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
        
        // For Reuse Socket Set
        int optval = 1;
        setsockopt(TempSkt, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));

        int recvBufSize = MAX_SOCK;
        setsockopt(TempSkt, SOL_SOCKET, SO_RCVBUF, (char*)&recvBufSize, sizeof(recvBufSize));

        int sendBufSize = MAX_SOCK;
        setsockopt(TempSkt, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sizeof(sendBufSize));

        if (TempSkt == INVALID_SOCKET) {
            std::cout << "Client socket Error : " << GetLastError() << std::endl;
            return false;
        }

        auto tIOCPHandle = CreateIoCompletionPort((HANDLE)TempSkt, sIOCPHandle, (ULONG_PTR)(this), 0);

        if (tIOCPHandle == INVALID_HANDLE_VALUE)
        {
            std::cout << "reateIoCompletionPort()함수 실패 :" << GetLastError() << std::endl;
            return false;
        }

        ConnUser* connUser = new ConnUser(TempSkt, MAX_RECV_DATA,i, sIOCPHandle);

        AcceptQueue.push(connUser); // Push ConnUser
        p_ConnUsersManagerManager->InsertUser(TempSkt); // Init ConnUsers
    }

    for (int i = 0; i < maxClientCount; i++) { // Make Waittint Users Queue
        SOCKET TempSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

        // For Reuse Socket
        int optval = 1;
        setsockopt(TempSkt, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));

        int recvBufSize = MAX_SOCK;
        setsockopt(TempSkt, SOL_SOCKET, SO_RCVBUF, (char*)&recvBufSize, sizeof(recvBufSize));

        int sendBufSize = MAX_SOCK;
        setsockopt(TempSkt, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sizeof(sendBufSize));

        if (TempSkt == INVALID_SOCKET) {
            std::cout << "Client socket Error : " << GetLastError() << std::endl;
            return false;
        }

        auto tIOCPHandle = CreateIoCompletionPort((HANDLE)TempSkt, sIOCPHandle, (ULONG_PTR)(this), 0);

        if (tIOCPHandle == INVALID_HANDLE_VALUE)
        {
            std::cout << "reateIoCompletionPort()함수 실패 :" << GetLastError() << std::endl;
            return false;
        }

        ConnUser* connUser = new ConnUser(TempSkt, MAX_RECV_DATA, maxClientCount, sIOCPHandle);

        WaittingQueue.push(connUser); // Push ConnUser
    }

    p_RedisManager->init(MaxThreadCnt, maxClientCount, sIOCPHandle);// Run MySQL && Run Redis Threads (The number of Clsuter Master Nodes + 1)
    p_RedisManager->SetConnUserManager(p_ConnUsersManagerManager); // 

    return true;
}

bool QuokkaServer::CreateWorkThread() {
    auto threadCnt = MaxThreadCnt; // core
    for (int i = 0; i < threadCnt; i++) {
        workThreads.emplace_back([this]() { WorkThread(); });
    }
    std::cout << "WorkThread Start" << std::endl;
    return true;
}

bool QuokkaServer::CreateAccepterThread() {
    auto threadCnt = MaxThreadCnt/4+1; // (core/4)
    for (int i = 0; i < threadCnt; i++) {
        acceptThreads.emplace_back([this]() { AccepterThread(); });
    }
    std::cout << "AcceptThread Start" << std::endl;
    return true;
}

void QuokkaServer::WorkThread() {
    LPOVERLAPPED lpOverlapped = NULL;
    ConnUser* connUser = nullptr;
    DWORD dwIoSize = 0;
    bool gqSucces = TRUE;

    while (WorkRun) {
        gqSucces = GetQueuedCompletionStatus(
            sIOCPHandle,
            &dwIoSize,
            (PULONG_PTR)&connUser,
            &lpOverlapped,
            INFINITE
        );

        if (gqSucces && dwIoSize == 0 && lpOverlapped == NULL) { // Server End Request
            WorkRun = false;
            AccepterRun = false;

            continue;
        }
        
        auto overlappedTCP = (OverlappedTCP*)lpOverlapped;
        connUser = p_ConnUsersManagerManager->FindUser(overlappedTCP->userSkt);
        SOCKET tempUserSkt = overlappedTCP->userSkt;

        if (!gqSucces || (dwIoSize == 0 && overlappedTCP->taskType != TaskType::ACCEPT)) { // User Disconnect
            p_RedisManager->Disconnect(tempUserSkt);
            std::cout << "socket " << tempUserSkt << " Disconnect && Data Update Fail" << std::endl;
            connUser->Reset(); // Reset ConnUser
            UserCnt.fetch_sub(1); // UserCnt -1
            continue;
        }

        if (overlappedTCP->taskType == TaskType::ACCEPT) { // User Connect
            if (connUser) {
                if (connUser->ConnUserRecv()) {
                    UserCnt.fetch_add(1); // UserCnt +1
                    p_ConnUsersManagerManager->InsertUser(tempUserSkt);
                    std::cout << "socket " << tempUserSkt << " Connect" << std::endl;
                }
                else { // Bind Fail
                    connUser->Reset(); // Reset ConnUser
                    AcceptQueue.push(connUser);
                }
            }
        }
        else if (overlappedTCP->taskType == TaskType::RECV) {
            p_RedisManager->PushRedisPacket(tempUserSkt, dwIoSize, connUser->GetRecvBuffer()); // Proccess In Redismanager
            connUser->ConnUserRecv(); // Wsarecv Again
            delete[] overlappedTCP->wsaBuf.buf;
            delete overlappedTCP;
        }

        else if (overlappedTCP->taskType == TaskType::SEND) {
            connUser->SendComplete();
        }
    }
}

void QuokkaServer::AccepterThread() {
    ConnUser* connUser;
    while (AccepterRun) {
        if (AcceptQueue.pop(connUser)) { // AcceptQueue not empty
            if (!connUser->PostAccept(ServerSKT)) {
                AcceptQueue.push(connUser);
            }
        }
        else { // AcceptQueue empty
            while (AccepterRun) {
                if (WaittingQueue.pop(connUser)) { // WaittingQueue not empty
                    WaittingQueue.push(connUser);
                }
                else { // WaittingQueue empty
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    break;
                }
            }
        }
    }
}

void QuokkaServer::ServerEnd() {
    for (int i = 0; i < workThreads.size(); i++) { // Work 쓰레드 종료
        if (workThreads[i].joinable()) {
            workThreads[i].join();
        }
    }

    for (int i = 0; i < acceptThreads.size(); i++) { // Accept 쓰레드 종료
        if (acceptThreads[i].joinable()) { 
            acceptThreads[i].join();
        }
    }

    ConnUser* connUser;

    while (AcceptQueue.pop(connUser)) { // 요청 대기큐 유저 객체 삭제
        closesocket(connUser->GetSocket());
        delete connUser;
    }

    while (WaittingQueue.pop(connUser)) { // 접속 대기큐 유저 객체 삭제
        closesocket(connUser->GetSocket());
        delete connUser;
    }

    delete p_RedisManager;
    delete p_ConnUsersManagerManager;
    CloseHandle(sIOCPHandle); // 핸들 종료
    closesocket(ServerSKT); // 서버 소켓 종료

    std::cout << "종료 5초 대기" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // 5초 대기
    std::cout << "종료" << std::endl;
}