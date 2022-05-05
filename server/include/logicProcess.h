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
		LOGIN, // 登录请求
		REGISTER, // 注册请求
		CHAT, // 聊天请求
		RELEASE, // 发布请求
		GETRELEASE //请求发布信息
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
	int m_readBufferMax;// 缓存的最大值
	char* m_readBuffer;
	int m_readIdx; //读到的下标
	int m_readCpt; //处理到的下标
	std::list<std::string> m_requestQueue;

	// 账号 密码
	long m_id;
	std::string m_account, m_password;
	requestStatus m_readStatus;
	static std::unordered_map<std::string, int> m_usersfd; // 登录之后加入该用户信息

	// 聊天
	std::list<int> m_Friendfd;
	const char* m_chatBuffer;
	int m_chatLength;

	// 发布
	std::string m_releaseTitle, m_releaseContext;


	// mysql
	myDB* m_db;

	// mutex
	static locker m_mutex;
	locker m_readBuffer_lock;
};
#endif // !logicProcess_h
