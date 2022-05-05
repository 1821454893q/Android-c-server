#ifndef MYDB_H
#define MYDB_H
#include "myhead.h"

class myDB
{
public:
	myDB();
	~myDB();

public:
	std::vector<std::vector<std::string>> readDB(const char* query);
	void writeDB(const char* query);

private:
	// check connection timeout
	MYSQL*checkReConnetion(MYSQL* mysql, const char* query = nullptr);

private:
	std::string m_host, m_user, m_passwd, m_database;
	int m_port;
	locker m_mutex;
};
#endif // !MYDB_H


