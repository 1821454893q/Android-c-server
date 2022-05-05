#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>

#include "locker.h"


template<typename T>
class threadpool
{
public:
	/// <summary>
	/// initialization threadpool
	/// </summary>
	/// <param name="thread_number">threadpool quantity is thread_number</param>
	/// <param name="max_requests">request queue max allow quantity</param>
	threadpool(int thread_number = 8, int max_requests = 1000);
	~threadpool();

	/// <summary>
	/// add request for request queue
	/// </summary>
	/// <param name="request"></param>
	/// <returns>failed return false,sucess return true</returns>
	bool append(T* request);

private:
	static void* worker(void* msg);
	void run();

private:
	int m_thread_number;
	int m_max_requests;
	pthread_t* m_threads; //describe thread pool array,it size is m_thread_number
	std::list<T*> m_workqueue;
	locker m_queueLocker;
	sem m_queuestat; //are there any tasks to deal with
	bool m_stop; //end thread?
};

template<typename T>
inline threadpool<T>::threadpool(int thread_number, int max_requests) :
	m_thread_number(thread_number),
	m_max_requests(max_requests),
	m_threads(NULL)
{
	if ((thread_number <= 0) || (max_requests <= 0))
	{
		throw std::exception();
	}

	m_threads = new pthread_t[thread_number];
	if (!m_threads)
		throw std::exception();

	for (int i = 0; i < thread_number; i++)
	{
		printf("create the %dth thread\n", i);
		if (pthread_create(m_threads + i, NULL, worker, this) != 0)
		{
			delete[] m_threads;
			throw std::exception();
		}
		if (pthread_detach(m_threads[i]))
		{
			delete[] m_threads;
			throw std::exception();
		}
	}
}

template<typename T>
inline threadpool<T>::~threadpool()
{
	delete[] m_threads;
	m_stop = true;
}

template<typename T>
inline bool threadpool<T>::append(T* request)
{
	m_queueLocker.lock();
	if (m_workqueue.size() > m_max_requests)
	{
		m_queueLocker.unlock();
		return false;
	}
	m_workqueue.push_back(request);
	m_queueLocker.unlock();
	m_queuestat.post();
	return true;
}

template<typename T>
inline void* threadpool<T>::worker(void* msg)
{
	threadpool* pool = (threadpool*)msg;
	pool->run();
	return pool;
}

template<typename T>
inline void threadpool<T>::run()
{
	while (!m_stop)
	{
		m_queuestat.wait();
		m_queueLocker.lock();
		if (m_workqueue.empty())
		{
			m_queueLocker.unlock();
			continue;
		}
		T* request = m_workqueue.front();
		m_workqueue.pop_front();
		m_queueLocker.unlock();
		if (!request)
		{
			continue;
		}
		request->process();
	}
}


#endif // THREADPOOL_H
