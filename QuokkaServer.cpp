#include "QuokkaServer.h"

bool QuokkaServer::init(const UINT16 MaxThreadCnt_, int port_) {
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

    sIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MaxThreadCnt-1);
    if (sIOCPHandle == NULL) {
        std::cout << "iocp 핸들 생성 실패" << std::endl;
        return false;
    }

    auto bIOCPHandle = CreateIoCompletionPort((HANDLE)ServerSKT, sIOCPHandle, (UINT32)0, 0);
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

    p_ConnUsersManagerManager = std::make_unique<ConnUsersManager>();

    for (int i = 1; i <= maxClientCount; i++) { // Make ConnUsers Queue
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

        ConnUser* connUser = new ConnUser(TempSkt, MAX_RECV_DATA);

        AcceptQueue.push(connUser); // Push ConnUser
        p_ConnUsersManagerManager->InsertUser(TempSkt); // Init ConnUsers
    }

    for (int i = 1; i <= maxClientCount; i++) { // Make Waittint Users Queue
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

        ConnUser* connUser = new ConnUser(TempSkt, MAX_RECV_DATA);

        WaittingQueue.push(connUser); // Push ConnUser
    }
    p_RedisManager->init(MaxThreadCnt);// Run MySQL && Run Redis Threads (The number of Clsuter Master Nodes + 1)
    p_RedisManager->SetConnUserManager(p_ConnUsersManagerManager.get()); // 

    return true;
}

bool QuokkaServer::CreateWorkThread() {
    auto threadCnt = MaxThreadCnt; // core
    for (int i = 0; i < threadCnt; i++) {
        workThreads.emplace_back([this]() { WorkThread(); });
    }
    std::cout << "WorkThread start" << std::endl;
    return true;
}

bool QuokkaServer::CreateAccepterThread() {
    auto threadCnt = MaxThreadCnt/4; // (core/4)
    for (int i = 0; i < threadCnt; i++) {
        acceptThreads.emplace_back([this]() {AccepterThread();});
    }
    std::cout << "AcceptThread 시작" << std::endl;
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

        if (gqSucces && dwIoSize == 0 && lpOverlapped == NULL) {
            WorkRun = false;
            continue;
        }

        auto pOverlappedEx = (OverlappedEx*)lpOverlapped;
        connUser = p_ConnUsersManagerManager->FindUser(pOverlappedEx->userSkt);

        if (!gqSucces || (dwIoSize == 0 && pOverlappedEx->taskType != TaskType::ACCEPT)) { // User Disconnect
            p_RedisManager->Disconnect(pOverlappedEx->userSkt);
            std::cout << "socket " << pOverlappedEx->userSkt << " Disconnect && Data Update Fail" << std::endl;
            UserCnt.fetch_sub(1); // UserCnt -1
            CloseSocket(connUser);
            continue;
        }

        if (pOverlappedEx->taskType == TaskType::ACCEPT) { // User Connect
            if (connUser) {
                if (connUser->BindUser()) {
                    UserCnt.fetch_add(1); // UserCnt +1
                    std::cout << "socket " << pOverlappedEx->userSkt << " Connect" << std::endl;
                }
                else { // Bind Fail
                    CloseSocket(connUser, true);
                    connUser->Reset(); // Reset ConnUser
                    AcceptQueue.push(connUser);
                }
            }
        }
        else if (pOverlappedEx->taskType == TaskType::RECV) {
            p_RedisManager->PushRedisPacket(pOverlappedEx->userSkt, dwIoSize, connUser->GetRecvBuffer()); // Proccess In Redismanager
            connUser->ConnUserRecv(); // Wsarecv Again
        }
        else if (pOverlappedEx->taskType == TaskType::SEND) {

            connUser->SendComplete();
        }
    }
}

void QuokkaServer::AccepterThread() {
    ConnUser* connUser = nullptr;
    while (AccepterRun) {
        if (AcceptQueue.pop(connUser)) { // AcceptQueue not empty
            if (!connUser->PostAccept(ServerSKT)) {
                AcceptQueue.push(connUser);
            }
        }
        else { // AcceptQueue empty
            if (WaittingQueue.pop(connUser)) { // WaittingQueue not empty
                if (!connUser->PostAccept(ServerSKT)) {
                    AcceptQueue.push(connUser);
                }
            }
            else { // WaittingQueue empty
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }
}