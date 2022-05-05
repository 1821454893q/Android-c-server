CC=g++
FLAGS=-Wall -std=c++11 -g

INCLUDE=-I/usr/include/mysql -I./ -I./include/
LIBDIR=-L/usr/lib64/mysql

TARGET=run
SRCS=main.cpp source/logicProcess.cpp source/myDB.cpp source/Md5Encode.cpp source/db_pool.cpp
OBJS=$(SRCS:.cpp=.o)

LIBS=-lpthread -lmysqlclient


$(TARGET):$(OBJS)
	$(CC) $(FLAGS) $^ -o $@ $(INCLUDE) $(LIBDIR) $(LIBS)

%.o:%.cpp
	$(CC) $(FLAGS) -c $^ -o $@ $(INCLUDE) $(LIBDIR) $(LIBS)

.PHONY:
clear:
	rm -rf $(OBJS) $(TARGET)
