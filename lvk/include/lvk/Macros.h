#pragma once
#include "lvk/Log.h"

#define VK_CHECK(X) {int _lineNumber = __LINE__; const char* _filePath = __FILE__;\
if(X != VK_SUCCESS){\
LVK_LOG_ERR("VK check failed at %s Line %s: %s", _filePath, _lineNumber, #X);}}

static constexpr int        MAX_FRAMES_IN_FLIGHT = 2;
