#include "stdafx.h"
#include "CppUnitTest.h"
#include "../common/allocator.h"
#include "../common/dedicated_heap.h"

using namespace std;
using namespace std::chrono;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

class cls1
{ 
public:
	char b[1000];   // just takes up space

	void* operator new (size_t size);
	void* operator new (size_t size, void*);
	void operator delete(void* p);
};
static dedicated_heap<cls1, std_allocator> heap1;
void* cls1::operator new (size_t size) { return heap1.get(); }
void* cls1::operator new (size_t size, void* ptr) { return ::operator new (size, ptr); }
void cls1::operator delete(void* p) { return heap1.release((cls1*)p); }
template <> 
std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString<cls1>(cls1* q) {	return L"(cls1)"; }



class cls2
{ 
public:
	char b[1000];  // just takes up space

	void* operator new (size_t size);
	void* operator new (size_t size, void*);
	void operator delete(void* p);
};
static dedicated_heap<cls2, Virtual_allocator> heap2;
void* cls2::operator new (size_t size) { return heap2.get(); }
void* cls2::operator new (size_t size, void* ptr) { return ::operator new (size, ptr); }
void cls2::operator delete(void* p) { return heap2.release((cls2*)p); }
template <> 
std::wstring Microsoft::VisualStudio::CppUnitTestFramework::ToString<cls2>(cls2* q) {	return L"(cls2)"; }



namespace UnitTests
{		
	TEST_CLASS(heap_tester)
	{
	public:

		template<typename T, typename heap_T>
		void heap_test(heap_T& heap)
		{
			heap.allocate(100);

			T* p = new T; delete p; p = new T; 
			T* q = new T; 
			delete p; 
			T* r = new T;
			delete q;

			// Only r should have survived.
			Assert::AreEqual(heap.free_space(), heap.capacity()-1);
			delete r;

			// Use up the heap.
			vector<T*> ptrs;
			for(;;)
			{
				T* p = new T;
				if(p == nullptr)
					break;
				ptrs.push_back(p);
			} 
			Assert::AreEqual(heap.free_space(), size_t(0));

			// delete some random ptrs
			delete ptrs[39]; ptrs.erase(ptrs.begin()+39);
			delete ptrs[73]; ptrs.erase(ptrs.begin()+73);
			delete ptrs[0];  ptrs.erase(ptrs.begin());
			Assert::AreEqual(heap.free_space(), size_t(3));

			ptrs.push_back(new T);
			ptrs.push_back(new T);
			ptrs.push_back(new T);
			Assert::AreEqual(heap.free_space(), size_t(0));

			p = new T;
			Assert::AreEqual(p, (T*)NULL);

			for(auto ptr : ptrs)
				delete ptr;
			Assert::AreEqual(heap.free_space(), heap.capacity());

			p = new T;
			delete p;
			Assert::AreEqual(heap.free_space(), heap.capacity());
		}

		TEST_METHOD(heap_test1)
		{
			heap_test<cls1, dedicated_heap<cls1, std_allocator>>(heap1);
		}

		TEST_METHOD(heap_test2)	 // same as above but uses Virtual_allocator.
		{
			heap_test<cls2, dedicated_heap<cls2, Virtual_allocator>>(heap2);
		}
	};
}