#pragma once
extern void lvk_internal_printf(const char* fmt, ...);

#define LVK_PRINTF_IMPL(...)	lvk_internal_printf(__VA_ARGS__);lvk_internal_printf("\n")
#define LVK_LOG_INFO(...)		lvk_internal_printf("INFO: ");	LVK_PRINTF_IMPL(__VA_ARGS__)
#define LVK_LOG_ERR(...)		lvk_internal_printf("ERROR: ");	LVK_PRINTF_IMPL(__VA_ARGS__)
#define LVK_LOG_WARN(...)		lvk_internal_printf("WARN: ");	LVK_PRINTF_IMPL(__VA_ARGS__)
