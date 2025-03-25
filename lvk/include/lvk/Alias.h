#pragma once
#include "lvk/STLAlias.h"
#include "lvk/Allocator.h"

#include LVK_IOSTREAM_ALIAS
#include LVK_UNORDERED_MAP_ALIAS
#include LVK_VECTOR_ALIAS
#include LVK_FUNCTIONAL_ALIAS
#include LVK_ARRAY_ALIAS
#include LVK_OPTIONAL_ALIAS

namespace lvk
{
	using String = std::basic_string<char, std::char_traits<char>, STLAllocator<char>>;
	
	using StringStream = std::basic_stringstream<char, std::char_traits<char>, STLAllocator<char >>;
	
	using IStringStream = std::basic_istringstream<char, std::char_traits<char>, STLAllocator<char>>;
	
	template<typename _Ty>
	using Vector = std::vector<_Ty, STLAllocator<_Ty>>;

	template<typename _Ty>
	using StaticVector = std::vector<_Ty>;

	template<typename _Ty, size_t _Size>
	using Array = std::array<_Ty, _Size>;

	template<typename _Key, typename _Value>
	using HashMap = std::unordered_map<
		_Key, 
		_Value, 
		std::hash<_Key>, 
		std::equal_to<_Key>,
		STLAllocator<std::pair<const _Key, _Value>>>;

	template<typename _Ty>
	using Optional = std::optional<_Ty>;

	template<typename _Ty>
	using Unique = std::unique_ptr<_Ty>;

	template<typename _Ty>
	using RefCntPtr = std::shared_ptr<_Ty>;
}

