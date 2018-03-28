#include "stdafx.h"
// Copyright (c) Shaun O'Kane. 2012
//
// Quick and dirty thread pool.
// 1. The number of threads is specified at start-up.
// 2. Add thread_pool_tasks.
// 3. Call stop() to stop it! It doesn't finish all the tasks in the queue.
//
using namespace std::chrono;

typedef std::function<void()> thread_pool_task;

class thread_pool
{
private:
	thread_pool(const thread_pool&);
public:
	typedef enum class state { stopped, running, stopping } state_t;

	thread_pool()
		: state(state::stopped),
		finish_queued_tasks(true)
	{
	}

	void start_up(size_t no_of_threads)
	{
		state = state::stopped;
		threads.reserve(no_of_threads);
		for(size_t i=0; i<no_of_threads; i++)
			threads.push_back(new std::thread(process_tasks, this));
		state = state::running;
	}

	~thread_pool() { stop(); }

	bool add_task(thread_pool_task t)
	{
		if(stopping())
			return false;
		{
			std::unique_lock<std::mutex> lock(mtx);
			Queue.push(t);
		}
		cv.notify_one();
		return true;
	}

	void immediate_stop()	
	{
		finish_queued_tasks = false;
		stop();
	}

	// Does not return until all the threads have completed.
	void stop()
	{
		if(state == state::running)
		{
			state = state::stopping;
			cv.notify_all();
			for(auto t : threads) 
			{
				try
				{
					t->join();
					delete t;
				}
				catch(std::runtime_error&)
				{
					assert(false);
				}
			}
			state = state::stopped;
		}
	}

	bool stopping() const { return state == state::stopping; }
	bool stopped() const { return state == state::stopped; }
protected:
	std::queue<thread_pool_task> Queue;
	std::vector<std::thread*> threads;
	std::mutex mtx;
	std::condition_variable cv;
	state_t state;
	bool finish_queued_tasks;

	// This method waits for a task to appear in the Queue and then consumes it, providing we are not stopping.
	void consume_task()
	{
		thread_pool_task t;
		{
			std::unique_lock<std::mutex> _lock(mtx);
			cv.wait(_lock, [this] { return !Queue.empty() || stopping(); }); 
			if(!Queue.empty())
			{
				t = Queue.front();
				Queue.pop();
			}
		}
		if(t)
			t();
	}

	// This is the function that executes on each thread while the thread pool is running.
	static void process_tasks(thread_pool* pool) // using a reference does not compile. BUG in VC++?
	{
		for(;;)
		{
			if(pool->stopping() && (!pool->finish_queued_tasks || pool->Queue.empty()))
				break;
			pool->consume_task();
		}
	}
};