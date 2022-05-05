#ifndef MYHEAD_H
#define MYHEAD_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <pthread.h>
#include <string>
#include <../include/mysql/mysql.h>
#include <exception>
#include <vector>
#include <unordered_map>
#include <math.h>
#include "threadpool.h"
#include "locker.h"
#include "logicProcess.h"
#include "myException.h"
#include "myDB.h"
#include "Md5Encode.h"
#include "db_pool.h"

#endif // !MYHEAD_H
