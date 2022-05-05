#ifndef LOGICPROCESS_H
#define LOGICPROCESS_H
#include "myhead.h"

class myDB;

class logicProcess
{
public:
	logicProcess();
	~logicProcess();

private:
	enum class requestStatus
	{
		NONE,
		LOGIN, // ��¼����
		REGISTER, // ע������
		CHAT, // ��������
		RELEASE, // ��������
		GETRELEASE //���󷢲���Ϣ
	};

public:
	void init(int epollfd, int clientfd);
	void process();
	void close();

private:
	void processRead();
	requestStatus readAnalysis();
	void login();
	void regis();
	void chat();
	std::vector<std::string> mySplit(std::string str, char wc);
	void j_release(std::string& msg);
	void release();
	void getRelease();

private:
	int m_clientfd, m_epollfd;
	// read
	int m_readBufferMax;// ��������ֵ
	char* m_readBuffer;
	int m_readIdx; //�������±�
	int m_readCpt; //�������±�
	std::list<std::string> m_requestQueue;

	// �˺� ����
	long m_id;
	std::string m_account, m_password;
	requestStatus m_readStatus;
	static std::unordered_map<std::string, int> m_usersfd; // ��¼֮�������û���Ϣ

	// ����
	std::list<int> m_Friendfd;
	const char* m_chatBuffer;
	int m_chatLength;

	// ����
	std::string m_releaseTitle, m_releaseContext;


	// mysql
	myDB* m_db;

	// mutex
	static locker m_mutex;
	locker m_readBuffer_lock;
};
#endif // !logicProcess_h
