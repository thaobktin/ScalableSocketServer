#pragma once
// Copyright (c) Shaun O'Kane. 2012

template<typename T, typename allocator_T=Virtual_allocator>
class dedicated_heap
{
	struct T_ex 
	{
		char block[sizeof(T)];
		T_ex* next;
	};
	size_t capacity_;

	std::unique_ptr<allocator_T> allocator;
	SRWLOCK lock;

	T_ex* heap = nullptr;
	T_ex* free_ = nullptr;
public:
	dedicated_heap()
		: heap(nullptr),
		  allocator(new allocator_T)
	{
		InitializeSRWLock(&lock);
	}

	~dedicated_heap() 
	{
		for(size_t i=0; i<capacity_; i++)
		{
			T* p = reinterpret_cast<T*>(&heap[i]);
			p->~T();
		}
	}

	void allocate(size_t capacity = 100)
	{
		capacity_ = capacity;
		size_t bytes = capacity*sizeof(T_ex);
		heap = (T_ex*)allocator->allocate(bytes);
		assert(heap != nullptr);
		free_ = heap;
		for(size_t i=0; i<capacity; i++)
		{
			new ((void*)&heap[i]) T;

			if(i != capacity-1)
				heap[i].next = &heap[i+1];
			else
				heap[i].next = nullptr;
		}
	}

	size_t capacity() const { return capacity_; }

	T* get()
	{
		if(free_ == nullptr) 
			return nullptr;  // no slots left;
		::AcquireSRWLockExclusive(&lock);
		T_ex* p = free_;
		free_ = p->next;
		p->next = nullptr; // safety measure.
		::ReleaseSRWLockExclusive(&lock);
		return (T*)p;
	}

	void release(T* p)
	{
		if(p != nullptr)
		{
			T_ex* q = reinterpret_cast<T_ex*>(p);
			::AcquireSRWLockExclusive(&lock);
			q->next = free_;
			free_ = q;
			::ReleaseSRWLockExclusive(&lock);
		}
	}

	size_t free_space() const
	{
		size_t cnt = 0;
		T_ex* p = free_;
		while(p != nullptr)
		{
			cnt++;
			p = p->next;
		}
		return cnt;
	}

	// Kludge - currently used to initialise objects, should pass lambda into allocate instead.
	// Should not be used after heap has been modified (as then we don't know where the heap begins)
	void visit_all(std::function<void(T*)> fn)
	{
		T_ex* q = reinterpret_cast<T_ex*>(heap);
		while (q != nullptr)
		{ 
			T* p = reinterpret_cast<T*>(q);
			fn(p);
			q = q->next;
		}
	}
};


