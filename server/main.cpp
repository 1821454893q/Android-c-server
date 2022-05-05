#include "include/myhead.h"

#define EVENT_NUMBER 10
#define MAX_USER 65535

extern void addfd(int epollfd, int fd, bool on_eshot);
extern void removefd(int epollfd, int fd);
extern void modfd(int epollfd, int fd, int ev);

struct fds
{
	int epollfd;
	int sockfd;
};

void* worker(void* msg)
{
	int clientfd = ((fds*)msg)->sockfd;
	int epollfd = ((fds*)msg)->epollfd;
	delete (fds*)msg;
	int ret = 0;
	char buf[1024];
	while (1)
	{
		memset(buf, '\0', 1024);
		ret = recv(clientfd, buf, 1024, 0);
		if (ret < 0)
		{
			if (errno == EAGAIN)
			{
				modfd(epollfd, clientfd, EPOLLIN);
				break;
			}
			printf("read failed!clientfd:%d\n", clientfd);
			break;
		}
		else if (ret == 0)
		{
			printf("read over!clientfd:%d\n", clientfd);
		}
		send(clientfd, buf, ret, 0);
		printf("%s", buf);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc <= 2)
	{
		printf("usage: %s ip_address port_number\n", basename(argv[0]));
		return 1;
	}

	const char* ip = argv[1];
	size_t port = atoi(argv[2]);
	threadpool<logicProcess>* pool;
	try
	{
		pool = new threadpool<logicProcess>;
	}
	catch (const std::exception& e)
	{
		printf("create thread failed!\n");
		return 1;
	}

	logicProcess* proess = new logicProcess[MAX_USER];

	sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);

	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);

	int ret = bind(listenfd, (sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	ret = listen(listenfd, 5);
	assert(ret != -1);

	epoll_event events[EVENT_NUMBER];
	int epollfd = epoll_create(5);
	assert(epollfd != -1);
	addfd(epollfd, listenfd, false);

	while (true)
	{
		int number = epoll_wait(epollfd, events, EVENT_NUMBER, -1);
		if ((number < 0) && (errno != EINTR))
		{
			printf("epoll failure!\n");
			break;
		}

		for (int i = 0; i < number; i++)
		{
			if (events[i].data.fd == listenfd)
			{
				sockaddr_in clientServer;
				socklen_t clientLen = sizeof(clientServer);
				int clientfd = accept(listenfd, (sockaddr*)&clientServer, &clientLen);
				if (clientfd < 0)
				{
					printf("errno is:%d\n", errno);
					continue;
				}
				addfd(epollfd, clientfd, true);
				proess[clientfd].init(epollfd,clientfd);
			}
			else if (events[i].events & EPOLLIN)
			{
				//fds* epollfd_clientfd = new fds;
				//epollfd_clientfd->epollfd = epollfd;
				//epollfd_clientfd->sockfd = events[i].data.fd;
				//pthread_t thread;
				//pthread_create(&thread, NULL, worker, (void*)epollfd_clientfd);
				// ·ÅÖÃÌí¼ÓÇëÇó
				pool->append(&proess[events[i].data.fd]);
			}
			if (events[i].events & EPOLLRDHUP)
			{
				removefd(epollfd, events[i].data.fd);
				close(events[i].data.fd);
				proess[events[i].data.fd].close();
			}
		}
	}

	return 0;
}