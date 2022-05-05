#include "../include/logicProcess.h"

std::unordered_map<std::string, int> logicProcess::m_usersfd;
locker logicProcess::m_mutex;

int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

void addfd(int epollfd, int fd, bool on_eshot)
{
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	if (on_eshot)
	{
		event.events |= EPOLLONESHOT;
	}
	setnonblocking(fd);
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

void modfd(int epollfd, int fd, int ev)
{
	epoll_event event;
	event.data.fd = fd;
	event.events = ev | EPOLLONESHOT | EPOLLET | EPOLLRDHUP;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void removefd(int epollfd, int fd)
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

logicProcess::logicProcess()
{
	m_epollfd = -1;
	m_clientfd = -1;

	// read
	m_readBuffer = NULL;
	m_readIdx = 0;
	m_readStatus = requestStatus::NONE;
	m_readCpt = 0;

	// chat
	m_chatBuffer = NULL;
	m_chatLength = 0;

	// mysql
	m_db = NULL;
}

logicProcess::~logicProcess()
{
}

void logicProcess::init(int epollfd, int clientfd)
{
	this->m_epollfd = epollfd;
	this->m_clientfd = clientfd;
	printf("fd:%d client connection!\n", clientfd);
	m_readBuffer_lock.lock();
	// read
	m_readBufferMax = 1024;
	m_readBuffer = new char[m_readBufferMax];
	m_readIdx = 0;
	m_readStatus = requestStatus::NONE;
	m_readCpt = 0;

	// userAccount
	m_account.clear();
	m_password.clear();

	// chat
	m_chatBuffer = NULL;
	m_chatLength = 0;

	// mysql
	m_db = new myDB();
	m_readBuffer_lock.unlock();
}

void logicProcess::process()
{
#if 0
	const int bufferSize = 1024;
	char* buffer = new char[bufferSize];
	int ret = 0;

	while (true)
	{
		memset(buffer, '\0', bufferSize);
		ret = recv(m_clientfd, buffer, bufferSize, 0);
		if (ret < 0)
		{
			if (errno == EAGAIN)
			{
				printf("read EAGAIN!clientfd:%d\n", m_clientfd);
				modfd(m_epollfd, m_clientfd, EPOLLIN);
				break;
			}
			printf("read failed!clientfd:%d\n", m_clientfd);
			break;
		}
		else if (ret == 0)
		{
			printf("read over!clientfd:%d\n", m_clientfd);
		}
		send(m_clientfd, buffer, ret, 0);
		printf("%s\n", buffer);
	}

	delete[] buffer;
#endif
	// 读取客户端的请求,根据请求进行分析
	try
	{
		processRead();
		switch (readAnalysis())
		{
		case logicProcess::requestStatus::NONE:
			printf("-------------requestStatus::NONE----------------------\n");
			throw myException("Request format error!status failed\n");
			break;
		case logicProcess::requestStatus::LOGIN:
			printf("-------------requestStatus::LOGIN----------------------\n");
			login();
			printf("user:%s\t login..\n", m_account.c_str());
			break;
		case logicProcess::requestStatus::REGISTER:
			printf("-------------requestStatus::REGISTER----------------------\n");
			regis();
			break;
		case logicProcess::requestStatus::CHAT:
			printf("-------------requestStatus::CHAT----------------------\n");
			if (m_account.empty())
				throw myException("you not login,pleas login!\n");
			chat();
			break;
		case logicProcess::requestStatus::RELEASE:
			if (m_account.empty())
				throw myException("you not login,pleas login!\n");
			printf("-------------requestStatus::RELEASE----------------------\n");
			release();
			break;
		case logicProcess::requestStatus::GETRELEASE:
			if (m_account.empty())
				throw myException("you not login,pleas login!\n");
			printf("-------------requestStatus::GETRELEASE----------------------\n");
			getRelease();
			break;
		default:
			break;
		}
	}
	catch (const myException& e)
	{
		send(m_clientfd, e.what(), e.size(), 0);
		printf(e.what());
	}
	catch (const std::exception& e)
	{
		printf(e.what());
	}
}

void logicProcess::close()
{
	printf("fd:%d client close!\n", m_clientfd);
	m_epollfd = -1;
	m_clientfd = -1;
	m_readBuffer_lock.lock();
	if (m_readBuffer != NULL)
	{
		delete[] m_readBuffer;
		m_readBuffer = NULL;
		m_chatBuffer = NULL;
		m_chatLength = 0;
	}
	if (m_db != NULL)
	{
		delete m_db;
		m_db = NULL;
	}
	m_usersfd.erase(m_account);
	m_account.clear();
	m_password.clear();
	m_readBuffer_lock.unlock();
}

void logicProcess::processRead()
{
	int ret = 0;
	m_readBuffer_lock.lock();
	if (m_readBuffer == NULL)
	{
		m_readBuffer_lock.unlock();
		throw myException("m_ReadBuffer is NULL!");
	}
	memset(m_readBuffer, '\0', m_readBufferMax);
	m_readIdx = 0;
	m_readCpt = 0;
	m_readBuffer_lock.unlock();
	while (1)
	{
		ret = recv(m_clientfd, m_readBuffer + m_readIdx, m_readBufferMax - m_readIdx, 0);
		if (ret < 0)
		{
			if (errno == EAGAIN)
			{
				printf("recv is EAGAIN, fd%d\n", m_clientfd);
				break;
			}
			char err[1024];
			sprintf(err, "recv return is less 0, fd:%d\terrno%d\n", m_clientfd, errno);
			throw myException(err);
		}
		else if (ret == 0)
		{
			break;
		}
		else if (ret == m_readBufferMax - m_readIdx)
		{
			m_readIdx += ret;
			m_readBufferMax = ((2 * m_readBufferMax));
			printf("read is MAX, fd%d\nnow readBufferMax:%d\n", m_clientfd, m_readBufferMax);
			char* temp = new char[m_readBufferMax];
			memset(temp, '\0', m_readBufferMax);
			strcpy(temp, m_readBuffer);
			delete[] m_readBuffer;
			m_readBuffer = temp;
			continue;
		}
		m_readIdx += ret;
		break;
	}
	modfd(m_epollfd, m_clientfd, EPOLLIN);
}


logicProcess::requestStatus logicProcess::readAnalysis()
{
	std::string temp(m_readBuffer);
	//数据格式为以下形式 空格为结尾!
	//status:login,account:tainanle,passwd:haoge123 
	// s 000
	//for (; m_readCpt < m_readIdx; m_readCpt++)
	//{
	//	if (m_readBuffer[m_readCpt] == ' ')
	//	{
	//		//遇到空格 说明一条请求已经读取完毕
	//		break;
	//	}
	//	temp += m_readBuffer[m_readCpt];
	//}
	printf("readBuffer:%s\n", temp.c_str());
	//// 把处理完成的地方弄成放弃掉
	//m_readBuffer = m_readBuffer + m_readCpt + 1;
	//m_readBufferMax = m_readBufferMax - m_readCpt - 1;
	//m_readIdx = m_readIdx - m_readCpt - 1;
	//m_readCpt = 0;

	//if (m_readCpt == m_readIdx && m_readBuffer[m_readCpt] != ' ')
	//{
	//	send(m_clientfd, "Request format error,No spaces!\n", 32, 0);
	//	throw myException("Request format error,No spaces!");
	//}

	std::vector<std::string> clientStatus = mySplit(temp, ',');
	if (clientStatus.empty())
	{
		throw myException("Request format error : ','!");
	}

	for (std::string str : clientStatus)
	{
		std::vector<std::string> sstr = mySplit(str, ':');
		if (sstr.size() != 2)
		{
			throw myException("Request format error : ':'!");
		}

		if (sstr[0] == "status")
		{
			if (sstr[1] == "login")
			{
				m_readStatus = requestStatus::LOGIN;
			}
			else if (sstr[1] == "register")
			{
				m_readStatus = requestStatus::REGISTER;
			}
			else if (sstr[1] == "chat")
			{
				m_readStatus = requestStatus::CHAT;
				break;
			}
			else if (sstr[1] == "release")
			{
				m_readStatus = requestStatus::RELEASE;
				j_release(temp);
				break;
			}
			else if (sstr[1] == "getRelease")
			{
				m_readStatus = requestStatus::GETRELEASE;
				break;
			}
		}
		else if (sstr[0] == "account")
		{
			m_account = sstr[1];
		}
		else if (sstr[0] == "passwd")
		{
			m_password = sstr[1];
		}
	}

	return m_readStatus;
}

void logicProcess::login()
{
	// 登录首先获取数据库的账号信息 核对是否有该账号

	std::vector<std::vector<std::string>> accData =
		m_db->readDB("select userAccount,userPassword,id from user_account where isDelete=0");

	Md5Encode md5;
	std::string safetyPassword = md5.Encode(m_password);

	for (size_t i = 0; i < accData.size(); i++)
	{
		// 判断账号密码 与 是否逻辑删除  后面可添加是否封号
		if (accData[i][0] == m_account && accData[i][1] == safetyPassword)
		{

			//有该账号
			send(m_clientfd, "status:success", 14, 0);
			m_readBuffer_lock.lock();
			m_usersfd.insert({ m_account,m_clientfd });
			m_readBuffer_lock.unlock();
			m_id = atol(accData[i][2].c_str());
			// 可以返回给客户端已经有多少用户连接了 并且可以返回一共发布了什么内容
			return;
		}
	}
	// 无该账号
	m_account.clear();
	m_password.clear();
	throw myException("status:fail\n");
}

void logicProcess::regis()
{
	// 登录首先获取数据库的账号信息 核对是否有该账号
	std::vector<std::vector<std::string>> accData = m_db->readDB("select userAccount from user_account where isDelete=0");
	for (std::vector<std::string> strs : accData)
		for (std::string str : strs)
		{
			if (m_account == str)
			{
				//有该账号 注册失败
				send(m_clientfd, "status:fail", 11, 0);
				return;
			}
		}

	// 无该账号 登记该注册信息
	char ch[1024] = { '\0' };
	// 密码加密
	Md5Encode md5;
	std::string safetyPassword = md5.Encode(m_password);
	sprintf(ch, "insert into user_account(userAccount,userPassword) values (\"%s\",\"%s\")", m_account.c_str(), safetyPassword.c_str());
	printf("regis:%s\n", ch);
	m_db->writeDB(ch);
	send(m_clientfd, "status:success", 14, 0);

}

void logicProcess::chat()
{

	// 聊天之前 先处理一下 status:chat,<user>用户名</user><msg>消息</msg>
	std::string msg(m_readBuffer);

	size_t lu = msg.find("<user>");
	if (lu == std::string::npos)
		throw myException("Request chat format error : not find <user>!");
	size_t ru = msg.find("</user>");
	if (ru == std::string::npos)
		throw myException("Request chat format error : not find </user>!");
	size_t lm = msg.find("<msg>");
	if (lm == std::string::npos)
		throw myException("Request chat format error : not find <msg>!");
	size_t rm = msg.find("</msg>");
	if (rm == std::string::npos)
		throw myException("Request chat format error : not find </msg>!");

	// 匹配现有登录账号 查看该联系人是否登录中 未登录的DOTO
	lu += 6;
	std::string account = msg.substr(lu, ru - lu);
	std::unordered_map<std::string, int>::iterator it = m_usersfd.find(account);
	char buf[128];
	memset(buf, '\0', 128);
	if (it == m_usersfd.end())
	{
		// 该用户未登录
		sprintf(buf, "%s not login ...\n", account.c_str());
		throw myException(buf);
	}

	// 向该用户发送消息 status:chat,<user>用户名</user><msg>消息</msg>
	lm += 5;
	std::string sendMsg = "status:chat,<user>" + m_account + "</user><msg>" + msg.substr(lm, rm - lm) + "</msg>";
	printf((sendMsg + "\n").c_str());
	send(it->second, sendMsg.c_str(), sendMsg.size(), 0);

	memset(buf, '\0', 128);
	sprintf(buf, "status:success,sendUser:%s\n", account.c_str());
	// 发送完毕 用异常给用户回应一下发送成功了
	throw myException(buf);


}

std::vector<std::string> logicProcess::mySplit(std::string str, char wc)
{
	std::string temp;
	std::vector<std::string> res;
	for (char var : str)
	{
		if (var == wc)
		{
			if (!temp.empty())
				res.push_back(temp);
			temp.clear();
		}
		else
		{
			temp += var;
		}
	}
	res.push_back(temp);
	return res;
}

void logicProcess::j_release(std::string& msg)
{
	// 我规定: title内容 必须@title@ @title@包裹 context 必须@context@ @context@ 否则格式错误 而且内容中不可出现一模一样的关键字
	int title_length = sizeof("@title@") - 1;
	int context_length = sizeof("@context@") - 1;

	int title_l = msg.find("@title@");
	if (title_l == -1) throw myException("Request format error!");
	int title_r = msg.find("@title@", title_l + 1);
	if (title_r == -1) throw myException("Request format error!");
	m_releaseTitle = msg.substr(title_l + title_length, title_r - title_l - title_length);

	int context_l = msg.find("@context@");
	if (context_l == -1) throw myException("Request format error!");
	int context_r = msg.find("@context@", context_l + 1);
	if (context_r == -1) throw myException("Request format error!");
	m_releaseContext = msg.substr(context_l + context_length, context_r - context_l - context_length);
}

void logicProcess::release()
{
	//insert into releaseInfo(acc_id,r_title,r_context) values(1,'title测试','context测试');
	int size = m_releaseContext.size() + m_releaseTitle.size() + 200;
	char buf[size];
	memset(buf, '\0', size);
	sprintf(buf, "insert into release_info(acc_id,r_title,r_context) values(%ld,'%s','%s')", m_id,
		m_releaseTitle.c_str(), m_releaseContext.c_str());
	m_db->writeDB(buf);
	send(m_clientfd, "status:success", 14, 0);
}

void logicProcess::getRelease()
{
	//获取所有发布信息
	std::vector<std::vector<std::string>> data =
		m_db->readDB("select r.id,userAccount,r_title,r_context,hot,r.createTime \
			from user_account a,release_info r where r.acc_id=a.id and r.isDelete=0 and a.isDelete=0;");
	char buf[1024];
	memset(buf, '\0', 1024);
	int size = sprintf(buf, "status:getRelease,size:%ld\r\n", data.size());
	send(m_clientfd, buf, size, 0);
	std::string result;
	for (std::vector<std::string> v : data)
	{
		result.clear();

		result += "<id>";
		result += v[0];
		result += "</id>";

		result += "<account>";
		result += v[1];
		result += "</account>";

		result += "<title>";
		result += v[2];
		result += "</title>";

		result += "<context>";
		result += v[3];
		result += "</context>";

		result += "<hot>";
		result += v[4];
		result += "</hot>";

		result += "<releaseDate>";
		result += v[5];
		result += "</releaseDate>\r\n";

		send(m_clientfd, result.c_str(), result.size(), 0);
	}
}
