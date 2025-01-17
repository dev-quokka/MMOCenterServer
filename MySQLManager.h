#pragma once
#include <mysql.h>
#include <string>
#include <iostream>
#include <thread>
#pragma comment (lib, "libmysql.lib") // mysql ¿¬µ¿

class MySQLManager {
public:
	void Run(); // Connect Mysql
	void CloseMySQL(); // Close Mysql + End Mysql Thread

private:
	void PushMysqlPacket(); // Push Mysqlk Packet
	bool CreateMySQLThread();
	void MySQLThread();

	// 1 bytes
	bool mySQLRun = false;

	// 4 bytes
	int MysqlResult;

	// 8 bytes
	MYSQL_ROW Row;

	// 16 bytes
	std::thread mySQLThread;

	// 104 bytes
	MYSQL_RES* Result;

	// 1096 bytes
	MYSQL Conn;
	MYSQL* ConnPtr = NULL;
};