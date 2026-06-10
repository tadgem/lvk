#pragma once
#include <concepts>
#include <cstdlib>


namespace lvk
{
	template<typename T>
	concept AllocatorType = requires(T a)
	{
		{a.allocate(sizeof(T))} -> std::same_as<void*>;
		{a.deallocate(nullptr)} -> std::same_as<void>;
	};

	class MallocAllocator
	{
	public:
		void* allocate(size_t size) {
			return std::malloc(size);
		}

		void deallocate(void* addr)
		{
			std::free(addr);
		}
	};


	template <class T, AllocatorType A = MallocAllocator>
	struct STLAllocator
	{
		typedef T value_type;
		A& _allocator;

		STLAllocator(A alloc = A()) : _allocator(alloc) {} //default ctor not required by C++ Standard Library

		// A converting copy constructor:
		template<class U> STLAllocator(const STLAllocator<U>& o) : _allocator(o._allocator) {}
		template<class U> bool operator==(const STLAllocator<U>&) const
		{
			return true;
		}
		template<class U> bool operator!=(const STLAllocator<U>&) const
		{
			return false;
		}
		T* allocate(const size_t n) const
		{
			return static_cast<T*>(_allocator.allocate(sizeof(T) * n));
		}
		void deallocate(T* const p, size_t) const
		{
			_allocator.deallocate((void*)p);
		}
	};



}
