#pragma once
#include "pch.h"

/***************************

		     SQL

***************************/
// - MySQL에 연결하여 쿼리를 보내기 위한 용도로 분리한 클래스

class SQL
{
public:
	SQL(const char* ip, const char* user, const char* password, const char* table, const int port);
	~SQL();

	MYSQL_RES* ExecQuery(const char* query);

private:
	MYSQL	_conn;
	MYSQL*	_connection = NULL;
};