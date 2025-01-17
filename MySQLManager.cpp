#include "MySQLManager.h"

void MySQLManager::Run() {
	mysql_init(&Conn);
	ConnPtr = mysql_real_connect(&Conn, "127.0.0.1", "quokka", "1234", "Quokka", 3306, (char*)NULL, 0);

	if (ConnPtr == NULL) std::cout << "MySQL ���� ����" << std::endl; // mysql ���� ����
	else std::cout << "MySQL ���� ����" << std::endl; // mysql ���� ����

	CreateMySQLThread();
}

void MySQLManager::MySQLThread() {
	while (mySQLRun) {
		
	}
}

bool MySQLManager::CreateMySQLThread() {
	mySQLThread = std::thread([this]() {MySQLThread();});
	std::cout << "Create MySQLThread" << std::endl;
	return true;
}

void MySQLManager::CloseMySQL() { // mysql ����
	mysql_close(ConnPtr);

}