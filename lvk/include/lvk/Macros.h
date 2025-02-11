#pragma once
#define VK_CHECK(X) {int _lineNumber = __LINE__; const char* _filePath = __FILE__;\
if(X != VK_SUCCESS){\
spdlog::error("VK check failed at {} Line {} : {}",_filePath, _lineNumber, #X);}}
