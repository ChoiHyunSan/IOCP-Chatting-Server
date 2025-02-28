#pragma once
#include "pch.h"

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