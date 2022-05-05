#include "../include/myhead.h"

myDB::myDB()
{
}

myDB::~myDB()
{
}

std::vector<std::vector<std::string>> myDB::readDB(const char* query)
{
	std::vector<std::string> res_temp;
	std::vector<std::vector<std::string>> res;

	dbpool* pool = dbpool::GetInstance();
	MYSQL* mysql = pool->GetConnMYSQL();

	if (mysql_query(mysql, query))
	{
		// 如果没有抛异常 表示不是服务器断开连接
		MYSQL* temp = checkReConnetion(mysql, query);
		if (nullptr == temp)
		{
			char buf[1024];
			memset(buf, '\0', 1024);
			sprintf(buf, "mysql readData query failed!mysql_errno:%d", mysql_errno(mysql));
			pool->FreeConnMYSQL(mysql);
			throw myException(buf);
		}
		mysql = temp;
	}

	MYSQL_RES* result = mysql_store_result(mysql);
	if (!result)
	{
		char buf[1024];
		memset(buf, '\0', 1024);
		sprintf(buf, "mysql readData query for NULL");
		pool->FreeConnMYSQL(mysql);
		throw myException(buf);
	}
	pool->FreeConnMYSQL(mysql);

	MYSQL_ROW row;
	unsigned long row_length = mysql_num_fields(result);

	while ((row = mysql_fetch_row(result)) != NULL)
	{
		res_temp.clear();
		for (unsigned long i = 0; i < row_length; i++)
		{
			res_temp.push_back(row[i]);
		}
		res.push_back(res_temp);
	}

	mysql_free_result(result);
	return res;
}

void myDB::writeDB(const char* query)
{
	dbpool* pool = dbpool::GetInstance();
	MYSQL* mysql = pool->GetConnMYSQL();
	if (mysql_query(mysql, query))
	{
		// 如果没有抛异常 表示不是服务器断开连接
		MYSQL* temp = checkReConnetion(mysql, query);
		if (nullptr == temp)
		{
			char buf[1024];
			memset(buf, '\0', 1024);
			sprintf(buf, "mysql readData query failed!mysql_errno:%d", mysql_errno(mysql));
			pool->FreeConnMYSQL(mysql);
			throw myException(buf);
		}
		mysql = temp;
	}
	pool->FreeConnMYSQL(mysql);
}

MYSQL* myDB::checkReConnetion(MYSQL* mysql, const char* query)
{
	if (mysql_ping(mysql))
	{
		dbpool* pool = dbpool::GetInstance();
		if (mysql_reset_connection(mysql))
		{
			printf("mysql_reset_connection failed!,mysql_errno:%d", mysql_errno(mysql));
			char ch[1024] = { '\0' };
			sprintf(ch, "mysql_reset_connection failed!,mysql_errno:%d", mysql_errno(mysql));
			// 保证连接池数量一直相同
			pool->FreeConnMYSQL(pool->pool_create_connection());
			throw myException(ch);
		}
		if (query != nullptr && mysql_query(mysql, query))
		{
			char buf[1024];
			memset(buf, '\0', 1024);
			sprintf(buf, "mysql readData query failed!mysql_errno:%d", mysql_errno(mysql));
			pool->FreeConnMYSQL(mysql);
			throw myException(buf);
		}
		return mysql;
	}
	return nullptr;
}

