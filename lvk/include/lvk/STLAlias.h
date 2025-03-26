#pragma once

// Namespace alias that allows replacement (e.g. std / eastl)
#ifndef LVK_UNORDERED_MAP_ALIAS
	#define LVK_UNORDERED_MAP_ALIAS <unordered_map>
	#define LVK_UNORDERED_MAP_NS std 
#endif

#ifndef LVK_VECTOR_ALIAS
	#define LVK_VECTOR_ALIAS <vector>
	#define LVK_VECTOR_NS std 
#endif

#ifndef LVK_ARRAY_ALIAS
	#define LVK_ARRAY_ALIAS <array>
	#define LVK_ARRAY_NS std 
#endif

#ifndef LVK_FUNCTIONAL_ALIAS
	#define LVK_FUNCTIONAL_ALIAS <functional>
	#define LVK_FUNCTIONAL_NS std 
#endif

#ifndef LVK_OPTIONAL_ALIAS
	#define LVK_OPTIONAL_ALIAS <optional>
	#define LVK_OPTIONAL_NS std 
#endif

#ifndef LVK_IOSTREAM_ALIAS
	#define LVK_IOSTREAM_ALIAS <iostream>
	#define LVK_IOSTREAM_NS std 
#endif

#ifndef LVK_MEMORY_ALIAS
	#define LVK_MEMORY_ALIAS <memory>
	#define LVK_MEMORY_NS std 
#endif

#ifndef LVK_UTILITY_ALIAS
	#define LVK_UTILITY_ALIAS <utility>
	#define LVK_UTILITY_NS std 
#endif

#ifndef LVK_FILESYSTEM_ALIAS
	#define LVK_FILESYSTEM_ALIAS <filesystem>
	#define LVK_FILESYSTEM_NS std 
#endif

#ifndef LVK_STRING_STREAM_ALIAS
	#define LVK_STRING_ALIAS <string>
	#define LVK_STRING_NS std 
#endif

#ifndef LVK_STRING_STREAM_ALIAS
	#define LVK_STRING_STREAM_ALIAS <sstream>
	#define LVK_STRING_STREAM_NS std 
#endif

#ifndef LVK_REGEX_ALIAS
	#define LVK_REGEX_ALIAS <regex>
	#define LVK_REGEX_NS std 
#endif

#ifndef LVK_STDLIB_ALIAS
	#define LVK_STDLIB_ALIAS <cstdlib>
	#define LVK_STDLIB_NS std 
#endif
