#include "QuokkaServer.h"

QuokkaServer::QuokkaServer() {}
QuokkaServer::~QuokkaServer() {
    WSACleanup();
}

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

    sIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MaxThreadCnt+1);
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

bool QuokkaServer::StartWork(UINT32 maxClientCount_) {
    bool check = CreateWorkThread();
    if (!check) {
        std::cout << "WorkThread 생성 실패" << std::endl;
        return false;
    }

    for (int i = 0; i < maxClientCount_; i++) { // UserPool 풀 생성
        SOCKET TempSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
        
        if (TempSkt == INVALID_SOCKET) {
            std::cout << "Client socket Error : " << GetLastError() << std::endl;
            return false;
        }

        auto TempConnUser = std::make_unique<ConnUser>(TempSkt);
        ConnUsers.emplace_back(std::move(TempConnUser));
    }

    std::cout << "Created UserPool : " << ConnUsers.size() << std::endl;

    p_RedisManager->Run(MaxThreadCnt-1); // Run Redis Threads (The number of mater nodes)

    maxClientCount = maxClientCount_;
    WorkThread();
    return true;
}

bool QuokkaServer::CreateWorkThread() {
    auto threadCnt = MaxThreadCnt + 1; // core +1
    for (int i = 0; i < threadCnt; i++) {
        WorkThreads.emplace_back([this]() { WorkThread(); });
    }
    std::cout << "WorkThread start" << std::endl;
    return true;
}

bool QuokkaServer::CreateAccepterThread() {
    AcceptThread = std::thread([this]() { AccepterThread(); });
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

        if (!gqSucces || (dwIoSize == 0 && pOverlappedEx->taskType != TaskType::ACCEPT)) {
            std::cout << "socket " << connUser->GetSktNum() << " Connection Lost" << std::endl;

            UserCnt.fetch_sub(1); // UserCnt -1 (atomic)
            CloseSocket(connUser);
            continue;
        }

        if (pOverlappedEx->taskType == TaskType::ACCEPT) {
            connUser = GetClientInfo(pOverlappedEx->taskType);
            std::cout << "User Accept req" << std::endl;

            if (connUser->BindUser()) {
                if (connUser->ConnUserRecv()) {
                    UserCnt.fetch_add(1); // UserCnt +1
                    OnConnect(pOverlappedEx->UserIdx);
                    std::cout << "socket " << connUser->GetSktNum() << " Connect" << std::endl;
                }
            }

            else {
                CloseSocket(connUser, true);
            }

        }
        else if (pOverlappedEx->taskType == TaskType::RECV) {
            
        }
        else if (pOverlappedEx->taskType == TaskType::SEND) {
            
        }
    }
}

void QuokkaServer::AccepterThread() {
    while (AccepterRun) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        for (int i = 0; i < maxClientCount; i++) {
            if (ConnUsers[i]->IsConn()) continue; // User Connection check

            ConnUsers[i]->PrepareAccept(ServerSKT); // Prepare Accept
        }
    }
}
