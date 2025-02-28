#include "SQL.h"

SQL::SQL(const char* ip, const char* user, const char* password, const char* table, const int port)
{
	// 초기화
	mysql_init(&_conn);

	// DB 연결
	_connection = mysql_real_connect(&_conn, ip, user, password, table, port, (char*)NULL, 0);
	if (_connection == NULL)
	{
		DebugBreak();
	}
}

SQL::~SQL()
{
	mysql_close(_connection);
}

MYSQL_RES* SQL::ExecQuery(const char* query)
{
	int query_stat = mysql_query(_connection, query);
	if (query_stat != 0)
	{	
		printf("Mysql query error : %s", mysql_error(&_conn));
		return nullptr;
	}

	MYSQL_RES* sql_result = mysql_store_result(_connection);
	mysql_free_result(sql_result);
	return sql_result;
}
