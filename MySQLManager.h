#pragma once
#include <mysql.h>
#include <string>
#include <iostream>
#pragma comment (lib, "libmysql.lib") // mysql ¿¬µ¿

class MySQLManager {
public:
	void Run(); // Connect Mysql
	void CloseMySQL(); // Close Mysql + End Mysql Thread

private:
	void ProcMysqlPacket(); // Process Mysql Packet
	void PushMysqlPacket(); // Push Mysqlk Packet

private:
	// 4 bytes
	int MysqlResult;

	// 8 bytes
	MYSQL_ROW Row;

	// 104 bytes
	MYSQL_RES* Result;

	// 1096 bytes
	MYSQL Conn;
	MYSQL* ConnPtr = NULL;
};