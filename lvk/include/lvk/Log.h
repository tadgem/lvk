#pragma once

#define LVK_PRINTF_IMPL(...)
#define LVK_LOG_INFO(...) LVK_PRINTF_IMPL(__VA_ARGS__)
#define LVK_LOG_ERR(...) LVK_PRINTF_IMPL(__VA_ARGS__)
#define LVK_LOG_WARN(...) LVK_PRINTF_IMPL(__VA_ARGS__)