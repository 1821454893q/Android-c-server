#ifndef _DB_POOL_H_
#define _DB_POOL_H_
#include <mysql/mysql.h>
#include <queue>
#include "locker.h"
#include "myException.h"

class dbpool
{
public:
	static dbpool* GetInstance(); // 单例

	MYSQL* GetConnMYSQL() // 获取数据库连接对象
	{
		m_queueStat.wait();
		m_queueLock.lock();
		if (m_connQue.empty())
		{
			m_queueLock.unlock();
			return nullptr;
		}
		MYSQL* mysql = m_connQue.front();
		m_connQue.pop();
		m_queueLock.unlock();
		return mysql;
	}
	void FreeConnMYSQL(MYSQL* mysql) // 释放数据库连接对象
	{
		m_queueLock.lock();
		m_connQue.push(mysql);
		m_queueStat.post();
		m_queueLock.unlock();
	}

	void init(const char* host, const char* user, const char* passwd, const char* database, int port, int poolSize)
	{
		for (int i = 0; i < poolSize; i++)
		{
			MYSQL* mysql = new MYSQL;
			mysql_init(mysql);
			if (!mysql_real_connect(mysql, host, user, passwd, database, port, NULL, 0))
			{
				printf("mysql connection failed!,mysql_errno:%d", mysql_errno(mysql));
				char ch[1024] = { '\0' };
				sprintf(ch, "mysql connection failed!,mysql_errno:%d", mysql_errno(mysql));
				throw myException(ch);
			}
			m_connQue.push(mysql);
			m_queueStat.post();
		}
	}

	MYSQL* pool_create_connection()
	{
		MYSQL* mysql = new MYSQL;
		mysql_init(mysql);
		if (!mysql_real_connect(mysql, m_host.c_str(), m_user.c_str(), m_passwd.c_str(), m_database.c_str(), m_port, NULL, 0))
		{
			delete[] mysql;
			return nullptr;
		}
		return mysql;
	}

	void close()
	{
	}
private:
	dbpool()
	{
		m_host = "175.178.217.212";
		m_user = "tainanle";
		m_passwd = "123456";
		m_database = "chatserver";
		m_port = 3306;
		m_database_pool_size = 10;
		init(m_host.c_str(), m_user.c_str(), m_passwd.c_str(), m_database.c_str(), m_port, m_database_pool_size);
	}
	~dbpool()
	{

	}
private:
	static locker m_queueLock;
	static sem m_queueStat;
	static dbpool* g_pool;
	std::queue<MYSQL*> m_connQue; // 连接池队列

	std::string m_host, m_user, m_passwd, m_database;
	int m_port;
	int m_database_pool_size;
};
#endif // _DB_POOL_H_
