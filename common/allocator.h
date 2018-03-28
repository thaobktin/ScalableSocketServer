// Copyright (c) Shaun O'Kane. 2012
//
// Alternate memory allocation. VirtualAlloc() should provide better performance according to MSDN.
// A lot of code required to find out...
// To demonstrate the effect of locking pages in memory, use Virtual_allocator rather than std_allocator
//
#pragma once

struct allocator_type
{
	virtual void* allocate(size_t bytes) = 0;
};

// This is the standard allocator which uses standard new to allocate memory.
class std_allocator : public allocator_type
{
	std::vector<unsigned char> mem;
public:
	void* allocate(size_t bytes) override
	{
		mem.assign(bytes, 0);
		return &mem.front();
	}
};

// This is only necessary if the allocated virtual memory takes up a significant part of the
// default working set of the application.
inline bool Declare_alloctor_pool_size(SIZE_T total_bytes)  // This affects the application working set
{
	DWORD flags; 
	SIZE_T minimum_working_set_size, maximum_working_set_size;
	{
		BOOL ok = ::GetProcessWorkingSetSizeEx(GetCurrentProcess(), &minimum_working_set_size, &maximum_working_set_size, &flags);
		assert(ok != FALSE); 
	}
	minimum_working_set_size = minimum_working_set_size + total_bytes;
	maximum_working_set_size = maximum_working_set_size + total_bytes;
	flags |= QUOTA_LIMITS_HARDWS_MIN_ENABLE;
	flags &= ~QUOTA_LIMITS_HARDWS_MIN_DISABLE;
	{
		BOOL ok = ::SetProcessWorkingSetSizeEx(GetCurrentProcess(), minimum_working_set_size, maximum_working_set_size, flags);
		assert(ok != FALSE);
		return (ok != FALSE);
	}
}

class Virtual_allocator : public allocator_type
{
	void* mem = nullptr;
	SIZE_T total_bytes = 0;
public:
	void* allocate(size_t bytes) override  
	{ 
		total_bytes = bytes;
		mem = ::VirtualAlloc(NULL, total_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); 
		assert(mem != nullptr);
		BOOL locked = ::VirtualLock(mem, total_bytes);
		if (locked == FALSE)
		{
			::VirtualFree(mem, 0, MEM_RELEASE); 
			return nullptr;
		}
		::memset(mem, 0, total_bytes);
		return mem;
	}

	~Virtual_allocator() 
	{ 
		if (mem != nullptr && total_bytes > 0)
		{ 
			BOOL unlocked = ::VirtualUnlock(mem, total_bytes);
			assert(unlocked == TRUE);
			::VirtualFree(mem, 0, MEM_RELEASE); 
		}
	}
};
