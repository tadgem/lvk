#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <functional>
#include <array>

namespace lvk
{
	using String = std::string;

	template<typename _Ty>
	using Vector = std::vector<_Ty>;

	template<typename _Ty, size_t _Size>
	using Array = std::array<_Ty, _Size>;

	template<typename _Key, typename _Value>
	using HashMap = std::unordered_map<_Key, _Value>;
}

