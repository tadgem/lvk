#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <functional>

namespace lvk
{
	using String = std::string;

	template<typename _Ty>
	using Vector = std::vector<_Ty>;

	template<typename _Key, typename _Value>
	using HashMap = std::unordered_map<_Key, _Value>;
}

