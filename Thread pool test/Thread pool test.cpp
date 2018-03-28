#include "stdafx.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "../common/logging.h"
#include "../common/thread_pool.h"
#include "../common/dedicated_heap.h"

using namespace std;
using namespace std::chrono;

static LONG thread_cnt = 0;
static LONG value = 0;
std::mutex mtx;

class sleep_task
{ 
	int ID;
public:

	sleep_task(int _ID)
		: ID(_ID)
	{
	}

	void operator()() 
	{
		{
			unique_lock<std::mutex> lock(mtx);
			Log_display("Thread pool test", "Task " << ID << " executing.");
		}

		InterlockedIncrement(&thread_cnt);
		InterlockedIncrement(&value);
		this_thread::sleep_for(seconds(5));
		InterlockedDecrement(&thread_cnt);
	}
};

void thread_pool_test1()
{
	thread_pool pool;
	
	pool.start_up(4);

	value = 0;

	for(int i=0; i<10; ++i)
		pool.add_task(sleep_task(i));

	for(int i=0; i<10; ++i)
		pool.add_task([]()
				{
					{
						unique_lock<std::mutex> lock(mtx);
						Log_display("Thread pool test", "Lambda task executing.");
					}

					InterlockedIncrement(&thread_cnt);
					InterlockedIncrement(&value);
					this_thread::sleep_for(seconds(5));
					InterlockedDecrement(&thread_cnt);
				});
	
	pool.stop();
	assert(thread_cnt == 0);
	assert(value == 20); // All 20 tasks completed.
}

void thread_pool_test2()
{
	thread_pool pool;
	
	pool.start_up(2);

	value = 0;

	for(int i=0; i<10; ++i)
		pool.add_task([]()
				{
					InterlockedIncrement(&thread_cnt);
					InterlockedIncrement(&value);
					this_thread::sleep_for(seconds(5));
					InterlockedDecrement(&thread_cnt);
				});
	
	pool.immediate_stop();
	assert(value == 2);  // only 2 of the 10 tasks completed.
}

int _tmain(int argc, _TCHAR* argv[])
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF);
	thread_pool_test1();
	thread_pool_test2();
	return 0;
}

