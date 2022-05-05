#include "../include/db_pool.h"
locker dbpool::m_queueLock;
sem dbpool::m_queueStat;
dbpool* dbpool::g_pool = nullptr;

dbpool* dbpool::GetInstance()
{
	{
		if (g_pool == nullptr)
		{
			// ¼ÓËø
			m_queueLock.lock();
			if (g_pool == nullptr)
				g_pool = new dbpool();
			m_queueLock.unlock();
		}
		return g_pool;
	}
}
