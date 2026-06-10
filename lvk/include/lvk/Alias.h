// #pragma once
#include "lvk/Allocator.h"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <functional>
#include <array>
#include <optional>
#include <memory>
#include <string>
#include <sstream>
#include <span>

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

	template<typename _Ty>
	using Span		= std::span<_Ty>;
}

