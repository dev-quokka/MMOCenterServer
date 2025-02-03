#pragma once

#include "Define.h"
#include "ConnUser.h"
#include "RedisManager.h"
#include "ConnUsersManager.h"

#include <atomic>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <deque>
#include <queue>
#include <boost/lockfree/queue.hpp>
#include <tbb/concurrent_hash_map.h>

#pragma comment(lib, "ws2_32.lib") // 소켓 프로그래밍용
#pragma comment(lib, "mswsock.lib") // AcceptEx 사용용

class QuokkaServer {
public:
    QuokkaServer(UINT16 maxClientCount_) : maxClientCount(maxClientCount_), AcceptQueue(maxClientCount_), WaittingQueue(maxClientCount_) {}
    ~QuokkaServer() {}

    bool init(const UINT16 MaxThreadCnt_, int port_);
    bool StartWork();

private:
    bool CreateWorkThread();
    bool CreateRedisThread();
    bool CreateAccepterThread();

    void WorkThread(); // IOCP Complete Event Thread
    void RedisThread(); // Redis req Thread
    void AccepterThread(); // Accept req Thread

    void OnConnect(const UINT32 clientIndex_);
    void OnReceive(const UINT32 clientIndex_, const DWORD size_, char* pData_);
    void CloseSocket(ConnUser* connUser, bool isForce_ = false);

    ConnUser* GetClientInfo(TaskType taskType);

    // 1 bytes
    bool WorkRun = true;
    bool AccepterRun = true;
    std::atomic<bool> UserMaxCheck = false;

    // 2 bytes
    UINT16 MaxThreadCnt = 0;
    UINT16 maxClientCount = 0;

    // 4 bytes
    std::atomic<int> UserCnt = 0; //Check Current UserCnt

    // 8 bytes
    SOCKET ServerSKT = INVALID_SOCKET;
    HANDLE sIOCPHandle = INVALID_HANDLE_VALUE;
    std::unique_ptr<RedisManager> p_RedisManager;
    std::unique_ptr<ConnUsersManager> p_ConnUsersManagerManager;

    // 32 bytes
    std::vector<std::thread> workThreads;
    std::thread acceptThread;

    // 136 bytes 
    boost::lockfree::queue<ConnUser*> AcceptQueue; // For Aceept User Queue
    boost::lockfree::queue<ConnUser*> WaittingQueue; // Waitting User Queue
};
