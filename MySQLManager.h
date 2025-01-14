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
	MYSQL Conn;
	MYSQL* ConnPtr = NULL;
	MYSQL_RES* Result;
	MYSQL_ROW Row;
	int MysqlResult;
};