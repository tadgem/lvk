#pragma once
#include "lvk/STLAlias.h"
#include "lvk/Allocator.h"

#include LVK_IOSTREAM_ALIAS
#include LVK_UNORDERED_MAP_ALIAS
#include LVK_VECTOR_ALIAS
#include LVK_FUNCTIONAL_ALIAS
#include LVK_ARRAY_ALIAS
#include LVK_OPTIONAL_ALIAS
#include LVK_MEMORY_ALIAS

namespace lvk
{
	using String = LVK_STL::basic_string<char, LVK_STL::char_traits<char>, STLAllocator<char>>;
	
	using StringStream = LVK_STL::basic_stringstream<char, LVK_STL::char_traits<char>, STLAllocator<char >>;
	
	using IStringStream = LVK_STL::basic_istringstream<char, LVK_STL::char_traits<char>, STLAllocator<char>>;
	
	template<typename _Ty>
	using Vector = LVK_STL::vector<_Ty, STLAllocator<_Ty>>;

	template<typename _Ty>
	using StaticVector = LVK_STL::vector<_Ty>;

	template<typename _Ty, size_t _Size>
	using Array = LVK_STL::array<_Ty, _Size>;

	template<typename _Key, typename _Value>
	using HashMap = LVK_STL::unordered_map<
		_Key, 
		_Value, 
		LVK_STL::hash<_Key>,
		LVK_STL::equal_to<_Key>,
		STLAllocator<LVK_STL::pair<const _Key, _Value>>>;

	template<typename _Ty>
	using Optional = LVK_STL::optional<_Ty>;

	template<typename _Ty>
	using Unique = LVK_STL::unique_ptr<_Ty>;

	template<typename _Ty>
	using RefCntPtr = LVK_STL::shared_ptr<_Ty>;
}

