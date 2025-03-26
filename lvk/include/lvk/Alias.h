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
#include LVK_STRING_ALIAS
#include LVK_STRING_STREAM_ALIAS

namespace lvk
{
	using String = LVK_STRING_NS::basic_string<char, LVK_STRING_NS::char_traits<char>, STLAllocator<char>>;
	
	using StringStream = LVK_STRING_STREAM_NS::basic_stringstream<char, LVK_STRING_STREAM_NS::char_traits<char>, STLAllocator<char >>;
	
	using IStringStream = LVK_STRING_STREAM_NS::basic_istringstream<char, LVK_STRING_STREAM_NS::char_traits<char>, STLAllocator<char>>;
	
	template<typename _Ty>
	using Vector = LVK_VECTOR_NS::vector<_Ty, STLAllocator<_Ty>>;

	template<typename _Ty>
	using StaticVector = LVK_VECTOR_NS::vector<_Ty>;

	template<typename _Ty, size_t _Size>
	using Array = LVK_ARRAY_NS::array<_Ty, _Size>;

	template<typename _Key, typename _Value>
	using HashMap = LVK_UNORDERED_MAP_NS::unordered_map<
		_Key, 
		_Value, 
		LVK_UNORDERED_MAP_NS::hash<_Key>,
		LVK_UNORDERED_MAP_NS::equal_to<_Key>,
		STLAllocator<LVK_UNORDERED_MAP_NS::pair<const _Key, _Value>>>;

	template<typename _Ty>
	using Optional = LVK_OPTIONAL_NS::optional<_Ty>;

	template<typename _Ty>
	using Unique = LVK_MEMORY_NS::unique_ptr<_Ty>;

	template<typename _Ty>
	using RefCntPtr = LVK_MEMORY_NS::shared_ptr<_Ty>;
}

